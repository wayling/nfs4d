/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <syslog.h>
#include <sys/time.h>
#include <rpc/pmap_clnt.h>
#include <netinet/in.h>
#include "server.h"
#include "nfs4_prot.h"


enum {
	NFS_PORT		= 2049,
	LISTEN_SIZE		= 100,
};

struct timeval current_time;
GList *client_list = NULL;


static int init_sock(void)
{
	int sock, val;
	struct sockaddr_in saddr;

	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		perror("socket");
		return -1;
	}

	val = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val,
		       sizeof(val)) < 0) {
		perror("setsockopt");
		return -1;
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(NFS_PORT);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		perror("bind");
		return -1;
	}

	if (listen(sock, LISTEN_SIZE) < 0) {
		perror("listen");
		return -1;
	}

	return sock;
}

static enum auth_stat check_auth(struct svc_req *rqstp)
{
	switch (rqstp->rq_cred.oa_flavor) {
	case AUTH_SYS:
		return AUTH_OK;
	default:
		return AUTH_TOOWEAK;
	}

	return AUTH_FAILED;	/* never reached; kill warning */
}

static void
nfs4_program_4(struct svc_req *rqstp, register SVCXPRT *transp)
{
	COMPOUND4args argument;
	COMPOUND4res result;
	bool_t retval;
	xdrproc_t _xdr_argument, _xdr_result;
	bool_t (*local)(char *, void *, struct svc_req *);
	enum auth_stat auth_stat;
	struct timezone tz = { 0, 0 };

	switch (rqstp->rq_proc) {
	case NFSPROC4_NULL:
		_xdr_argument = (xdrproc_t) xdr_void;
		_xdr_result = (xdrproc_t) xdr_void;
		local = (bool_t (*) (char *, void *,  struct svc_req *))nfsproc4_null_4_svc;
		break;

	case NFSPROC4_COMPOUND:
		_xdr_argument = (xdrproc_t) xdr_COMPOUND4args;
		_xdr_result = (xdrproc_t) xdr_COMPOUND4res;
		local = (bool_t (*) (char *, void *,  struct svc_req *))nfsproc4_compound_4_svc;
		break;

	default:
		svcerr_noproc (transp);
		return;
	}
	memset ((char *)&argument, 0, sizeof (argument));
	if (!svc_getargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
		svcerr_decode (transp);
		return;
	}

	gettimeofday(&current_time, &tz);

	auth_stat = check_auth(rqstp);
	if (auth_stat != AUTH_OK) {
		retval = FALSE;
		svcerr_auth(transp, auth_stat);
	} else
		retval = (bool_t) (*local)((char *)&argument,
					   (void *)&result, rqstp);

	if (retval > 0 && !svc_sendreply(transp, (xdrproc_t) _xdr_result, (char *)&result)) {
		svcerr_systemerr (transp);
	}
	if (!svc_freeargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
		fprintf (stderr, "%s", "unable to free arguments");
		exit (1);
	}
	if (!nfs4_program_4_freeresult (transp, _xdr_result, &result))
		fprintf (stderr, "%s", "unable to free results");

	return;
}

/* Linux is missing this prototype */
#if 0
bool_t gssrpc_pmap_unset(u_long prognum, u_long versnum);
#endif

static void init_rpc(int sock)
{
	register SVCXPRT *transp;

	transp = svctcp_create(sock, 0, 0);
	if (transp == NULL) {
		fprintf (stderr, "%s", "cannot create tcp service.");
		exit(1);
	}
	if (!svc_register(transp, NFS4_PROGRAM, NFS_V4, nfs4_program_4, IPPROTO_TCP)) {
		fprintf (stderr, "%s", "unable to register (NFS4_PROGRAM, NFS_V4, tcp).");
		exit(1);
	}
}

int
main (int argc, char **argv)
{
	struct timezone tz = { 0, 0 };
	int sock;

	openlog("nfs4_ramd", LOG_PID, LOG_LOCAL2);

	pmap_unset (NFS4_PROGRAM, NFS_V4);

	sock = init_sock();
	if (sock < 0)
		return 1;

	gettimeofday(&current_time, &tz);
	inode_table_init();

	init_rpc(sock);

	svc_run ();
	fprintf (stderr, "%s", "svc_run returned");
	exit (1);
	/* NOTREACHED */
}
