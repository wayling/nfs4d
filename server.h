#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>
#include <glib.h>
#include <rpc/auth.h>
#include "nfs4_prot.h"
#include "elist.h"

typedef uint32_t nfsino_t;

#define XDR_QUADLEN(l)		(((l) + 3) >> 2)

#define CUR_SKIP(count)		cur_skip(cur, (count))
#define CUR32()			cur_read32(cur)
#define CURMEM(count)		cur_readmem(cur, (count))

#define WRSKIP(count)		wr_skip(writes, wr, (count))
#define WR32(val)		wr_write32(writes, wr, (val))
#define WRBUF(buf_ptr)		wr_buf(writes, wr, (buf_ptr))
#define WRSTR(buf_ptr)		wr_str(writes, wr, (buf_ptr))

enum {
	INO_ROOT		= 10,
	INO_FIRST		= INO_ROOT,
	INO_RESERVED_LAST	= 999,

	NFS_CLI_CONFIRMED	= (1 << 0),

	RPC_WRITE_BUFSZ		= 8192 - 32,
};

enum server_limits {
	SRV_MAX_LINK		= 0xffffffff,	/* max hard links per inode*/
	SRV_MAX_NAME		= 512,		/* max pathname length */
	SRV_MAX_READ		= 1024 * 128,	/* max contig. read */
	SRV_MAX_WRITE		= 1024 * 128,	/* max contig. write */

	SRV_UID_NOBODY		= 65537,	/* arbitrary >64K number */
	SRV_GID_NOBODY		= 65537,	/* arbitrary >64K number */

	SRV_MAX_COMPOUND	= 30000,	/* arbitrary */

	SRV_STATE_HIGH_WAT	= 100000,	/* start gc at this size */
	SRV_STATE_LOW_WAT	= 33000,	/* stop gc at this limit */

	MODE4_ALL		= MODE4_SUID |
				  MODE4_SGID |
				  MODE4_SVTX |
				  MODE4_RUSR |
				  MODE4_WUSR |
				  MODE4_XUSR |
				  MODE4_RGRP |
				  MODE4_WGRP |
				  MODE4_XGRP |
				  MODE4_ROTH |
				  MODE4_WOTH |
				  MODE4_XOTH,
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

enum other_blob_info {
	BLOB_MAGIC		= 0xdeadbeef
};

enum fattr_types {
	FATTR_TYPE_OBJ,
	FATTR_TYPE_FS,
	FATTR_TYPE_SRV,
};

enum {
	fattr_mandatory_ro_mask =
		1ULL << FATTR4_SUPPORTED_ATTRS |
		1ULL << FATTR4_TYPE |
		1ULL << FATTR4_FH_EXPIRE_TYPE |
		1ULL << FATTR4_CHANGE |
		1ULL << FATTR4_LINK_SUPPORT |
		1ULL << FATTR4_SYMLINK_SUPPORT |
		1ULL << FATTR4_NAMED_ATTR |
		1ULL << FATTR4_FSID |
		1ULL << FATTR4_UNIQUE_HANDLES |
		1ULL << FATTR4_LEASE_TIME |
		1ULL << FATTR4_RDATTR_ERROR |
		1ULL << FATTR4_FILEHANDLE,

	fattr_mandatory_rw_mask =
		1ULL << FATTR4_SIZE,

	fattr_write_only_mask =
		1ULL << FATTR4_TIME_ACCESS_SET |
		1ULL << FATTR4_TIME_MODIFY_SET,

	fattr_read_write_mask = fattr_mandatory_rw_mask |
#if 0
		1ULL << FATTR4_ACL |
#endif
		1ULL << FATTR4_ARCHIVE |
		1ULL << FATTR4_HIDDEN |
		1ULL << FATTR4_MODE |
		1ULL << FATTR4_OWNER |
		1ULL << FATTR4_OWNER_GROUP |
		1ULL << FATTR4_SYSTEM |
		1ULL << FATTR4_TIME_BACKUP |
		1ULL << FATTR4_TIME_CREATE,

	fattr_read_only_mask = fattr_mandatory_ro_mask |
#if 0
		1ULL << FATTR4_ACLSUPPORT |
#endif
		1ULL << FATTR4_CANSETTIME |
		1ULL << FATTR4_CASE_INSENSITIVE |
		1ULL << FATTR4_CASE_PRESERVING |
		1ULL << FATTR4_CHOWN_RESTRICTED |
		1ULL << FATTR4_FILEID |
		1ULL << FATTR4_FILES_AVAIL |
		1ULL << FATTR4_FILES_FREE |
		1ULL << FATTR4_FILES_TOTAL |
		1ULL << FATTR4_HOMOGENEOUS |
		1ULL << FATTR4_MAXFILESIZE |
		1ULL << FATTR4_MAXLINK |
		1ULL << FATTR4_MAXNAME |
		1ULL << FATTR4_MAXREAD |
		1ULL << FATTR4_MAXWRITE |
		1ULL << FATTR4_NO_TRUNC |
		1ULL << FATTR4_NUMLINKS |
		1ULL << FATTR4_QUOTA_AVAIL_HARD |
		1ULL << FATTR4_QUOTA_AVAIL_SOFT |
		1ULL << FATTR4_QUOTA_USED |
		1ULL << FATTR4_RAWDEV |
		1ULL << FATTR4_SPACE_AVAIL |
		1ULL << FATTR4_SPACE_FREE |
		1ULL << FATTR4_SPACE_TOTAL |
		1ULL << FATTR4_SPACE_USED |
		1ULL << FATTR4_TIME_ACCESS |
		1ULL << FATTR4_TIME_DELTA |
		1ULL << FATTR4_TIME_METADATA |
		1ULL << FATTR4_TIME_MODIFY |
		1ULL << FATTR4_MOUNTED_ON_FILEID,

	fattr_supported_mask =
		fattr_read_only_mask |
		fattr_read_write_mask |
		fattr_write_only_mask,
};

struct blob {
	unsigned int		magic;
	unsigned int		len;
	void			*buf;
};

struct curbuf {
	char		*buf_start;
	char		*buf;

	unsigned int	len_total;
	unsigned int	len;
};

struct rpc_write {
	unsigned int		len;		/* data buffer space used */
	struct list_head	node;
	char			buf[RPC_WRITE_BUFSZ];
};

struct nfs_buf {
	unsigned int		len;
	char			*val;
};

enum cxn_auth_type {
	auth_none,
	auth_unix
};

struct cxn_auth {
	enum cxn_auth_type		type;

	union {
		struct {
			int		uid;
			int		gid;
		} up;
	} u;
};

struct nfs_cxn {
	nfsino_t		current_fh;
	nfsino_t		save_fh;

	struct cxn_auth		auth;		/* RPC creds */
};

enum nfs_state_type {
	nst_any,
	nst_dead,
	nst_open,
	nst_lock
};

struct nfs_state {
	clientid4		cli;

	enum nfs_state_type	type;

	uint32_t		id;

	char			*owner;

	nfsino_t		ino;

	uint32_t		seq;

	uint32_t		share_ac;
	uint32_t		share_dn;

	nfs_lock_type4		locktype;
	uint64_t		lock_ofs;
	uint64_t		lock_len;

	struct list_head	dead_node;
};

/* overlays stateid4 with our own info in place of 'other' */
struct nfs_stateid {
	uint32_t		seqid;		/* native endian */
	uint32_t		id;		/* fixed endian (LE) */
	verifier4		server_verf;
};

struct nfs_inode {
	nfsino_t		ino;
	enum nfs_ftype4		type;		/* inode type: link, dir, ...*/
	GArray			*parents;	/* list of parent dirs */
	uint64_t		version;

	void			*data;
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
	struct list_head	dead_state;
	unsigned int		n_dead;

	unsigned int		lease_time;

	struct drand48_data	rng;

	verifier4		instance_verf;

	uint64_t		space_used;
};

/* global variables */
extern struct timeval current_time;
extern struct nfs_server srv;
extern int debugging;

/* inode.c */
extern nfsino_t next_ino;
extern struct nfs_inode *inode_get(nfsino_t inum);
extern void inode_touch(struct nfs_inode *ino);
extern bool inode_table_init(void);
extern void inode_unlink(struct nfs_inode *ino, nfsino_t dir_ref);
extern bool nfs_op_create(struct nfs_cxn *cxn, CREATE4args *arg, COMPOUND4res *cres);
extern nfsstat4 nfs_op_access(struct nfs_cxn *cxn, struct curbuf *cur,
		       struct list_head *writes, struct rpc_write **wr);
extern bool nfs_op_getattr(struct nfs_cxn *cxn, GETATTR4args *arg,
		      COMPOUND4res *cres);
extern bool nfs_op_setattr(struct nfs_cxn *cxn, SETATTR4args *arg,
		      COMPOUND4res *cres);
extern bool nfs_op_verify(struct nfs_cxn *cxn, VERIFY4args *arg,
		     COMPOUND4res *cres, int nverify);
extern nfsstat4 inode_add(struct nfs_inode *dir_ino, struct nfs_inode *new_ino,
		   fattr4 *attr, utf8string *name, bitmap4 *attrset,
		   change_info4 *cinfo);
extern struct nfs_inode *inode_new_file(struct nfs_cxn *cxn);
extern enum nfsstat4 inode_apply_attrs(struct nfs_inode *ino, fattr4 *raw_attr,
			        uint64_t *bitmap_set_out,
			        struct nfs_stateid *sid,
			        bool in_setattr);

/* data.c */
extern bool nfs_op_commit(struct nfs_cxn *cxn, COMMIT4args *arg, COMPOUND4res *cres);
extern bool nfs_op_write(struct nfs_cxn *cxn, WRITE4args *arg, COMPOUND4res *cres);
extern bool nfs_op_read(struct nfs_cxn *cxn, READ4args *arg, COMPOUND4res *cres);
extern bool nfs_op_lock(struct nfs_cxn *cxn, LOCK4args *arg, COMPOUND4res *cres);
extern bool nfs_op_testlock(struct nfs_cxn *cxn, LOCKT4args *arg, COMPOUND4res *cres);
extern bool nfs_op_unlock(struct nfs_cxn *cxn, LOCKU4args *arg, COMPOUND4res *cres);

/* dir.c */
bool nfs_op_lookup(struct nfs_cxn *cxn, LOOKUP4args *arg, COMPOUND4res *cres);
bool nfs_op_lookupp(struct nfs_cxn *cxn, COMPOUND4res *cres);
bool nfs_op_link(struct nfs_cxn *cxn, LINK4args *arg, COMPOUND4res *cres);
bool nfs_op_remove(struct nfs_cxn *cxn, REMOVE4args *arg, COMPOUND4res *cres);
bool nfs_op_rename(struct nfs_cxn *cxn, RENAME4args *arg, COMPOUND4res *cres);
enum nfsstat4 dir_add(struct nfs_inode *dir_ino, utf8string *name_in,
		      nfsino_t inum);
void dirent_free(gpointer p);
bool nfs_op_readdir(struct nfs_cxn *cxn, READDIR4args *arg,
		      COMPOUND4res *cres);
nfsstat4 dir_curfh(const struct nfs_cxn *cxn, struct nfs_inode **ino_out);
nfsstat4 dir_lookup(struct nfs_inode *dir_ino, utf8string *str,
		    struct nfs_dirent **dirent_out);
void nfs_readdir_free(READDIR4res *res);

/* fattr.c */
extern bool fattr_encode(fattr4 *raw, struct nfs_fattr_set *attr);
extern bool fattr_decode(fattr4 *raw, struct nfs_fattr_set *attr);
extern void fattr_free(struct nfs_fattr_set *attr);
extern void fattr_fill(struct nfs_inode *ino, struct nfs_fattr_set *attr);
extern void fattr4_free(fattr4 *attr);
extern void print_fattr(const char *pfx, fattr4 *attr);
extern void print_fattr_bitmap(const char *pfx, uint64_t bitmap);

/* fh.c */
bool nfs_op_getfh(struct nfs_cxn *cxn, COMPOUND4res *cres);
bool nfs_op_putfh(struct nfs_cxn *cxn, PUTFH4args *arg, COMPOUND4res *cres);
bool nfs_op_putrootfh(struct nfs_cxn *cxn, COMPOUND4res *cres);
bool nfs_op_putpubfh(struct nfs_cxn *cxn, COMPOUND4res *cres);
bool nfs_op_restorefh(struct nfs_cxn *cxn, COMPOUND4res *cres);
bool nfs_op_savefh(struct nfs_cxn *cxn, COMPOUND4res *cres);
void nfs_getfh_free(GETFH4res *opgetfh);

/* main.c */
extern void *cur_skip(struct curbuf *cur, unsigned int n);
extern uint32_t cur_read32(struct curbuf *cur);
extern void *cur_readmem(struct curbuf *cur, unsigned int n);
extern uint32_t *wr_write32(struct list_head *writes, struct rpc_write **wr_io,uint32_t val);
extern void *wr_skip(struct list_head *writes, struct rpc_write **wr_io,
		     unsigned int n);
extern void *wr_buf(struct list_head *writes, struct rpc_write **wr_io,
		    struct nfs_buf *nb);
extern void *wr_str(struct list_head *writes, struct rpc_write **wr_io, char *s);

/* open.c */
bool nfs_op_open(struct nfs_cxn *cxn, OPEN4args *args, COMPOUND4res *cres);
bool nfs_op_open_confirm(struct nfs_cxn *cxn, OPEN_CONFIRM4args *args, COMPOUND4res *cres);
bool nfs_op_open_downgrade(struct nfs_cxn *cxn, OPEN_DOWNGRADE4args *args, COMPOUND4res *cres);
bool nfs_op_close(struct nfs_cxn *cxn, CLOSE4args *arg, COMPOUND4res *cres);

/* server.c */
extern const char *name_nfs_ftype4[];
extern int cxn_getuid(const struct nfs_cxn *cxn);
extern int cxn_getgid(const struct nfs_cxn *cxn);

extern bool push_resop(COMPOUND4res *res, const nfs_resop4 *resop, nfsstat4 stat);
extern bool valid_utf8string(utf8string *str);
extern char *copy_utf8string(utf8string *str);
extern void nfs_fh_set(nfs_fh4 *fh, nfsino_t fh_int);
extern uint64_t get_bitmap(const bitmap4 *map);
extern void __set_bitmap(uint64_t map_in, bitmap4 *map_out);
extern int set_bitmap(uint64_t map_in, bitmap4 *map_out);
extern int nfs_fh_decode(const nfs_fh4 *fh_in, nfsino_t *fh_out);
extern guint clientid_hash(gconstpointer data);
extern gboolean clientid_equal(gconstpointer _a, gconstpointer _b);
extern void nfsproc_null(struct opaque_auth *cred, struct opaque_auth *verf,
			 struct curbuf *cur, struct list_head *writes,
			 struct rpc_write **wr);
extern void nfsproc_compound(struct opaque_auth *cred, struct opaque_auth *verf,
			     struct curbuf *cur, struct list_head *writes,
			     struct rpc_write **wr);

/* state.c */
extern nfsstat4 clientid_test(clientid4 id);
extern void client_free(gpointer data);
extern void state_free(gpointer data);
extern uint32_t gen_stateid(void);
extern bool nfs_op_setclientid(struct nfs_cxn *cxn, SETCLIENTID4args *args,
			 COMPOUND4res *cres);
extern bool nfs_op_setclientid_confirm(struct nfs_cxn *cxn,
				 SETCLIENTID_CONFIRM4args *arg,
				 COMPOUND4res *cres);
extern void rand_verifier(verifier4 *verf);
extern unsigned long blob_hash(unsigned long hash, const void *_buf, size_t buflen);
extern nfsstat4 stateid_lookup(uint32_t id, nfsino_t ino, enum nfs_state_type type,
			struct nfs_state **st_out);
extern void state_trash(struct nfs_state *st);

static inline void free_bitmap(bitmap4 *map)
{
	free(map->bitmap4_val);
	map->bitmap4_len = 0;
	map->bitmap4_val = NULL;
}

#endif /* __SERVER_H__ */
