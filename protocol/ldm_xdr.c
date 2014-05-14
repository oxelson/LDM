#include "config.h"
/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "ldm.h"

#include <string.h>

#ifndef NDEBUG
#include <assert.h>
#include <ulog.h>
#define pIf(a,b) (!(a) || (b)) /* a implies b */

static bool_t
xdr_stringck(XDR *xdrs, char **cpp, unsigned int maxsize)
{
 assert(pIf(xdrs->x_op == XDR_ENCODE, *cpp != NULL && **cpp != 0));
 return(xdr_string(xdrs, cpp, maxsize));
}

static bool_t
xdr_referenceck(XDR *xdrs, char* *pp, unsigned int size, const xdrproc_t proc)
{
 assert(pIf(xdrs->x_op == XDR_ENCODE, *pp != NULL));
 return(xdr_reference(xdrs, pp, size, proc));
}

/* N.B. Names only scoped to this file */
#undef xdr_string /* in case it's a macro */
#undef xdr_pointer /* in case it's a macro */
#define xdr_string xdr_stringck
#define xdr_pointer xdr_referenceck
#else
#undef xdr_pointer /* in case it's a macro */
#define xdr_pointer xdr_reference
#endif /*!NDEBUG*/

/*
 * feedtypet
 * The purpose of this type is to provide a coarse discriminant on
 * the origin and format of data. Think of it as an "address class"
 * to help decided the format of prod_info.ident.
 */

bool_t
xdr_feedtypet (XDR *xdrs, feedtypet *objp)
{
	register int32_t *buf;

	 if (!xdr_u_int (xdrs, objp))
		 return FALSE;
	return TRUE;
}

/*
 * Data to build an RPC connection using the portmapper
 */

bool_t
xdr_ldm_addr_rpc (XDR *xdrs, ldm_addr_rpc *objp)
{
	register int32_t *buf;

	 if (!xdr_string (xdrs, &objp->hostname, HOSTNAMESIZE))
		 return FALSE;
	 if (!xdr_u_long (xdrs, &objp->prog))
		 return FALSE;
	 if (!xdr_u_long (xdrs, &objp->vers))
		 return FALSE;
	return TRUE;
}

/*
 * Data to build an IP connection directly
 */

bool_t
xdr_ldm_addr_ip (XDR *xdrs, ldm_addr_ip *objp)
{
	register int32_t *buf;

	 if (!xdr_int (xdrs, &objp->protocol))
		 return FALSE;
	 if (!xdr_u_short (xdrs, &objp->port))
		 return FALSE;
	 if (!xdr_u_long (xdrs, &objp->addr))
		 return FALSE;
	return TRUE;
}

/*
 * What type of a rendezvous
 */

bool_t
xdr_ldm_addrt (XDR *xdrs, ldm_addrt *objp)
{
	register int32_t *buf;

	 if (!xdr_enum (xdrs, (enum_t *) objp))
		 return FALSE;
	return TRUE;
}

/*
 * A REDIRECT reply is a rendezvous,
 * specifies where to really send data.
 */

bool_t
xdr_rendezvoust (XDR *xdrs, rendezvoust *objp)
{
	register int32_t *buf;

	 if (!xdr_ldm_addrt (xdrs, &objp->type))
		 return FALSE;
	switch (objp->type) {
	case LDM_ADDR_NONE:
		break;
	case LDM_ADDR_RPC:
		 if (!xdr_ldm_addr_rpc (xdrs, &objp->rendezvoust_u.rpc))
			 return FALSE;
		break;
	case LDM_ADDR_IP:
		 if (!xdr_ldm_addr_ip (xdrs, &objp->rendezvoust_u.ip))
			 return FALSE;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

/* md5 digest */

bool_t
xdr_signaturet (XDR *xdrs, signaturet objp)
{
	register int32_t *buf;

	 if (!xdr_opaque(xdrs, (char*)objp, 16))
		 return FALSE;
	return TRUE;
}

/*
 * pkey: product identification string (Not used as a key anymore).
 * max length of pkey, _POSIX_PATH_MAX.
 */

bool_t
xdr_keyt (XDR *xdrs, keyt *objp)
{
	register int32_t *buf;

	 if (!xdr_string (xdrs, objp, KEYSIZE))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_prod_spec(XDR *xdrs, prod_spec *objp)
{
 if (!xdr_feedtypet(xdrs, &objp->feedtype)) {
 return (FALSE);
 }
 if (!xdr_string(xdrs, &objp->pattern, MAXPATTERN)) {
 return (FALSE);
 }
 if (xdrs->x_op == XDR_DECODE) {
 memset(&objp->rgx, 0, sizeof(regex_t));
 }
 if (xdrs->x_op == XDR_FREE
 && objp->pattern != NULL) {
 regfree(&objp->rgx);
 }
 return (TRUE);
}

/*
 * prod_class_t is a set of products
 */

bool_t
xdr_prod_class (XDR *xdrs, prod_class *objp)
{
	register int32_t *buf;

	 if (!xdr_timestampt (xdrs, &objp->from))
		 return FALSE;
	 if (!xdr_timestampt (xdrs, &objp->to))
		 return FALSE;
	 if (!xdr_array (xdrs, (char **)&objp->psa.psa_val, (u_int *) &objp->psa.psa_len, PSA_MAX,
		sizeof (prod_spec), (xdrproc_t) xdr_prod_spec))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_prod_class_t (XDR *xdrs, prod_class_t *objp)
{
	register int32_t *buf;

	 if (!xdr_prod_class (xdrs, objp))
		 return FALSE;
	return TRUE;
}

/*
 * The maximum size of a HEREIS data product.  Products larger than this will
 * be sent using COMINGSOON/BLKDATA messages.
 */

bool_t
xdr_max_hereis_t (XDR *xdrs, max_hereis_t *objp)
{
	register int32_t *buf;

	 if (!xdr_u_int (xdrs, objp))
		 return FALSE;
	return TRUE;
}

/*
 * The parameters of a feed:
 */

bool_t
xdr_feedpar (XDR *xdrs, feedpar *objp)
{
	register int32_t *buf;

	 if (!xdr_pointer (xdrs, (char **)&objp->prod_class, sizeof (prod_class_t), (xdrproc_t) xdr_prod_class_t))
		 return FALSE;
	 if (!xdr_max_hereis_t (xdrs, &objp->max_hereis))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_feedpar_t (XDR *xdrs, feedpar_t *objp)
{
	register int32_t *buf;

	 if (!xdr_feedpar (xdrs, objp))
		 return FALSE;
	return TRUE;
}

/*
 * prod_info describes a specific data product.
 * (not a class of products).
 *
 */

bool_t
xdr_prod_info (XDR *xdrs, prod_info *objp)
{
	register int32_t *buf;

	 if (!xdr_timestampt (xdrs, &objp->arrival))
		 return FALSE;
	 if (!xdr_signaturet (xdrs, objp->signature))
		 return FALSE;
	 if (!xdr_string (xdrs, &objp->origin, HOSTNAMESIZE))
		 return FALSE;
	 if (!xdr_feedtypet (xdrs, &objp->feedtype))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->seqno))
		 return FALSE;
	 if (!xdr_keyt (xdrs, &objp->ident))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->sz))
		 return FALSE;
	return TRUE;
}

/*
 * Transfer of a product begins with one of these.
 */

bool_t
xdr_comingsoon_args (XDR *xdrs, comingsoon_args *objp)
{
	register int32_t *buf;

	 if (!xdr_pointer (xdrs, (char **)&objp->infop, sizeof (prod_info), (xdrproc_t) xdr_prod_info))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->pktsz))
		 return FALSE;
	return TRUE;
}

/*
 * Transfer of a product begins with the prod_info.
 * Then, Send a sequence of these datapkts to
 * transfer the actual data.
 */

bool_t
xdr_datapkt (XDR *xdrs, datapkt *objp)
{
	register int32_t *buf;

	 if (!xdr_pointer (xdrs, (char **)&objp->signaturep, sizeof (signaturet), (xdrproc_t) xdr_signaturet))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->pktnum))
		 return FALSE;
	 if (!xdr_dbuf (xdrs, &objp->data))
		 return FALSE;
	return TRUE;
}

/*
 * Used to request a missed datapkt.
 * (UDP only)
 */

bool_t
xdr_datapktd (XDR *xdrs, datapktd *objp)
{
	register int32_t *buf;

	 if (!xdr_pointer (xdrs, (char **)&objp->signaturep, sizeof (signaturet), (xdrproc_t) xdr_signaturet))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->pktnum))
		 return FALSE;
	return TRUE;
}

/*
 * Descriminant for ldm_replyt
 */

bool_t
xdr_ldm_errt (XDR *xdrs, ldm_errt *objp)
{
	register int32_t *buf;

	 if (!xdr_enum (xdrs, (enum_t *) objp))
		 return FALSE;
	return TRUE;
}

/*
 * Remote procedure return values.
 */

bool_t
xdr_ldm_replyt (XDR *xdrs, ldm_replyt *objp)
{
	register int32_t *buf;

	 if (!xdr_ldm_errt (xdrs, &objp->code))
		 return FALSE;
	switch (objp->code) {
	case OK:
		break;
	case SHUTTING_DOWN:
		break;
	case BADPATTERN:
		break;
	case DONT_SEND:
		break;
	case RESEND:
		 if (!xdr_pointer (xdrs, (char **)&objp->ldm_replyt_u.dpktdp, sizeof (datapktd), (xdrproc_t) xdr_datapktd))
			 return FALSE;
		break;
	case RESTART:
		 if (!xdr_pointer (xdrs, (char **)&objp->ldm_replyt_u.signaturep, sizeof (signaturet), (xdrproc_t) xdr_signaturet))
			 return FALSE;
		break;
	case REDIRECT:
		 if (!xdr_pointer (xdrs, (char **)&objp->ldm_replyt_u.alternatep, sizeof (rendezvoust), (xdrproc_t) xdr_rendezvoust))
			 return FALSE;
		break;
	case RECLASS:
		 if (!xdr_pointer (xdrs, (char **)&objp->ldm_replyt_u.newclssp, sizeof (prod_class_t), (xdrproc_t) xdr_prod_class_t))
			 return FALSE;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

bool_t
xdr_hiya_reply_t (XDR *xdrs, hiya_reply_t *objp)
{
	register int32_t *buf;

	 if (!xdr_ldm_errt (xdrs, &objp->code))
		 return FALSE;
	switch (objp->code) {
	case OK:
		 if (!xdr_max_hereis_t (xdrs, &objp->hiya_reply_t_u.max_hereis))
			 return FALSE;
		break;
	case DONT_SEND:
		break;
	case RECLASS:
		 if (!xdr_feedpar_t (xdrs, &objp->hiya_reply_t_u.feedPar))
			 return FALSE;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

bool_t
xdr_fornme_reply_t (XDR *xdrs, fornme_reply_t *objp)
{
	register int32_t *buf;

	 if (!xdr_ldm_errt (xdrs, &objp->code))
		 return FALSE;
	switch (objp->code) {
	case OK:
		 if (!xdr_u_int (xdrs, &objp->fornme_reply_t_u.id))
			 return FALSE;
		break;
	case BADPATTERN:
		break;
	case RECLASS:
		 if (!xdr_pointer (xdrs, (char **)&objp->fornme_reply_t_u.prod_class, sizeof (prod_class_t), (xdrproc_t) xdr_prod_class_t))
			 return FALSE;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

bool_t
xdr_comingsoon_reply_t (XDR *xdrs, comingsoon_reply_t *objp)
{
	register int32_t *buf;

	 if (!xdr_ldm_errt (xdrs, objp))
		 return FALSE;
	return TRUE;
}


#include <stddef.h>

#include "ulog.h"
#include "xdr_data.h"


bool_t
xdr_product(XDR *xdrs, product *objp)
{
 if (!xdr_prod_info(xdrs, &objp->info)) {
 return (FALSE);
 }

 switch (xdrs->x_op) {

 case XDR_DECODE:
 if (objp->info.sz == 0) {
 return (TRUE);
 }
 if (objp->data == NULL) {
 objp->data = xd_getBuffer(objp->info.sz);
 if(objp->data == NULL) {
 return (FALSE);
 }
 }
 /*FALLTHRU*/

 case XDR_ENCODE:
 return (xdr_opaque(xdrs, objp->data, objp->info.sz));

 case XDR_FREE:
 objp->data = NULL;
 return (TRUE);

 }
 return (FALSE); /* never reached */
}


bool_t
xdr_dbuf(XDR* xdrs, dbuf* objp)
{
 /*
     * First, deal with the length since dbuf-s are counted.
     */
 if (!xdr_u_int(xdrs, &objp->dbuf_len))
 return FALSE;

 /*
     * Now, deal with the actual bytes.
     */
 switch (xdrs->x_op) {

 case XDR_DECODE:
 if (objp->dbuf_len == 0)
 return TRUE;

 if (NULL == (objp->dbuf_val =
 (char*)xd_getNextSegment(objp->dbuf_len))) {
 serror("xdr_dbuf()");
 return FALSE;
 }

 /*FALLTHROUGH*/

 case XDR_ENCODE:
 return (xdr_opaque(xdrs, objp->dbuf_val, objp->dbuf_len));

 case XDR_FREE:
 objp->dbuf_val = NULL;

 return TRUE;
 }

 return FALSE;
}
#include "../multicast/vcmtp_c_api.h"

/*
 * Successful multicast subscription return value:
 */

bool_t
xdr_McastGroupInfo (XDR *xdrs, McastGroupInfo *objp)
{
	register int32_t *buf;

	 if (!xdr_string (xdrs, &objp->mcastName, ~0))
		 return FALSE;
	 if (!xdr_string (xdrs, &objp->serverAddr, ~0))
		 return FALSE;
	 if (!xdr_u_short (xdrs, &objp->serverPort))
		 return FALSE;
	 if (!xdr_string (xdrs, &objp->groupAddr, ~0))
		 return FALSE;
	 if (!xdr_u_short (xdrs, &objp->groupPort))
		 return FALSE;
	return TRUE;
}

/*
 * LDM-7 status values:
 */

bool_t
xdr_Ldm7Status (XDR *xdrs, Ldm7Status *objp)
{
	register int32_t *buf;

	 if (!xdr_enum (xdrs, (enum_t *) objp))
		 return FALSE;
	return TRUE;
}

/*
 * Missed data-product:
 */

bool_t
xdr_MissedProduct (XDR *xdrs, MissedProduct *objp)
{
	register int32_t *buf;

	 if (!xdr_VcmtpFileId (xdrs, &objp->fileId))
		 return FALSE;
	 if (!xdr_product (xdrs, &objp->prod))
		 return FALSE;
	return TRUE;
}

/*
 * Multicast subscription return values:
 */

bool_t
xdr_SubscriptionReply (XDR *xdrs, SubscriptionReply *objp)
{
	register int32_t *buf;

	 if (!xdr_Ldm7Status (xdrs, &objp->status))
		 return FALSE;
	switch (objp->status) {
	case LDM7_OK:
		 if (!xdr_McastGroupInfo (xdrs, &objp->SubscriptionReply_u.groupInfo))
			 return FALSE;
		break;
	case LDM7_INVAL:
		break;
	case LDM7_UNAUTH:
		break;
	default:
		return FALSE;
	}
	return TRUE;
}
