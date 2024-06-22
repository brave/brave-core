/*
* Credentials Manager
* (C) 2011,2012 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_CREDENTIALS_MANAGER_H_
#define BOTAN_CREDENTIALS_MANAGER_H_

#include <botan/asn1_obj.h>
#include <botan/certstor.h>
#include <botan/pk_keys.h>
#include <botan/strong_type.h>
#include <botan/symkey.h>
#include <botan/tls_external_psk.h>
#include <botan/tls_magic.h>
#include <botan/x509cert.h>
#include <string>

namespace Botan {

class X509_DN;
class BigInt;

/**
* Interface for a credentials manager.
*
* A type is a fairly static value that represents the general nature
* of the transaction occurring. Currently used values are "tls-client"
* and "tls-server". Context represents a hostname, email address,
* username, or other identifier.
*/
class BOTAN_PUBLIC_API(2, 0) Credentials_Manager {
   public:
      virtual ~Credentials_Manager() = default;

      /**
      * Return a list of the certificates of CAs that we trust in this
      * type/context.
      *
      * @param type specifies the type of operation occurring
      *
      * @param context specifies a context relative to type. For instance
      *        for type "tls-client", context specifies the servers name.
      */
      virtual std::vector<Certificate_Store*> trusted_certificate_authorities(const std::string& type,
                                                                              const std::string& context);

      /**
      * Return a cert chain we can use, ordered from leaf to root,
      * or else an empty vector.
      *
      * It is assumed that the caller can get the private key of the
      * leaf with private_key_for
      *
      * For a comprehensive write-up of how to select certificates for TLS
      * CertificateVerify messages, see RFC 8446 Sections 4.4.2.2 and 4.4.2.3.
      *
      * @param cert_key_types specifies the key types desired ("RSA",
      *                       "DSA", "ECDSA", etc), or empty if there
      *                       is no preference by the caller.
      * @param cert_signature_schemes specifies the signature types desired
      *                               as signatures in the certificate(s) itself,
      *                               or empty for no preference by the caller.
      *
      * @param acceptable_CAs the CAs the requestor will accept (possibly empty)
      * @param type specifies the type of operation occurring
      * @param context specifies a context relative to type.
      */
      virtual std::vector<X509_Certificate> find_cert_chain(
         const std::vector<std::string>& cert_key_types,
         const std::vector<AlgorithmIdentifier>& cert_signature_schemes,
         const std::vector<X509_DN>& acceptable_CAs,
         const std::string& type,
         const std::string& context);

      /**
      * Return a raw public key to be used for authentication or nullptr if no
      * public key was found.
      *
      * It is assumed that the caller can get the private key of the leaf with
      * private_key_for().
      *
      * @param key_types  specifies the key types desired ("RSA", "DSA",
      *                   "ECDSA", etc), or empty if there is no preference by
      *                   the caller.
      * @param type       specifies the type of operation occurring
      * @param context    specifies a context relative to type.
      */
      virtual std::shared_ptr<Public_Key> find_raw_public_key(const std::vector<std::string>& key_types,
                                                              const std::string& type,
                                                              const std::string& context);

      /**
      * Return a cert chain we can use, ordered from leaf to root,
      * or else an empty vector.
      *
      * This virtual function is deprecated, and will be removed in a
      * future release. Use (and override) find_cert_chain instead.
      *
      * It is assumed that the caller can get the private key of the
      * leaf with private_key_for
      *
      * @param cert_key_types specifies the key types desired ("RSA",
      *                       "DSA", "ECDSA", etc), or empty if there
      *                       is no preference by the caller.
      * @param cert_signature_schemes specifies the signature types desired
      *                               as signatures in the certificate(s) itself,
      *                               or empty for no preference by the caller.
      *
      * @param type specifies the type of operation occurring
      *
      * @param context specifies a context relative to type.
      */
      virtual std::vector<X509_Certificate> cert_chain(const std::vector<std::string>& cert_key_types,
                                                       const std::vector<AlgorithmIdentifier>& cert_signature_schemes,
                                                       const std::string& type,
                                                       const std::string& context);

      /**
      * Return a cert chain we can use, ordered from leaf to root,
      * or else an empty vector.
      *
      * It is assumed that the caller can get the private key of the
      * leaf with private_key_for
      *
      * @param cert_key_type specifies the type of key requested
      *                      ("RSA", "DSA", "ECDSA", etc)
      * @param cert_signature_schemes specifies the signature types desired
      *                               as signatures in the certificate(s) itself,
      *                               or empty for no preference by the caller.
      *
      * @param type specifies the type of operation occurring
      *
      * @param context specifies a context relative to type.
      */
      std::vector<X509_Certificate> cert_chain_single_type(
         const std::string& cert_key_type,
         const std::vector<AlgorithmIdentifier>& cert_signature_schemes,
         const std::string& type,
         const std::string& context);

      /**
      * @return private key associated with this certificate if we should
      *         use it with this context. cert was returned by cert_chain
      * This function should either return null or throw an exception if
      * the key is unavailable.
      */
      virtual std::shared_ptr<Private_Key> private_key_for(const X509_Certificate& cert,
                                                           const std::string& type,
                                                           const std::string& context);

      /**
      * This function should either return nullptr or throw an exception if
      * the key is unavailable.
      *
      * @return private key associated with this raw public key if we should
      *         use it with this context. @p raw_public_key was returned by
      *         find_raw_public_key()
      */
      virtual std::shared_ptr<Private_Key> private_key_for(const Public_Key& raw_public_key,
                                                           const std::string& type,
                                                           const std::string& context);

      /**
       * Provides a secret value to encrypt session tickets for stateless
       * session resumptions. The default implementation returns an empty
       * key that effectively disables session tickets.
       *
       * @returns a secret value to be used to encrypt session tickets in
       *          subclasses of Session_Manager_Stateless.
       */
      virtual secure_vector<uint8_t> session_ticket_key();

      /**
       * Provides a secret to authenticate DTLS hello cookies. The default
       * implementation returns an empty key that effectively disables hello
       * cookies. Applications that wish to use DTLS are strongly advised to
       * implement this method.
       *
       * @returns a secret value to authenticate DTLS hello cookies
       */
      virtual secure_vector<uint8_t> dtls_cookie_secret();

      /**
      * @param type specifies the type of operation occurring
      * @param context specifies a context relative to type.
      * @return the PSK identity hint for this type/context
      */
      virtual std::string psk_identity_hint(const std::string& type, const std::string& context);

      /**
      * @param type specifies the type of operation occurring
      * @param context specifies a context relative to type.
      * @param identity_hint was passed by the server (but may be empty)
      * @return the PSK identity we want to use
      */
      virtual std::string psk_identity(const std::string& type,
                                       const std::string& context,
                                       const std::string& identity_hint);

      /**
      * Retrieves the PSK with the given @p identity or throws an exception.
      * It's default implementation uses find_preshared_keys() with @p identity
      * as the single allowed identity.
      *
      * This method is called by the TLS 1.2 implementation exclusively and will
      * eventually be deprecated in favor of find_preshared_keys(). Going
      * forward, new applications should implement find_preshared_keys() and
      * rely on psk()'s default implementation.
      *
      * Also, the default implementation delegates @p context "session-ticket"
      * and "dtls-cookie-secret" to the methods session_ticket_key() and
      * dtls_cookie_secret() respectively. New applications should implement
      * those methods and rely on the default implementation of psk().
      *
      * @param type specifies the type of operation occurring
      * @param context specifies a context relative to type.
      * @param identity is a PSK identity previously returned by
               psk_identity for the same type and context.
      * @return the PSK used for identity, or throw an exception if no
      *         key exists
      */
      virtual SymmetricKey psk(const std::string& type, const std::string& context, const std::string& identity);

      /**
       * Filters all available PSKs with the given criterions. Note that omitted
       * criterions (like an empty @p identities list or an unspecified @p PRF)
       * must be interpreted as "no restriction".
       *
       * Note that this is used as the underlying API for the legacy psk()
       * method currently still used in TLS 1.2. New applications should override
       * find_preshared_keys() and leave psk() with the default implementation.
       *
       * In TLS 1.3 the @p identities might contain opaque session ticket data
       * that is not necessarily a printable string, despite the utilized
       * std::string type. Implementations must be prepared to ignore identities
       * generated via the TLS 1.3 resumption mechanism.
       *
       * @param host        the host name for which a PSK is requested (may be empty)
       * @param whoami      the type of the host (client or server) that is requesting
       * @param identities  an optional filter for PSK identities to be returned
       *                    (an empty list means: all identities are welcome)
       * @param prf         an optional filter for the Pseudo Random Function the PRFs
       *                    must be provisioned for
       *
       * @returns a list of PSKs that meet the defined criterions in preference order
       */
      virtual std::vector<TLS::ExternalPSK> find_preshared_keys(std::string_view host,
                                                                TLS::Connection_Side whoami,
                                                                const std::vector<std::string>& identities = {},
                                                                const std::optional<std::string>& prf = std::nullopt);

      /**
       * Selects a single PSK identity from the given @p identities and returns
       * its details (i.e. the secret value) for it to be used in the handshake.
       *
       * The default implementation relies on the filtering capabilities
       * provided by find_preshared_keys() and simply selects the first PSK
       * returned. If applications need finer grained control, they should
       * override this method.
       *
       * In TLS 1.3 the @p identities might contain opaque session ticket data
       * that is not necessarily a printable string, despite the utilized
       * std::string type. Implementations must be prepared to ignore identities
       * generated via the TLS 1.3 resumption mechanism.
       *
       * @param host        the host name for which a PSK is requested (may be empty)
       * @param whoami      the type of the host (client or server) that is requesting
       * @param identities  an optional filter for PSK identities to be returned
       *                    (an empty list means: all identities are welcome)
       * @param prf         an optional filter for the Pseudo Random Function the PRFs
       *                    must be provisioned for
       *
       * @returns the PSK for the selected identity or std::nullopt if no PSK
       *          meets the requirements
       */
      virtual std::optional<TLS::ExternalPSK> choose_preshared_key(
         std::string_view host,
         TLS::Connection_Side whoami,
         const std::vector<std::string>& identities,
         const std::optional<std::string>& prf = std::nullopt);
};

}  // namespace Botan

#endif
