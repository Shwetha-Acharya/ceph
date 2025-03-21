#ifndef CEPH_RADOS_H
#define CEPH_RADOS_H

/*
 * Data types for the Ceph distributed object storage layer RADOS
 * (Reliable Autonomic Distributed Object Store).
 */

#include <string.h>
#include <stdbool.h>
#include "msgr.h"

/* See comment in ceph_fs.h.  */
#ifndef __KERNEL__
#include "byteorder.h"
#define __le16 ceph_le16
#define __le32 ceph_le32
#define __le64 ceph_le64
#endif

/*
 * fs id
 */
struct ceph_fsid {
	unsigned char fsid[16];
};

static inline int ceph_fsid_compare(const struct ceph_fsid *a,
				    const struct ceph_fsid *b)
{
	return memcmp(a, b, sizeof(*a));
}

/*
 * ino, object, etc.
 */
typedef __le64 ceph_snapid_t;
#define CEPH_SNAPDIR ((__u64)(-1))  /* reserved for hidden .snap dir */
#define CEPH_NOSNAP  ((__u64)(-2))  /* "head", "live" revision */
#define CEPH_MAXSNAP ((__u64)(-3))  /* largest valid snapid */

struct ceph_timespec {
	__le32 tv_sec;
	__le32 tv_nsec;
} __attribute__ ((packed));


/*
 * object layout - how objects are mapped into PGs
 */
#define CEPH_OBJECT_LAYOUT_HASH     1
#define CEPH_OBJECT_LAYOUT_LINEAR   2
#define CEPH_OBJECT_LAYOUT_HASHINO  3

/*
 * pg layout -- how PGs are mapped onto (sets of) OSDs
 */
#define CEPH_PG_LAYOUT_CRUSH  0
#define CEPH_PG_LAYOUT_HASH   1
#define CEPH_PG_LAYOUT_LINEAR 2
#define CEPH_PG_LAYOUT_HYBRID 3

#define CEPH_PG_MAX_SIZE      16  /* max # osds in a single pg */

/*
 * placement group.
 * we encode this into one __le64.
 */
struct ceph_pg {
	__le16 preferred; /* preferred primary osd */
	__le16 ps;        /* placement seed */
	__le32 pool;      /* object pool */
} __attribute__ ((packed));

/*
 * pg pool types
 *
 * NOTE: These map 1:1 on to the pg_pool_t::TYPE_* values.  They are
 * duplicated here only for CrushCompiler's benefit.
 */
#define CEPH_PG_TYPE_REPLICATED 1
/* #define CEPH_PG_TYPE_RAID4   2   never implemented */
#define CEPH_PG_TYPE_ERASURE 3

/*
 * stable_mod func is used to control number of placement groups.
 * similar to straight-up modulo, but produces a stable mapping as b
 * increases over time.  b is the number of bins, and bmask is the
 * containing power of 2 minus 1.
 *
 * b <= bmask and bmask=(2**n)-1
 * e.g., b=12 -> bmask=15, b=123 -> bmask=127
 *
 * ** This function is released to the public domain by the author. **
 */
static inline int ceph_stable_mod(int x, int b, int bmask)
{
	if ((x & bmask) < b)
		return x & bmask;
	else
		return x & (bmask >> 1);
}

/*
 * object layout - how a given object should be stored.
 */
struct ceph_object_layout {
	struct ceph_pg ol_pgid;   /* raw pg, with _full_ ps precision. */
	__le32 ol_stripe_unit;    /* for per-object parity, if any */
} __attribute__ ((packed));

/*
 * compound epoch+version, used by storage layer to serialize mutations
 */
struct ceph_eversion {
	__le32 epoch;
	__le64 version;
} __attribute__ ((packed));

/*
 * osd map bits
 */

/* status bits */
#define CEPH_OSD_EXISTS       (1<<0)
#define CEPH_OSD_UP           (1<<1)
#define CEPH_OSD_AUTOOUT      (1<<2)  /* osd was automatically marked out */
#define CEPH_OSD_NEW          (1<<3)  /* osd is new, never marked in */
#define CEPH_OSD_FULL         (1<<4)  /* osd is at or above full threshold */
#define CEPH_OSD_NEARFULL     (1<<5)  /* osd is at or above nearfull threshold */
#define CEPH_OSD_BACKFILLFULL (1<<6)  /* osd is at or above backfillfull threshold */
#define CEPH_OSD_DESTROYED    (1<<7)  /* osd has been destroyed */
#define CEPH_OSD_NOUP         (1<<8)  /* osd can not be marked up */
#define CEPH_OSD_NODOWN       (1<<9)  /* osd can not be marked down */
#define CEPH_OSD_NOIN         (1<<10) /* osd can not be marked in */
#define CEPH_OSD_NOOUT        (1<<11) /* osd can not be marked out */
#define CEPH_OSD_STOP         (1<<12) /* osd has been stopped by admin */

extern const char *ceph_osd_state_name(int s);

/* osd weights.  fixed point value: 0x10000 == 1.0 ("in"), 0 == "out" */
#define CEPH_OSD_IN  0x10000
#define CEPH_OSD_OUT 0

#define CEPH_OSD_MAX_PRIMARY_AFFINITY 0x10000
#define CEPH_OSD_DEFAULT_PRIMARY_AFFINITY 0x10000


/*
 * osd map flag bits
 */
#define CEPH_OSDMAP_NEARFULL         (1<<0)  /* sync writes (near ENOSPC), deprecated since mimic*/
#define CEPH_OSDMAP_FULL             (1<<1)  /* no data writes (ENOSPC), deprecated since mimic */
#define CEPH_OSDMAP_PAUSERD          (1<<2)  /* pause all reads */
#define CEPH_OSDMAP_PAUSEWR          (1<<3)  /* pause all writes */
#define CEPH_OSDMAP_PAUSEREC         (1<<4)  /* pause recovery */
#define CEPH_OSDMAP_NOUP             (1<<5)  /* block osd boot */
#define CEPH_OSDMAP_NODOWN           (1<<6)  /* block osd mark-down/failure */
#define CEPH_OSDMAP_NOOUT            (1<<7)  /* block osd auto mark-out */
#define CEPH_OSDMAP_NOIN             (1<<8)  /* block osd auto mark-in */
#define CEPH_OSDMAP_NOBACKFILL       (1<<9)  /* block osd backfill */
#define CEPH_OSDMAP_NORECOVER        (1<<10) /* block osd recovery and backfill */
#define CEPH_OSDMAP_NOSCRUB          (1<<11) /* block periodic scrub */
#define CEPH_OSDMAP_NODEEP_SCRUB     (1<<12) /* block periodic deep-scrub */
#define CEPH_OSDMAP_NOTIERAGENT      (1<<13) /* disable tiering agent */
#define CEPH_OSDMAP_NOREBALANCE      (1<<14) /* block osd backfill unless pg is degraded */
#define CEPH_OSDMAP_SORTBITWISE      (1<<15) /* use bitwise hobject_t sort */
#define CEPH_OSDMAP_REQUIRE_JEWEL    (1<<16) /* require jewel for booting osds */
#define CEPH_OSDMAP_REQUIRE_KRAKEN   (1<<17) /* require kraken for booting osds */
#define CEPH_OSDMAP_REQUIRE_LUMINOUS (1<<18) /* require l for booting osds */
#define CEPH_OSDMAP_RECOVERY_DELETES (1<<19) /* deletes performed during recovery instead of peering */
#define CEPH_OSDMAP_PURGED_SNAPDIRS  (1<<20) /* osds have converted snapsets */
#define CEPH_OSDMAP_NOSNAPTRIM       (1<<21) /* disable snap trimming */
#define CEPH_OSDMAP_PGLOG_HARDLIMIT  (1<<22) /* put a hard limit on pg log length */
#define CEPH_OSDMAP_NOAUTOSCALE      (1<<23)  /* block pg autoscale */

/* these are hidden in 'ceph status' view */
#define CEPH_OSDMAP_SEMIHIDDEN_FLAGS (CEPH_OSDMAP_REQUIRE_JEWEL|	\
				      CEPH_OSDMAP_REQUIRE_KRAKEN |	\
				      CEPH_OSDMAP_REQUIRE_LUMINOUS |	\
				      CEPH_OSDMAP_RECOVERY_DELETES |	\
				      CEPH_OSDMAP_SORTBITWISE |		\
				      CEPH_OSDMAP_PURGED_SNAPDIRS |     \
                                      CEPH_OSDMAP_PGLOG_HARDLIMIT)
#define CEPH_OSDMAP_LEGACY_REQUIRE_FLAGS (CEPH_OSDMAP_REQUIRE_JEWEL |	\
					  CEPH_OSDMAP_REQUIRE_KRAKEN |	\
					  CEPH_OSDMAP_REQUIRE_LUMINOUS)

/*
 * major ceph release numbers
 */
#define CEPH_RELEASE_ARGONAUT    1
#define CEPH_RELEASE_BOBTAIL     2
#define CEPH_RELEASE_CUTTLEFISH  3
#define CEPH_RELEASE_DUMPLING    4
#define CEPH_RELEASE_EMPEROR     5
#define CEPH_RELEASE_FIREFLY     6
#define CEPH_RELEASE_GIANT       7
#define CEPH_RELEASE_HAMMER      8
#define CEPH_RELEASE_INFERNALIS  9
#define CEPH_RELEASE_JEWEL      10
#define CEPH_RELEASE_KRAKEN     11
#define CEPH_RELEASE_LUMINOUS   12
#define CEPH_RELEASE_MIMIC      13
#define CEPH_RELEASE_NAUTILUS   14
#define CEPH_RELEASE_OCTOPUS    15
#define CEPH_RELEASE_PACIFIC    16
#define CEPH_RELEASE_QUINCY     17
#define CEPH_RELEASE_REEF       18
#define CEPH_RELEASE_SQUID      19
#define CEPH_RELEASE_TENTACLE   20
#define CEPH_RELEASE_MAX        21  /* highest + 1 */

/*
 * The error code to return when an OSD can't handle a write
 * because it is too large.
 */
#define OSD_WRITETOOBIG EMSGSIZE

/*
 * osd ops
 *
 * WARNING: do not use these op codes directly.  Use the helpers
 * defined below instead.  In certain cases, op code behavior was
 * redefined, resulting in special-cases in the helpers.
 */
#define CEPH_OSD_OP_MODE       0xf000
#define CEPH_OSD_OP_MODE_RD    0x1000
#define CEPH_OSD_OP_MODE_WR    0x2000
#define CEPH_OSD_OP_MODE_RMW   0x3000
#define CEPH_OSD_OP_MODE_SUB   0x4000
#define CEPH_OSD_OP_MODE_CACHE 0x8000

#define CEPH_OSD_OP_TYPE       0x0f00
#define CEPH_OSD_OP_TYPE_DATA  0x0200
#define CEPH_OSD_OP_TYPE_ATTR  0x0300
#define CEPH_OSD_OP_TYPE_EXEC  0x0400
#define CEPH_OSD_OP_TYPE_PG    0x0500
//      LEAVE UNUSED           0x0600 used to be multiobject ops

#define __CEPH_OSD_OP1(mode, nr) \
	(CEPH_OSD_OP_MODE_##mode | (nr))

#define __CEPH_OSD_OP(mode, type, nr) \
	(CEPH_OSD_OP_MODE_##mode | CEPH_OSD_OP_TYPE_##type | (nr))

#define __CEPH_FORALL_OSD_OPS(f)					    \
	/** data **/							    \
	/* read */							    \
	f(READ,		__CEPH_OSD_OP(RD, DATA, 1),	"read")		    \
	f(STAT,		__CEPH_OSD_OP(RD, DATA, 2),	"stat")		    \
	f(MAPEXT,	__CEPH_OSD_OP(RD, DATA, 3),	"mapext")	    \
	f(CHECKSUM,	__CEPH_OSD_OP(RD, DATA, 31),	"checksum")	    \
									    \
	/* fancy read */						    \
	f(MASKTRUNC,	__CEPH_OSD_OP(RD, DATA, 4),	"masktrunc")	    \
	f(SPARSE_READ,	__CEPH_OSD_OP(RD, DATA, 5),	"sparse-read")	    \
									    \
	f(NOTIFY,	__CEPH_OSD_OP(RD, DATA, 6),	"notify")	    \
	f(NOTIFY_ACK,	__CEPH_OSD_OP(RD, DATA, 7),	"notify-ack")	    \
									    \
	/* versioning */						    \
	f(ASSERT_VER,	__CEPH_OSD_OP(RD, DATA, 8),	"assert-version")   \
									    \
	f(LIST_WATCHERS, __CEPH_OSD_OP(RD, DATA, 9),	"list-watchers")    \
									    \
	f(LIST_SNAPS,	__CEPH_OSD_OP(RD, DATA, 10),	"list-snaps")	    \
									    \
	/* sync */							    \
	f(SYNC_READ,	__CEPH_OSD_OP(RD, DATA, 11),	"sync_read")	    \
									    \
	/* write */							    \
	f(WRITE,	__CEPH_OSD_OP(WR, DATA, 1),	"write")	    \
	f(WRITEFULL,	__CEPH_OSD_OP(WR, DATA, 2),	"writefull")	    \
	f(TRUNCATE,	__CEPH_OSD_OP(WR, DATA, 3),	"truncate")	    \
	f(ZERO,		__CEPH_OSD_OP(WR, DATA, 4),	"zero")		    \
	f(DELETE,	__CEPH_OSD_OP(WR, DATA, 5),	"delete")	    \
									    \
	/* fancy write */						    \
	f(APPEND,	__CEPH_OSD_OP(WR, DATA, 6),	"append")	    \
	f(STARTSYNC,	__CEPH_OSD_OP(WR, DATA, 7),	"startsync")	    \
	f(SETTRUNC,	__CEPH_OSD_OP(WR, DATA, 8),	"settrunc")	    \
	f(TRIMTRUNC,	__CEPH_OSD_OP(WR, DATA, 9),	"trimtrunc")	    \
									    \
	f(TMAPUP,	__CEPH_OSD_OP(RMW, DATA, 10),	"tmapup")	    \
	f(TMAPPUT,	__CEPH_OSD_OP(WR, DATA, 11),	"tmapput")	    \
	f(TMAPGET,	__CEPH_OSD_OP(RD, DATA, 12),	"tmapget")	    \
									    \
	f(CREATE,	__CEPH_OSD_OP(WR, DATA, 13),	"create")	    \
	f(ROLLBACK,	__CEPH_OSD_OP(WR, DATA, 14),	"rollback")	    \
									    \
	f(WATCH,	__CEPH_OSD_OP(WR, DATA, 15),	"watch")	    \
									    \
	/* omap */							    \
	f(OMAPGETKEYS,	__CEPH_OSD_OP(RD, DATA, 17),	"omap-get-keys")    \
	f(OMAPGETVALS,	__CEPH_OSD_OP(RD, DATA, 18),	"omap-get-vals")    \
	f(OMAPGETHEADER, __CEPH_OSD_OP(RD, DATA, 19),	"omap-get-header")  \
	f(OMAPGETVALSBYKEYS, __CEPH_OSD_OP(RD, DATA, 20), "omap-get-vals-by-keys") \
	f(OMAPSETVALS,	__CEPH_OSD_OP(WR, DATA, 21),	"omap-set-vals")    \
	f(OMAPSETHEADER, __CEPH_OSD_OP(WR, DATA, 22),	"omap-set-header")  \
	f(OMAPCLEAR,	__CEPH_OSD_OP(WR, DATA, 23),	"omap-clear")	    \
	f(OMAPRMKEYS,	__CEPH_OSD_OP(WR, DATA, 24),	"omap-rm-keys")	    \
	f(OMAPRMKEYRANGE, __CEPH_OSD_OP(WR, DATA, 44),	"omap-rm-key-range") \
	f(OMAP_CMP,	__CEPH_OSD_OP(RD, DATA, 25),	"omap-cmp")	    \
									    \
	/* tiering */							    \
	f(COPY_FROM,	__CEPH_OSD_OP(WR, DATA, 26),	"copy-from")	    \
	f(COPY_FROM2,	__CEPH_OSD_OP(WR, DATA, 45),	"copy-from2")	    \
	/* was copy-get-classic */					\
	f(UNDIRTY,	__CEPH_OSD_OP(WR, DATA, 28),	"undirty")	    \
	f(ISDIRTY,	__CEPH_OSD_OP(RD, DATA, 29),	"isdirty")	    \
	f(COPY_GET,	__CEPH_OSD_OP(RD, DATA, 30),	"copy-get")	    \
	f(CACHE_FLUSH,	__CEPH_OSD_OP(CACHE, DATA, 31),	"cache-flush")	    \
	f(CACHE_EVICT,	__CEPH_OSD_OP(CACHE, DATA, 32),	"cache-evict")	    \
	f(CACHE_TRY_FLUSH, __CEPH_OSD_OP(CACHE, DATA, 33), "cache-try-flush") \
									    \
	/* convert tmap to omap */					    \
	f(TMAP2OMAP,	__CEPH_OSD_OP(RMW, DATA, 34),	"tmap2omap")	    \
									    \
	/* hints */							    \
	f(SETALLOCHINT,	__CEPH_OSD_OP(WR, DATA, 35),	"set-alloc-hint")   \
                                                                            \
	/* cache pin/unpin */						    \
	f(CACHE_PIN,	__CEPH_OSD_OP(WR, DATA, 36),	"cache-pin")        \
	f(CACHE_UNPIN,	__CEPH_OSD_OP(WR, DATA, 37),	"cache-unpin")      \
									    \
	/* ESX/SCSI */							    \
	f(WRITESAME,	__CEPH_OSD_OP(WR, DATA, 38),	"write-same")	    \
	f(CMPEXT,	__CEPH_OSD_OP(RD, DATA, 32),	"cmpext")	    \
									    \
	/* Extensible */						    \
	f(SET_REDIRECT,	__CEPH_OSD_OP(WR, DATA, 39),	"set-redirect")	    \
	f(SET_CHUNK,	__CEPH_OSD_OP(CACHE, DATA, 40),	"set-chunk")	    \
	f(TIER_PROMOTE,	__CEPH_OSD_OP(WR, DATA, 41),	"tier-promote")	    \
	f(UNSET_MANIFEST, __CEPH_OSD_OP(WR, DATA, 42),	"unset-manifest")   \
	f(TIER_FLUSH, __CEPH_OSD_OP(CACHE, DATA, 43),	"tier-flush")	    \
	f(TIER_EVICT, __CEPH_OSD_OP(CACHE, DATA, 44),	"tier-evict")	    \
									    \
	/** attrs **/							    \
	/* read */							    \
	f(GETXATTR,	__CEPH_OSD_OP(RD, ATTR, 1),	"getxattr")	    \
	f(GETXATTRS,	__CEPH_OSD_OP(RD, ATTR, 2),	"getxattrs")	    \
	f(CMPXATTR,	__CEPH_OSD_OP(RD, ATTR, 3),	"cmpxattr")	    \
									    \
	/* write */							    \
	f(SETXATTR,	__CEPH_OSD_OP(WR, ATTR, 1),	"setxattr")	    \
	f(SETXATTRS,	__CEPH_OSD_OP(WR, ATTR, 2),	"setxattrs")	    \
	f(RESETXATTRS,	__CEPH_OSD_OP(WR, ATTR, 3),	"resetxattrs")	    \
	f(RMXATTR,	__CEPH_OSD_OP(WR, ATTR, 4),	"rmxattr")	    \
									    \
	/** subop **/							    \
	f(PULL,		__CEPH_OSD_OP1(SUB, 1),		"pull")		    \
	f(PUSH,		__CEPH_OSD_OP1(SUB, 2),		"push")		    \
	f(BALANCEREADS,	__CEPH_OSD_OP1(SUB, 3),		"balance-reads")    \
	f(UNBALANCEREADS, __CEPH_OSD_OP1(SUB, 4),	"unbalance-reads")  \
	f(SCRUB,	__CEPH_OSD_OP1(SUB, 5),		"scrub")	    \
	f(SCRUB_RESERVE, __CEPH_OSD_OP1(SUB, 6),	"scrub-reserve")    \
	f(SCRUB_UNRESERVE, __CEPH_OSD_OP1(SUB, 7),	"scrub-unreserve")  \
	/* 8 used to be scrub-stop */					\
	f(SCRUB_MAP,	__CEPH_OSD_OP1(SUB, 9),		"scrub-map")	    \
									    \
	/** exec **/							    \
	/* note: the RD bit here is wrong; see special-case below in helper */ \
	f(CALL,		__CEPH_OSD_OP(RD, EXEC, 1),	"call")		    \
									    \
	/** pg **/							    \
	f(PGLS,		__CEPH_OSD_OP(RD, PG, 1),	"pgls")		    \
	f(PGLS_FILTER,	__CEPH_OSD_OP(RD, PG, 2),	"pgls-filter")	    \
	f(PG_HITSET_LS,	__CEPH_OSD_OP(RD, PG, 3),	"pg-hitset-ls")	    \
	f(PG_HITSET_GET, __CEPH_OSD_OP(RD, PG, 4),	"pg-hitset-get")    \
	f(PGNLS,	__CEPH_OSD_OP(RD, PG, 5),	"pgnls")	    \
	f(PGNLS_FILTER,	__CEPH_OSD_OP(RD, PG, 6),	"pgnls-filter")     \
	f(SCRUBLS, __CEPH_OSD_OP(RD, PG, 7), "scrubls")

enum {
#define GENERATE_ENUM_ENTRY(op, opcode, str)	CEPH_OSD_OP_##op = (opcode),
__CEPH_FORALL_OSD_OPS(GENERATE_ENUM_ENTRY)
#undef GENERATE_ENUM_ENTRY
};

static inline int ceph_osd_op_type_data(int op)
{
	return (op & CEPH_OSD_OP_TYPE) == CEPH_OSD_OP_TYPE_DATA;
}
static inline int ceph_osd_op_type_attr(int op)
{
	return (op & CEPH_OSD_OP_TYPE) == CEPH_OSD_OP_TYPE_ATTR;
}
static inline int ceph_osd_op_type_exec(int op)
{
	return (op & CEPH_OSD_OP_TYPE) == CEPH_OSD_OP_TYPE_EXEC;
}
static inline int ceph_osd_op_type_pg(int op)
{
	return (op & CEPH_OSD_OP_TYPE) == CEPH_OSD_OP_TYPE_PG;
}

static inline int ceph_osd_op_mode_subop(int op)
{
	return (op & CEPH_OSD_OP_MODE) == CEPH_OSD_OP_MODE_SUB;
}
static inline int ceph_osd_op_mode_read(int op)
{
	return (op & CEPH_OSD_OP_MODE_RD) &&
		op != CEPH_OSD_OP_CALL;
}
static inline int ceph_osd_op_mode_modify(int op)
{
	return op & CEPH_OSD_OP_MODE_WR;
}
static inline int ceph_osd_op_mode_cache(int op)
{
	return op & CEPH_OSD_OP_MODE_CACHE;
}
static inline bool ceph_osd_op_uses_extent(int op)
{
	switch(op) {
	case CEPH_OSD_OP_READ:
	case CEPH_OSD_OP_MAPEXT:
	case CEPH_OSD_OP_MASKTRUNC:
	case CEPH_OSD_OP_SPARSE_READ:
	case CEPH_OSD_OP_SYNC_READ:
	case CEPH_OSD_OP_WRITE:
	case CEPH_OSD_OP_WRITEFULL:
	case CEPH_OSD_OP_TRUNCATE:
	case CEPH_OSD_OP_ZERO:
	case CEPH_OSD_OP_APPEND:
	case CEPH_OSD_OP_TRIMTRUNC:
	case CEPH_OSD_OP_CMPEXT:
		return true;
	default:
		return false;
	}
}

/*
 * note that the following tmap stuff is also defined in the ceph librados.h
 * and objclass.h. Any modification here needs to be updated there
 */
#define CEPH_OSD_TMAP_HDR 'h'
#define CEPH_OSD_TMAP_SET 's'
#define CEPH_OSD_TMAP_CREATE 'c' /* create key */
#define CEPH_OSD_TMAP_RM  'r'
#define CEPH_OSD_TMAP_RMSLOPPY 'R'

extern const char *ceph_osd_op_name(int op);

/*
 * osd op flags
 *
 * An op may be READ, WRITE, or READ|WRITE.
 */
enum {
	CEPH_OSD_FLAG_ACK =            0x0001,  /* want (or is) "ack" ack */
	CEPH_OSD_FLAG_ONNVRAM =        0x0002,  /* want (or is) "onnvram" ack */
	CEPH_OSD_FLAG_ONDISK =         0x0004,  /* want (or is) "ondisk" ack */
	CEPH_OSD_FLAG_RETRY =          0x0008,  /* resend attempt */
	CEPH_OSD_FLAG_READ =           0x0010,  /* op may read */
	CEPH_OSD_FLAG_WRITE =          0x0020,  /* op may write */
	CEPH_OSD_FLAG_ORDERSNAP =      0x0040,  /* EOLDSNAP if snapc is out of order */
	CEPH_OSD_FLAG_PEERSTAT_OLD =   0x0080,  /* DEPRECATED msg includes osd_peer_stat */
	CEPH_OSD_FLAG_BALANCE_READS =  0x0100,
	CEPH_OSD_FLAG_PARALLELEXEC =   0x0200,  /* execute op in parallel */
	CEPH_OSD_FLAG_PGOP =           0x0400,  /* pg op, no object */
	CEPH_OSD_FLAG_EXEC =           0x0800,  /* op may exec */
	CEPH_OSD_FLAG_EXEC_PUBLIC =    0x1000,  /* DEPRECATED op may exec (public) */
	CEPH_OSD_FLAG_LOCALIZE_READS = 0x2000,  /* read from nearby replica, if any */
	CEPH_OSD_FLAG_RWORDERED =      0x4000,  /* order wrt concurrent reads */
	CEPH_OSD_FLAG_IGNORE_CACHE =   0x8000,  /* ignore cache logic */
	CEPH_OSD_FLAG_SKIPRWLOCKS =   0x10000,  /* skip rw locks */
	CEPH_OSD_FLAG_IGNORE_OVERLAY =0x20000,  /* ignore pool overlay */
	CEPH_OSD_FLAG_FLUSH =         0x40000,  /* this is part of flush */
	CEPH_OSD_FLAG_MAP_SNAP_CLONE =0x80000,  /* map snap direct to clone id
						 */
	CEPH_OSD_FLAG_ENFORCE_SNAPC    =0x100000,  /* use snapc provided even if
						      pool uses pool snaps */
	CEPH_OSD_FLAG_REDIRECTED   = 0x200000,  /* op has been redirected */
	CEPH_OSD_FLAG_KNOWN_REDIR = 0x400000,  /* redirect bit is authoritative */
	CEPH_OSD_FLAG_FULL_TRY =    0x800000,  /* try op despite full flag */
	CEPH_OSD_FLAG_FULL_FORCE = 0x1000000,  /* force op despite full flag */
	CEPH_OSD_FLAG_IGNORE_REDIRECT = 0x2000000,  /* ignore redirection */
	CEPH_OSD_FLAG_RETURNVEC = 0x4000000, /* allow overall result >= 0, and return >= 0 and buffer for each op in opvec */
	CEPH_OSD_FLAG_SUPPORTSPOOLEIO = 0x8000000,   /* client understands pool EIO flag */
};

enum {
	CEPH_OSD_OP_FLAG_EXCL = 0x1,      /* EXCL object create */
	CEPH_OSD_OP_FLAG_FAILOK = 0x2,    /* continue despite failure */
	CEPH_OSD_OP_FLAG_FADVISE_RANDOM     = 0x4, /* the op is random */
	CEPH_OSD_OP_FLAG_FADVISE_SEQUENTIAL = 0x8, /* the op is sequential */
	CEPH_OSD_OP_FLAG_FADVISE_WILLNEED   = 0x10,/* data will be accessed in the near future */
	CEPH_OSD_OP_FLAG_FADVISE_DONTNEED   = 0x20,/* data will not be accessed in the near future */
	CEPH_OSD_OP_FLAG_FADVISE_NOCACHE   = 0x40, /* data will be accessed only once by this client */
	CEPH_OSD_OP_FLAG_WITH_REFERENCE   = 0x80, /* need reference couting */
	CEPH_OSD_OP_FLAG_BYPASS_CLEAN_CACHE = 0x100, /* bypass ObjectStore cache, mainly for deep-scrub */
};

#define EOLDSNAPC    85  /* ORDERSNAP flag set; writer has old snapc*/
#define EBLOCKLISTED 108 /* blocklisted */
#define EBLACKLISTED 108 /* deprecated */

/* xattr comparison */
enum {
	CEPH_OSD_CMPXATTR_OP_EQ  = 1,
	CEPH_OSD_CMPXATTR_OP_NE  = 2,
	CEPH_OSD_CMPXATTR_OP_GT  = 3,
	CEPH_OSD_CMPXATTR_OP_GTE = 4,
	CEPH_OSD_CMPXATTR_OP_LT  = 5,
	CEPH_OSD_CMPXATTR_OP_LTE = 6
};

enum {
	CEPH_OSD_CMPXATTR_MODE_STRING = 1,
	CEPH_OSD_CMPXATTR_MODE_U64    = 2
};

enum {
	CEPH_OSD_COPY_FROM_FLAG_FLUSH = 1,     /* part of a flush operation */
	CEPH_OSD_COPY_FROM_FLAG_IGNORE_OVERLAY = 2,  /* ignore pool overlay */
	CEPH_OSD_COPY_FROM_FLAG_IGNORE_CACHE = 4, /* ignore osd cache logic */
	CEPH_OSD_COPY_FROM_FLAG_MAP_SNAP_CLONE = 8, /* map snap direct to
						     * cloneid */
	CEPH_OSD_COPY_FROM_FLAG_RWORDERED = 16, /* order with write */
	CEPH_OSD_COPY_FROM_FLAG_TRUNCATE_SEQ = 32, /* use provided truncate_{seq,size} (copy-from2 only) */
};

#define CEPH_OSD_COPY_FROM_FLAGS			\
	(CEPH_OSD_COPY_FROM_FLAG_FLUSH |		\
	 CEPH_OSD_COPY_FROM_FLAG_IGNORE_OVERLAY |	\
	 CEPH_OSD_COPY_FROM_FLAG_IGNORE_CACHE |		\
	 CEPH_OSD_COPY_FROM_FLAG_MAP_SNAP_CLONE |	\
	 CEPH_OSD_COPY_FROM_FLAG_RWORDERED |		\
	 CEPH_OSD_COPY_FROM_FLAG_TRUNCATE_SEQ)

enum {
	CEPH_OSD_TMAP2OMAP_NULLOK = 1,
};

enum {
	CEPH_OSD_WATCH_OP_UNWATCH = 0,
	CEPH_OSD_WATCH_OP_LEGACY_WATCH = 1,
	/* note: use only ODD ids to prevent pre-giant code from
	   interpreting the op as UNWATCH */
	CEPH_OSD_WATCH_OP_WATCH = 3,
	CEPH_OSD_WATCH_OP_RECONNECT = 5,
	CEPH_OSD_WATCH_OP_PING = 7,
};

enum {
	CEPH_OSD_CHECKSUM_OP_TYPE_XXHASH32 = 0,
	CEPH_OSD_CHECKSUM_OP_TYPE_XXHASH64 = 1,
	CEPH_OSD_CHECKSUM_OP_TYPE_CRC32C   = 2
};

const char *ceph_osd_watch_op_name(int o);

enum {
	CEPH_OSD_ALLOC_HINT_FLAG_SEQUENTIAL_WRITE = 1,
	CEPH_OSD_ALLOC_HINT_FLAG_RANDOM_WRITE = 2,
	CEPH_OSD_ALLOC_HINT_FLAG_SEQUENTIAL_READ = 4,
	CEPH_OSD_ALLOC_HINT_FLAG_RANDOM_READ = 8,
	CEPH_OSD_ALLOC_HINT_FLAG_APPEND_ONLY = 16,
	CEPH_OSD_ALLOC_HINT_FLAG_IMMUTABLE = 32,
	CEPH_OSD_ALLOC_HINT_FLAG_SHORTLIVED = 64,
	CEPH_OSD_ALLOC_HINT_FLAG_LONGLIVED = 128,
	CEPH_OSD_ALLOC_HINT_FLAG_COMPRESSIBLE = 256,
	CEPH_OSD_ALLOC_HINT_FLAG_INCOMPRESSIBLE = 512,
	CEPH_OSD_ALLOC_HINT_FLAG_LOG = 1024,
};

const char *ceph_osd_alloc_hint_flag_name(int f);

enum {
	CEPH_OSD_BACKOFF_OP_BLOCK = 1,
	CEPH_OSD_BACKOFF_OP_ACK_BLOCK = 2,
	CEPH_OSD_BACKOFF_OP_UNBLOCK = 3,
};

const char *ceph_osd_backoff_op_name(int op);

/*
 * an individual object operation.  each may be accompanied by some data
 * payload
 */
struct ceph_osd_op {
	__le16 op;           /* CEPH_OSD_OP_* */
	__le32 flags;        /* CEPH_OSD_OP_FLAG_* */
	union {
		struct {
			__le64 offset, length;
			__le64 truncate_size;
			__le32 truncate_seq;
		} __attribute__ ((packed)) extent;
		struct {
			__le32 name_len;
			__le32 value_len;
			__u8 cmp_op;       /* CEPH_OSD_CMPXATTR_OP_* */
			__u8 cmp_mode;     /* CEPH_OSD_CMPXATTR_MODE_* */
		} __attribute__ ((packed)) xattr;
		struct {
			__u8 class_len;
			__u8 method_len;
			__u8 argc;
			__le32 indata_len;
		} __attribute__ ((packed)) cls;
		struct {
			__le64 count;
			__le32 start_epoch; /* for the pgls sequence */
		} __attribute__ ((packed)) pgls;
	        struct {
		        __le64 snapid;
	        } __attribute__ ((packed)) snap;
		struct {
			__le64 cookie;
			__le64 ver;     /* no longer used */
			__u8 op;	/* CEPH_OSD_WATCH_OP_* */
			__u32 gen;      /* registration generation */
			__u32 timeout; /* connection timeout */
		} __attribute__ ((packed)) watch;
		struct {
			__le64 cookie;
		} __attribute__ ((packed)) notify;
		struct {
			__le64 unused;
			__le64 ver;
		} __attribute__ ((packed)) assert_ver;
		struct {
			__le64 offset, length;
			__le64 src_offset;
		} __attribute__ ((packed)) clonerange;
		struct {
			__le64 max;     /* max data in reply */
		} __attribute__ ((packed)) copy_get;
		struct {
			__le64 snapid;
			__le64 src_version;
			__u8 flags; /* CEPH_OSD_COPY_FROM_FLAG_* */
			/*
			 * CEPH_OSD_OP_FLAG_FADVISE_*: fadvise flags
			 * for src object, flags for dest object are in
			 * ceph_osd_op::flags.
			 */
			__le32 src_fadvise_flags;
		} __attribute__ ((packed)) copy_from;
		struct {
			struct ceph_timespec stamp;
		} __attribute__ ((packed)) hit_set_get;
		struct {
			__u8 flags;
		} __attribute__ ((packed)) tmap2omap;
		struct {
			__le64 expected_object_size;
			__le64 expected_write_size;
			__le32 flags;  /* CEPH_OSD_OP_ALLOC_HINT_FLAG_* */
		} __attribute__ ((packed)) alloc_hint;
		struct {
			__le64 offset;
			__le64 length;
			__le64 data_length;
		} __attribute__ ((packed)) writesame;
		struct {
			__le64 offset;
			__le64 length;
			__le32 chunk_size;
			__u8 type;              /* CEPH_OSD_CHECKSUM_OP_TYPE_* */
		} __attribute__ ((packed)) checksum;
	} __attribute__ ((packed));
	__le32 payload_len;
} __attribute__ ((packed));

/*
 * Check the compatibility of struct ceph_osd_op
 *  (2+4+(2*8+8+4)+4) = (sizeof(ceph_osd_op::op) +
 *                     sizeof(ceph_osd_op::flags) +
 *                     sizeof(ceph_osd_op::extent) +
 *                     sizeof(ceph_osd_op::payload_len))
 */
#ifdef __cplusplus
static_assert(sizeof(ceph_osd_op) == (2+4+(2*8+8+4)+4),
              "sizeof(ceph_osd_op) breaks the compatibility");
#endif

struct ceph_osd_reply_head {
	__le32 client_inc;                /* client incarnation */
	__le32 flags;
	struct ceph_object_layout layout;
	__le32 osdmap_epoch;
	struct ceph_eversion reassert_version; /* for replaying uncommitted */

	__le32 result;                    /* result code */

	__le32 object_len;                /* length of object name */
	__le32 num_ops;
	struct ceph_osd_op ops[0];  /* ops[], object */
} __attribute__ ((packed));

#ifndef __KERNEL__
#undef __le16
#undef __le32
#undef __le64
#endif

#endif
