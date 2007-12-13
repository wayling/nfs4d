/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <glib.h>
#include "nfs4_prot.h"
#include "server.h"

const char *name_nfs_ftype4[] = {
	[NF4REG] = "NF4REG",
	[NF4DIR] = "NF4DIR",
	[NF4BLK] = "NF4BLK",
	[NF4CHR] = "NF4CHR",
	[NF4LNK] = "NF4LNK",
	[NF4SOCK] = "NF4SOCK",
	[NF4FIFO] = "NF4FIFO",
	[NF4ATTRDIR] = "NF4ATTRDIR",
	[NF4NAMEDATTR] = "NF4NAMEDATTR",
};

static const char *name_nfs4status[] = {
	[NFS4_OK] = "NFS4_OK",
	[NFS4ERR_PERM] = "NFS4ERR_PERM",
	[NFS4ERR_NOENT] = "NFS4ERR_NOENT",
	[NFS4ERR_IO] = "NFS4ERR_IO",
	[NFS4ERR_NXIO] = "NFS4ERR_NXIO",
	[NFS4ERR_ACCESS] = "NFS4ERR_ACCESS",
	[NFS4ERR_EXIST] = "NFS4ERR_EXIST",
	[NFS4ERR_XDEV] = "NFS4ERR_XDEV",
	[NFS4ERR_NOTDIR] = "NFS4ERR_NOTDIR",
	[NFS4ERR_ISDIR] = "NFS4ERR_ISDIR",
	[NFS4ERR_INVAL] = "NFS4ERR_INVAL",
	[NFS4ERR_FBIG] = "NFS4ERR_FBIG",
	[NFS4ERR_NOSPC] = "NFS4ERR_NOSPC",
	[NFS4ERR_ROFS] = "NFS4ERR_ROFS",
	[NFS4ERR_MLINK] = "NFS4ERR_MLINK",
	[NFS4ERR_NAMETOOLONG] = "NFS4ERR_NAMETOOLONG",
	[NFS4ERR_NOTEMPTY] = "NFS4ERR_NOTEMPTY",
	[NFS4ERR_DQUOT] = "NFS4ERR_DQUOT",
	[NFS4ERR_STALE] = "NFS4ERR_STALE",
	[NFS4ERR_BADHANDLE] = "NFS4ERR_BADHANDLE",
	[NFS4ERR_BAD_COOKIE] = "NFS4ERR_BAD_COOKIE",
	[NFS4ERR_NOTSUPP] = "NFS4ERR_NOTSUPP",
	[NFS4ERR_TOOSMALL] = "NFS4ERR_TOOSMALL",
	[NFS4ERR_SERVERFAULT] = "NFS4ERR_SERVERFAULT",
	[NFS4ERR_BADTYPE] = "NFS4ERR_BADTYPE",
	[NFS4ERR_DELAY] = "NFS4ERR_DELAY",
	[NFS4ERR_SAME] = "NFS4ERR_SAME",
	[NFS4ERR_DENIED] = "NFS4ERR_DENIED",
	[NFS4ERR_EXPIRED] = "NFS4ERR_EXPIRED",
	[NFS4ERR_LOCKED] = "NFS4ERR_LOCKED",
	[NFS4ERR_GRACE] = "NFS4ERR_GRACE",
	[NFS4ERR_FHEXPIRED] = "NFS4ERR_FHEXPIRED",
	[NFS4ERR_SHARE_DENIED] = "NFS4ERR_SHARE_DENIED",
	[NFS4ERR_WRONGSEC] = "NFS4ERR_WRONGSEC",
	[NFS4ERR_CLID_INUSE] = "NFS4ERR_CLID_INUSE",
	[NFS4ERR_RESOURCE] = "NFS4ERR_RESOURCE",
	[NFS4ERR_MOVED] = "NFS4ERR_MOVED",
	[NFS4ERR_NOFILEHANDLE] = "NFS4ERR_NOFILEHANDLE",
	[NFS4ERR_MINOR_VERS_MISMATCH] = "NFS4ERR_MINOR_VERS_MISMATCH",
	[NFS4ERR_STALE_CLIENTID] = "NFS4ERR_STALE_CLIENTID",
	[NFS4ERR_STALE_STATEID] = "NFS4ERR_STALE_STATEID",
	[NFS4ERR_OLD_STATEID] = "NFS4ERR_OLD_STATEID",
	[NFS4ERR_BAD_STATEID] = "NFS4ERR_BAD_STATEID",
	[NFS4ERR_BAD_SEQID] = "NFS4ERR_BAD_SEQID",
	[NFS4ERR_NOT_SAME] = "NFS4ERR_NOT_SAME",
	[NFS4ERR_LOCK_RANGE] = "NFS4ERR_LOCK_RANGE",
	[NFS4ERR_SYMLINK] = "NFS4ERR_SYMLINK",
	[NFS4ERR_RESTOREFH] = "NFS4ERR_RESTOREFH",
	[NFS4ERR_LEASE_MOVED] = "NFS4ERR_LEASE_MOVED",
	[NFS4ERR_ATTRNOTSUPP] = "NFS4ERR_ATTRNOTSUPP",
	[NFS4ERR_NO_GRACE] = "NFS4ERR_NO_GRACE",
	[NFS4ERR_RECLAIM_BAD] = "NFS4ERR_RECLAIM_BAD",
	[NFS4ERR_RECLAIM_CONFLICT] = "NFS4ERR_RECLAIM_CONFLICT",
	[NFS4ERR_BADXDR] = "NFS4ERR_BADXDR",
	[NFS4ERR_LOCKS_HELD] = "NFS4ERR_LOCKS_HELD",
	[NFS4ERR_OPENMODE] = "NFS4ERR_OPENMODE",
	[NFS4ERR_BADOWNER] = "NFS4ERR_BADOWNER",
	[NFS4ERR_BADCHAR] = "NFS4ERR_BADCHAR",
	[NFS4ERR_BADNAME] = "NFS4ERR_BADNAME",
	[NFS4ERR_BAD_RANGE] = "NFS4ERR_BAD_RANGE",
	[NFS4ERR_LOCK_NOTSUPP] = "NFS4ERR_LOCK_NOTSUPP",
	[NFS4ERR_OP_ILLEGAL] = "NFS4ERR_OP_ILLEGAL",
	[NFS4ERR_DEADLOCK] = "NFS4ERR_DEADLOCK",
	[NFS4ERR_FILE_OPEN] = "NFS4ERR_FILE_OPEN",
	[NFS4ERR_ADMIN_REVOKED] = "NFS4ERR_ADMIN_REVOKED",
	[NFS4ERR_CB_PATH_DOWN] = "NFS4ERR_CB_PATH_DOWN",
};

bool valid_utf8string(utf8string *str)
{
	if (!str || !str->utf8string_len || !str->utf8string_val)
		return false;
	if (!g_utf8_validate(str->utf8string_val, str->utf8string_len, NULL))
		return false;
	return true;
}

char *copy_utf8string(utf8string *str)
{
	return strndup(str->utf8string_val, str->utf8string_len);
}

uint64_t get_bitmap(const bitmap4 *map)
{
	uint64_t v = 0;

	if (!map || !map->bitmap4_val || !map->bitmap4_len)
		goto out;

	if (map->bitmap4_len > 0)
		v |= map->bitmap4_val[0];
	if (map->bitmap4_len > 1)
		v |= ((uint64_t)map->bitmap4_val[1]) << 32;

out:
	return v;
}

void __set_bitmap(uint64_t map_in, bitmap4 *map_out)
{
	map_out->bitmap4_len = 2;
	map_out->bitmap4_val[0] = map_in;
	map_out->bitmap4_val[1] = (map_in >> 32);
}

int set_bitmap(uint64_t map_in, bitmap4 *map_out)
{
	map_out->bitmap4_val = calloc(2, sizeof(uint32_t));
	if (!map_out->bitmap4_val) {
		map_out->bitmap4_len = 0;
		return -1;
	}

	__set_bitmap(map_in, map_out);

	return 0;
}

int cxn_getuid(const struct nfs_cxn *cxn)
{
	switch (cxn->auth.type) {
	case auth_none:
		return SRV_UID_NOBODY;
	case auth_unix:
		return cxn->auth.u.up.uid;
		break;
	}

	return -EINVAL;
}

int cxn_getgid(const struct nfs_cxn *cxn)
{
	switch (cxn->auth.type) {
	case auth_none:
		return SRV_GID_NOBODY;
	case auth_unix:
		return cxn->auth.u.up.gid;
		break;
	}

	return -EINVAL;
}

static nfsstat4 cli_init(struct opaque_auth *cred, struct opaque_auth *verf,
			 struct nfs_cxn **cxn_out)
{
	struct nfs_cxn *cxn = calloc(1, sizeof(struct nfs_cxn));
	nfsstat4 status = NFS4_OK;
	uint32_t *p, v, ql;

	if (!cxn) {
		status = NFS4ERR_RESOURCE;
		goto out;
	}

	switch (cred->oa_flavor) {
	case AUTH_NONE:
		syslog(LOG_INFO, "AUTH_NONE");
		cxn->auth.type = auth_none;

		if (debugging)
			syslog(LOG_INFO, "RPC CRED None (len %d)",
				cred->oa_length);
		break;

	case AUTH_SYS:
		if (!cred->oa_base || (cred->oa_length < 16)) {
			syslog(LOG_INFO, "AUTH_SYS null");
			status = NFS4ERR_DENIED;
			goto err_out;
		}

		p = (uint32_t *) cred->oa_base;

		p++;					/* stamp */
		v = ntohl(*p++);			/* machinename len */

		ql = XDR_QUADLEN(v);
		if (cred->oa_length < ((ql + 4) * 4)) {
			syslog(LOG_INFO, "AUTH_SYS null");
			status = NFS4ERR_DENIED;
			goto err_out;
		}

		p += ql;				/* machinename */
		cxn->auth.u.up.uid = ntohl(*p++);	/* uid */
		cxn->auth.u.up.gid = ntohl(*p++);	/* gid */

		/* we ignore the list of gids that follow */

		cxn->auth.type = auth_unix;

		if (debugging)
			syslog(LOG_INFO, "RPC CRED Unix (uid %d gid %d len %d)",
				cxn->auth.u.up.uid,
				cxn->auth.u.up.gid,
				cred->oa_length);
		break;

	default:
		syslog(LOG_INFO, "AUTH unknown");
		status = NFS4ERR_DENIED;
		goto err_out;
	}

out:
	*cxn_out = cxn;
	return status;

err_out:
	free(cxn);
	goto out;
}

bool push_resop(COMPOUND4res *res, const nfs_resop4 *resop, nfsstat4 stat)
{
	void *mem;
	u_int array_len = res->resarray.resarray_len;

	mem = realloc(res->resarray.resarray_val,
		((array_len + 1) * sizeof(nfs_resop4)));
	if (!mem)
		return false;

	res->resarray.resarray_len++;
	res->resarray.resarray_val = mem;
	memcpy(&res->resarray.resarray_val[array_len], resop,
	       sizeof(struct nfs_resop4));
	res->status = stat;

	return stat == NFS4_OK ? true : false;
}

static nfsstat4 nfs_op_readlink(struct nfs_cxn *cxn, struct curbuf *cur,
		       struct list_head *writes, struct rpc_write **wr)
{
	nfsstat4 status = NFS4_OK;
	struct nfs_inode *ino;
	char *linktext;

	if (debugging)
		syslog(LOG_INFO, "op READLINK");

	ino = inode_get(cxn->current_fh);
	if (!ino) {
		status = NFS4ERR_NOFILEHANDLE;
		goto out;
	}
	if (ino->type != NF4LNK) {
		status = NFS4ERR_INVAL;
		goto out;
	}

	linktext = ino->u.linktext;

	if (debugging)
		syslog(LOG_INFO, "   READLINK -> '%s'", linktext);

out:
	WR32(status);
	if (status == NFS4_OK)
		WRSTR(linktext);
	return status;
}

static bool nfs_op_secinfo(struct nfs_cxn *cxn, SECINFO4args *arg, COMPOUND4res *cres)
{
	struct nfs_resop4 resop;
	SECINFO4res *res;
	SECINFO4resok *resok;
	nfsstat4 status = NFS4_OK;
	secinfo4 *val;
	bool printed = false;

	memset(&resop, 0, sizeof(resop));
	resop.resop = OP_SECINFO;
	res = &resop.nfs_resop4_u.opsecinfo;
	resok = &res->SECINFO4res_u.resok4;

	if (!cxn->current_fh) {
		status = NFS4ERR_NOFILEHANDLE;
		goto out;
	}

	val = calloc(1, sizeof(secinfo4));
	if (!val) {
		status = NFS4ERR_RESOURCE;
		goto out;
	}

	switch (cxn->auth.type) {
	case auth_none:
		val->flavor = AUTH_NONE;
		break;
	case auth_unix:
		val->flavor = AUTH_SYS;
		break;
	}

	resok->SECINFO4resok_len = 1;
	resok->SECINFO4resok_val = val;

	if (debugging) {
		syslog(LOG_INFO, "op SECINFO -> AUTH_%s",
		       (val->flavor == AUTH_NONE) ? "NONE" : "SYS");
		printed = true;
	}

out:
	if (!printed) {
		if (debugging)
			syslog(LOG_INFO, "op SECINFO");
	}

	res->status = status;
	return push_resop(cres, &resop, status);
}

static const char *arg_str[] = {
	"<n/a>",
	"<n/a>",
	"<n/a>",
	"ACCESS",
	"CLOSE",
	"COMMIT",
	"CREATE",
	"DELEGPURGE",
	"DELEGRETURN",
	"GETATTR",
	"GETFH",
	"LINK",
	"LOCK",
	"LOCKT",
	"LOCKU",
	"LOOKUP",
	"LOOKUPP",
	"NVERIFY",
	"OPEN",
	"OPENATTR",
	"OPEN_CONFIRM",
	"OPEN_DOWNGRADE",
	"PUTFH",
	"PUTPUBFH",
	"PUTROOTFH",
	"READ",
	"READDIR",
	"READLINK",
	"REMOVE",
	"RENAME",
	"RENEW",
	"RESTOREFH",
	"SAVEFH",
	"SECINFO",
	"SETATTR",
	"SETCLIENTID",
	"SETCLIENTID_CONFIRM",
	"VERIFY",
	"WRITE",
	"RELEASE_LOCKOWNER",
};

static nfsstat4 nfs_op(struct nfs_cxn *cxn, struct curbuf *cur,
		       struct list_head *writes, struct rpc_write **wr)
{
	uint32_t op;

	if (cur->len < 4)
		return NFS4ERR_BADXDR;

	op = CUR32();			/* read argop */
	WR32(op);			/* write resop */

	switch (op) {
	case OP_ACCESS:
		return nfs_op_access(cxn, cur, writes, wr);
	case OP_READLINK:
		return nfs_op_readlink(cxn, cur, writes, wr);

#if 0
	case OP_CLOSE:
		return nfs_op_close(cxn, &arg->nfs_argop4_u.opclose, res);
	case OP_COMMIT:
		return nfs_op_commit(cxn, &arg->nfs_argop4_u.opcommit, res);
	case OP_CREATE:
		return nfs_op_create(cxn, &arg->nfs_argop4_u.opcreate, res);
	case OP_GETATTR:
		return nfs_op_getattr(cxn, &arg->nfs_argop4_u.opgetattr, res);
	case OP_GETFH:
		return nfs_op_getfh(cxn, res);
	case OP_LINK:
		return nfs_op_link(cxn, &arg->nfs_argop4_u.oplink, res);
	case OP_LOCK:
		return nfs_op_lock(cxn, &arg->nfs_argop4_u.oplock, res);
	case OP_LOCKT:
		return nfs_op_testlock(cxn, &arg->nfs_argop4_u.oplockt, res);
	case OP_LOCKU:
		return nfs_op_unlock(cxn, &arg->nfs_argop4_u.oplocku, res);
	case OP_LOOKUP:
		return nfs_op_lookup(cxn, &arg->nfs_argop4_u.oplookup, res);
	case OP_LOOKUPP:
		return nfs_op_lookupp(cxn, res);
	case OP_NVERIFY:
		return nfs_op_verify(cxn,
				(VERIFY4args *) &arg->nfs_argop4_u.opnverify,
				res, 1);
	case OP_OPEN:
		return nfs_op_open(cxn, &arg->nfs_argop4_u.opopen, res);
	case OP_OPEN_CONFIRM:
		return nfs_op_open_confirm(cxn, &arg->nfs_argop4_u.opopen_confirm, res);
	case OP_OPEN_DOWNGRADE:
		return nfs_op_open_downgrade(cxn, &arg->nfs_argop4_u.opopen_downgrade, res);
	case OP_PUTFH:
		return nfs_op_putfh(cxn, &arg->nfs_argop4_u.opputfh, res);
	case OP_PUTPUBFH:
		return nfs_op_putpubfh(cxn, res);
	case OP_PUTROOTFH:
		return nfs_op_putrootfh(cxn, res);
	case OP_READ:
		return nfs_op_read(cxn, &arg->nfs_argop4_u.opread, res);
	case OP_READDIR:
		return nfs_op_readdir(cxn, &arg->nfs_argop4_u.opreaddir, res);
	case OP_REMOVE:
		return nfs_op_remove(cxn, &arg->nfs_argop4_u.opremove, res);
	case OP_RENAME:
		return nfs_op_rename(cxn, &arg->nfs_argop4_u.oprename, res);
	case OP_RESTOREFH:
		return nfs_op_restorefh(cxn, res);
	case OP_SAVEFH:
		return nfs_op_savefh(cxn, res);
	case OP_SECINFO:
		return nfs_op_secinfo(cxn, &arg->nfs_argop4_u.opsecinfo, res);
	case OP_SETATTR:
		return nfs_op_setattr(cxn, &arg->nfs_argop4_u.opsetattr, res);
	case OP_SETCLIENTID:
		return nfs_op_setclientid(cxn,
					&arg->nfs_argop4_u.opsetclientid, res);
	case OP_SETCLIENTID_CONFIRM:
		return nfs_op_setclientid_confirm(cxn,
				&arg->nfs_argop4_u.opsetclientid_confirm, res);
	case OP_WRITE:
		return nfs_op_write(cxn, &arg->nfs_argop4_u.opwrite, res);
	case OP_VERIFY:
		return nfs_op_verify(cxn, &arg->nfs_argop4_u.opverify, res, 0);
#endif

	case OP_DELEGPURGE:
	case OP_DELEGRETURN:
	case OP_RENEW:
	case OP_RELEASE_LOCKOWNER:
	case OP_OPENATTR:
		if (debugging)
			syslog(LOG_INFO, "compound op %s",
			       (op > 39) ?  "<n/a>" : arg_str[op]);

		WR32(NFS4ERR_NOTSUPP);		/* op status */
		return NFS4ERR_NOTSUPP;		/* compound status */

	default:
		WR32(NFS4ERR_OP_ILLEGAL);	/* op status */
		return NFS4ERR_OP_ILLEGAL;	/* compound status */
	}

	return NFS4ERR_INVAL;	/* never reached */
}

static void nfs_free(nfs_resop4 *res)
{
	if (!res)
		return;

	/* FIXME: need OPEN, others here too */

	switch(res->resop) {
	case OP_CREATE:
		free(res->nfs_resop4_u.opcreate.CREATE4res_u.resok4.attrset.bitmap4_val);
		break;
	case OP_GETATTR:
		fattr4_free(&res->nfs_resop4_u.opgetattr.GETATTR4res_u.resok4.obj_attributes);
		break;
	case OP_GETFH:
		nfs_getfh_free(&res->nfs_resop4_u.opgetfh);
		break;

	case OP_LOCK:
	case OP_LOCKT:
		/* FIXME */
		break;

	case OP_OPEN:
		free(res->nfs_resop4_u.opopen.OPEN4res_u.resok4.attrset.bitmap4_val);
		break;
	case OP_READ:
		free(res->nfs_resop4_u.opread.READ4res_u.resok4.data.data_val);
		break;
	case OP_READDIR:
		nfs_readdir_free(&res->nfs_resop4_u.opreaddir);
		break;
	case OP_READLINK:
		free(res->nfs_resop4_u.opreadlink.READLINK4res_u.resok4.link.utf8string_val);
		break;
	case OP_SECINFO:
		free(res->nfs_resop4_u.opsecinfo.SECINFO4res_u.resok4.SECINFO4resok_val);
		break;
	case OP_SETATTR:
		free(res->nfs_resop4_u.opsetattr.attrsset.bitmap4_val);
		break;
	default:
		/* nothing to free */
		break;
	}
}

int nfs4_program_4_freeresult (SVCXPRT *transp, xdrproc_t xdr_result,
			       COMPOUND4res *res)
{
	unsigned int i;

	if (!res)
		goto out;

	for (i = 0; i < res->resarray.resarray_len; i++)
		nfs_free(&res->resarray.resarray_val[i]);

	free(res->resarray.resarray_val);

out:
	return true;
}

void nfsproc_null(struct opaque_auth *cred, struct opaque_auth *verf,
		  struct curbuf *cur, struct list_head *writes,
		  struct rpc_write **wr)
{
	/* FIXME */
}

static void nfsxdr_buf(struct curbuf *cur, struct nfs_buf *nb)
{
	nb->len = CUR32();
	nb->val = CURMEM(nb->len);
}

void nfsproc_compound(struct opaque_auth *cred, struct opaque_auth *verf,
		      struct curbuf *cur, struct list_head *writes,
		      struct rpc_write **wr)
{
	struct nfs_buf tag;
	uint32_t *stat_p, *result_p, n_args, minor;
	nfsstat4 status = NFS4_OK;
	unsigned int i = 0, results = 0;
	struct nfs_cxn *cxn;

	nfsxdr_buf(cur, &tag);		/* COMPOUND tag */
	minor = CUR32();		/* minor version */
	n_args = CUR32();		/* arg array size */

	stat_p = WRSKIP(4);		/* COMPOUND result status */
	WRBUF(&tag);			/* tag */
	result_p = WRSKIP(4);		/* result array size */

	if (!g_utf8_validate(tag.val, tag.len, NULL)) {
		status = NFS4ERR_INVAL;
		goto out;
	}
	if (minor != 0) {
		status = NFS4ERR_MINOR_VERS_MISMATCH;
		goto out;
	}

	status = cli_init(cred, verf, &cxn);
	if (status != NFS4_OK)
		goto out;

	/* honestly, this was put here more to shortcut a
	 * pathological case in pynfs.  we don't really have
	 * any inherent limits here.
	 */
	if (n_args > SRV_MAX_COMPOUND) {
		status = NFS4ERR_RESOURCE;
		goto out;
	}

	for (i = 0; i < n_args; i++) {
		results++;	/* even failed operations have results */

		status = nfs_op(cxn, cur, writes, wr);
		if (status != NFS4_OK) {
			if (debugging)
				syslog(LOG_WARNING, "compound failed (%s)",
					name_nfs4status[status]);
			break;
		}
	}

out:
	if (debugging || (i > 500))
		syslog(LOG_INFO, "arg list end (%u of %u args)",
		       (i == 0 || i == n_args) ? i : i + 1,
		       n_args);

	free(cxn);
	*stat_p = status;
	*result_p = results;
}

