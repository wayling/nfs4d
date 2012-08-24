
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
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <glib.h>
#include "nfs4_prot.h"
#include "server.h"

const char *name_nfs4status[] = {
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

void mk_datapfx(char *datapfx, nfsino_t inum)
{
	char inum_s[33];

#ifdef NFSD_INO64
	sprintf(inum_s, "%016llX", (unsigned long long) inum);
#else
	sprintf(inum_s, "%08llX", (unsigned long long) inum);
#endif

	datapfx[0] = inum_s[0];
	datapfx[1] = inum_s[1];
	datapfx[2] = '/';
	datapfx[3] = 0;
}

char *cxn_getuser(const struct nfs_cxn *cxn)
{
	char *s = NULL;

	switch (cxn->auth.type) {
	case auth_none:
		/* do nothing */
		break;
	case auth_unix:
		s = id_lookup(idt_user, cxn->auth.u.up.uid);
		break;
	}

	if (!s)
		s = strdup("nobody@localdomain");

	return s;
}

char *cxn_getgroup(const struct nfs_cxn *cxn)
{
	char *s = NULL;

	switch (cxn->auth.type) {
	case auth_none:
		/* do nothing */
		break;
	case auth_unix:
		s = id_lookup(idt_group, cxn->auth.u.up.gid);
		break;
	}

	if (!s)
		s = strdup("nobody@localdomain");

	return s;
}

static nfsstat4 cli_init(const char *host, struct opaque_auth *cred, struct opaque_auth *verf,
			 struct nfs_cxn **cxn_out)
{
	struct nfs_cxn *cxn = calloc(1, sizeof(struct nfs_cxn));
	nfsstat4 status = NFS4_OK;
	uint32_t *p, v, ql, lim;

	if (!cxn) {
		status = NFS4ERR_RESOURCE;
		goto out;
	}

	strncpy(cxn->auth.host, host, sizeof(cxn->auth.host) - 1);

	switch (cred->oa_flavor) {
	case AUTH_NONE:
		applog(LOG_INFO, "AUTH_NONE");
		cxn->auth.type = auth_none;

		if (debugging > 1)
			applog(LOG_INFO, "RPC CRED None (len %d)",
				cred->oa_length);
		break;

	case AUTH_SYS:
		if (!cred->oa_base || (cred->oa_length < 16)) {
			applog(LOG_INFO, "AUTH_SYS null");
			status = NFS4ERR_DENIED;
			goto err_out;
		}

		p = (uint32_t *) cred->oa_base;

		cxn->auth.u.up.stamp = ntohl(*p++);	/* stamp */
		v = ntohl(*p++);			/* machinename len */

		if (v < 1) {
			applog(LOG_INFO, "AUTH_SYS machinename null");
			status = NFS4ERR_DENIED;
			goto err_out;
		}

		ql = XDR_QUADLEN(v);
		if (cred->oa_length < ((ql + 4) * 4)) {
			applog(LOG_INFO, "AUTH_SYS null");
			status = NFS4ERR_DENIED;
			goto err_out;
		}

		lim = v;
		if (lim > sizeof(cxn->auth.u.up.machine))
			lim = sizeof(cxn->auth.u.up.machine);
		memcpy(&cxn->auth.u.up.machine, p, lim);
		cxn->auth.u.up.machine[lim - 1] = 0;
		p += ql;				/* machinename */

		cxn->auth.u.up.uid = ntohl(*p++);	/* uid */
		cxn->auth.u.up.gid = ntohl(*p++);	/* gid */

		/* we ignore the list of gids that follow */

		cxn->auth.type = auth_unix;

		if (debugging > 1)
			applog(LOG_INFO, "RPC CRED Unix (uid %d gid %d len %d)",
				cxn->auth.u.up.uid,
				cxn->auth.u.up.gid,
				cred->oa_length);
		break;

	default:
		applog(LOG_INFO, "AUTH unknown");
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

static nfsstat4 nfs_op_readlink(struct nfs_cxn *cxn,
		       struct list_head *writes, struct rpc_write **wr)
{
	nfsstat4 status = NFS4_OK;
	struct nfs_inode *ino = NULL;
	char *linktext = NULL;

	if (debugging)
		applog(LOG_INFO, "op READLINK");

	ino = inode_fhdec(NULL, cxn->current_fh, 0);
	if (!ino) {
		status = NFS4ERR_NOFILEHANDLE;
		goto out;
	}
	if (ino->type != NF4LNK) {
		status = NFS4ERR_INVAL;
		goto out;
	}

	linktext = ino->linktext;

	if (debugging)
		applog(LOG_INFO, "   READLINK -> '%s'", linktext);

out:
	WR32(status);
	if (status == NFS4_OK)
		WRSTR(linktext);
	inode_free(ino);
	return status;
}

static nfsstat4 nfs_op_secinfo(struct nfs_cxn *cxn, const SECINFO4args *args,
		       struct list_head *writes, struct rpc_write **wr)
{
	nfsstat4 status;
	struct nfs_buf name;
	struct nfs_inode *ino = NULL;
	nfsino_t dummy;
	DB_ENV *dbenv = srv.fsdb.env;
	DB_TXN *txn;
	int rc;

	if (debugging)
		applog(LOG_INFO, "op SECINFO");

	name.len = args->name.utf8string_len;
	name.val = args->name.utf8string_val;

	if (!name.len || !g_utf8_validate(name.val, name.len, NULL)) {
		status = NFS4ERR_INVAL;
		goto out;
	}

	/* open transaction */
	rc = dbenv->txn_begin(dbenv, NULL, &txn, 0);
	if (rc) {
		status = NFS4ERR_IO;
		dbenv->err(dbenv, rc, "DB_ENV->txn_begin");
		goto out;
	}

	status = dir_curfh(txn, cxn, &ino, 0);
	if (status != NFS4_OK)
		goto out_abort;

	status = dir_lookup(txn, ino, &name, 0, &dummy);
	if (status != NFS4_OK)
		goto out_abort;

	/* close transaction */
	rc = txn->commit(txn, 0);
	if (rc) {
		dbenv->err(dbenv, rc, "DB_ENV->txn_commit");
		status = NFS4ERR_IO;
		goto out;
	}

out:
	WR32(status);
	if (status == NFS4_OK) {
		WR32(2);		/* secinfo array size */
		WR32(AUTH_SYS);
		WR32(AUTH_NONE);
	}
	inode_free(ino);
	return status;

out_abort:
	if (txn->abort(txn))
		dbenv->err(dbenv, rc, "DB_ENV->txn_abort");
	goto out;
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
	"BACKCHANNEL_CTL",
	"BIND_CONN_TO_SESSION",
	"EXCHANGE_ID",
	"CREATE_SESSION",
	"DESTROY_SESSION",
	"FREE_STATEID",
	"GET_DIR_DELEGATION",
	"GETDEVICEINFO",
	"GETDEVICELIST",
	"LAYOUTCOMMIT",
	"LAYOUTGET",
	"LAYOUTRETURN",
	"SECINFO_NO_NAME",
	"SEQUENCE",
	"SET_SSV",
	"TEST_STATEID",
	"WANT_DELEGATION",
	"DESTROY_CLIENTID",
	"RECLAIM_COMPLETE",
};

static const char *argstr(uint32_t op)
{
	if (op >= ARRAY_SIZE(arg_str))
		return "<unknown>";
	
	return arg_str[op];
}

static nfsstat4 nfs_op(struct nfs_cxn *cxn,
		       unsigned int arg_pos, nfs_argop4 *arg,
		       struct list_head *writes, struct rpc_write **wr)
{
	uint32_t op;

	op = arg->argop;

	switch (op) {
	case OP_ACCESS:
	case OP_BIND_CONN_TO_SESSION:
	case OP_CLOSE:
	case OP_COMMIT:
	case OP_CREATE:
	case OP_CREATE_SESSION:
	case OP_DESTROY_SESSION:
	case OP_EXCHANGE_ID:
	case OP_GETATTR:
	case OP_GETFH:
	case OP_LINK:
	case OP_LOCK:
	case OP_LOCKT:
	case OP_LOCKU:
	case OP_LOOKUP:
	case OP_LOOKUPP:
	case OP_NVERIFY:
	case OP_OPEN:
	case OP_OPEN_DOWNGRADE:
	case OP_PUTFH:
	case OP_PUTPUBFH:
	case OP_PUTROOTFH:
	case OP_READ:
	case OP_READDIR:
	case OP_READLINK:
	case OP_REMOVE:
	case OP_RENAME:
	case OP_RESTOREFH:
	case OP_SAVEFH:
	case OP_SECINFO:
	case OP_SEQUENCE:
	case OP_SETATTR:
	case OP_VERIFY:
	case OP_WRITE:
	case OP_DELEGPURGE:
	case OP_DELEGRETURN:
	case OP_OPENATTR:
		WR32(op);			/* write resop */
		break;

	default:
		if (debugging)
			applog(LOG_INFO, "unknown op %s", argstr(op));
		WR32(NFS4ERR_OP_ILLEGAL);	/* write resop */
		break;
	}

	if (arg_pos == 0) {
		switch (op) {
		case OP_SEQUENCE:
		case OP_BIND_CONN_TO_SESSION:
		case OP_EXCHANGE_ID:
		case OP_CREATE_SESSION:
		case OP_DESTROY_SESSION:
			break;

		default:
			WR32(NFS4ERR_OP_NOT_IN_SESSION);
			return NFS4ERR_OP_NOT_IN_SESSION;
		}
	}

	switch (op) {
	case OP_ACCESS:
		srv.stats.op_access++;
		return nfs_op_access(cxn, &arg->nfs_argop4_u.opaccess, writes, wr);
	case OP_CLOSE:
		srv.stats.op_close++;
		return nfs_op_close(cxn, &arg->nfs_argop4_u.opclose, writes, wr);
	case OP_COMMIT:
		srv.stats.op_commit++;
		return nfs_op_commit(cxn, &arg->nfs_argop4_u.opcommit, writes, wr);
	case OP_CREATE:
		srv.stats.op_create++;
		return nfs_op_create(cxn, &arg->nfs_argop4_u.opcreate, writes, wr);
	case OP_EXCHANGE_ID:
		srv.stats.op_exchange_id++;
		return nfs_op_exchange_id(cxn, &arg->nfs_argop4_u.opexchange_id, writes, wr);
	case OP_GETATTR:
		srv.stats.op_getattr++;
		return nfs_op_getattr(cxn, &arg->nfs_argop4_u.opgetattr, writes, wr);
	case OP_GETFH:
		srv.stats.op_getfh++;
		return nfs_op_getfh(cxn, writes, wr);
	case OP_LINK:
		srv.stats.op_link++;
		return nfs_op_link(cxn, &arg->nfs_argop4_u.oplink, writes, wr);
	case OP_LOCK:
		srv.stats.op_lock++;
		return nfs_op_lock(cxn, &arg->nfs_argop4_u.oplock, writes, wr);
	case OP_LOCKT:
		srv.stats.op_testlock++;
		return nfs_op_testlock(cxn, &arg->nfs_argop4_u.oplockt, writes, wr);
	case OP_LOCKU:
		srv.stats.op_unlock++;
		return nfs_op_unlock(cxn, &arg->nfs_argop4_u.oplocku, writes, wr);
	case OP_LOOKUP:
		srv.stats.op_lookup++;
		return nfs_op_lookup(cxn, &arg->nfs_argop4_u.oplookup, writes, wr);
	case OP_LOOKUPP:
		srv.stats.op_lookupp++;
		return nfs_op_lookupp(cxn, writes, wr);
	case OP_NVERIFY:
		srv.stats.op_nverify++;
		return nfs_op_verify(cxn, &arg->nfs_argop4_u.opverify, writes, wr, true);
	case OP_OPEN:
		srv.stats.op_open++;
		return nfs_op_open(cxn, &arg->nfs_argop4_u.opopen, writes, wr);
	case OP_OPEN_DOWNGRADE:
		srv.stats.op_open_downgrade++;
		return nfs_op_open_downgrade(cxn, &arg->nfs_argop4_u.opopen_downgrade, writes, wr);
	case OP_PUTFH:
		srv.stats.op_putfh++;
		return nfs_op_putfh(cxn, &arg->nfs_argop4_u.opputfh, writes, wr);
	case OP_PUTPUBFH:
		srv.stats.op_putpubfh++;
		return nfs_op_putpubfh(cxn, writes, wr);
	case OP_PUTROOTFH:
		srv.stats.op_putrootfh++;
		return nfs_op_putrootfh(cxn, writes, wr);
	case OP_READ:
		srv.stats.op_read++;
		return nfs_op_read(cxn, &arg->nfs_argop4_u.opread, writes, wr);
	case OP_READDIR:
		srv.stats.op_readdir++;
		return nfs_op_readdir(cxn, &arg->nfs_argop4_u.opreaddir, writes, wr);
	case OP_READLINK:
		srv.stats.op_readlink++;
		return nfs_op_readlink(cxn, writes, wr);
	case OP_REMOVE:
		srv.stats.op_remove++;
		return nfs_op_remove(cxn, &arg->nfs_argop4_u.opremove, writes, wr);
	case OP_RENAME:
		srv.stats.op_rename++;
		return nfs_op_rename(cxn, &arg->nfs_argop4_u.oprename, writes, wr);
	case OP_RESTOREFH:
		srv.stats.op_restorefh++;
		return nfs_op_restorefh(cxn, writes, wr);
	case OP_SAVEFH:
		srv.stats.op_savefh++;
		return nfs_op_savefh(cxn, writes, wr);
	case OP_SECINFO:
		srv.stats.op_secinfo++;
		return nfs_op_secinfo(cxn, &arg->nfs_argop4_u.opsecinfo, writes, wr);
	case OP_SEQUENCE:
		srv.stats.op_sequence++;
		if (arg_pos != 0) {
			WR32(NFS4ERR_SEQUENCE_POS);
			return NFS4ERR_SEQUENCE_POS;
		}

		srv.stats.op_notsupp++;
		WR32(NFS4ERR_NOTSUPP);		/* op status */
		return NFS4ERR_NOTSUPP;		/* compound status */
		//return nfs_op_seqence(cxn, &arg->nfs_argop4_u.opsequence, writes, wr);

	case OP_SETATTR:
		srv.stats.op_setattr++;
		return nfs_op_setattr(cxn, &arg->nfs_argop4_u.opsetattr, writes, wr);
	case OP_VERIFY:
		srv.stats.op_verify++;
		return nfs_op_verify(cxn, &arg->nfs_argop4_u.opverify, writes, wr, false);
	case OP_WRITE:
		srv.stats.op_write++;
		return nfs_op_write(cxn, &arg->nfs_argop4_u.opwrite, writes, wr);

	case OP_DELEGPURGE:
	case OP_DELEGRETURN:
	case OP_OPENATTR:
		if (debugging)
			applog(LOG_INFO, "unsupported op %s", argstr(op));

		srv.stats.op_notsupp++;
		WR32(NFS4ERR_NOTSUPP);		/* op status */
		return NFS4ERR_NOTSUPP;		/* compound status */

	default:
		if (debugging)
			applog(LOG_INFO, "illegal or unsupported op %s", argstr(op));
		srv.stats.op_illegal++;
		WR32(NFS4ERR_OP_ILLEGAL);	/* op status */
		return NFS4ERR_OP_ILLEGAL;	/* compound status */
	}

	return NFS4ERR_INVAL;	/* never reached */
}

int nfsproc_null(const char *host, struct opaque_auth *cred, struct opaque_auth *verf,
		  struct curbuf *cur, struct list_head *writes,
		  struct rpc_write **wr)
{
	if (debugging)
		applog(LOG_ERR, "NULL proc invoked");

	return 0;
}

int nfsproc_compound(const char *host, struct opaque_auth *cred, struct opaque_auth *verf,
		      struct curbuf *cur, struct list_head *writes,
		      struct rpc_write **wr)
{
	struct nfs_buf tag = {};
	uint32_t *stat_p = NULL, *result_p = NULL, n_args = 0, minor;
	nfsstat4 status = NFS4_OK;
	unsigned int i = 0, results = 0;
	struct nfs_cxn *cxn = NULL;
	int drc_mask = 0;
	XDR xdrs = {};
	COMPOUND4args args = {};

	xdrmem_create(&xdrs, (char *) cur->buf, cur->len, XDR_DECODE);

	if (!xdr_COMPOUND4args(&xdrs, &args)) {
		status = NFS4ERR_BADXDR;
		stat_p = WRSKIP(4);		/* COMPOUND result status */
		WRBUF(&tag);			/* tag */
		result_p = WRSKIP(4);		/* result array size */
		goto out;
	}

	tag.val = args.tag.utf8string_val;
	tag.len = args.tag.utf8string_len;
	minor = args.minorversion;
	n_args = args.argarray.argarray_len;

	stat_p = WRSKIP(4);		/* COMPOUND result status */
	WRBUF(&tag);			/* tag */
	result_p = WRSKIP(4);		/* result array size */

	if (!g_utf8_validate(tag.val, tag.len, NULL)) {
		status = NFS4ERR_INVAL;
		goto out;
	}
	if (minor != 1) {
		status = NFS4ERR_MINOR_VERS_MISMATCH;
		goto out;
	}

	status = cli_init(host, cred, verf, &cxn);
	if (status != NFS4_OK)
		goto out;

	/* honestly, this was put here more to shortcut a
	 * pathological case in pynfs.  we don't really have
	 * any inherent limits here.
	 */
	if (n_args > SRV_MAX_COMPOUND_OPS) {
		status = NFS4ERR_RESOURCE;
		goto out;
	}

	for (i = 0; i < n_args; i++) {
		results++;	/* even failed operations have results */

		status = nfs_op(cxn, i,
				&args.argarray.argarray_val[i],writes, wr);
		if (status != NFS4_OK)
			break;
	}

out:
	if (debugging || (i > 500))
		applog(LOG_INFO, "compound end (%u args, %u results, status %s)",
		       n_args, results,
		       name_nfs4status[status]);

	if (cxn) {
		drc_mask = cxn->drc_mask;
		free(cxn);
	}

	if (status == NFS4_OK)
		srv.stats.compound_ok++;
	else
		srv.stats.compound_fail++;

	*stat_p = htonl(status);
	*result_p = htonl(results);

	xdr_free((xdrproc_t) xdr_COMPOUND4args, (char *) &args);
	xdr_destroy(&xdrs);

	return drc_mask;
}

