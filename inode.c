
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <syslog.h>
#include <errno.h>
#include <glib.h>
#include "server.h"
#include "nfs4_prot.h"

static nfsino_t next_ino;		/* start next free-inode scan here */

struct nfs_inode *__inode_get(nfsino_t inum)
{
	struct nfs_inode *ino;

	if (!srv.inode_table || !inum || (inum >= srv.inode_table_len))
		return NULL;

	ino = srv.inode_table[inum];
	if (!ino || !ino->parents)
		return NULL;

	return ino;
}

struct nfs_inode *inode_get(nfsino_t inum)
{
	struct nfs_inode *ino = __inode_get(inum);
	if (!ino)
		return NULL;
	return ino;
}

void inode_touch(struct nfs_inode *ino)
{
	ino->version++;
	ino->mtime = current_time.tv_sec;
}

static void inode_free(struct nfs_inode *ino)
{
	GList *tmp;
	struct nfs_openfile *of, *iter;
	nfsino_t inum;

	if (!ino)
		return;

	inum = ino->inum;

	if (debugging > 1)
		syslog(LOG_DEBUG, "freeing inode %llu",
			(unsigned long long) inum);

	if (ino->parents)
		g_array_free(ino->parents, TRUE);
	else {
		syslog(LOG_ERR, "BUG: inode double-free: %llu",
			(unsigned long long) inum);
		return;
	}

	switch (ino->type) {
	case NF4DIR:
		if (ino->dir) {
			if (g_tree_nnodes(ino->dir) > 0)
				syslog(LOG_ERR,
					"BUG: freeing non-empty dir inode");
			g_tree_destroy(ino->dir);
		}
		break;
	case NF4LNK:
		free(ino->linktext);
		break;
	case NF4REG:
		tmp = ino->buf_list;
		while (tmp) {
			refbuf_unref(tmp->data);
			tmp = tmp->next;
		}
		g_list_free(ino->buf_list);
		break;
	default:
		/* do nothing */
		break;
	}

	free(ino->mimetype);
	free(ino->user);
	free(ino->group);

	list_for_each_entry_safe(of, iter, &ino->openfile_list, inode_node) {
		if (of->type == nst_open)
			openfile_trash(of, false);
	}

	memset(ino, 0, sizeof(*ino));

	/* restore the few fields whose values are important across uses */
	ino->inum = inum;
	INIT_LIST_HEAD(&ino->openfile_list);

	if (inum < next_ino)
		next_ino = inum;
}

static struct nfs_inode *inode_alloc(void)
{
	unsigned int i;
	struct nfs_inode *ino;

	if (next_ino <= INO_RESERVED_LAST)
		next_ino = INO_RESERVED_LAST + 1;

	for (i = MIN(next_ino, srv.inode_table_len);
	     i < srv.inode_table_len; i++) {
		ino = srv.inode_table[i];
		if (!ino)
			break;
		if (ino->parents)
			continue;

		return ino;
	}

	if (i == srv.inode_table_len) {
		unsigned int old_size = srv.inode_table_len;
		unsigned int new_size = old_size * 2;
		void *mem = realloc(srv.inode_table, 
				    new_size * sizeof(struct nfs_inode *));
		if (!mem)
			return NULL;

		srv.inode_table = mem;
		srv.inode_table_len = new_size;

		old_size *= sizeof(struct nfs_inode *);
		new_size *= sizeof(struct nfs_inode *);
		memset(mem + old_size, 0, new_size - old_size);
	}

	ino = calloc(1, sizeof(struct nfs_inode));
	if (!ino)
		return NULL;

	ino->inum = i;
	INIT_LIST_HEAD(&ino->openfile_list);

	srv.inode_table[i] = ino;

	next_ino = ino->inum + 1;

	return ino;
}

static struct nfs_inode *inode_new(struct nfs_cxn *cxn)
{
	struct nfs_inode *ino;

	ino = inode_alloc();
	if (!ino)
		goto out;

	ino->parents = g_array_new(false, false, sizeof(struct nfs_fh));
	if (!ino->parents)
		goto err_out;

	ino->version = 1ULL;
	ino->ctime =
	ino->atime =
	ino->mtime = current_time.tv_sec;
	ino->mode = MODE4_RUSR;

	/* connected users (cxn==NULL is internal allocation, e.g. root inode)*/
	if (cxn) {
		ino->user = cxn_getuser(cxn);
		ino->group = cxn_getgroup(cxn);
	}

out:
	return ino;

err_out:
	inode_free(ino);
	ino = NULL;
	goto out;
}

struct nfs_inode *inode_new_file(struct nfs_cxn *cxn)
{
	struct nfs_inode *ino = inode_new(cxn);
	if (!ino)
		return NULL;

	ino->type = NF4REG;
	ino->buf_list = NULL;

	return ino;
}

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

static struct nfs_inode *inode_new_dir(struct nfs_cxn *cxn)
{
	struct nfs_inode *ino = inode_new(cxn);
	if (!ino)
		return NULL;

	ino->type = NF4DIR;
	ino->size = 4096;

	ino->dir = g_tree_new(nfstr_compare);
	if (!ino->dir) {
		inode_free(ino);
		return NULL;
	}

	return ino;
}

static struct nfs_inode *inode_new_dev(struct nfs_cxn *cxn,
				enum nfs_ftype4 type, const uint32_t *devdata)
{
	struct nfs_inode *ino = inode_new(cxn);
	if (!ino)
		return NULL;

	ino->type = type;
	memcpy(&ino->devdata[0], devdata, sizeof(uint32_t) * 2);

	return ino;
}

static struct nfs_inode *inode_new_symlink(struct nfs_cxn *cxn, char *linktext)
{
	struct nfs_inode *ino = inode_new(cxn);
	if (!ino)
		return NULL;

	ino->type = NF4LNK;
	ino->linktext = linktext;

	return ino;
}

static nfsstat4 inode_new_type(struct nfs_cxn *cxn, uint32_t objtype,
			       const struct nfs_buf *linkdata,
			       const uint32_t *specdata,
			       struct nfs_inode **ino_out)
{
	struct nfs_inode *new_ino;
	nfsstat4 status;

	*ino_out = NULL;

	switch(objtype) {
	case NF4DIR:
		new_ino = inode_new_dir(cxn);
		break;
	case NF4BLK:
	case NF4CHR:
		new_ino = inode_new_dev(cxn, objtype, specdata);
		break;
	case NF4LNK: {
		char *linktext = strndup(linkdata->val, linkdata->len);
		if (!linktext) {
			status = NFS4ERR_RESOURCE;
			goto out;
		}
		new_ino = inode_new_symlink(cxn, linktext);
		if (!new_ino)
			free(linktext);
		break;
	}
	case NF4SOCK:
	case NF4FIFO:
		new_ino = inode_new(cxn);
		break;
	default:
		status = NFS4ERR_BADTYPE;
		goto out;
	}

	new_ino->type = objtype;

	if (!new_ino) {
		status = NFS4ERR_RESOURCE;
		goto out;
	}

	*ino_out = new_ino;
	status = NFS4_OK;

out:
	return status;
}

void inode_openfile_add(struct nfs_inode *ino, struct nfs_openfile *of)
{
	list_add(&of->inode_node, &ino->openfile_list);
}

bool inode_table_init(void)
{
	struct nfs_inode *root;

	srv.inode_table = calloc(SRV_INIT_INO, sizeof(struct nfs_inode *));
	if (!srv.inode_table)
		return false;

	srv.inode_table_len = SRV_INIT_INO;

	root = calloc(1, sizeof(struct nfs_inode));
	if (!root)
		return false;
	root->inum = INO_ROOT;

	INIT_LIST_HEAD(&root->openfile_list);

	srv.inode_table[INO_ROOT] = root;

	root->mode = 0755;

	root->type = NF4DIR;
	root->version = 1ULL;
	root->size = 4096ULL;
	root->ctime =
	root->atime =
	root->mtime = current_time.tv_sec;

	root->dir = g_tree_new(nfstr_compare);
	if (!root->dir)
		return false;

	root->parents = g_array_new(false, false, sizeof(struct nfs_fh));
	if (!root->parents)
		return false;

	root->user = id_lookup_name(idt_user, "root@localdomain", 4);
	if (!root->user)
		root->user = strdup("nobody@localdomain");

	root->group = id_lookup_name(idt_group, "root@localdomain", 4);
	if (!root->group)
		root->group = strdup("nobody@localdomain");

	return true;
}

void inode_unlink(struct nfs_inode *ino, nfsino_t dir_ref)
{
	unsigned int i;
	bool found = false;
	struct nfs_fh *fh;

	if (!ino || !ino->parents) {
		syslog(LOG_ERR, "BUG: null in inode_unlink");
		return;
	}

	for (i = 0; i < ino->parents->len; i++) {
		fh = &g_array_index(ino->parents, struct nfs_fh, i);
		if (fh->ino == dir_ref) {
			g_array_remove_index(ino->parents, i);
			inode_touch(ino);
			found = true;
			break;
		}
	}

	if (!found)
		syslog(LOG_ERR, "BUG: dir_ref %llu not found in inode_unlink",
			(unsigned long long) dir_ref);

	if (ino->parents->len == 0)
		inode_free(ino);
}

enum nfsstat4 inode_apply_attrs(struct nfs_inode *ino,
				const struct nfs_fattr_set *attr,
			        uint64_t *bitmap_set_out,
			        struct nfs_stateid *sid,
			        bool in_setattr)
{
	uint64_t bitmap_set = 0;
	enum nfsstat4 status = NFS4_OK;

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
		struct refbuf *rb;

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

		/* add zero-filled buffer */
		if (new_size > ino->size) {
			rb = refbuf_new(len, true);
			if (!rb) {
				status = NFS4ERR_NOSPC;
				goto out;
			}
			ino->buf_list = g_list_append(ino->buf_list, rb);
		}

		/* truncate buffers until we reach desired length */
		else {
			GList *ent, *tmp = g_list_last(ino->buf_list);
			while (len > 0) {
				ent = tmp;
				tmp = tmp->prev;

				rb = ent->data;
				if (rb->len <= len) {
					len -= rb->len;
					refbuf_unref(rb);
					ino->buf_list = g_list_delete_link(
						ino->buf_list, ent);
				} else {
					rb->len -= len;
					len = 0;
				}
			}
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
	}
	if ((attr->bitmap & (1ULL << FATTR4_OWNER)) &&
	    (attr->owner.len)) {
		char *ostr = id_lookup_name(idt_user, attr->owner.val,
					    attr->owner.len);
		if (!ostr) {
			status = NFS4ERR_DENIED;
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
			status = NFS4ERR_DENIED;
			goto out;
		}

		ino->group = ostr;

		bitmap_set |= (1ULL << FATTR4_OWNER_GROUP);
	}

out:
	if (in_setattr && bitmap_set)
		inode_touch(ino);

	*bitmap_set_out = bitmap_set;
	return status;
}

nfsstat4 inode_add(struct nfs_inode *dir_ino, struct nfs_inode *new_ino,
		   const struct nfs_fattr_set *attr, const struct nfs_buf *name,
		   uint64_t *attrset, change_info4 *cinfo)
{
	nfsstat4 status = NFS4_OK;
	struct nfs_fh fh;

	if (attr)
		status = inode_apply_attrs(new_ino, attr, attrset, NULL, false);
	if (status != NFS4_OK) {
		inode_free(new_ino);
		goto out;
	}

	cinfo->atomic = true;
	cinfo->before =
	cinfo->after = dir_ino->version;

	status = dir_add(dir_ino, name, new_ino);
	if (status != NFS4_OK) {
		inode_free(new_ino);
		goto out;
	}

	fh_set(&fh, dir_ino->inum);
	g_array_append_val(new_ino->parents, fh);

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
		syslog(LOG_INFO, "op CREATE (%s, '%.*s', %u %u)",
		       name_nfs_ftype4[objtype],
		       objname->len, objname->val,
		       specdata[0],
		       specdata[1]);
		break;
	case NF4LNK:
		syslog(LOG_INFO, "op CREATE (%s, '%.*s', '%.*s')",
		       name_nfs_ftype4[objtype],
		       objname->len, objname->val,
		       linkdata->len, linkdata->val);
		break;
	default:
		syslog(LOG_INFO, "op CREATE (%s, '%.*s')",
		       name_nfs_ftype4[objtype],
		       objname->len, objname->val);
		break;
	}

	print_fattr("op CREATE attr", attr);
}

nfsstat4 nfs_op_create(struct nfs_cxn *cxn, struct curbuf *cur,
		       struct list_head *writes, struct rpc_write **wr)
{
	nfsstat4 status;
	struct nfs_inode *dir_ino, *new_ino;
	uint32_t objtype, specdata[2] = { 0, };
	struct nfs_buf objname, linkdata = { 0, NULL };
	struct nfs_fattr_set attr;
	uint64_t attrset = 0;
	change_info4 cinfo;

	objtype = CR32();				/* type */
	if (objtype == NF4BLK || objtype == NF4CHR) {
		specdata[0] = CR32();			/* devdata */
		specdata[1] = CR32();
	} else if (objtype == NF4LNK)
		CURBUF(&linkdata);			/* linkdata */
	CURBUF(&objname);				/* objname */

	status = cur_readattr(cur, &attr);		/* createattrs */
	if (status != NFS4_OK)
		goto out;

	if (debugging)
		print_create_args(objtype, &objname, &linkdata,
				  specdata, &attr);

	status = dir_curfh(cxn, &dir_ino);
	if (status != NFS4_OK)
		goto err_out;

	if (dir_ino->type != NF4DIR) {
		status = NFS4ERR_NOTDIR;
		goto err_out;
	}

	status = inode_new_type(cxn, objtype, &linkdata, specdata, &new_ino);
	if (status != NFS4_OK)
		goto err_out;

	status = inode_add(dir_ino, new_ino, &attr,
			   &objname, &attrset, &cinfo);
	if (status != NFS4_OK)
		goto err_out;

	fh_set(&cxn->current_fh, new_ino->inum);

	if (debugging)
		syslog(LOG_INFO, "   CREATE -> %llu",
			(unsigned long long) cxn->current_fh.ino);

err_out:
	fattr_free(&attr);
out:
	WR32(status);
	if (status == NFS4_OK) {
		WR32(cinfo.atomic ? 1 : 0);
		WR64(cinfo.before);
		WR64(cinfo.after);
		WRMAP(attrset);
	}
	return status;
}

nfsstat4 nfs_op_getattr(struct nfs_cxn *cxn, struct curbuf *cur,
		        struct list_head *writes, struct rpc_write **wr)
{
	nfsstat4 status = NFS4_OK;
	struct nfs_inode *ino;
	struct nfs_fattr_set attrset;
	bool printed = false;
	uint32_t *status_p;
	uint64_t bitmap_out = 0;

	memset(&attrset, 0, sizeof(attrset));

	status_p = WRSKIP(4);		/* ending status */

	if (cur->len < 4) {
		status = NFS4ERR_BADXDR;
		goto out;
	}

	attrset.bitmap = CURMAP();

	ino = inode_fhget(cxn->current_fh);
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
		syslog(LOG_INFO, "op GETATTR");

	*status_p = htonl(status);
	return status;
}

nfsstat4 nfs_op_setattr(struct nfs_cxn *cxn, struct curbuf *cur,
		        struct list_head *writes, struct rpc_write **wr)
{
	struct nfs_inode *ino;
	nfsstat4 status = NFS4_OK;
	uint64_t bitmap_out = 0;
	struct nfs_stateid sid;
	struct nfs_fattr_set attr;

	if (cur->len < 20) {
		status = NFS4ERR_BADXDR;
		goto out;
	}

	CURSID(&sid);

	status = cur_readattr(cur, &attr);
	if (status != NFS4_OK) {
		if (attr.bitmap & fattr_read_only_mask)
			status = NFS4ERR_INVAL;
		goto out;
	}

	if (debugging) {
		syslog(LOG_INFO, "op SETATTR (ID:%x)", sid.id);
		print_fattr("   SETATTR", &attr);
	}

	ino = inode_fhget(cxn->current_fh);
	if (!ino) {
		status = NFS4ERR_NOFILEHANDLE;
		goto err_out;
	}

	status = inode_apply_attrs(ino, &attr, &bitmap_out, &sid, true);
	if (status != NFS4_OK)
		goto err_out;

	if (debugging)
		print_fattr_bitmap("   SETATTR result", bitmap_out);

err_out:
	fattr_free(&attr);
out:
	WR32(status);
	WRMAP(bitmap_out);
	return status;
}

unsigned int inode_access(const struct nfs_cxn *cxn,
			  const struct nfs_inode *ino, unsigned int req_access)
{
	unsigned int mode, rc;
	char *user, *group;
	bool user_mat, group_mat;
	bool root_user, root_group;

	user = cxn_getuser(cxn);
	group = cxn_getgroup(cxn);
	if (!user || !group) {
		if (debugging)
			syslog(LOG_INFO, "invalid cxn%s%s",
			       user ? "" : " user",
			       group ? "" : " group");
		return 0;
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

	return rc;
}

nfsstat4 nfs_op_access(struct nfs_cxn *cxn, struct curbuf *cur,
		       struct list_head *writes, struct rpc_write **wr)
{
	nfsstat4 status = NFS4_OK;
	struct nfs_inode *ino;
	uint32_t arg_access;
	ACCESS4resok resok;

	if (cur->len < sizeof(ACCESS4args)) {
		status = NFS4ERR_BADXDR;
		goto out;
	}

	arg_access = CR32();

	if (debugging)
		syslog(LOG_INFO, "op ACCESS (0x%x)", arg_access);

	ino = inode_fhget(cxn->current_fh);
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
		syslog(LOG_INFO, "   ACCESS -> (ACC:%x SUP:%x)",
		       resok.access,
		       resok.supported);

out:
	WR32(status);
	if (status == NFS4_OK) {
		WR32(resok.supported);
		WR32(resok.access);
	}
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
        if (bitmap & (1ULL << FATTR4_FILES_TOTAL))
		if (attr->files_total != srv.inode_table_len)
			return false;
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
		if (attr->numlinks != ino->parents->len)
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

nfsstat4 nfs_op_verify(struct nfs_cxn *cxn, struct curbuf *cur,
		       struct list_head *writes, struct rpc_write **wr,
		       bool nverify)
{
	nfsstat4 status = NFS4_OK;
	struct nfs_inode *ino;
	struct nfs_fattr_set fattr;
	bool match;
	bool printed = false;

	status = cur_readattr(cur, &fattr);
	if (status != NFS4_OK)
		goto out;

	if (debugging) {
		printed = true;
		syslog(LOG_DEBUG, "op %sVERIFY", nverify ? "N" : "");
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

	ino = inode_fhget(cxn->current_fh);
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
		syslog(LOG_DEBUG, "op %sVERIFY", nverify ? "N" : "");

	WR32(status);
	return status;
}

