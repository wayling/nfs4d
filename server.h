#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdint.h>
#include <sys/time.h>
#include <glib.h>
#include "nfs4_prot.h"

struct nfs_client;

typedef uint32_t nfsino_t;

enum {
	INO_ROOT		= 10,
	INO_FIRST		= INO_ROOT,
	INO_RESERVED_LAST	= 999,

	NFS_CLI_CONFIRMED	= (1 << 0),
};

enum server_limits {
	SRV_MAX_LINK		= 0xffffffff,	/* max hard links per inode*/
	SRV_MAX_NAME		= 512,		/* max pathname length */
	SRV_MAX_READ		= 1024 * 128,	/* max contig. read */
	SRV_MAX_WRITE		= 1024 * 128,	/* max contig. write */
};

enum server_fs_settings {
	SRV_FH_EXP_TYPE		= FH4_PERSISTENT,
};

enum big_server_fs_settings {
	SRV_MAX_FILESIZE	= 0xffffffffULL,
};

enum blob_hash_init_info {
	BLOB_HASH_INIT		= 5381UL
};

enum fattr_types {
	FATTR_TYPE_OBJ,
	FATTR_TYPE_FS,
	FATTR_TYPE_SRV,
};

struct blob {
	unsigned int		len;
	void			*buf;
};

struct nfs_cxn {
	nfsino_t		current_fh;
	nfsino_t		save_fh;

	/* RPC credentials */
	uint32_t		uid;
	uint32_t		gid;
};

struct nfs_state {
	struct nfs_client	*cli;

	uint32_t		id;

	char			*owner;

	nfsino_t		ino;

	uint32_t		share_ac;
	uint32_t		share_dn;
};

struct nfs_clientid {
	struct blob		id;
	verifier4		cli_verf;	/* client-supplied verifier */
	clientid4		id_short;
	verifier4		confirm_verf;	/* clientid confirm verifier */
	cb_client4		callback;
	guint32			callback_ident;
};

struct nfs_client {
	struct nfs_clientid	*id;

	GList			*pending;	/* unconfirmed requests */
};

struct nfs_inode {
	nfsino_t		ino;
	enum nfs_ftype4		type;		/* inode type: link, dir, ...*/
	GArray			*parents;	/* list of parent dirs */
	uint64_t		version;

	uint64_t		size;

	uint64_t		ctime;		/* creation time */
	uint64_t		atime;		/* last-accessed time */
	uint64_t		mtime;		/* last-modified time */
	uint32_t		mode;
	uint32_t		uid;
	uint32_t		gid;

	union {
		GHashTable	*dir;		/* state for a directory */
		gchar		*linktext;	/* state for a symlink */
		specdata4	devdata;	/* block/chrdev info */
	} u;
};

#define FATTR_DEFINE(a,b,c) \
	fattr4_##b b;

struct nfs_fattr_set {
	uint64_t		bitmap;

#include "fattr.h"
};

#undef FATTR_DEFINE

struct nfs_dirent {
	nfsino_t		ino;
};

struct nfs_server {
	GHashTable		*inode_table;

	GHashTable		*client_ids;

	GHashTable		*clid_idx;

	GHashTable		*state;

	unsigned int		lease_time;

	struct drand48_data	rng;

	verifier4		instance_verf;
};

/* global variables */
extern struct timeval current_time;
extern GList *client_list;
extern struct nfs_server srv;
extern int debugging;

/* inode.c */
struct nfs_inode *inode_get(nfsino_t inum);
void inode_touch(struct nfs_inode *ino);
bool_t inode_table_init(void);
void inode_unlink(struct nfs_inode *ino, nfsino_t dir_ref);
bool_t nfs_op_create(struct nfs_cxn *cxn, CREATE4args *arg, COMPOUND4res *cres);
bool_t nfs_op_access(struct nfs_cxn *cxn, ACCESS4args *arg, COMPOUND4res *cres);
bool_t nfs_op_getattr(struct nfs_cxn *cxn, GETATTR4args *arg,
		      COMPOUND4res *cres);
void nfs_getattr_free(GETATTR4res *res);
bool_t nfs_op_verify(struct nfs_cxn *cxn, VERIFY4args *arg,
		     COMPOUND4res *cres, int nverify);
struct nfs_inode *inode_new_file(void);
nfsstat4 inode_add(struct nfs_inode *dir_ino, struct nfs_inode *new_ino,
		   fattr4 *attr, utf8string *name, bitmap4 *attrset,
		   change_info4 *cinfo);

/* dir.c */
bool_t nfs_op_lookup(struct nfs_cxn *cxn, LOOKUP4args *arg, COMPOUND4res *cres);
bool_t nfs_op_lookupp(struct nfs_cxn *cxn, COMPOUND4res *cres);
bool_t nfs_op_link(struct nfs_cxn *cxn, LINK4args *arg, COMPOUND4res *cres);
bool_t nfs_op_remove(struct nfs_cxn *cxn, REMOVE4args *arg, COMPOUND4res *cres);
bool_t nfs_op_rename(struct nfs_cxn *cxn, RENAME4args *arg, COMPOUND4res *cres);
enum nfsstat4 dir_add(struct nfs_inode *dir_ino, utf8string *name_in,
		      nfsino_t inum);
void dirent_free(gpointer p);
bool_t nfs_op_readdir(struct nfs_cxn *cxn, READDIR4args *arg,
		      COMPOUND4res *cres);
nfsstat4 dir_curfh(const struct nfs_cxn *cxn, struct nfs_inode **ino_out);
nfsstat4 dir_lookup(struct nfs_inode *dir_ino, utf8string *str,
		    struct nfs_dirent **dirent_out);

/* fattr.c */
extern const uint64_t fattr_write_only_mask;
extern const uint64_t fattr_read_write_mask;
extern const uint64_t fattr_read_only_mask;
extern const uint64_t fattr_supported_mask;
extern bool_t fattr_encode(fattr4 *raw, struct nfs_fattr_set *attr);
extern bool_t fattr_decode(fattr4 *raw, struct nfs_fattr_set *attr);
extern void fattr_free(struct nfs_fattr_set *attr);
extern void fattr_fill(struct nfs_inode *ino, struct nfs_fattr_set *attr);
extern void fattr4_free(fattr4 *attr);
extern void print_fattr(const char *pfx, fattr4 *attr);

/* fh.c */
bool_t nfs_op_getfh(struct nfs_cxn *cxn, COMPOUND4res *cres);
bool_t nfs_op_putfh(struct nfs_cxn *cxn, PUTFH4args *arg, COMPOUND4res *cres);
bool_t nfs_op_putrootfh(struct nfs_cxn *cxn, COMPOUND4res *cres);
bool_t nfs_op_putpubfh(struct nfs_cxn *cxn, COMPOUND4res *cres);
bool_t nfs_op_restorefh(struct nfs_cxn *cxn, COMPOUND4res *cres);
bool_t nfs_op_savefh(struct nfs_cxn *cxn, COMPOUND4res *cres);
void nfs_getfh_free(GETFH4res *opgetfh);

/* open.c */
bool_t nfs_op_open(struct nfs_cxn *cxn, OPEN4args *args, COMPOUND4res *cres);

/* server.c */
bool_t push_resop(COMPOUND4res *res, const nfs_resop4 *resop, nfsstat4 stat);
bool_t valid_utf8string(utf8string *str);
gchar *copy_utf8string(utf8string *str);
bool_t has_dots(utf8string *str);
void nfs_fh_set(nfs_fh4 *fh, nfsino_t fh_int);
guint64 get_bitmap(const bitmap4 *map);
int set_bitmap(guint64 map_in, bitmap4 *map_out);
nfsino_t nfs_fh_decode(const nfs_fh4 *fh_in);
guint clientid_hash(gconstpointer data);
gboolean clientid_equal(gconstpointer _a, gconstpointer _b);
guint short_clientid_hash(gconstpointer data);
gboolean short_clientid_equal(gconstpointer _a, gconstpointer _b);

/* state.c */
extern void client_free(gpointer data);
extern void state_free(gpointer data);
extern uint32_t gen_stateid(void);
extern bool_t nfs_op_setclientid(struct nfs_cxn *cxn, SETCLIENTID4args *args,
			 COMPOUND4res *cres);
extern bool_t nfs_op_setclientid_confirm(struct nfs_cxn *cxn,
				 SETCLIENTID_CONFIRM4args *arg,
				 COMPOUND4res *cres);
extern void rand_verifier(verifier4 *verf);
extern unsigned long blob_hash(unsigned long hash, const void *_buf, size_t buflen);

static inline void free_bitmap(bitmap4 *map)
{
	g_free(map->bitmap4_val);
	map->bitmap4_len = 0;
	map->bitmap4_val = NULL;
}

#endif /* __SERVER_H__ */
