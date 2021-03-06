
/*
 * Copyright 2008-2009 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#define _GNU_SOURCE
#include "nfs4d-config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <syslog.h>
#include <errno.h>
#include <glib.h>
#include "server.h"
#include "nfs4_prot.h"

bool inode_check(DB_TXN *txn, nfsino_t inum)
{
	struct fsdb_inode *dbino = NULL;
	int rc;

	rc = fsdb_inode_get(&srv.fsdb, txn, inum, 0, &dbino);

	free(dbino);

	if (rc)
		return false;

	return true;
}

struct nfs_inode *inode_getdec(DB_TXN *txn, nfsino_t inum, int flags)
{
	struct nfs_inode *ino = NULL;

	if (fsdb_inode_getdec(&srv.fsdb, txn, inum, flags, &ino))
		return NULL;

	return ino;
}

int inode_touch(DB_TXN *txn, struct nfs_inode *ino)
{
	ino->version++;
	ino->mtime = current_time.tv_sec;

	return fsdb_inode_putenc(&srv.fsdb, txn, ino, 0);
}

void inode_free(struct nfs_inode *ino)
{
	nfsino_t inum;

	(void) inum;		/* silence set-but-not-used warning */

	if (!ino)
		return;

	inum = ino->inum;

	switch (ino->type) {
	case NF4LNK:
		free(ino->linktext);
		break;
	default:
		/* do nothing */
		break;
	}

	free(ino->mimetype);
	free(ino->user);
	free(ino->group);
	free(ino);
}

static struct nfs_inode *inode_new(DB_TXN *txn, struct nfs_cxn *cxn)
{
	struct nfs_inode *ino;
	int limit = 100000;
	bool found = false;
	nfsino_t new_inum = 0;

	while (limit-- > 0) {
#ifdef NFSD_INO64
		nrand32(&new_inum, 2);
#else
		nrand32(&new_inum, 1);
#endif /* NFSD_INO64 */

		if (new_inum <= INO_RESERVED_LAST)
			continue;

		if (inode_check(txn, new_inum) == false) {
			found = true;
			break;
		}
	}

	if (!found)
		return NULL;

	ino = calloc(1, sizeof(struct nfs_inode));
	if (!ino)
		return NULL;

	ino->inum = new_inum;
	ino->version = 1ULL;
	ino->ctime =
	ino->atime =
	ino->mtime = current_time.tv_sec;
	ino->mode = MODE4_RUSR;
	ino->n_link = 1;

	/* connected user */
	ino->user = cxn_getuser(cxn);
	ino->group = cxn_getgroup(cxn);

	return ino;
}

#if 0 /* we may reuse some day */
static gint nfstr_compare(gconstpointer _a, gconstpointer _b)
{
	const struct nfs_buf *a = _a;
	const struct nfs_buf *b = _b;
	int res;

	res = strncmp(a->val, b->val, MIN(a->len, b->len));
	if (res)
		return res;

	if (a->len < b->len)
		return -1;
	if (a->len > b->len)
		return 1;

	return 0;
}
#endif

static nfsstat4 inode_new_file(nfsino_t inum)
{
	char *s = alloca(strlen(srv.data_dir) + INO_FNAME_LEN + 2);
	nfsstat4 status = NFS4_OK;
	int fd;
	char datapfx[4];

	mk_datapfx(datapfx, inum);
	sprintf(s, INO_DATAFN_FMT, srv.data_dir, datapfx,
		(unsigned long long) inum);

	fd = open(s, O_CREAT | O_EXCL | O_WRONLY, 0666);
	if (fd < 0) {
		syslogerr2("inode_new_file create", s);
		return NFS4ERR_IO;
	}

	if (close(fd) < 0)
		syslogerr2("inode_new_file close", s);

	return status;
}

nfsstat4 inode_new_type(DB_TXN *txn, struct nfs_cxn *cxn, uint32_t objtype,
			const struct nfs_inode *dir_ino,
			const struct nfs_buf *linkdata,
			const uint32_t *specdata,
			struct nfs_inode **ino_out)
{
	struct nfs_inode *new_ino;
	nfsstat4 status;

	*ino_out = NULL;

	new_ino = inode_new(txn, cxn);
	if (!new_ino) {
		status = NFS4ERR_RESOURCE;
		goto out;
	}

	new_ino->type = objtype;
	new_ino->parent = dir_ino->inum;

	switch(objtype) {
	case NF4DIR:
		new_ino->size = 4096;
		break;
	case NF4BLK:
	case NF4CHR:
		memcpy(&new_ino->devdata[0], specdata, sizeof(uint32_t) * 2);
		break;
	case NF4LNK: {
		char *linktext = g_strndup(linkdata->val, linkdata->len);
		if (!linktext) {
			status = NFS4ERR_RESOURCE;
			goto out;
		}
		new_ino->linktext = linktext;
		break;
	}
	case NF4REG:
		status = inode_new_file(new_ino->inum);
		if (status != NFS4_OK)
			goto out;
		break;
	case NF4SOCK:
	case NF4FIFO:
		break;
	default:
		status = NFS4ERR_BADTYPE;
		goto out;
	}

	new_ino->type = objtype;

	*ino_out = new_ino;
	status = NFS4_OK;

out:
	return status;
}

void inode_openfile_add(struct nfs_inode *ino, struct nfs_openfile *of)
{
	list_add(&of->inode_node, &ino_openfile_list);
}

int inode_unlink(DB_TXN *txn, struct nfs_inode *ino)
{
	int rc;
	char *fdpath;
	nfsino_t inum = ino->inum;
	char datapfx[4];

	if (!ino || !inum || (ino->inum == INO_ROOT)) {
		applog(LOG_ERR, "BUG: null in inode_unlink");
		return -EINVAL;
	}

	/* decrement link count */
	ino->n_link--;

	/* if links remain, update the inode and exit */
	if (ino->n_link > 0)
		return inode_touch(txn, ino);

	/* no links remain. delete inode. */
	rc = fsdb_inode_del(&srv.fsdb, txn, ino->inum, 0);
	if (rc == 0)
		ino->inum = 0;		/* if deleted, poison inode num */

	/* if error, or not a regular file, exit */
	if (rc || ino->type != NF4REG)
		return rc;

	/*
	 * remove data associated with inode
	 */

	mk_datapfx(datapfx, inum);
	fdpath = alloca(strlen(srv.data_dir) + INO_FNAME_LEN + 1);
	sprintf(fdpath, INO_DATAFN_FMT, srv.data_dir, datapfx,
		(unsigned long long) inum);

	/*
	 * FIXME: metadata transaction could still be aborted, at this
	 * point.  That results in intact metadata pointing to a file
	 * that no longer exists
	 */

	if (unlink(fdpath) < 0) {
		rc = -errno;
		syslogerr(fdpath);
	}

	return rc;
}

enum nfsstat4 inode_apply_attrs(DB_TXN *txn, struct nfs_inode *ino,
				const struct nfs_fattr_set *attr,
			        uint64_t *bitmap_set_out,
			        struct nfs_stateid *sid,
			        bool in_setattr)
{
	uint64_t bitmap_set = 0;
	enum nfsstat4 status = NFS4_OK;
	char *fdpath;

	if (attr->bitmap & fattr_read_only_mask) {
		status = NFS4ERR_INVAL;
		goto out;
	}
	if (attr->bitmap & ~fattr_supported_mask) {
		status = NFS4ERR_ATTRNOTSUPP;
		goto out;
	}

	if (attr->bitmap & (1ULL << FATTR4_SIZE)) {
		uint64_t new_size = attr->size;
		uint64_t ofs, len;
		struct nfs_access ac = { NULL, };
		char datapfx[4];

		if (debugging > 1)
			applog(LOG_DEBUG, "   apply SIZE %llu",
				(unsigned long long) attr->size);

		/* only permit size attribute manip on files */
		if (ino->type != NF4REG) {
			if (ino->type == NF4DIR)
				status = NFS4ERR_ISDIR;
			else
				status = NFS4ERR_INVAL;
			goto out;
		}

		if (new_size < ino->size) {
			ofs = new_size;
			len = ino->size - new_size;
		} else {
			ofs = ino->size;
			len = new_size - ino->size;
		}

		ac.sid = sid;
		ac.ino = ino;
		ac.op = OP_SETATTR;
		ac.ofs = ofs;
		ac.len = len;
		status = access_ok(&ac);
		if (status != NFS4_OK)
			goto out;

		if (new_size == ino->size)
			goto size_done;

		mk_datapfx(datapfx, ino->inum);
		fdpath = alloca(strlen(srv.data_dir) + INO_FNAME_LEN + 1);
		sprintf(fdpath, INO_DATAFN_FMT, srv.data_dir, datapfx,
			(unsigned long long) ino->inum);

		if (truncate(fdpath, new_size) < 0) {
			status = NFS4ERR_IO;
			goto out;
		}

		ino->size = new_size;

size_done:
		bitmap_set |= (1ULL << FATTR4_SIZE);
	}

	if (attr->bitmap & (1ULL << FATTR4_TIME_ACCESS_SET)) {
		if (attr->time_access_set.nseconds > 999999999){
			status = NFS4ERR_INVAL;
			goto out;
		}

		ino->atime = attr->time_access_set.seconds;

		bitmap_set |= (1ULL << FATTR4_TIME_ACCESS_SET);
	}
	if (attr->bitmap & (1ULL << FATTR4_TIME_MODIFY_SET)) {
		if (attr->time_modify_set.nseconds > 999999999){
			status = NFS4ERR_INVAL;
			goto out;
		}

		ino->mtime = attr->time_modify_set.seconds;

		bitmap_set |= (1ULL << FATTR4_TIME_MODIFY_SET);
	}
	if (attr->bitmap & (1ULL << FATTR4_MODE)) {
		if (attr->mode & ~MODE4_ALL) {
			status = NFS4ERR_BADXDR;
			goto out;
		}
		ino->mode = attr->mode;
		bitmap_set |= (1ULL << FATTR4_MODE);

		if (debugging > 1)
			applog(LOG_DEBUG, "   apply MODE %o", ino->mode);
	}
	if ((attr->bitmap & (1ULL << FATTR4_OWNER)) &&
	    (attr->owner.len)) {
		char *ostr = id_lookup_name(idt_user, attr->owner.val,
					    attr->owner.len);
		if (!ostr) {
			if (debugging)
				applog(LOG_DEBUG,
				       "   SETATTR DENIED owner \"%.*s\"",
				       attr->owner.len,
				       attr->owner.val);
			status = NFS4ERR_PERM;
			goto out;
		}

		ino->user = ostr;

		bitmap_set |= (1ULL << FATTR4_OWNER);
	}
	if ((attr->bitmap & (1ULL << FATTR4_OWNER_GROUP)) &&
	    (attr->owner_group.len)) {
		char *ostr = id_lookup_name(idt_group, attr->owner_group.val,
					    attr->owner_group.len);
		if (!ostr) {
			if (debugging)
				applog(LOG_DEBUG,
				       "   SETATTR DENIED group \"%.*s\"",
				       attr->owner_group.len,
				       attr->owner_group.val);
			status = NFS4ERR_PERM;
			goto out;
		}

		ino->group = ostr;

		bitmap_set |= (1ULL << FATTR4_OWNER_GROUP);
	}

out:
	if (in_setattr && bitmap_set) {
		if (inode_touch(txn, ino))
			status = NFS4ERR_IO;
	}

	*bitmap_set_out = bitmap_set;
	return status;
}

nfsstat4 inode_add(DB_TXN *txn, struct nfs_inode *dir_ino,
		   struct nfs_inode *new_ino, const struct nfs_fattr_set *attr,
		   const struct nfs_buf *name, uint64_t *attrset,
		   change_info4 *cinfo)
{
	nfsstat4 status = NFS4_OK;
	int rc;

	if (attr)
		status = inode_apply_attrs(txn, new_ino, attr, attrset,
					   NULL, false);
	if (status != NFS4_OK)
		goto out;

	cinfo->atomic = true;
	cinfo->before =
	cinfo->after = dir_ino->version;

	status = dir_add(txn, dir_ino, name, new_ino);
	if (status != NFS4_OK)
		goto out;

	rc = fsdb_inode_putenc(&srv.fsdb, txn, new_ino, 0);
	if (rc) {
		status = NFS4ERR_IO;
		goto out;
	}

	cinfo->after = dir_ino->version;

out:
	return status;
}

static void print_create_args(uint32_t objtype, const struct nfs_buf *objname,
			      const struct nfs_buf *linkdata,
			      const uint32_t *specdata,
			      const struct nfs_fattr_set *attr)
{
	switch (objtype) {
	case NF4BLK:
	case NF4CHR:
		applog(LOG_INFO, "op CREATE (%s, '%.*s', %u %u)",
		       name_nfs_ftype4[objtype],
		       objname->len, objname->val,
		       specdata[0],
		       specdata[1]);
		break;
	case NF4LNK:
		applog(LOG_INFO, "op CREATE (%s, '%.*s', '%.*s')",
		       name_nfs_ftype4[objtype],
		       objname->len, objname->val,
		       linkdata->len, linkdata->val);
		break;
	default:
		applog(LOG_INFO, "op CREATE (%s, '%.*s')",
		       name_nfs_ftype4[objtype],
		       objname->len, objname->val);
		break;
	}

	print_fattr("op CREATE attr", attr);
}

nfsstat4 nfs_op_create(struct nfs_cxn *cxn, const CREATE4args *args,
		       struct list_head *writes, struct rpc_write **wr)
{
	nfsstat4 status;
	struct nfs_inode *dir_ino = NULL, *new_ino = NULL;
	uint32_t objtype, specdata[2] = { 0, };
	struct nfs_buf objname, linkdata = { 0, NULL };
	struct nfs_fattr_set attr;
	uint64_t attrset = 0;
	change_info4 cinfo;
	int rc;
	DB_ENV *dbenv = srv.fsdb.env;
	DB_TXN *txn;

	objtype = args->objtype.type;
	if (objtype == NF4BLK || objtype == NF4CHR) {
		specdata[0] = args->objtype.createtype4_u.devdata.specdata1;
		specdata[1] = args->objtype.createtype4_u.devdata.specdata2;
	} else if (objtype == NF4LNK) {
		linkdata.val = args->objtype.createtype4_u.linkdata.utf8string_val;
		linkdata.len = args->objtype.createtype4_u.linkdata.utf8string_len;
	}
	objname.val = args->objname.utf8string_val;
	objname.len = args->objname.utf8string_len;

	status = copy_attr(&attr, &args->createattrs);
	if (status != NFS4_OK)
		goto out_noattr;

	if (debugging)
		print_create_args(objtype, &objname, &linkdata,
				  specdata, &attr);

	rc = dbenv->txn_begin(dbenv, NULL, &txn, 0);
	if (rc) {
		status = NFS4ERR_IO;
		dbenv->err(dbenv, rc, "DB_ENV->txn_begin");
		goto out;
	}

	status = dir_curfh(txn, cxn, &dir_ino, 0);
	if (status != NFS4_OK)
		goto out_abort;

	if (dir_ino->type != NF4DIR) {
		status = NFS4ERR_NOTDIR;
		goto out_abort;
	}
	if (objtype == NF4REG) {
		status = NFS4ERR_BADTYPE;
		goto out_abort;
	}

	status = inode_new_type(txn, cxn, objtype, dir_ino, &linkdata,
				specdata, &new_ino);
	if (status != NFS4_OK)
		goto out_abort;

	status = inode_add(txn, dir_ino, new_ino, &attr,
			   &objname, &attrset, &cinfo);
	if (status != NFS4_OK)
		goto out_abort;

	rc = txn->commit(txn, 0);
	if (rc) {
		dbenv->err(dbenv, rc, "DB_ENV->txn_commit");
		status = NFS4ERR_IO;
		goto out;
	}

	fh_set(&cxn->current_fh, new_ino->inum);

	if (debugging)
		applog(LOG_INFO, "   CREATE -> %016llX",
			(unsigned long long) cxn->current_fh.inum);

out:
	fattr_free(&attr);
out_noattr:
	WR32(status);
	if (status == NFS4_OK) {
		WR32(cinfo.atomic ? 1 : 0);
		WR64(cinfo.before);
		WR64(cinfo.after);
		WRMAP(attrset);
	}
	inode_free(dir_ino);
	inode_free(new_ino);
	return status;

out_abort:
	if (txn->abort(txn))
		dbenv->err(dbenv, rc, "DB_ENV->txn_abort");
	goto out;
}

nfsstat4 nfs_op_getattr(struct nfs_cxn *cxn, const GETATTR4args *args,
		        struct list_head *writes, struct rpc_write **wr)
{
	nfsstat4 status = NFS4_OK;
	struct nfs_inode *ino = NULL;
	struct nfs_fattr_set attrset;
	bool printed = false;
	uint32_t *status_p;
	uint64_t bitmap_out = 0;

	memset(&attrset, 0, sizeof(attrset));

	status_p = WRSKIP(4);		/* ending status */

	attrset.bitmap = bitmap4_decode(&args->attr_request);

	ino = inode_fhdec(NULL, cxn->current_fh, 0);
	if (!ino) {
		status = NFS4ERR_NOFILEHANDLE;
		goto out;
	}

	if (debugging && (status == NFS4_OK)) {
		print_fattr_bitmap("op GETATTR", attrset.bitmap);
		printed = true;
	}

	/* GETATTR not permitted to process write-only attrs */
	if (attrset.bitmap & ((1ULL << FATTR4_TIME_ACCESS_SET) |
			      (1ULL << FATTR4_TIME_MODIFY_SET))) {
		status = NFS4ERR_INVAL;
		goto out;
	}

	attrset.bitmap &= fattr_supported_mask;

	fattr_fill(ino, &attrset);

	status = wr_fattr(&attrset, &bitmap_out, writes, wr);

	fattr_free(&attrset);

	if (debugging) {
		print_fattr_bitmap("   GETATTR ->", bitmap_out);
		printed = true;
	}

out:
	if (debugging && !printed)
		applog(LOG_INFO, "op GETATTR");

	*status_p = htonl(status);
	inode_free(ino);
	return status;
}

nfsstat4 nfs_op_setattr(struct nfs_cxn *cxn, const SETATTR4args *args,
		        struct list_head *writes, struct rpc_write **wr)
{
	struct nfs_inode *ino = NULL;
	nfsstat4 status = NFS4_OK;
	uint64_t bitmap_out = 0;
	struct nfs_stateid sid;
	struct nfs_fattr_set attr;
	DB_ENV *dbenv = srv.fsdb.env;
	DB_TXN *txn;
	int rc;

	copy_sid(&sid, &args->stateid);

	status = copy_attr(&attr, &args->obj_attributes);
	if (status != NFS4_OK) {
		if (attr.bitmap & fattr_read_only_mask)
			status = NFS4ERR_INVAL;
		goto out_noattr;
	}

	if (debugging) {
		applog(LOG_INFO, "op SETATTR (ID:%x)", sid.id);
		print_fattr("   SETATTR", &attr);
	}

	rc = dbenv->txn_begin(dbenv, NULL, &txn, 0);
	if (rc) {
		status = NFS4ERR_IO;
		dbenv->err(dbenv, rc, "DB_ENV->txn_begin");
		goto out;
	}

	ino = inode_fhdec(txn, cxn->current_fh, DB_RMW);
	if (!ino) {
		status = NFS4ERR_NOFILEHANDLE;
		goto out_abort;
	}

	status = inode_apply_attrs(txn, ino, &attr, &bitmap_out, &sid, true);
	if (status != NFS4_OK)
		goto out_abort;

	rc = txn->commit(txn, 0);
	if (rc) {
		dbenv->err(dbenv, rc, "DB_ENV->txn_commit");
		status = NFS4ERR_IO;
		goto out;
	}

	if (debugging)
		print_fattr_bitmap("   SETATTR result", bitmap_out);

out:
	fattr_free(&attr);
out_noattr:
	WR32(status);
	WRMAP(bitmap_out);
	inode_free(ino);
	return status;

out_abort:
	if (txn->abort(txn))
		dbenv->err(dbenv, rc, "DB_ENV->txn_abort");
	goto out;
}

unsigned int inode_access(const struct nfs_cxn *cxn,
			  const struct nfs_inode *ino, unsigned int req_access)
{
	unsigned int mode, rc = 0;
	char *user = NULL, *group = NULL;
	bool user_mat, group_mat;
	bool root_user, root_group;

	user = cxn_getuser(cxn);
	group = cxn_getgroup(cxn);
	if (!user || !group) {
		if (debugging)
			applog(LOG_INFO, "invalid cxn%s%s",
			       user ? "" : " user",
			       group ? "" : " group");
		goto out;
	}

	user_mat = (strcmp(user, ino->user) == 0);
	group_mat = (strcmp(group, ino->group) == 0);
	root_user = (strcmp(user, "root@localdomain") == 0);
	root_group = (strcmp(group, "root@localdomain") == 0);

	mode = ino->mode & 0x7;
	if (user_mat || root_user)
		mode |= (ino->mode >> 6) & 0x7;
	if (group_mat || root_group)
		mode |= (ino->mode >> 3) & 0x7;

	rc = 0;
	if (mode & MODE4_ROTH)
		rc |= ACCESS4_READ;
	if ((mode & MODE4_XOTH) && (ino->type == NF4DIR))
		rc |= ACCESS4_LOOKUP;
	if (mode & MODE4_WOTH)
		rc |= ACCESS4_MODIFY;
	if (mode & MODE4_WOTH)
		rc |= ACCESS4_EXTEND;
	if ((mode & MODE4_XOTH) && (ino->type != NF4DIR))
		rc |= ACCESS4_EXECUTE;

	rc &= req_access;

	/* FIXME: check ACCESS4_DELETE */

out:
	free(user);
	free(group);
	return rc;
}

nfsstat4 nfs_op_access(struct nfs_cxn *cxn, const ACCESS4args *args,
		       struct list_head *writes, struct rpc_write **wr)
{
	nfsstat4 status = NFS4_OK;
	struct nfs_inode *ino = NULL;
	uint32_t arg_access;
	ACCESS4resok resok = { 0, 0 };

	arg_access = args->access;

	if (debugging)
		applog(LOG_INFO, "op ACCESS (0x%x)", arg_access);

	ino = inode_fhdec(NULL, cxn->current_fh, 0);
	if (!ino) {
		status = NFS4ERR_NOFILEHANDLE;
		goto out;
	}

	resok.access = inode_access(cxn, ino, arg_access);
	resok.supported =
		ACCESS4_READ |
		ACCESS4_LOOKUP |
		ACCESS4_MODIFY |
		ACCESS4_EXTEND |
		/* ACCESS4_DELETE | */	/* FIXME */
		ACCESS4_EXECUTE;

	resok.supported &= resok.access;

	if (debugging)
		applog(LOG_INFO, "   ACCESS -> (ACC:%x SUP:%x)",
		       resok.access,
		       resok.supported);

out:
	WR32(status);
	if (status == NFS4_OK) {
		WR32(resok.supported);
		WR32(resok.access);
	}
	inode_free(ino);
	return status;
}

static bool inode_attr_cmp(const struct nfs_inode *ino,
			     const struct nfs_fattr_set *attr)
{
	uint64_t bitmap = attr->bitmap;

	/*
	 * per-server attributes
	 */
        if (bitmap & (1ULL << FATTR4_LEASE_TIME))
		if (attr->lease_time != srv.lease_time)
			return false;

	/*
	 * per-filesystem attributes
	 */
        if (bitmap & (1ULL << FATTR4_SUPPORTED_ATTRS))
		if (attr->supported_attrs != fattr_supported_mask)
			return false;
        if (bitmap & (1ULL << FATTR4_FH_EXPIRE_TYPE))
		if (attr->fh_expire_type != SRV_FH_EXP_TYPE)
			return false;
        if (bitmap & (1ULL << FATTR4_LINK_SUPPORT))
		if (attr->link_support != true)
			return false;
        if (bitmap & (1ULL << FATTR4_SYMLINK_SUPPORT))
		if (attr->symlink_support != true)
			return false;
        if (bitmap & (1ULL << FATTR4_UNIQUE_HANDLES))
		if (attr->unique_handles != true)
			return false;
        if (bitmap & (1ULL << FATTR4_CANSETTIME))
		if (attr->cansettime != true)
			return false;
        if (bitmap & (1ULL << FATTR4_CASE_INSENSITIVE))
		if (attr->case_insensitive != false)
			return false;
        if (bitmap & (1ULL << FATTR4_CASE_PRESERVING))
		if (attr->case_preserving != true)
			return false;
        if (bitmap & (1ULL << FATTR4_FILES_TOTAL)) {
#if 0 /* FIXME */
		if (attr->files_total != srv.inode_table_len)
			return false;
#endif
	}
        if (bitmap & (1ULL << FATTR4_HOMOGENEOUS))
		if (attr->homogeneous != true)
			return false;
        if (bitmap & (1ULL << FATTR4_MAXFILESIZE))
		if (attr->maxfilesize != SRV_MAX_FILESIZE)
			return false;
        if (bitmap & (1ULL << FATTR4_MAXLINK))
		if (attr->maxlink != SRV_MAX_LINK)
			return false;
        if (bitmap & (1ULL << FATTR4_MAXNAME))
		if (attr->maxname != SRV_MAX_NAME)
			return false;
        if (bitmap & (1ULL << FATTR4_MAXREAD))
		if (attr->maxread != SRV_MAX_READ)
			return false;
        if (bitmap & (1ULL << FATTR4_MAXWRITE))
		if (attr->maxwrite != SRV_MAX_WRITE)
			return false;
        if (bitmap & (1ULL << FATTR4_NO_TRUNC))
		if (attr->no_trunc != true)
			return false;
        if (bitmap & (1ULL << FATTR4_TIME_DELTA))
		if ((attr->time_delta.seconds != 1) ||
		    (attr->time_delta.nseconds != 0))
			return false;

	/*
	 * per-object attributes
	 */
        if (bitmap & (1ULL << FATTR4_TYPE))
		if (attr->type != ino->type)
			return false;
        if (bitmap & (1ULL << FATTR4_CHANGE))
		if (attr->change != ino->version)
			return false;
        if (bitmap & (1ULL << FATTR4_SIZE))
		if (attr->size != ino->size)
			return false;
        if (bitmap & (1ULL << FATTR4_NAMED_ATTR))
		if (attr->named_attr != false)
			return false;
        if (bitmap & (1ULL << FATTR4_FSID))
		if ((attr->fsid.major != 1) || (attr->fsid.minor != 0))
			return false;
        if (bitmap & (1ULL << FATTR4_FILEHANDLE))
		if (attr->filehandle != ino->inum)
			return false;
        if (bitmap & (1ULL << FATTR4_FILEID))
		if (attr->fileid != ino->inum)
			return false;
        if (bitmap & (1ULL << FATTR4_NUMLINKS))
		if (attr->numlinks != ino->n_link)
			return false;
        if (bitmap & (1ULL << FATTR4_RAWDEV))
		if ((attr->rawdev.specdata1 != ino->devdata[0]) ||
		    (attr->rawdev.specdata2 != ino->devdata[1]))
			return false;
        if (bitmap & (1ULL << FATTR4_TIME_ACCESS))
		if ((attr->time_access.seconds != ino->atime) ||
		    (attr->time_access.nseconds != 0))
			return false;
        if (bitmap & (1ULL << FATTR4_TIME_CREATE))
		if ((attr->time_create.seconds != ino->ctime) ||
		    (attr->time_create.nseconds != 0))
			return false;
        if (bitmap & (1ULL << FATTR4_TIME_MODIFY))
		if ((attr->time_modify.seconds != ino->mtime) ||
		    (attr->time_modify.nseconds != 0))
			return false;
        if (bitmap & (1ULL << FATTR4_MOUNTED_ON_FILEID))
		if (attr->mounted_on_fileid != ino->inum)
			return false;

	return true;
}

nfsstat4 nfs_op_verify(struct nfs_cxn *cxn, const VERIFY4args *args,
		       struct list_head *writes, struct rpc_write **wr,
		       bool nverify)
{
	nfsstat4 status = NFS4_OK;
	struct nfs_inode *ino = NULL;
	struct nfs_fattr_set fattr;
	bool match;
	bool printed = false;

	status = copy_attr(&fattr, &args->obj_attributes);
	if (status != NFS4_OK)
		goto out;

	if (debugging) {
		printed = true;
		applog(LOG_DEBUG, "op %sVERIFY", nverify ? "N" : "");
		print_fattr("   [N]VERIFY", &fattr);
	}

	if ((fattr.bitmap & (1ULL << FATTR4_RDATTR_ERROR)) ||
	    (fattr.bitmap & fattr_write_only_mask)) {
		status = NFS4ERR_INVAL;
		goto out_free;
	}

	if (fattr.bitmap & (!(fattr_supported_mask))) {
		status = NFS4ERR_ATTRNOTSUPP;
		goto out_free;
	}

	ino = inode_fhdec(NULL, cxn->current_fh, 0);
	if (!ino) {
		status = NFS4ERR_NOFILEHANDLE;
		goto out_free;
	}

	match = inode_attr_cmp(ino, &fattr);

	if (nverify) {
		if (match)
			status = NFS4ERR_SAME;
	} else {
		if (!match)
			status = NFS4ERR_NOT_SAME;
	}

out_free:
	fattr_free(&fattr);
out:
	if (!printed && debugging)
		applog(LOG_DEBUG, "op %sVERIFY", nverify ? "N" : "");

	WR32(status);
	inode_free(ino);
	return status;
}

