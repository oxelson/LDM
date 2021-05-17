/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#ifndef _LDM_H_RPCGEN
#define _LDM_H_RPCGEN

#include <rpc/rpc.h>


#ifdef __cplusplus
extern "C" {
#endif

#ifndef __USE_BSD
# define __USE_BSD // to get `u_int`
#endif

#include <netinet/in.h> /* struct in_addr */
#include <signal.h> /* sig_atomic_t */
#include <stdlib.h> /* at least malloc() */
#include <sys/time.h> /* timeval */
#include <sys/types.h>
#include <regex.h>

#include "timestamp.h"
/*
 * these define the range of "transient program numbers"
 */
#define TRANSIENT_BEGIN 0x40000000
#define TRANSIENT_END 0x5fffffff

/*
 * This is the Internet port number assigned to the ldm by the NIC.
 * We wanted a reserved port so IP layer port moniters could be
 * used for statistics.
 */
#ifndef LDM_PORT
#define LDM_PORT 388
#endif

/*
 * Note: there is a dependency between these #defines and atofeed.c fassoc -aw
 */

/*
 * the empty set
 */
#define NONE 0
#define FT0 1
/*
 * Public Products Service
 */
#define PPS 1
#define FT1 2
/*
 * Domestic Data Service
 */
#define DDS 2
/*
 * Zephyr Domestic Data PLUS = PPS union DDS
 */
#define DDPLUS 3
#define FT2 4
/*
 * High Res. Data Service. Replaces NPS
 */
#define HDS 4
/*
 * Another name for High Res. Data Service.
 */
#define HRS 4
#define FT3 8
/*
 * International products
 */
#define IDS 8
/*
 * Old name for International products
 */
#define INTNL 8
#define FT4 16
/*
 * spare, formerly Numerical Products Service
 */
#define SPARE 16
/*
 * Any of the above... WMO format products except SPARE
 */
#define WMO 15
#define FT5 32
/*
 * Unidata/Wisconsin Broadcast
 */
#define UNIWISC 32
#define MCIDAS 32
/*
 * All of the above
 */
#define UNIDATA 47
#define FT6 64
/*
 * Forecast Systems Lab PC DARE workstation feed
 */
#define PCWS 64
#define ACARS 64
#define FT7 128
/*
 * FSL profiler data
 */
#define FSL2 128
#define PROFILER 128
#define FT8 256
#define FSL3 256
#define FT9 512
#define FSL4 512
#define FT10 1024
#define FSL5 1024
/*
 * Any of 64,128,256,512,or 1024
 */
#define FSL 1984
#define FT11 2048
#define AFOS 2048
/*
 * GPS gathering feed
 */
#define GPSSRC 2048
#define FT12 4096
/*
 * CONDUIT data
 */
#define CONDUIT 4096
#define NMC2 4096
#define NCEPH 4096
#define FT13 8192
#define NMC3 8192
#define FNEXRAD 8192
/*
 * Any of 2048, 4096, 8192
 */
#define NMC 14336
#define FT14 16384
/*
 * National Lighting Data Network
 */
#define NLDN 16384
#define FT15 32768
/*
 * NIDS products
 */
#define WSI 32768
#define FT16 65536
/*
 * SATELLITE products (was DIFAX)
 */
#define DIFAX 65536
#define SATELLITE 65536
#define FT17 131072
/*
 * FAA604 products
 */
#define FAA604 131072
#define FT18 262144
/*
 * GPS data - UNAVACO
 */
#define GPS 262144
#define FT19 524288
/*
 * Seismic data - IRIS
 */
#define SEISMIC 524288
#define NOGAPS 524288
#define FNMOC 524288
#define FT20 1048576
/*
 * Canadian Model Data
 */
#define CMC 1048576
#define GEM 1048576
#define FT21 2097152
/*
 * NOAAport imagery
 */
#define NIMAGE 2097152
#define IMAGE 2097152
#define FT22 4194304
/*
 * NOAAport text
 */
#define NTEXT 4194304
#define TEXT 4194304
#define FT23 8388608
/*
 * NOAAport grided products
 */
#define NGRID 8388608
#define GRID 8388608
#define FT24 16777216
/*
 * NOAAport point
 */
#define NPOINT 16777216
#define POINT 16777216
/*
 * NOAAport BUFR
 */
#define NBUFR 16777216
#define BUFR 16777216
#define FT25 33554432
/*
 * NOAAport graphics
 */
#define NGRAPH 33554432
#define GRAPH 33554432
#define FT26 67108864
/*
 * NOAAport other data
 */
#define NOTHER 67108864
#define OTHER 67108864
/*
 * NPORT consists of NTEXT, NGRID, NPOINT, NGRAPH, and NOTHER
 */
#define NPORT 130023424
#define FT27 134217728
/*
 * NEXRAD Level-III
 */
#define NEXRAD3 134217728
#define NNEXRAD 134217728
#define NEXRAD 134217728
#define FT28 268435456
/*
 * NEXRAD Level-II
 */
#define CRAFT 268435456
#define NEXRD2 268435456
#define NEXRAD2 268435456
#define FT29 536870912
/*
 * NEXRAD gathering for archiving
 */
#define NXRDSRC 536870912
#define FT30 1073741824
/*
 * For testing & experiments
 */
#define EXP 0x40000000
/*
 * wildcard
 */
#define ANY 0xffffffff

typedef u_int feedtypet;

/*
 * max length of a network hostname, aka MAXHOSTNAMELEN
 */
#define HOSTNAMESIZE 64

/*
 * The maximum length of a host name is 255 bytes
 * according to the Single UNIX� Specification, Version 2
 * <http://www.opengroup.org/onlinepubs/7908799/xns/gethostname.html>.  On a
 * SunOS 5.8 system, MAXHOSTNAMELEN is defined in /usr/include/netdb.h; hence,
 * the definition here is conditioned.
 */
#include <netdb.h>
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 255
#endif

/*
 * Data to build an RPC connection using the portmapper
 */

struct ldm_addr_rpc {
	char *hostname;
	u_long prog;
	u_long vers;
};
typedef struct ldm_addr_rpc ldm_addr_rpc;

/*
 * Data to build an IP connection directly
 */

struct ldm_addr_ip {
	int protocol;
	u_short port;
	u_long addr;
};
typedef struct ldm_addr_ip ldm_addr_ip;

/*
 * What type of a rendezvous
 */

enum ldm_addrt {
	LDM_ADDR_NONE = 0,
	LDM_ADDR_RPC = 1,
	LDM_ADDR_IP = 2,
};
typedef enum ldm_addrt ldm_addrt;

/*
 * A REDIRECT reply is a rendezvous,
 * specifies where to really send data.
 */

struct rendezvoust {
	ldm_addrt type;
	union {
		ldm_addr_rpc rpc;
		ldm_addr_ip ip;
	} rendezvoust_u;
};
typedef struct rendezvoust rendezvoust;

/* md5 digest */

typedef unsigned char signaturet[16];

/*
 * pkey: product identification string (Not used as a key anymore).
 * max length of pkey, _POSIX_PATH_MAX.
 */
#define KEYSIZE 255

typedef char *keyt;

/*
 * max length of a regular expression
 */
#define MAXPATTERN 255

/*
 * prod_spec is a feedtype, pattern pair.
 */
struct prod_spec {
 feedtypet feedtype;
 char *pattern;
 regex_t rgx; /* volatile, not sent over the wire */
};
typedef struct prod_spec prod_spec;
bool_t xdr_prod_spec(XDR *, prod_spec*);

/*
 * max number of specs in a class. There are at most 32 feedtypes
 */
#define PSA_MAX 32

/*
 * prod_class_t is a set of products
 */

struct prod_class {
	timestampt from;
	timestampt to;
	struct {
		u_int psa_len;
		prod_spec *psa_val;
	} psa;
};
typedef struct prod_class prod_class;

typedef prod_class prod_class_t;

/*
 * The maximum size of a HEREIS data product.  Products larger than this will
 * be sent using COMINGSOON/BLKDATA messages.
 */

typedef u_int max_hereis_t;

/*
 * The parameters of a feed:
 */

struct feedpar {
	prod_class_t *prod_class;
	max_hereis_t max_hereis;
};
typedef struct feedpar feedpar;

typedef feedpar feedpar_t;

/*
 * prod_info describes a specific data product.
 * (not a class of products).
 *
 */

struct prod_info {
	timestampt arrival;
	signaturet signature;
	char *origin;
	feedtypet feedtype;
	u_int seqno;
	keyt ident;
	u_int sz;
};
typedef struct prod_info prod_info;
/*
 * HEREIS/COMINGSOON threshold in bytes.
 * IF THIS VALUE IS INCREASED, THEN DISTRIBUTION AND INSTALLATION OF THE NEXT 
 * VERSION OF THE LDM FOR THE IDD WILL HAVE TO BE MANAGED.
 */
#define DBUFMAX 16384

struct dbuf {
 unsigned int dbuf_len;
 char *dbuf_val;
};
typedef struct dbuf dbuf;

/*
 * Transfer of a product begins with one of these.
 */

struct comingsoon_args {
	prod_info *infop;
	u_int pktsz;
};
typedef struct comingsoon_args comingsoon_args;

/*
 * Number of bytes needed in a dbuf_len == 0 BLKDATA call,
 * (auth AUTH_NONE)
 * Determined empirically to be 68.
 * Round it up to 72 (something divisible by 8 == sizeof(double).
 */
#define DATAPKT_RPC_OVERHEAD ((unsigned int)72)
/*
 * The size of the RPC receiving buffer.  Such a buffer is like a stdio
 * buffer: it doesn't limit the size of an entity, only the efficiency 
 * with which it's transmitted.
 */
#define MAX_RPC_BUF_NEEDED (DATAPKT_RPC_OVERHEAD + 262144)

/*
 * Transfer of a product begins with the prod_info.
 * Then, Send a sequence of these datapkts to
 * transfer the actual data.
 */

struct datapkt {
	signaturet *signaturep;
	u_int pktnum;
	dbuf data;
};
typedef struct datapkt datapkt;

/*
 * Used to request a missed datapkt.
 * (UDP only)
 */

struct datapktd {
	signaturet *signaturep;
	u_int pktnum;
};
typedef struct datapktd datapktd;

/*
 * Descriminant for ldm_replyt
 */

enum ldm_errt {
	OK = 0,
	SHUTTING_DOWN = 1,
	BADPATTERN = 2,
	DONT_SEND = 3,
	RESEND = 4,
	RESTART = 5,
	REDIRECT = 6,
	RECLASS = 7,
};
typedef enum ldm_errt ldm_errt;

#define LDM6_OK OK
#define LDM6_SHUTTING_DOWN SHUTTING_DOWN
#define LDM6_BADPATTERN BADPATTERN
#define LDM6_DONT_SEND DONT_SEND
#define LDM6_RECLASS RECLASS

/*
 * Remote procedure return values.
 */

struct ldm_replyt {
	ldm_errt code;
	union {
		datapktd *dpktdp;
		signaturet *signaturep;
		rendezvoust *alternatep;
		prod_class_t *newclssp;
	} ldm_replyt_u;
};
typedef struct ldm_replyt ldm_replyt;

struct hiya_reply_t {
	ldm_errt code;
	union {
		max_hereis_t max_hereis;
		feedpar_t feedPar;
	} hiya_reply_t_u;
};
typedef struct hiya_reply_t hiya_reply_t;

struct fornme_reply_t {
	ldm_errt code;
	union {
		u_int id;
		prod_class_t *prod_class;
	} fornme_reply_t_u;
};
typedef struct fornme_reply_t fornme_reply_t;

typedef ldm_errt comingsoon_reply_t;

#define MIN_LDM_VERSION 5
#define MAX_LDM_VERSION 6

void ldmprog_5(struct svc_req *rqstp, register SVCXPRT *transp);
void ldmprog_6(struct svc_req *rqstp, register SVCXPRT *transp);
int one_svc_run(const int xp_sock, const int inactive_timeo);
void* nullproc_6(void *argp, CLIENT *clnt);
enum clnt_stat clnt_stat(CLIENT *clnt);
struct product {
 prod_info info;
 void *data;
};
typedef struct product product;

bool_t xdr_product(XDR *, product*);
bool_t xdr_dbuf(XDR* xdrs, dbuf* objp);


struct ServiceAddr {
	char *inetId;
	u_short port;
};
typedef struct ServiceAddr ServiceAddr;

#define LDMPROG LDM_PROG
#define FIVE 5

#if defined(__STDC__) || defined(__cplusplus)
#define HEREIS 1
extern  ldm_replyt * hereis_5(product *, CLIENT *);
extern  ldm_replyt * hereis_5_svc(product *, struct svc_req *);
#define FEEDME 4
extern  ldm_replyt * feedme_5(prod_class_t *, CLIENT *);
extern  ldm_replyt * feedme_5_svc(prod_class_t *, struct svc_req *);
#define HIYA 5
extern  ldm_replyt * hiya_5(prod_class_t *, CLIENT *);
extern  ldm_replyt * hiya_5_svc(prod_class_t *, struct svc_req *);
#define NOTIFICATION 8
extern  ldm_replyt * notification_5(prod_info *, CLIENT *);
extern  ldm_replyt * notification_5_svc(prod_info *, struct svc_req *);
#define NOTIFYME 9
extern  ldm_replyt * notifyme_5(prod_class_t *, CLIENT *);
extern  ldm_replyt * notifyme_5_svc(prod_class_t *, struct svc_req *);
#define COMINGSOON 12
extern  ldm_replyt * comingsoon_5(comingsoon_args *, CLIENT *);
extern  ldm_replyt * comingsoon_5_svc(comingsoon_args *, struct svc_req *);
#define BLKDATA 13
extern  ldm_replyt * blkdata_5(datapkt *, CLIENT *);
extern  ldm_replyt * blkdata_5_svc(datapkt *, struct svc_req *);
extern int ldmprog_5_freeresult (SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define HEREIS 1
extern  ldm_replyt * hereis_5();
extern  ldm_replyt * hereis_5_svc();
#define FEEDME 4
extern  ldm_replyt * feedme_5();
extern  ldm_replyt * feedme_5_svc();
#define HIYA 5
extern  ldm_replyt * hiya_5();
extern  ldm_replyt * hiya_5_svc();
#define NOTIFICATION 8
extern  ldm_replyt * notification_5();
extern  ldm_replyt * notification_5_svc();
#define NOTIFYME 9
extern  ldm_replyt * notifyme_5();
extern  ldm_replyt * notifyme_5_svc();
#define COMINGSOON 12
extern  ldm_replyt * comingsoon_5();
extern  ldm_replyt * comingsoon_5_svc();
#define BLKDATA 13
extern  ldm_replyt * blkdata_5();
extern  ldm_replyt * blkdata_5_svc();
extern int ldmprog_5_freeresult ();
#endif /* K&R C */
#define SIX 6

#if defined(__STDC__) || defined(__cplusplus)
extern  fornme_reply_t * feedme_6(feedpar_t *, CLIENT *);
extern  fornme_reply_t * feedme_6_svc(feedpar_t *, struct svc_req *);
extern  fornme_reply_t * notifyme_6(prod_class_t *, CLIENT *);
extern  fornme_reply_t * notifyme_6_svc(prod_class_t *, struct svc_req *);
#define IS_ALIVE 14
extern  bool_t * is_alive_6(u_int *, CLIENT *);
extern  bool_t * is_alive_6_svc(u_int *, struct svc_req *);
extern  hiya_reply_t * hiya_6(prod_class_t *, CLIENT *);
extern  hiya_reply_t * hiya_6_svc(prod_class_t *, struct svc_req *);
extern  void * notification_6(prod_info *, CLIENT *);
extern  void * notification_6_svc(prod_info *, struct svc_req *);
extern  void * hereis_6(product *, CLIENT *);
extern  void * hereis_6_svc(product *, struct svc_req *);
extern  comingsoon_reply_t * comingsoon_6(comingsoon_args *, CLIENT *);
extern  comingsoon_reply_t * comingsoon_6_svc(comingsoon_args *, struct svc_req *);
extern  void * blkdata_6(datapkt *, CLIENT *);
extern  void * blkdata_6_svc(datapkt *, struct svc_req *);
extern int ldmprog_6_freeresult (SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
extern  fornme_reply_t * feedme_6();
extern  fornme_reply_t * feedme_6_svc();
extern  fornme_reply_t * notifyme_6();
extern  fornme_reply_t * notifyme_6_svc();
#define IS_ALIVE 14
extern  bool_t * is_alive_6();
extern  bool_t * is_alive_6_svc();
extern  hiya_reply_t * hiya_6();
extern  hiya_reply_t * hiya_6_svc();
extern  void * notification_6();
extern  void * notification_6_svc();
extern  void * hereis_6();
extern  void * hereis_6_svc();
extern  comingsoon_reply_t * comingsoon_6();
extern  comingsoon_reply_t * comingsoon_6_svc();
extern  void * blkdata_6();
extern  void * blkdata_6_svc();
extern int ldmprog_6_freeresult ();
#endif /* K&R C */

/* the xdr functions */

#if defined(__STDC__) || defined(__cplusplus)
extern  bool_t xdr_feedtypet (XDR *, feedtypet*);
extern  bool_t xdr_ldm_addr_rpc (XDR *, ldm_addr_rpc*);
extern  bool_t xdr_ldm_addr_ip (XDR *, ldm_addr_ip*);
extern  bool_t xdr_ldm_addrt (XDR *, ldm_addrt*);
extern  bool_t xdr_rendezvoust (XDR *, rendezvoust*);
extern  bool_t xdr_signaturet (XDR *, signaturet);
extern  bool_t xdr_keyt (XDR *, keyt*);
extern  bool_t xdr_prod_class (XDR *, prod_class*);
extern  bool_t xdr_prod_class_t (XDR *, prod_class_t*);
extern  bool_t xdr_max_hereis_t (XDR *, max_hereis_t*);
extern  bool_t xdr_feedpar (XDR *, feedpar*);
extern  bool_t xdr_feedpar_t (XDR *, feedpar_t*);
extern  bool_t xdr_prod_info (XDR *, prod_info*);
extern  bool_t xdr_comingsoon_args (XDR *, comingsoon_args*);
extern  bool_t xdr_datapkt (XDR *, datapkt*);
extern  bool_t xdr_datapktd (XDR *, datapktd*);
extern  bool_t xdr_ldm_errt (XDR *, ldm_errt*);
extern  bool_t xdr_ldm_replyt (XDR *, ldm_replyt*);
extern  bool_t xdr_hiya_reply_t (XDR *, hiya_reply_t*);
extern  bool_t xdr_fornme_reply_t (XDR *, fornme_reply_t*);
extern  bool_t xdr_comingsoon_reply_t (XDR *, comingsoon_reply_t*);
extern  bool_t xdr_ServiceAddr (XDR *, ServiceAddr*);

#else /* K&R C */
extern bool_t xdr_feedtypet ();
extern bool_t xdr_ldm_addr_rpc ();
extern bool_t xdr_ldm_addr_ip ();
extern bool_t xdr_ldm_addrt ();
extern bool_t xdr_rendezvoust ();
extern bool_t xdr_signaturet ();
extern bool_t xdr_keyt ();
extern bool_t xdr_prod_class ();
extern bool_t xdr_prod_class_t ();
extern bool_t xdr_max_hereis_t ();
extern bool_t xdr_feedpar ();
extern bool_t xdr_feedpar_t ();
extern bool_t xdr_prod_info ();
extern bool_t xdr_comingsoon_args ();
extern bool_t xdr_datapkt ();
extern bool_t xdr_datapktd ();
extern bool_t xdr_ldm_errt ();
extern bool_t xdr_ldm_replyt ();
extern bool_t xdr_hiya_reply_t ();
extern bool_t xdr_fornme_reply_t ();
extern bool_t xdr_comingsoon_reply_t ();
extern bool_t xdr_ServiceAddr ();

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_LDM_H_RPCGEN */
