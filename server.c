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

bool valid_utf8string(const struct nfs_buf *str)
{
	if (!str || !str->len || !str->val)
		return false;
	if (!g_utf8_validate(str->val, str->len, NULL))
		return false;
	return true;
}

char *copy_utf8string(const struct nfs_buf *str)
{
	return strndup(str->val, str->len);
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

static nfsstat4 nfs_op_secinfo(struct nfs_cxn *cxn, struct curbuf *cur,
		       struct list_head *writes, struct rpc_write **wr)
{
	nfsstat4 status = NFS4_OK;
	bool printed = false;
	struct nfs_buf name;
	uint32_t flavor = 0;

	CURBUF(&name);				/* component name */

	if (!cxn->current_fh) {
		status = NFS4ERR_NOFILEHANDLE;
		goto out;
	}

	switch (cxn->auth.type) {
	case auth_none:
		flavor = AUTH_NONE;
		break;
	case auth_unix:
		flavor = AUTH_SYS;
		break;
	}

	if (debugging) {
		syslog(LOG_INFO, "op SECINFO -> AUTH_%s",
		       (flavor == AUTH_NONE) ? "NONE" : "SYS");
		printed = true;
	}

out:
	if (!printed) {
		if (debugging)
			syslog(LOG_INFO, "op SECINFO");
	}

	WR32(status);
	if (status == NFS4_OK) {
		WR32(1);		/* secinfo array size */
		WR32(flavor);		/* secinfo flavor */
	}
	return status;
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

	op = CR32();			/* read argop */
	WR32(op);			/* write resop */

	switch (op) {
	case OP_ACCESS:
		return nfs_op_access(cxn, cur, writes, wr);
	case OP_CLOSE:
		return nfs_op_close(cxn, cur, writes, wr);
	case OP_COMMIT:
		return nfs_op_commit(cxn, cur, writes, wr);
	case OP_CREATE:
		return nfs_op_create(cxn, cur, writes, wr);
	case OP_GETATTR:
		return nfs_op_getattr(cxn, cur, writes, wr);
	case OP_GETFH:
		return nfs_op_getfh(cxn, cur, writes, wr);
	case OP_LINK:
		return nfs_op_link(cxn, cur, writes, wr);
	case OP_LOCK:
		return nfs_op_lock(cxn, cur, writes, wr);
	case OP_LOCKT:
		return nfs_op_testlock(cxn, cur, writes, wr);
	case OP_LOCKU:
		return nfs_op_unlock(cxn, cur, writes, wr);
	case OP_LOOKUP:
		return nfs_op_lookup(cxn, cur, writes, wr);
	case OP_LOOKUPP:
		return nfs_op_lookupp(cxn, cur, writes, wr);
	case OP_NVERIFY:
		return nfs_op_verify(cxn, cur, writes, wr, true);
	case OP_OPEN:
		return nfs_op_open(cxn, cur, writes, wr);
	case OP_OPEN_CONFIRM:
		return nfs_op_open_confirm(cxn, cur, writes, wr);
	case OP_OPEN_DOWNGRADE:
		return nfs_op_open_downgrade(cxn, cur, writes, wr);
	case OP_PUTFH:
		return nfs_op_putfh(cxn, cur, writes, wr);
	case OP_PUTPUBFH:
		return nfs_op_putpubfh(cxn, cur, writes, wr);
	case OP_PUTROOTFH:
		return nfs_op_putrootfh(cxn, cur, writes, wr);
	case OP_READ:
		return nfs_op_read(cxn, cur, writes, wr);
	case OP_READDIR:
		return nfs_op_readdir(cxn, cur, writes, wr);
	case OP_READLINK:
		return nfs_op_readlink(cxn, cur, writes, wr);
	case OP_REMOVE:
		return nfs_op_remove(cxn, cur, writes, wr);
	case OP_RENAME:
		return nfs_op_rename(cxn, cur, writes, wr);
	case OP_RESTOREFH:
		return nfs_op_restorefh(cxn, cur, writes, wr);
	case OP_SAVEFH:
		return nfs_op_savefh(cxn, cur, writes, wr);
	case OP_SECINFO:
		return nfs_op_secinfo(cxn, cur, writes, wr);
	case OP_SETATTR:
		return nfs_op_setattr(cxn, cur, writes, wr);
	case OP_SETCLIENTID:
		return nfs_op_setclientid(cxn, cur, writes, wr);
	case OP_SETCLIENTID_CONFIRM:
		return nfs_op_setclientid_confirm(cxn, cur, writes, wr);
	case OP_VERIFY:
		return nfs_op_verify(cxn, cur, writes, wr, false);
	case OP_WRITE:
		return nfs_op_write(cxn, cur, writes, wr);

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

void nfsproc_null(struct opaque_auth *cred, struct opaque_auth *verf,
		  struct curbuf *cur, struct list_head *writes,
		  struct rpc_write **wr)
{
	if (debugging)
		syslog(LOG_ERR, "NULL proc invoked");
}

void nfsproc_compound(struct opaque_auth *cred, struct opaque_auth *verf,
		      struct curbuf *cur, struct list_head *writes,
		      struct rpc_write **wr)
{
	struct nfs_buf tag;
	uint32_t *stat_p, *result_p, n_args, minor;
	nfsstat4 status = NFS4_OK;
	unsigned int i = 0, results = 0;
	struct nfs_cxn *cxn = NULL;

	CURBUF(&tag);			/* COMPOUND tag */
	minor = CR32();			/* minor version */
	n_args = CR32();		/* arg array size */

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
		syslog(LOG_INFO, "arg list end (%u of %u args, %u results)",
		       (i == 0 || i == n_args) ? i : i + 1,
		       n_args, results);

	free(cxn);
	*stat_p = htonl(status);
	*result_p = htonl(results);
}

