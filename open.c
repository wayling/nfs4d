
#define _GNU_SOURCE
#include <string.h>
#include <syslog.h>
#include "server.h"

static const char *name_open_claim_type4[] = {
	[CLAIM_NULL] = "NULL",
	[CLAIM_PREVIOUS] = "PREVIOUS",
	[CLAIM_DELEGATE_CUR] = "DELEGATE_CUR",
	[CLAIM_DELEGATE_PREV] = "DELEGATE_PREV",
};

static void print_open_args(OPEN4args *args)
{
	if (!debugging)
		return;

	syslog(LOG_INFO, "op OPEN ('%.*s')",
	       args->claim.open_claim4_u.file.utf8string_len,
	       args->claim.open_claim4_u.file.utf8string_val);

	syslog(LOG_INFO, "   OPEN (SEQ:%u SHAC:%x SHDN:%x HOW:%s CLM:%s)",
	       args->seqid,
	       args->share_access,
	       args->share_deny,
	       args->openhow.opentype == OPEN4_CREATE ? "CR" : "NOC",
	       name_open_claim_type4[args->claim.claim]);

	syslog(LOG_INFO, "   OPEN (CID:%Lx OWNER:%.*s)",
	       (unsigned long long) args->owner.clientid,
	       args->owner.owner.owner_len,
	       args->owner.owner.owner_val);
}

struct conflict_fe_state {
	OPEN4args *args;
	nfsino_t ino;
	bool match;
};

static void state_sh_conflict(gpointer key, gpointer val, gpointer user_data)
{
	struct nfs_state *st = val;
	struct conflict_fe_state *cfs = user_data;

	if (st->ino != cfs->ino)
		return;
	if (st->type != nst_open)
		return;

	if (cfs->args->share_access & st->share_dn)
		cfs->match = true;

	if (cfs->args->share_deny & st->share_ac)
		cfs->match = true;
}

bool nfs_op_open(struct nfs_cxn *cxn, OPEN4args *args, COMPOUND4res *cres)
{
	struct nfs_resop4 resop;
	OPEN4res *res;
	OPEN4resok *resok;
	nfsstat4 status = NFS4_OK, lu_stat;
	struct nfs_inode *dir_ino, *ino = NULL;
	struct nfs_dirent *de;
	struct nfs_state *st;
	struct nfs_stateid *sid;
	bool creating, recreating = false;

	print_open_args(args);

	memset(&resop, 0, sizeof(resop));
	resop.resop = OP_OPEN;
	res = &resop.nfs_resop4_u.opopen;
	resok = &res->OPEN4res_u.resok4;

	/* for the moment, we only support CLAIM_NULL */
	if (args->claim.claim != CLAIM_NULL) {
		status = NFS4ERR_NOTSUPP;
		goto out;
	}

	/* get directory handle */
	status = dir_curfh(cxn, &dir_ino);
	if (status != NFS4_OK)
		goto out;

	/* lookup component name; get inode if exists */
	lu_stat = dir_lookup(dir_ino, &args->claim.open_claim4_u.file, &de);
	switch (lu_stat) {
	case NFS4ERR_NOENT:
		break;

	case NFS4_OK:
		ino = inode_get(de->ino);
		if (!ino) {	/* should never happen */
			status = NFS4ERR_SERVERFAULT;
			goto out;
		}
		break;

	default:
		status = lu_stat;
		goto out;
	}

	creating = (args->openhow.opentype == OPEN4_CREATE);

	if (creating && ino &&
	    (args->openhow.openflag4_u.how.mode == UNCHECKED4)) {
		creating = false;
		recreating = true;
	}

	/*
	 * does the dirent's existence match our expectations?
	 */
	if (!creating && (lu_stat == NFS4ERR_NOENT)) {
		status = lu_stat;
		goto out;
	}
	if (creating && (lu_stat == NFS4_OK)) {
		status = NFS4ERR_EXIST;
		goto out;
	}

	/*
	 * validate share reservations
	 */
	if ((args->share_access & OPEN4_SHARE_ACCESS_BOTH) == 0) {
		status = NFS4ERR_INVAL;
		goto out;
	}

	if (ino) {
		struct conflict_fe_state cfs = { args, ino->ino, false };

		g_hash_table_foreach(srv.state, state_sh_conflict, &cfs);

		if (cfs.match) {
			status = NFS4ERR_SHARE_DENIED;
			goto out;
		}
	}

	/*
	 * look up shorthand client id (clientid4)
	 */
	status = clientid_test(args->owner.clientid);
	if (status != NFS4_OK)
		goto out;

	/*
	 * create file, if necessary
	 */
	if (creating) {
		ino = inode_new_file(cxn);
		if (!ino) {
			status = NFS4ERR_RESOURCE;
			goto out;
		}

		status = inode_add(dir_ino, ino,
		   &args->openhow.openflag4_u.how.createhow4_u.createattrs,
		   &args->claim.open_claim4_u.file,
		   &resok->attrset, &resok->cinfo);
		if (status != NFS4_OK)
			goto out;
	}

	/* FIXME: undo file creation, if this test fails? */
	if (ino->type != NF4REG) {
		if (ino->type == NF4DIR)
			status = NFS4ERR_ISDIR;
		else if (ino->type == NF4LNK)
			status = NFS4ERR_SYMLINK;
		else
			status = NFS4ERR_INVAL;
		goto out;
	}

	/*
	 * if re-creating, only size attribute applies
	 */
	if (recreating) {
		uint64_t bitmap_set = 0;
		args->openhow.openflag4_u.how.createhow4_u.createattrs.attrmask.bitmap4_val[0]
			&= GUINT32_TO_BE(1 << FATTR4_SIZE);
		args->openhow.openflag4_u.how.createhow4_u.createattrs.attrmask.bitmap4_val[1] = 0;

		status = inode_apply_attrs(ino,
			&args->openhow.openflag4_u.how.createhow4_u.createattrs,
			&bitmap_set, NULL, false);
		if (status != NFS4_OK)
			goto out;
	}

	st = calloc(1, sizeof(struct nfs_state));
	if (!st) {
		status = NFS4ERR_RESOURCE;
		goto out;
	}

	st->cli = args->owner.clientid;
	st->type = nst_open;
	st->id = gen_stateid();
	if (!st->id) {
		free(st);
		status = NFS4ERR_RESOURCE;
		goto out;
	}

	st->owner = strndup(args->owner.owner.owner_val,
			    args->owner.owner.owner_len);
	if (!st->owner) {
		free(st);
		status = NFS4ERR_RESOURCE;
		goto out;
	}

	st->ino = ino->ino;
	st->seq = args->seqid + 1;
	st->share_ac = args->share_access;
	st->share_dn = args->share_deny;

	INIT_LIST_HEAD(&st->dead_node);

	g_hash_table_insert(srv.state, GUINT_TO_POINTER(st->id), st);

	sid = (struct nfs_stateid *) &resok->stateid;
	sid->seqid = args->seqid + 1;
	sid->id = GUINT32_TO_LE(st->id);
	memcpy(&sid->server_verf, &srv.instance_verf,
	       sizeof(srv.instance_verf));
	resok->rflags = OPEN4_RESULT_LOCKTYPE_POSIX;
	resok->delegation.delegation_type = OPEN_DELEGATE_NONE;

	status = NFS4_OK;
	cxn->current_fh = ino->ino;

	if (debugging)
		syslog(LOG_INFO, "   OPEN -> (SEQ:%u ID:%x)",
		       sid->seqid, st->id);

out:
	res->status = status;
	return push_resop(cres, &resop, status);
}

nfsstat4 nfs_op_open_confirm(struct nfs_cxn *cxn, struct curbuf *cur,
			     struct list_head *writes, struct rpc_write **wr)
{
	nfsstat4 status = NFS4_OK;
	struct nfs_stateid sid;
	struct nfs_state *st = NULL;
	struct nfs_inode *ino;
	uint32_t seqid;

	if (cur->len < 20) {
		status = NFS4ERR_BADXDR;
		goto out;
	}

	CURSID(&sid);
	seqid = CR32();

	if (debugging)
		syslog(LOG_INFO, "op OPEN_CONFIRM (SEQ:%u IDSEQ:%u ID:%x)",
		       seqid, sid.seqid, sid.id);

	ino = inode_get(cxn->current_fh);
	if (!ino) {
		status = NFS4ERR_NOFILEHANDLE;
		goto out;
	}

	if (ino->type != NF4REG) {
		if (ino->type == NF4DIR)
			status = NFS4ERR_ISDIR;
		else
			status = NFS4ERR_INVAL;
		goto out;
	}

	status = stateid_lookup(sid.id, ino->ino, nst_open, &st);
	if (status != NFS4_OK)
		goto out;

	if (seqid != st->seq) {
		status = NFS4ERR_BAD_SEQID;
		goto out;
	}

	sid.seqid = seqid;
	sid.id = st->id;
	memcpy(&sid.server_verf, &srv.instance_verf, sizeof(srv.instance_verf));

out:
	WR32(status);
	if (status == NFS4_OK)
		WRSID(&sid);
	return status;
}

nfsstat4 nfs_op_open_downgrade(struct nfs_cxn *cxn, struct curbuf *cur,
			     struct list_head *writes, struct rpc_write **wr)
{
	nfsstat4 status = NFS4_OK;
	struct nfs_stateid sid;
	struct nfs_state *st = NULL;
	struct nfs_inode *ino;
	uint32_t seqid, share_access, share_deny;

	if (cur->len < 28) {
		status = NFS4ERR_BADXDR;
		goto out;
	}

	CURSID(&sid);
	seqid = CR32();
	share_access = CR32();
	share_deny = CR32();

	if (debugging)
		syslog(LOG_INFO, "op OPEN_DOWNGRADE (SEQ:%u IDSEQ:%u ID:%x "
		       "SHAC:%x SHDN:%x)",
		       seqid, sid.seqid, sid.id,
		       share_access, share_deny);

	ino = inode_get(cxn->current_fh);
	if (!ino) {
		status = NFS4ERR_NOFILEHANDLE;
		goto out;
	}

	if (ino->type != NF4REG) {
		status = NFS4ERR_INVAL;
		goto out;
	}

	status = stateid_lookup(sid.id, ino->ino, nst_open, &st);
	if (status != NFS4_OK)
		goto out;

	if (seqid != st->seq) {
		status = NFS4ERR_BAD_SEQID;
		goto out;
	}

	if ((!(share_access & st->share_ac)) ||
	    (!(share_deny & st->share_dn))) {
		status = NFS4ERR_INVAL;
		goto out;
	}

	st->share_ac = share_access;
	st->share_dn = share_deny;

	st->seq++;

	sid.seqid = st->seq;
	sid.id = st->id;
	memcpy(&sid.server_verf, &srv.instance_verf, sizeof(srv.instance_verf));

	if (debugging)
		syslog(LOG_INFO, "   OPEN_DOWNGRADE -> (SEQ:%u ID:%x)",
		       sid.seqid, st->id);

out:
	WR32(status);
	if (status == NFS4_OK)
		WRSID(&sid);
	return status;
}

nfsstat4 nfs_op_close(struct nfs_cxn *cxn, struct curbuf *cur,
		      struct list_head *writes, struct rpc_write **wr)
{
	nfsstat4 status = NFS4_OK;
	struct nfs_stateid sid;
	struct nfs_state *st;
	struct nfs_inode *ino;
	uint32_t seqid;

	if (cur->len < 20) {
		status = NFS4ERR_BADXDR;
		goto out;
	}

	seqid = CR32();
	CURSID(&sid);

	if (debugging)
		syslog(LOG_INFO, "op CLOSE (SEQ:%u IDSEQ:%u ID:%x)",
		       seqid, sid.seqid, sid.id);

	ino = inode_get(cxn->current_fh);
	if (!ino) {
		status = NFS4ERR_NOFILEHANDLE;
		goto out;
	}

	if (ino->type != NF4REG) {
		status = NFS4ERR_INVAL;
		goto out;
	}

	status = stateid_lookup(sid.id, ino->ino, nst_open, &st);
	if (status != NFS4_OK)
		goto out;

	if (seqid != st->seq) {
		status = NFS4ERR_BAD_SEQID;
		goto out;
	}

	state_trash(st);

out:
	WR32(status);
	if (status == NFS4_OK)
		WRSID(&sid);
	return status;
}

