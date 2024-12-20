/**
 *   Copyright 2018, University Corporation for Atmospheric Research.
 *   All Rights reserved.
 *
 *   See file COPYRIGHT in the top-level source-directory for copying and
 *   redistribution conditions.
 */
#ifndef _LCF_H
#define _LCF_H

#include "ldm.h"

#include "InetSockAddr.h"
#if WANT_MULTICAST
#include "mcast_info.h"
#endif
#include "peer_info.h"
#include "UpFilter.h"
#include "wordexp.h"

#include <regex.h>

#include <stdbool.h>
#include <sys/types.h>

#ifndef ENOERR
#define ENOERR 0
#endif

enum host_set_type { HS_NONE, HS_NAME, HS_DOTTED_QUAD, HS_REGEXP };
typedef struct {
	enum host_set_type type;
	const char *cp;	/* hostname or pattern */
	regex_t rgx; 
} host_set;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes this module. Defined in "parser.y".
 *
 * @param[in] defaultPort  Default LDM port-number if not specified in entry
 * @param[in] pathname     Pathname of LDM configuration-file. May be `NULL`, in
 *                         which case the module will have no entries.
 * @retval    0            Success.
 * @retval    -1           Failure. `log_add()` called.
 */
int lcf_init(
    const unsigned      defaultPort,
    const char* const   pathname);

/**
 * Adds an EXEC entry and executes the command as a child process.
 *
 * @param[in] words  Command-line words. Freed by `lcf_freeExec()`.
 * @retval    0      Success. Caller must *not* call `wordfree(wrdexpp)`.
 * @return           System error code. Caller should call `wordfree(wrdexpp)`
 *                   when `*wrdexpp` is no longer needed.
 * @see `lcf_freeExec()`
 */
int
lcf_addExec(wordexp_t *wrdexpp);

/**
 * Frees an EXEC entry.
 *
 * @param pid       [in] The process identifier of the child process whose
 *                  entry is to be freed.
 */
void
lcf_freeExec(
    const pid_t pid);

/**
 * Returns the command-line of an EXEC entry.
 *
 * @param pid       [in] The process identifier of the child process.
 * @param buf       [in] The buffer into which to write the command-line.
 * @param size      [in] The size of "buf".
 * @retval -2       The child process wasn't found.
 * @retval -1       Write error.  See errno.  Error-message(s) written via
 *                  log_*().
 * @return          The number of characters written into "buf" excluding any
 *                  terminating NUL.  If the number of characters equals "size",
 *                  then no terminating NUL was written.
 */
int
lcf_getCommandLine(
    const pid_t         pid,
    char* const         buf,
    size_t              size);

/**
 * Adds a REQUEST entry.
 *
 * @param feedtype      [in] Feedtype.
 * @param pattern       [in] Pattern. Client may free upon return.
 * @param hostId        [in] Host identifier. Client may free upon return.
 * @param port          [in] Port number.
 * @retval 0            Success.
 * @retval -1           System error. log_add() called.
 */
int
lcf_addRequest(
    const feedtypet     feedtype,
    const char* const   pattern,
    const char* const   hostId,
    const unsigned      port);

/**
 * Returns a new specification of a set of hosts.
 *
 * @param[in] type  Type of host-set
 * @param[in] cp    Pointer to host(s) specification. Caller must not free on
 *                  return if call is successful and "type" is `HS_REGEXP`.
 * @param[in] rgxp  Pointer to regular-expression structure.  Ignored if `type`
 *                  isn't `HS_REGEXP`. Caller may free on return but must not
 *                  call regfree() if call is successful and "type" is
 *                  `HS_REGEXP`.
 * @retval NULL     Out of memory. No error-message logged or started.
 */
host_set *
lcf_newHostSet(enum host_set_type type, const char *cp, const regex_t *rgxp);

/**
 * Frees a specification of a set of hosts.
 *
 * @param hsp       [in/out] The specification of a set of hosts.
 */
void
lcf_freeHostSet(host_set *hsp);

/**
 * Adds an ALLOW entry.
 *
 * @param ft            [in] The feedtype.
 * @param hostSet       [in] Pointer to allocated set of allowed downstream hosts.
 *                      Upon successful return, the client shall abandon
 *                      responsibility for calling "free_host_set(hostSet)".
 * @param okEre         [in] Pointer to the ERE that data-product identifiers
 *                      must match.  Caller may free upon return.
 * @param notEre        [in] Pointer to the ERE that data-product identifiers
 *                      must not match or NULL if such matching should be
 *                      disabled.  Caller may free upon return.
 * @retval NULL         Success.
 * @return              Failure error object.
 */
ErrorObj*
lcf_addAllow(
    const feedtypet             ft,
    host_set* const             hostSet,
    const char* const           okEre,
    const char* const           notEre);

/**
 * Returns the feeds that a remote host is allowed to receive.
 * @param[in]  name      Name of remote host
 * @param[in]  addr      Address of remote host
 * @param[out] feeds     Feeds that remote host is allowed to receive
 * @param[in]  maxFeeds  Size of `feeds`
 * @return               Number of feeds that remote host is allowed. May be
 *                       greater than `maxFeeds`.
 */
size_t
lcf_getAllowedFeeds(
        const char*           name,
        const struct in_addr* addr,
        const size_t          maxFeeds,
        feedtypet*            feeds);

/**
 * Returns the intersection of a desired feed and allowed feeds.
 * @param[in] desiredFeed   Desired feed
 * @param[in] allowedFeeds  Allowed feeds
 * @param[in] numFeeds      Number of allowed feeds
 * @return                  Intersection of `desiredFeed` and elements of
 *                          `feeds`
 */
feedtypet
lcf_reduceFeed(
        feedtypet    desiredFeed,
        feedtypet*   allowedFeeds,
        const size_t numFeeds);

/**
 * Returns the intersection of a desired feed and the feeds that a remote host
 * is allowed to receive.
 * @param[in] name         Name of remote host
 * @param[in] addr         Address of remote host
 * @param[in] desiredFeed  Feed desired by remote host
 * @return                 Intersection of `desiredFeed` and feeds host is
 *                         allowed to receive
 */
feedtypet
lcf_getAllowed(
        const char*           name,
        const struct in_addr* addr,
        const feedtypet       desiredFeed);

/**
 * Returns the class of products that a host is allowed to receive based on the
 * host and the feed-types of products that it wants to receive.  The pointer
 * to the product-class structure will reference allocated space on success;
 * otherwise, it won't be modified.  The returned product-class may be the
 * empty set.  The client is responsible for invoking
 * free_prod_class(prod_class_t*) on a non-NULL product-class structure when it
 * is no longer needed.
 *
 * @param name          [in] Pointer to the name of the host.
 * @param addr          [in] Pointer to the IP address of the host.
 * @param want          [in] Pointer to the class of products that the host wants.
 * @param intersect     [out] Pointer to a pointer to the intersection of the
 *                      wanted product class and the allowed product class.
 *                      References allocated space on success; otherwise won't
 *                      be modified.  Referenced product-class may be empty.
 *                      On success, the caller should eventually invoke
 *                      free_prod_class(*intersect).
 * @retval 0            Success.
 * @retval EINVAL       The regular expression pattern of a
 *                      product-specification couldn't be compiled.
 * @retval ENOMEM       Out-of-memory.
 */
int
lcf_reduceToAllowed(
    const char*           name,
    const struct in_addr* addr,
    const prod_class_t*   want,
    prod_class_t ** const intersect);

/**
 * Indicates if it's OK to feed or notify a given host a given class of
 * data-products.
 *
 * @param *rmtip        [in] Information on the remote host.  rmtip->clssp will
 *                      be set to the intersection unless there's an
 *                      error, or there are no matching host entries in the
 *                      ACL, or the intersection is the empty set, in which
 *                      case it will be unmodified.
 * @param *want         [in] The product-class that the host wants.
 * @retval 0            if successful.
 * @retval ENOMEM       if out-of-memory.
 * @retval EINVAL       if a regular expression of a product specification
 *                      couldn't be compiled.
 */
int
lcf_okToFeedOrNotify(peer_info *rmtip, prod_class_t *want);

/**
 * Returns the product-class appropriate for filtering data-products on the
 * upstream LDM before sending them to the downstream LDM.
 *
 * @param name          [in] Pointer to the name of the downstream host.
 * @param addr          [in] Pointer to the IP address of the downstream host.
 * @param want          [in] Pointer to the class of products that the downstream
 *                      host wants.
 * @param filter        [out] Pointer to a pointer to the upstream filter.
 *                      *filter is set on and only on success.  Caller
 *                      should call upFilter_free(*filter). *filter is set to
 *                      NULL if and only if no data-products should be sent to
 *                      the downstream LDM.
 * @retval NULL         Success.
 * @return              Failure error object.
 */
ErrorObj*
lcf_getUpstreamFilter(
    const char*                 name,
    const struct in_addr*       addr, 
    const prod_class_t*         want,
    UpFilter** const            upFilter);

/**
 * Adds an ACCEPT entry.
 *
 * @param ft            [in] Feedtype.
 * @param pattern       [in] Pointer to allocated memory containing extended
 *                      regular-expression for matching the product-identifier.
 *                      The client shall not free.
 * @param rgxp          [in] Pointer to allocated memory containing the
 *                      regular-expression structure for matching
 *                      product-identifiers.  The client shall not free.
 * @param hsp           [in] Pointer to allocated memory containing the host-set.
 *                      The client shall not free.
 * @param isPrimary     [in] Whether or not the initial data-product exchange-mode
 *                      is primary (i.e., uses HEREIS) or alternate (i.e., uses
 *                      COMINGSOON/BLKDATA).
 * @retval 0            Success
 * @retval !0           <errno.h> error-code
 */
int
lcf_addAccept(
    feedtypet     ft,
    char*         pattern,
    regex_t*      rgxp,
    host_set*     hsp,
    int           isPrimary);

#if WANT_MULTICAST

/**
 * Adds a potential multicast LDM sender. The sender is not started. This
 * function should be called for all potential senders before any child
 * process is forked so that all child processes will have this information.
 *
 * @param[in] mcastInfo    Information on the multicast group. Caller may free.
 * @param[in] ttl          Time-to-live for multicast packets:
 *                                0  Restricted to same host. Won't be output by
 *                                   any interface.
 *                                1  Restricted to same subnet. Won't be
 *                                   forwarded by a router.
 *                              <32  Restricted to same site, organization or
 *                                   department.
 *                              <64  Restricted to same region.
 *                             <128  Restricted to same continent.
 *                             <255  Unrestricted in scope. Global.
 * @param[in] subnetLen    Number of bits in network subnet
 * @param[in] vcEnd        Local virtual-circuit endpoint. Caller may
 *                         free.
 * @param[in] pqPathname   Pathname of product-queue. Caller may free.
 * @retval    0            Success.
 * @retval    EINVAL       Invalid specification. `log_add()` called.
 * @retval    ENOMEM       Out-of-memory. `log_add()` called.
 * @see `lcf_free()`
 */
int
lcf_addMulticast(
        const SepMcastInfo* const restrict    mcastInfo,
        const unsigned short                  ttl,
        const unsigned short                  subnetLen,
        const VcEndPoint* const restrict      vcEnd,
        const char* const restrict            pqPathname);

/**
 * Adds a potential downstream LDM-7.
 *
 * @param[in] feed         LDM feed to subscribe to.
 * @param[in] ldmSrvr      Sending LDM-7 server. Caller may free.
 * @param[in] fmtpIface    Name of virtual interface to be created for the FMTP
 *                         layer in the form `<name>.<tag>`, where <name> is
 *                         the name of an existing interface (e.g., "eth0") and
 *                         <tag> is the unique VLAN ID to be used by the FMTP
 *                         layer. May be `NULL`, in which case no virtual
 *                         interface will be created and used. Caller may free.
 * @param[in] switchId     ID of port on `switchID`. May be `NULL`, in which
 *                         case an AL2S multipoint VLAN won't be joined. Caller
 *                         may free.
 * @param[in] portId       ID of port on `switchID`. May be `NULL`, in which
 *                         case an AL2S multipoint VLAN won't be joined. Caller
 *                         may free.
 * @param[in] vlanTag      VLAN tag at switch `switchId`, port `portId`
 * @retval    0            Success.
 * @retval    EINVAL       Invalid argument. `log_add()` called.
 * @retval    ENOMEM       System failure. `log_add()` called.
 */
int
lcf_addReceive(
        const feedtypet                    feed,
        const InetSockAddr* const restrict ldmSrvr,
        const char* const restrict         fmtpIface,
        const char* restrict               switchId,
        const char* restrict               portId,
        const VlanId                       vlanTag);

#endif

/**
 * Checks the LDM configuration-file for ACCEPT entries that are relevant to a
 * given remote host.
 *
 * @param rmtip         [in/out] Information on the remote host. May be
 *                      modified.
 * @param offerd        [in] The product-class that the remote host is offering
 *                      to send.
 * @retval 0            if successful.
 * @retval ENOMEM       if out-of-memory.
 */
int
lcf_isHiyaAllowed(peer_info *rmtip, prod_class_t *offerd);

/**
 * Determines the set of acceptable products given the upstream host and the
 * offered set of products.
 *
 * @param name          [in] Pointer to name of host.
 * @param addr          [in] Pointer to Internet address of host.
 * @param dotAddr       [in] Pointer to the dotted-quad form of the IP address
 *                      of the host.
 * @param offered       [in] Pointer to the class of offered products.
 * @param accept        [out] Address of pointer to set of acceptable products.
 *                      On success, the pointer will be set to reference
 *                      allocated space; otherwise, it won't be modified.
 *                      Acceptable product set may be empty. The client should
 *                      call "free_prod_class(prod_class_t*)" when the
 *                      product-class is no longer needed.  In general, the
 *                      class of acceptable products will be a subset of
 *                      *offered.
 * @param isPrimary     [in] Pointer to flag indicating whether or not the
 *                      data-product exchange-mode should be primary (i.e., use
 *                      HEREIS) or alternate (i.e., use COMINGSOON/BLKDATA).
 * @retval 0            Success.
 * @retval ENOMEM       Failure.  Out-of-memory.
 */
int
lcf_reduceToAcceptable(
    const char*         name,
    const char*         dotAddr, 
    prod_class_t*       offerd,
    prod_class_t**      accept,
    int*                isPrimary);

/**
 * Starts the necessary downstream LDM-s.
 *
 * @retval 0            Success.
 * @return              System error code. log_add() called.
 */
int
lcf_startRequesters(void);

/**
 * Indicates if a given host is allowed to connect in any fashion. First line
 * of (weak) defense.
 *
 * Of course, a serious threat would spoof the IP address or name service.
 *
 * @retval 0        Iff the host is not allowed to connect.
 */
int
lcf_isHostOk(const peer_info *rmtip);

/**
 * Indicates whether or not a top-level LDM server is needed based on the
 * entries of the LDM configuration-file.
 *
 * @retval false  Server is not needed.
 * @retval true   Server is needed.
 */
bool
lcf_isServerNeeded(void);

/**
 * Indicates of the configuration-file contains something to do.
 *
 * @retval `true` iff the configuration-file contains something to do.
 */
bool
lcf_haveSomethingToDo(void);

/**
 * Destroys this module, freeing its resources. Idempotent.
 *
 * @param[in] final  Whether inter-process communication resources should also
 *                   be destroyed. Should be `true` in only one process per LDM
 *                   session.
 */
void
lcf_destroy(const bool final);

/**
 * Executes all EXEC, REQUEST, and RECEIVE entries of the configuration-file.
 *
 * @retval 0  Success
 * @return    System error code.
 */
int
lcf_execute(void);

/**
 * Saves information on the last, successfully-received product under a key
 * that comprises the relevant components of the data-request.
 */
void
lcf_savePreviousProdInfo(void);

int
decodeFeedtype(
    feedtypet*  ftp,
    const char* string);

/**
 * Decodes a MULTICAST entry.
 *
 * @param[in] feedStr         LDM feed specification
 * @param[in] mcastGrpStr     Multicast group Internet address in the form
 *                            "<nnn.nnn.nnn.nnn>[:<port>]". The default for
 *                            <port> is `regutil /server/port`.
 * @param[in] fmtpAddrStr     Address of local FMTP server in the form
 *                            "<nnn.nnn.nnn.nnn>[:<port]". If <port> isn't
 *                            specified, then it is chosen by the operating
 *                            system.
 * @param[in] subnetLenStr    Number of bits in the network prefix for the
 *                            multipoint VLAN or "0" indicating a multipoint
 *                            VLAN won't be used
 * @param[in] vlanIdStr       Outgoing VLAN tag  or "0" indicating a multipoint
 *                            VLAN won't be used
 * @param[in] switchStr       Identifier of nearest level-2 switch or "dummy"
 *                            indicating a multipoint VLAN won't be used
 * @param[in] switchPortStr   Identifier of port on `switchStr` or "dummy"
 *                            indicating a multipoint VLAN won't be used
 * @retval    0               Success
 * @retval    EINVAL          Invalid specification. `log_add()` called.
 * @retval    ENOMEM          Out-of-memory. `log_add()` called.
 */
int
decodeMulticastEntry(
    const char* const   feedStr,
    const char* const   mcastGrpStr,
    const char* const   fmtpAddrStr,
    const char* const   subnetLenStr,
    const char*         vlanIdStr,
    const char* const   switchStr,
    const char* const   switchPortStr);

/**
 * Decodes a RECEIVE entry.
 *
 * @param[in] feedStr       LDM feed
 * @param[in] ldmSrvrStr    Sending LDM7 server. Either hostname or IPv4
 *                          address. Caller may free.
 * @param[in] fmtpIfaceStr  Name of virtual interface to be created for the FMTP
 *                          layer in the form `<name>.<tag>`, where <name> is
 *                          the name of an existing interface (e.g., "eth0") and
 *                          <tag> is the unique VLAN ID to be used by the FMTP
 *                          layer. May be `NULL`, in which case no virtual
 *                          interface will be created and used.
 * @param[in] switchId      ID of nearest AL2S switch for joining a multipoint
 *                          VLAN. May be `NULL`, in which case an AL2S
 *                          multipoint VLAN won't be joined. Caller may free.
 * @param[in] portId        ID of port on `switchID`. May be `NULL`, in which
 *                          case an AL2S multipoint VLAN won't be joined. Caller
 *                          may free.
 * @param[in] vlanTagStr    VLAN tag at switch `switchId`, port `portId`. May be
 *                          `NULL`, in which case the VLAN tag of `fmtpIface` is
 *                          used if that parameter is specified. Caller may
 *                          free.
 * @retval    0             Success.
 * @retval    EINVAL        Invalid specification. `log_add()` called.
 * @retval    ENOMEM        Out-of-memory. `log_add()` called.
 */
int
decodeReceiveEntry(
        const char* const restrict feedStr,
        const char* const restrict ldmSrvrStr,
        const char* const restrict fmtpIfaceStr,
        const char* const restrict switchId,
        const char* const restrict portId,
        const char* restrict       vlanTagStr);

#ifdef __cplusplus
}
#endif

#endif /* !_LCF_H */
