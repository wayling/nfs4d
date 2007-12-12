#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <syslog.h>
#include <errno.h>
#include <glib.h>
#include "server.h"
#include "nfs4_prot.h"

nfsino_t next_ino = INO_RESERVED_LAST + 1;

static const struct idmap_ent {
	const char		*name;
	int			id;
} idmap[] = {
	{ "root@localdomain", 0 },
	{ "jgarzik@localdomain", 500 },

	{ }	/* terminate list */
};

static int srv_lookup_user(const utf8string *user)
{
	const struct idmap_ent *ent = &idmap[0];
	size_t len;

	while (ent->name) {
		len = strlen(ent->name);

		if ((user->utf8string_len == len) &&
	    	    (!memcmp(user->utf8string_val, ent->name, len)))
			return ent->id;

		ent++;
	}

	return -ENOENT;
}

static int srv_lookup_group(const utf8string *user)
{
	return srv_lookup_user(user);
}

#if 0
static const char *srv_lookup_id(int id)
{
	const struct idmap_ent *ent = &idmap[0];

	while (ent->name) {
		if (id == ent->id)
			return ent->name;

		ent++;
	}

	return NULL;
}
#endif

struct nfs_inode *inode_get(nfsino_t inum)
{
	g_assert(srv.inode_table != NULL);

	return g_hash_table_lookup(srv.inode_table, GUINT_TO_POINTER(inum));
}

void inode_touch(struct nfs_inode *ino)
{
	ino->version++;
	ino->mtime = current_time.tv_sec;
}

static void inode_free(struct nfs_inode *ino)
{
	g_array_free(ino->parents, TRUE);

	switch (ino->type) {
	case NF4DIR:
		if (ino->u.dir)
			g_hash_table_destroy(ino->u.dir);
		break;
	case NF4LNK:
		free(ino->u.linktext);
		break;
	default:
		/* do nothing */
		break;
	}

	if (ino->data) {
		free(ino->data);
		srv.space_used -= ino->size;
	}

	free(ino);
}

static struct nfs_inode *inode_new(struct nfs_cxn *cxn)
{
	struct nfs_inode *ino = calloc(1, sizeof(struct nfs_inode));
	if (!ino)
		goto out;

	ino->parents = g_array_new(FALSE, FALSE, sizeof(nfsino_t));
	if (!ino->parents)
		goto out_ino;

	ino->ino = next_ino++;

	ino->version = 1ULL;
	ino->ctime =
	ino->atime =
	ino->mtime = current_time.tv_sec;
	ino->mode = MODE4_RUSR;

	if (cxn) {
		ino->uid = cxn_getuid(cxn);
		ino->gid = cxn_getgid(cxn);
	}

	goto out;

out_ino:
	free(ino);
out:
	return ino;
}

struct nfs_inode *inode_new_file(struct nfs_cxn *cxn)
{
	struct nfs_inode *ino = inode_new(cxn);
	if (!ino)
		return NULL;

	ino->type = NF4REG;

	return ino;
}

static struct nfs_inode *inode_new_dir(struct nfs_cxn *cxn)
{
	struct nfs_inode *ino = inode_new(cxn);
	if (!ino)
		return NULL;

	ino->type = NF4DIR;

	ino->u.dir = g_hash_table_new_full(g_str_hash, g_str_equal,
					   free, dirent_free);
	if (!ino->u.dir) {
		inode_free(ino);
		return NULL;
	}

	return ino;
}

static struct nfs_inode *inode_new_dev(struct nfs_cxn *cxn,
				enum nfs_ftype4 type, specdata4 *devdata)
{
	struct nfs_inode *ino = inode_new(cxn);
	if (!ino)
		return NULL;

	ino->type = type;
	memcpy(&ino->u.devdata, devdata, sizeof(specdata4));

	return ino;
}

static struct nfs_inode *inode_new_symlink(struct nfs_cxn *cxn, gchar *linktext)
{
	struct nfs_inode *ino = inode_new(cxn);
	if (!ino)
		return NULL;

	ino->type = NF4LNK;
	ino->u.linktext = linktext;

	return ino;
}

static nfsstat4 inode_new_type(struct nfs_cxn *cxn, createtype4 *objtype,
			       struct nfs_inode **ino_out)
{
	struct nfs_inode *new_ino;
	nfsstat4 status;

	*ino_out = NULL;

	switch(objtype->type) {
	case NF4DIR:
		new_ino = inode_new_dir(cxn);
		break;
	case NF4BLK:
	case NF4CHR:
		new_ino = inode_new_dev(cxn, objtype->type,
				        &objtype->createtype4_u.devdata);
		break;
	case NF4LNK: {
		gchar *linktext =
			copy_utf8string(&objtype->createtype4_u.linkdata);
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

	new_ino->type = objtype->type;

	if (!new_ino) {
		status = NFS4ERR_RESOURCE;
		goto out;
	}

	*ino_out = new_ino;
	status = NFS4_OK;

out:
	return status;
}

bool_t inode_table_init(void)
{
	struct nfs_inode *root;

	srv.inode_table = g_hash_table_new(g_direct_hash, g_direct_equal);

	root = inode_new_dir(NULL);
	if (!root)
		return FALSE;
	root->ino = INO_ROOT;
	root->mode = 0755;
	root->uid = 0;
	root->gid = 0;
	root->size = 2;

	g_hash_table_insert(srv.inode_table, GUINT_TO_POINTER(INO_ROOT), root);

	return TRUE;
}

void inode_unlink(struct nfs_inode *ino, nfsino_t dir_ref)
{
	unsigned int i;

	for (i = 0; i < ino->parents->len; i++)
		if (g_array_index(ino->parents, nfsino_t, i) == dir_ref)
			break;

	if (i < ino->parents->len) {
		g_array_remove_index(ino->parents, i);
		inode_touch(ino);
	}

	if (ino->parents->len == 0) {
		g_hash_table_remove(srv.inode_table, GUINT_TO_POINTER(ino->ino));
		inode_free(ino);
	}
}

static enum nfsstat4 inode_apply_attrs(struct nfs_inode *ino, fattr4 *raw_attr,
				       uint64_t *bitmap_set_out,
				       struct nfs_stateid *sid,
				       gboolean in_setattr)
{
	struct nfs_fattr_set fattr;
	uint64_t bitmap_set = 0;
	enum nfsstat4 status = NFS4_OK;

	if (!fattr_decode(raw_attr, &fattr))
		return NFS4ERR_BADXDR;

	if (fattr.bitmap & fattr_read_only_mask) {
		status = NFS4ERR_INVAL;
		goto out;
	}
	if (fattr.bitmap & ~fattr_supported_mask) {
		status = NFS4ERR_ATTRNOTSUPP;
		goto out;
	}

	if (fattr.bitmap & (1ULL << FATTR4_SIZE)) {
		uint64_t new_size = fattr.size;
		void *mem;
		struct nfs_state *st = NULL;

		/* only permit size attribute manip on files */
		if (ino->type != NF4REG) {
			if (ino->type == NF4DIR)
				status = NFS4ERR_ISDIR;
			else
				status = NFS4ERR_INVAL;
			goto out;
		}

		if (sid && sid->seqid && (sid->seqid != 0xffffffffU)) {
			uint32_t id = GUINT32_FROM_LE(sid->id);

			status = stateid_lookup(id, &st);
			if (status != NFS4_OK)
				goto out;

			if (st->ino != ino->ino) {
				status = NFS4ERR_BAD_STATEID;
				goto out;
			}

			if (!(st->share_ac & OPEN4_SHARE_ACCESS_WRITE)) {
				status = NFS4ERR_OPENMODE;
				goto out;
			}
		}

		if (new_size == ino->size)
			goto size_done;

		mem = realloc(ino->data, new_size);
		if (!new_size) {
			ino->data = NULL;
			goto size_done;
		}
		if (!mem) {
			status = NFS4ERR_NOSPC;
			goto out;
		}

		ino->data = mem;

		if (new_size > ino->size) {
			uint64_t zero = new_size - ino->size;
			memset(ino->data + ino->size, 0, zero);
			srv.space_used += zero;
		} else {
			srv.space_used -= (ino->size - new_size);
		}

size_done:
		ino->size = new_size;
		bitmap_set |= (1ULL << FATTR4_SIZE);
	}

	if (fattr.bitmap & (1ULL << FATTR4_TIME_ACCESS_SET)) {
		if (fattr.time_access_set.settime4_u.time.nseconds > 999999999){
			status = NFS4ERR_INVAL;
			goto out;
		}

		if (fattr.time_access_set.set_it == SET_TO_CLIENT_TIME4)
			ino->atime =
			      fattr.time_access_set.settime4_u.time.seconds;
		else
			ino->atime = current_time.tv_sec;

		bitmap_set |= (1ULL << FATTR4_TIME_ACCESS_SET);
	}
	if (fattr.bitmap & (1ULL << FATTR4_TIME_MODIFY_SET)) {
		if (fattr.time_modify_set.settime4_u.time.nseconds > 999999999){
			status = NFS4ERR_INVAL;
			goto out;
		}

		if (fattr.time_modify_set.set_it == SET_TO_CLIENT_TIME4)
			ino->mtime =
			      fattr.time_modify_set.settime4_u.time.seconds;
		else
			ino->mtime = current_time.tv_sec;

		bitmap_set |= (1ULL << FATTR4_TIME_MODIFY_SET);
	}
	if (fattr.bitmap & (1ULL << FATTR4_MODE)) {
		if ((!fattr.mode) || (fattr.mode & ~MODE4_ALL)) {
			status = NFS4ERR_BADXDR;
			goto out;
		}
		ino->mode = fattr.mode;
		bitmap_set |= (1ULL << FATTR4_MODE);
	}
	if (fattr.bitmap & (1ULL << FATTR4_OWNER)) {
		int x = srv_lookup_user(&fattr.owner);
		if (x < 0) {
			if (debugging)
				syslog(LOG_INFO, "invalid OWNER attr: '%.*s'",
				       fattr.owner.utf8string_len,
				       fattr.owner.utf8string_val);
			status = NFS4ERR_INVAL;
			goto out;
		}

		ino->uid = x;
		bitmap_set |= (1ULL << FATTR4_OWNER);
	}
	if (fattr.bitmap & (1ULL << FATTR4_OWNER_GROUP)) {
		int x = srv_lookup_group(&fattr.owner_group);
		if (x < 0) {
			if (debugging)
				syslog(LOG_INFO, "invalid OWNER GROUP attr: '%.*s'",
				       fattr.owner_group.utf8string_len,
				       fattr.owner_group.utf8string_val);
			status = NFS4ERR_INVAL;
			goto out;
		}

		ino->gid = x;
		bitmap_set |= (1ULL << FATTR4_OWNER_GROUP);
	}

out:
	fattr_free(&fattr);

	if (in_setattr && bitmap_set)
		inode_touch(ino);

	*bitmap_set_out = bitmap_set;
	return status;
}

nfsstat4 inode_add(struct nfs_inode *dir_ino, struct nfs_inode *new_ino,
		   fattr4 *attr, utf8string *name, bitmap4 *attrset,
		   change_info4 *cinfo)
{
	uint64_t bitmap_set;
	nfsstat4 status;

	status = inode_apply_attrs(new_ino, attr, &bitmap_set, NULL, FALSE);
	if (status != NFS4_OK) {
		inode_free(new_ino);
		goto out;
	}

	if (set_bitmap(bitmap_set, attrset)) {
		inode_free(new_ino);
		status = NFS4ERR_RESOURCE;
		goto out;
	}

	g_hash_table_insert(srv.inode_table, GUINT_TO_POINTER(new_ino->ino),
			    new_ino);

	cinfo->atomic = TRUE;
	cinfo->before =
	cinfo->after = dir_ino->version;

	status = dir_add(dir_ino, name, new_ino->ino);
	if (status != NFS4_OK) {
		inode_unlink(new_ino, 0);
		free_bitmap(attrset);
		goto out;
	}

	g_array_append_val(new_ino->parents, dir_ino->ino);
	cinfo->after = dir_ino->version;

out:
	return status;
}

static void print_create_args(CREATE4args *arg)
{
	switch (arg->objtype.type) {
	case NF4BLK:
	case NF4CHR:
		syslog(LOG_INFO, "op CREATE (%s, '%.*s', %u %u)",
		       name_nfs_ftype4[arg->objtype.type],
		       arg->objname.utf8string_len,
		       arg->objname.utf8string_val,
		       arg->objtype.createtype4_u.devdata.specdata1,
		       arg->objtype.createtype4_u.devdata.specdata2);
		break;
	case NF4LNK:
		syslog(LOG_INFO, "op CREATE (%s, '%.*s', '%.*s')",
		       name_nfs_ftype4[arg->objtype.type],
		       arg->objname.utf8string_len,
		       arg->objname.utf8string_val,
		       arg->objtype.createtype4_u.linkdata.utf8string_len,
		       arg->objtype.createtype4_u.linkdata.utf8string_val);
		break;
	default:
		syslog(LOG_INFO, "op CREATE (%s, '%.*s')",
		       name_nfs_ftype4[arg->objtype.type],
		       arg->objname.utf8string_len,
		       arg->objname.utf8string_val);
		break;
	}

	print_fattr("op CREATE attr", &arg->createattrs);
}

bool_t nfs_op_create(struct nfs_cxn *cxn, CREATE4args *arg, COMPOUND4res *cres)
{
	struct nfs_resop4 resop;
	CREATE4res *res;
	CREATE4resok *resok;
	nfsstat4 status = NFS4_OK;
	struct nfs_inode *dir_ino, *new_ino;

	if (debugging)
		print_create_args(arg);

	memset(&resop, 0, sizeof(resop));
	resop.resop = OP_CREATE;
	res = &resop.nfs_resop4_u.opcreate;
	resok = &res->CREATE4res_u.resok4;

	status = dir_curfh(cxn, &dir_ino);
	if (status != NFS4_OK)
		goto out;

	if (dir_ino->type != NF4DIR) {
		status = NFS4ERR_NOTDIR;
		goto out;
	}

	status = inode_new_type(cxn, &arg->objtype, &new_ino);
	if (status != NFS4_OK)
		goto out;

	status = inode_add(dir_ino, new_ino, &arg->createattrs,
			   &arg->objname, &resok->attrset, &resok->cinfo);
	if (status != NFS4_OK)
		goto out;

	cxn->current_fh = new_ino->ino;

	if (debugging)
		syslog(LOG_INFO, "   CREATE -> %u", cxn->current_fh);

out:
	res->status = status;
	return push_resop(cres, &resop, status);
}

bool_t nfs_op_getattr(struct nfs_cxn *cxn, GETATTR4args *arg,
		      COMPOUND4res *cres)
{
	struct nfs_resop4 resop;
	GETATTR4res *res;
	GETATTR4resok *resok;
	nfsstat4 status = NFS4_OK;
	struct nfs_inode *ino;
	struct nfs_fattr_set attrset;
	gboolean printed = FALSE;

	memset(&resop, 0, sizeof(resop));
	resop.resop = OP_GETATTR;
	res = &resop.nfs_resop4_u.opgetattr;
	resok = &res->GETATTR4res_u.resok4;

	ino = inode_get(cxn->current_fh);
	if (!ino) {
		status = NFS4ERR_NOFILEHANDLE;
		goto out;
	}

	memset(&attrset, 0, sizeof(attrset));

	attrset.bitmap = get_bitmap(&arg->attr_request);

	if (debugging && (status == NFS4_OK)) {
		print_fattr_bitmap("op GETATTR", attrset.bitmap);
		printed = TRUE;
	}

	/* GETATTR not permitted to process write-only attrs */
	if (attrset.bitmap & ((1ULL << FATTR4_TIME_ACCESS_SET) |
			      (1ULL << FATTR4_TIME_MODIFY_SET))) {
		status = NFS4ERR_INVAL;
		goto out;
	}

	fattr_fill(ino, &attrset);

	if (!fattr_encode(&resok->obj_attributes, &attrset))
		status = NFS4ERR_IO;

	fattr_free(&attrset);

	if (debugging) {
		attrset.bitmap = get_bitmap(&resok->obj_attributes.attrmask);
		print_fattr_bitmap("   GETATTR ->", attrset.bitmap);
		printed = TRUE;
	}

out:
	if (debugging && !printed)
		syslog(LOG_INFO, "op GETATTR");

	res->status = status;
	return push_resop(cres, &resop, status);
}

bool_t nfs_op_setattr(struct nfs_cxn *cxn, SETATTR4args *arg,
		      COMPOUND4res *cres)
{
	struct nfs_resop4 resop;
	SETATTR4res *res;
	struct nfs_inode *ino;
	nfsstat4 status = NFS4_OK;
	uint64_t bitmap = get_bitmap(&arg->obj_attributes.attrmask);
	struct nfs_stateid *sid = (struct nfs_stateid *) &arg->stateid;
	uint32_t id = GUINT32_FROM_LE(sid->id);

	if (debugging) {
		syslog(LOG_INFO, "op SETATTR (ID:%x)", id);
		print_fattr_bitmap("   SETATTR", bitmap);
	}

	memset(&resop, 0, sizeof(resop));
	resop.resop = OP_SETATTR;
	res = &resop.nfs_resop4_u.opsetattr;

	ino = inode_get(cxn->current_fh);
	if (!ino) {
		status = NFS4ERR_NOFILEHANDLE;
		goto err_out;
	}

	bitmap = 0;
	status = inode_apply_attrs(ino, &arg->obj_attributes, &bitmap, sid,
				   TRUE);
	if (status != NFS4_OK)
		goto err_out;

	set_bitmap(bitmap, &res->attrsset);

	if (debugging)
		print_fattr_bitmap("   SETATTR result", bitmap);

out:
	res->status = status;
	return push_resop(cres, &resop, status);

err_out:
	free(res->attrsset.bitmap4_val);
	res->attrsset.bitmap4_val = NULL;
	res->attrsset.bitmap4_len = 0;
	goto out;
}

unsigned int inode_access(const struct nfs_cxn *cxn,
			  const struct nfs_inode *ino, unsigned int req_access)
{
	unsigned int mode, rc;
	int uid, gid;

	uid = cxn_getuid(cxn);
	gid = cxn_getgid(cxn);
	if ((uid < 0) || (gid < 0)) {
		if (debugging)
			syslog(LOG_INFO, "invalid cxn uid/gid (%d/%d)",
				uid, gid);
		return 0;
	}

	mode = ino->mode & 0x7;
	if (uid == ino->uid)
		mode |= (ino->mode >> 6) & 0x7;
	if (gid == ino->gid)
		mode |= (ino->mode >> 3) & 0x7;

	rc = 0;
	if (mode & MODE4_ROTH)
		rc |= ACCESS4_READ;
	else if ((mode & MODE4_XOTH) && (ino->type == NF4DIR))
		rc |= ACCESS4_LOOKUP;
	else if (mode & MODE4_WOTH)
		rc |= ACCESS4_MODIFY;
	else if (mode & MODE4_WOTH)
		rc |= ACCESS4_EXTEND;
	else if ((mode & MODE4_XOTH) && (ino->type != NF4DIR))
		rc |= ACCESS4_EXECUTE;

	rc &= req_access;

	/* FIXME: check ACCESS4_DELETE */

	return rc;
}

bool_t nfs_op_access(struct nfs_cxn *cxn, ACCESS4args *arg,
		     COMPOUND4res *cres)
{
	struct nfs_resop4 resop;
	ACCESS4res *res;
	ACCESS4resok *resok;
	nfsstat4 status = NFS4_OK;
	struct nfs_inode *ino;

	if (debugging)
		syslog(LOG_INFO, "op ACCESS (0x%x)", arg->access);

	memset(&resop, 0, sizeof(resop));
	resop.resop = OP_ACCESS;
	res = &resop.nfs_resop4_u.opaccess;
	resok = &res->ACCESS4res_u.resok4;

	ino = inode_get(cxn->current_fh);
	if (!ino) {
		status = NFS4ERR_NOFILEHANDLE;
		goto out;
	}

	resok->access = inode_access(cxn, ino, arg->access);
	resok->supported =
		ACCESS4_READ |
		ACCESS4_LOOKUP |
		ACCESS4_MODIFY |
		ACCESS4_EXTEND |
		/* ACCESS4_DELETE | */	/* FIXME */
		ACCESS4_EXECUTE;

	resok->supported &= resok->access;

	if (debugging)
		syslog(LOG_INFO, "   ACCESS -> (ACC:%x SUP:%x)",
		       resok->access,
		       resok->supported);

out:
	res->status = status;
	return push_resop(cres, &resop, status);
}

static bool_t inode_attr_cmp(const struct nfs_inode *ino,
			     const struct nfs_fattr_set *attr)
{
	guint64 bitmap = attr->bitmap;

	/*
	 * per-server attributes
	 */
        if (bitmap & (1ULL << FATTR4_LEASE_TIME))
		if (attr->lease_time != srv.lease_time)
			return FALSE;

	/*
	 * per-filesystem attributes
	 */
        if (bitmap & (1ULL << FATTR4_SUPPORTED_ATTRS)) {
		guint64 tmp = get_bitmap(&attr->supported_attrs);
		if (tmp != fattr_supported_mask)
			return FALSE;
	}
        if (bitmap & (1ULL << FATTR4_FH_EXPIRE_TYPE))
		if (attr->fh_expire_type != SRV_FH_EXP_TYPE)
			return FALSE;
        if (bitmap & (1ULL << FATTR4_LINK_SUPPORT))
		if (attr->link_support != TRUE)
			return FALSE;
        if (bitmap & (1ULL << FATTR4_SYMLINK_SUPPORT))
		if (attr->symlink_support != TRUE)
			return FALSE;
        if (bitmap & (1ULL << FATTR4_UNIQUE_HANDLES))
		if (attr->unique_handles != TRUE)
			return FALSE;
        if (bitmap & (1ULL << FATTR4_CANSETTIME))
		if (attr->cansettime != TRUE)
			return FALSE;
        if (bitmap & (1ULL << FATTR4_CASE_INSENSITIVE))
		if (attr->case_insensitive != FALSE)
			return FALSE;
        if (bitmap & (1ULL << FATTR4_CASE_PRESERVING))
		if (attr->case_preserving != TRUE)
			return FALSE;
        if (bitmap & (1ULL << FATTR4_FILES_TOTAL))
		if (attr->files_total != g_hash_table_size(srv.inode_table))
			return FALSE;
        if (bitmap & (1ULL << FATTR4_HOMOGENEOUS))
		if (attr->homogeneous != TRUE)
			return FALSE;
        if (bitmap & (1ULL << FATTR4_MAXFILESIZE))
		if (attr->maxfilesize != SRV_MAX_FILESIZE)
			return FALSE;
        if (bitmap & (1ULL << FATTR4_MAXLINK))
		if (attr->maxlink != SRV_MAX_LINK)
			return FALSE;
        if (bitmap & (1ULL << FATTR4_MAXNAME))
		if (attr->maxname != SRV_MAX_NAME)
			return FALSE;
        if (bitmap & (1ULL << FATTR4_MAXREAD))
		if (attr->maxread != SRV_MAX_READ)
			return FALSE;
        if (bitmap & (1ULL << FATTR4_MAXWRITE))
		if (attr->maxwrite != SRV_MAX_WRITE)
			return FALSE;
        if (bitmap & (1ULL << FATTR4_NO_TRUNC))
		if (attr->no_trunc != TRUE)
			return FALSE;
        if (bitmap & (1ULL << FATTR4_TIME_DELTA))
		if ((attr->time_delta.seconds != 1) ||
		    (attr->time_delta.nseconds != 0))
			return FALSE;

	/*
	 * per-object attributes
	 */
        if (bitmap & (1ULL << FATTR4_TYPE))
		if (attr->type != ino->type)
			return FALSE;
        if (bitmap & (1ULL << FATTR4_CHANGE))
		if (attr->change != ino->version)
			return FALSE;
        if (bitmap & (1ULL << FATTR4_SIZE))
		if (attr->size != ino->size)
			return FALSE;
        if (bitmap & (1ULL << FATTR4_NAMED_ATTR))
		if (attr->named_attr != FALSE)
			return FALSE;
        if (bitmap & (1ULL << FATTR4_FSID))
		if ((attr->fsid.major != 1) || (attr->fsid.minor != 0))
			return FALSE;
        if (bitmap & (1ULL << FATTR4_FILEHANDLE)) {
		nfsino_t fh = 0;
		if (nfs_fh_decode(&attr->filehandle, &fh) <= 0)
			return FALSE;
		if (fh != ino->ino)
			return FALSE;
	}
        if (bitmap & (1ULL << FATTR4_FILEID))
		if (attr->fileid != ino->ino)
			return FALSE;
        if (bitmap & (1ULL << FATTR4_NUMLINKS))
		if (attr->numlinks != ino->parents->len)
			return FALSE;
        if (bitmap & (1ULL << FATTR4_RAWDEV))
		if ((attr->rawdev.specdata1 != ino->u.devdata.specdata1) ||
		    (attr->rawdev.specdata2 != ino->u.devdata.specdata2))
			return FALSE;
        if (bitmap & (1ULL << FATTR4_TIME_ACCESS))
		if ((attr->time_access.seconds != ino->atime) ||
		    (attr->time_access.nseconds != 0))
			return FALSE;
        if (bitmap & (1ULL << FATTR4_TIME_CREATE))
		if ((attr->time_create.seconds != ino->ctime) ||
		    (attr->time_create.nseconds != 0))
			return FALSE;
        if (bitmap & (1ULL << FATTR4_TIME_MODIFY))
		if ((attr->time_modify.seconds != ino->mtime) ||
		    (attr->time_modify.nseconds != 0))
			return FALSE;
        if (bitmap & (1ULL << FATTR4_MOUNTED_ON_FILEID))
		if (attr->mounted_on_fileid != ino->ino)
			return FALSE;

	return TRUE;
}

bool_t nfs_op_verify(struct nfs_cxn *cxn, VERIFY4args *arg,
		     COMPOUND4res *cres, int nverify)
{
	struct nfs_resop4 resop;
	VERIFY4res *res;
	nfsstat4 status = NFS4_OK;
	struct nfs_inode *ino;
	struct nfs_fattr_set fattr;
	bool_t match;

	memset(&resop, 0, sizeof(resop));
	resop.resop = OP_VERIFY;
	res = &resop.nfs_resop4_u.opverify;

	if (!fattr_decode(&arg->obj_attributes, &fattr)) {
		status = NFS4ERR_BADXDR;
		goto out;
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

	ino = inode_get(cxn->current_fh);
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
	res->status = status;
	return push_resop(cres, &resop, status);
}

