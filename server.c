/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */

#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <glib.h>
#include "nfs4_prot.h"
#include "server.h"

static int debugging = 1;

bool_t nfsproc4_null_4_svc(void *argp, void *result, struct svc_req *rqstp)
{
	return TRUE;
}

bool_t valid_utf8string(utf8string *str)
{
	if (!str || !str->utf8string_len || !str->utf8string_val)
		return FALSE;
	if (!g_utf8_validate(str->utf8string_val, str->utf8string_len, NULL))
		return FALSE;
	return TRUE;
}

gchar *copy_utf8string(utf8string *str)
{
	return g_strndup(str->utf8string_val, str->utf8string_len);
}

bool_t has_dots(utf8string *str)
{
	if (!str)
		return FALSE;
	if ((str->utf8string_len == 1) &&
	    (!memcmp(str->utf8string_val, ".", 1)))
		return TRUE;
	if ((str->utf8string_len == 2) &&
	    (!memcmp(str->utf8string_val, "..", 2)))
		return TRUE;
	return FALSE;
}

static struct nfs_client *cli_init(struct svc_req *rqstp)
{
	struct nfs_client *cli = g_slice_new0(struct nfs_client);

	return cli;
}

static void cli_free(struct nfs_client *cli)
{
	g_slice_free(struct nfs_client, cli);
}

void nfs_fh_set(nfs_fh4 *fh, nfsino_t fh_int)
{
	nfsino_t *fh_val = g_slice_new(nfsino_t);
	*fh_val = GUINT32_TO_BE(fh_int);

	fh->nfs_fh4_len = sizeof(nfsino_t);
	fh->nfs_fh4_val = (char *)(void *) fh_val;
}

static void nfs_fh_free(nfs_fh4 *fh)
{
	if (fh) {
		nfsino_t *fh_val = (void *) fh->nfs_fh4_val;
		g_slice_free(nfsino_t, fh_val);
		fh->nfs_fh4_val = NULL;
	}
}

static nfsino_t nfs_fh_decode(nfs_fh4 *fh_in)
{
	nfsino_t *fhp;
	nfsino_t fh;

	if (!fh_in)
		return 0;
	if (fh_in->nfs_fh4_len != sizeof(nfsino_t))
		return 0;
	if (!fh_in->nfs_fh4_val)
		return 0;
	fhp = (void *) fh_in->nfs_fh4_val;
	fh = GUINT32_FROM_BE(*fhp);

	if (!inode_get(fh))
		return 0;

	return fh;
}

bool_t push_resop(COMPOUND4res *res, const nfs_resop4 *resop, nfsstat4 stat)
{
	void *mem;
	u_int array_len = res->resarray.resarray_len;

	mem = realloc(res->resarray.resarray_val,
		((array_len + 1) * sizeof(nfs_resop4)));
	if (!mem)
		return FALSE;

	res->resarray.resarray_len++;
	res->resarray.resarray_val = mem;
	memcpy(&res->resarray.resarray_val[array_len], resop,
	       sizeof(struct nfs_resop4));
	res->status = stat;

	return TRUE;
}

static void nfs_getfh_free(GETFH4res *opgetfh)
{
	nfs_fh_free(&opgetfh->GETFH4res_u.resok4.object);
}

static bool_t nfs_op_getfh(struct nfs_client *cli, COMPOUND4res *cres)
{
	struct nfs_resop4 resop;
	GETFH4res *res;
	GETFH4resok *resok;
	nfsstat4 status = NFS4_OK;

	memset(&resop, 0, sizeof(resop));
	resop.resop = OP_GETFH;
	res = &resop.nfs_resop4_u.opgetfh;
	resok = &res->GETFH4res_u.resok4;

	if (debugging)
		syslog(LOG_INFO, "CURRENT_FH == %u", cli->current_fh);

	if (!inode_get(cli->current_fh)) {
		status = NFS4ERR_NOFILEHANDLE;
		goto out;
	}

	nfs_fh_set(&resok->object, cli->current_fh);

out:
	res->status = status;
	return push_resop(cres, &resop, status);
}

static bool_t nfs_op_putfh(struct nfs_client *cli, PUTFH4args *arg, COMPOUND4res *cres)
{
	struct nfs_resop4 resop;
	PUTFH4res *res;
	nfsstat4 status = NFS4_OK;
	nfsino_t fh;

	memset(&resop, 0, sizeof(resop));
	resop.resop = OP_PUTFH;
	res = &resop.nfs_resop4_u.opputfh;

	fh = nfs_fh_decode(&arg->object);
	if (!fh) {
		status = NFS4ERR_BADHANDLE;
		goto out;
	}

	cli->current_fh = fh;

out:
	res->status = status;
	return push_resop(cres, &resop, status);
}

static bool_t nfs_op_putrootfh(struct nfs_client *cli, COMPOUND4res *cres)
{
	struct nfs_resop4 resop;
	PUTFH4res *res;
	nfsstat4 status = NFS4_OK;

	memset(&resop, 0, sizeof(resop));
	resop.resop = OP_PUTROOTFH;
	res = &resop.nfs_resop4_u.opputfh;

	cli->current_fh = INO_ROOT;

	res->status = status;
	return push_resop(cres, &resop, status);
}

static bool_t nfs_op_putpubfh(struct nfs_client *cli, COMPOUND4res *cres)
{
	struct nfs_resop4 resop;
	PUTFH4res *res;
	nfsstat4 status = NFS4_OK;

	memset(&resop, 0, sizeof(resop));
	resop.resop = OP_PUTPUBFH;
	res = &resop.nfs_resop4_u.opputfh;

	cli->current_fh = INO_ROOT;

	res->status = status;
	return push_resop(cres, &resop, status);
}

static bool_t nfs_op_restorefh(struct nfs_client *cli, COMPOUND4res *cres)
{
	struct nfs_resop4 resop;
	RESTOREFH4res *res;
	nfsstat4 status = NFS4_OK;

	memset(&resop, 0, sizeof(resop));
	resop.resop = OP_RESTOREFH;
	res = &resop.nfs_resop4_u.oprestorefh;

	if (!inode_get(cli->save_fh)) {
		status = NFS4ERR_RESTOREFH;
		goto out;
	}

	cli->current_fh = cli->save_fh;

out:
	res->status = status;
	return push_resop(cres, &resop, status);
}

static bool_t nfs_op_savefh(struct nfs_client *cli, COMPOUND4res *cres)
{
	struct nfs_resop4 resop;
	SAVEFH4res *res;
	nfsstat4 status = NFS4_OK;

	memset(&resop, 0, sizeof(resop));
	resop.resop = OP_SAVEFH;
	res = &resop.nfs_resop4_u.opsavefh;

	if (!inode_get(cli->current_fh)) {
		status = NFS4ERR_NOFILEHANDLE;
		goto out;
	}

	cli->save_fh = cli->current_fh;

out:
	res->status = status;
	return push_resop(cres, &resop, status);
}

static bool_t nfs_op_readlink(struct nfs_client *cli, COMPOUND4res *cres)
{
	struct nfs_resop4 resop;
	READLINK4res *res;
	READLINK4resok *resok;
	nfsstat4 status = NFS4_OK;
	struct nfs_inode *ino;
	gchar *linktext;

	memset(&resop, 0, sizeof(resop));
	resop.resop = OP_READLINK;
	res = &resop.nfs_resop4_u.opreadlink;
	resok = &res->READLINK4res_u.resok4;

	ino = inode_get(cli->current_fh);
	if (!ino) {
		status = NFS4ERR_NOFILEHANDLE;
		goto out;
	}
	if (ino->type != NF4LNK) {
		status = NFS4ERR_NOFILEHANDLE;
		goto out;
	}

	linktext = ino->u.linktext;
	g_assert(linktext != NULL);

	resok->link.utf8string_len = strlen(linktext);
	resok->link.utf8string_val = linktext;

out:
	res->status = status;
	return push_resop(cres, &resop, status);
}

static int client_id_cmp(nfs_client_id4 *a, nfs_client_id4 *b)
{
	if (a->id.id_len < b->id.id_len)
		return -1;
	if (a->id.id_len > b->id.id_len)
		return 1;
	return memcmp(a->id.id_val, b->id.id_val, a->id.id_len);
}

struct client_id_info {
	nfs_client_id4		*query_id;
	GList			*unconfirmed;
	GList			*confirmed;
	GList			*all;
};

struct client_pcpl_info {
	nfsstat4		*status;
	struct nfs_client	*cli;
};

static void client_accum(struct nfs_cli_state *ncs, struct client_id_info *ci)
{
	if (client_id_cmp(&ncs->id, ci->query_id))
		return;
	
	ci->all = g_list_append(ci->all, ncs);
	if (ncs->flags & NFS_CLI_CONFIRMED)
		ci->confirmed = g_list_append(ci->confirmed, ncs);
	else
		ci->unconfirmed = g_list_append(ci->unconfirmed, ncs);
}

static bool_t principal_cmp(struct nfs_client *cli, struct nfs_cli_state *ncs)
{
	/* FIXME */
	return -1;	/* no match, just like memcmp(3) return */
}

static void client_check_pcpl(struct nfs_cli_state *ncs, struct client_pcpl_info *pi)
{
	nfsstat4 status = NFS4_OK;

	if (!principal_cmp(pi->cli, ncs)) {
		status = NFS4ERR_CLID_INUSE;
		goto out;
	}

out:
	*(pi->status) = status;
}

static bool_t nfs_op_setclientid(struct nfs_client *cli, SETCLIENTID4args *arg,
				 COMPOUND4res *cres)
{
	struct nfs_resop4 resop;
	SETCLIENTID4res *res;
	SETCLIENTID4resok *resok;
	nfsstat4 status = NFS4_OK;
	struct client_id_info ci = { &arg->client, NULL, NULL, NULL };
	struct client_pcpl_info pi;

	memset(&resop, 0, sizeof(resop));
	resop.resop = OP_SETCLIENTID;
	res = &resop.nfs_resop4_u.opsetclientid;
	resok = &res->SETCLIENTID4res_u.resok4;

	/* TODO: do we care about duplicate request cache (DRC)? */

	/* build lists of records matching client.id */
	g_list_foreach(client_list, (GFunc) client_accum, &ci);

	/* check for confirmed record where principal does not match */
	pi.status = &status;
	pi.cli = cli;
	g_list_foreach(ci.confirmed, (GFunc) client_check_pcpl, &pi);
	if (status != NFS4_OK)
		goto out;

	/* FIXME */

out:
	res->status = status;
	return push_resop(cres, &resop, status);
}

static bool_t nfs_op_setclientid_confirm(struct nfs_client *cli,
					 SETCLIENTID_CONFIRM4args *arg,
					 COMPOUND4res *cres)
{
	struct nfs_resop4 resop;
	SETCLIENTID_CONFIRM4res *res;
#if 0
	nfsstat4 status = NFS4_OK;
#else
	nfsstat4 status = NFS4ERR_NOTSUPP;
#endif

	memset(&resop, 0, sizeof(resop));
	resop.resop = OP_SETCLIENTID_CONFIRM;
	res = &resop.nfs_resop4_u.opsetclientid_confirm;

	/* FIXME */

#if 0
out:
#endif
	res->status = status;
	return push_resop(cres, &resop, status);
}

static bool_t nfs_op_notsupp(struct nfs_client *cli, COMPOUND4res *cres,
			     nfs_opnum4 argop)
{
	struct nfs_resop4 resop;
	OPENATTR4res *res;
	nfsstat4 status = NFS4ERR_NOTSUPP;

	memset(&resop, 0, sizeof(resop));
	resop.resop = argop;
	res = &resop.nfs_resop4_u.opopenattr;
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

static bool_t nfs_arg(struct nfs_client *cli, nfs_argop4 *arg, COMPOUND4res *res)
{
	if (debugging)
		syslog(LOG_INFO, "compound op %s",
		       (arg->argop > 39) ?  "<n/a>" : arg_str[arg->argop]);

	switch (arg->argop) {
	case OP_CREATE:
		return nfs_op_create(cli, &arg->nfs_argop4_u.opcreate, res);
	case OP_GETFH:
		return nfs_op_getfh(cli, res);
	case OP_LINK:
		return nfs_op_link(cli, &arg->nfs_argop4_u.oplink, res);
	case OP_LOOKUP:
		return nfs_op_lookup(cli, &arg->nfs_argop4_u.oplookup, res);
	case OP_LOOKUPP:
		return nfs_op_lookupp(cli, res);
	case OP_PUTFH:
		return nfs_op_putfh(cli, &arg->nfs_argop4_u.opputfh, res);
	case OP_PUTPUBFH:
		return nfs_op_putpubfh(cli, res);
	case OP_PUTROOTFH:
		return nfs_op_putrootfh(cli, res);
	case OP_READLINK:
		return nfs_op_readlink(cli, res);
	case OP_REMOVE:
		return nfs_op_remove(cli, &arg->nfs_argop4_u.opremove, res);
	case OP_RENAME:
		return nfs_op_rename(cli, &arg->nfs_argop4_u.oprename, res);
	case OP_RESTOREFH:
		return nfs_op_restorefh(cli, res);
	case OP_SAVEFH:
		return nfs_op_savefh(cli, res);
	case OP_SETCLIENTID:
		return nfs_op_setclientid(cli,
					&arg->nfs_argop4_u.opsetclientid, res);
	case OP_SETCLIENTID_CONFIRM:
		return nfs_op_setclientid_confirm(cli,
				&arg->nfs_argop4_u.opsetclientid_confirm, res);

	case OP_ACCESS:
	case OP_CLOSE:
	case OP_COMMIT:
	case OP_DELEGPURGE:
	case OP_DELEGRETURN:
	case OP_GETATTR:
	case OP_LOCK:
	case OP_LOCKT:
	case OP_LOCKU:
	case OP_NVERIFY:
	case OP_OPEN:
	case OP_OPEN_CONFIRM:
	case OP_OPEN_DOWNGRADE:
	case OP_READ:
	case OP_READDIR:
	case OP_RENEW:
	case OP_SECINFO:
	case OP_SETATTR:
	case OP_VERIFY:
	case OP_WRITE:
	case OP_RELEASE_LOCKOWNER:
	case OP_OPENATTR:
		return nfs_op_notsupp(cli, res, arg->argop);
	default:
		return FALSE;
	}

	return FALSE;	/* never reached */
}

bool_t nfsproc4_compound_4_svc(COMPOUND4args *arg, COMPOUND4res *res,
			       struct svc_req *rqstp)
{
	struct nfs_client *cli;
	unsigned int i;

	memset(res, 0, sizeof(*res));
	res->status = NFS4_OK;
	memcpy(&res->tag, &arg->tag, sizeof(utf8str_cs));

	if (arg->minorversion != 0) {
		res->status = NFS4ERR_MINOR_VERS_MISMATCH;
		goto out;
	}

	cli = cli_init(rqstp);
	if (!cli) {
		res->status = NFS4ERR_RESOURCE;
		goto out;
	}

	if (debugging)
		syslog(LOG_INFO, "compound start");

	for (i = 0; i < arg->argarray.argarray_len; i++)
		if (!nfs_arg(cli, &arg->argarray.argarray_val[i], res)) {
			syslog(LOG_WARNING, "compound failed early");
			break;
		}

	if (debugging)
		syslog(LOG_INFO, "compound stop");

	cli_free(cli);
out:
	return TRUE;
}

static void nfs_free(nfs_resop4 *res)
{
	switch(res->resop) {
	case OP_CREATE:
		g_free(res->nfs_resop4_u.opcreate.CREATE4res_u.resok4.attrset.bitmap4_val);
		break;
	case OP_GETFH:
		nfs_getfh_free(&res->nfs_resop4_u.opgetfh);
		break;
	case OP_SETATTR:
		g_free(res->nfs_resop4_u.opsetattr.attrsset.bitmap4_val);
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

	for (i = 0; i < res->resarray.resarray_len; i++)
		nfs_free(&res->resarray.resarray_val[i]);

	free(res->resarray.resarray_val);

	return TRUE;
}

