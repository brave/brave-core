/*
* X.509 Certificate Authority
* (C) 1999-2008 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_X509_CA_H_
#define BOTAN_X509_CA_H_

#include <botan/x509_crl.h>
#include <botan/x509cert.h>
#include <chrono>
#include <map>

namespace Botan {

class RandomNumberGenerator;
class BigInt;
class Private_Key;
class PKCS10_Request;
class PK_Signer;

/**
* An interface capable of creating new X.509 certificates
*/
class BOTAN_PUBLIC_API(2, 0) X509_CA final {
   public:
      /**
      * Return the algorithm identifier used to identify signatures that
      * this CA will create.
      */
      const AlgorithmIdentifier& algorithm_identifier() const { return m_ca_sig_algo; }

      /**
      * Return the CA's certificate
      */
      const X509_Certificate& ca_certificate() const { return m_ca_cert; }

      /**
      * Return the hash function the CA is using to sign with
      */
      const std::string& hash_function() const { return m_hash_fn; }

      /**
      * Return the signature object this CA uses to sign with
      */
      PK_Signer& signature_op() { return *m_signer; }

      /**
      * Sign a PKCS#10 Request.
      * @param req the request to sign
      * @param rng the rng to use
      * @param not_before the starting time for the certificate
      * @param not_after the expiration time for the certificate
      * @return resulting certificate
      */
      X509_Certificate sign_request(const PKCS10_Request& req,
                                    RandomNumberGenerator& rng,
                                    const X509_Time& not_before,
                                    const X509_Time& not_after) const;

      /**
      * Sign a PKCS#10 Request.
      * @param req the request to sign
      * @param rng the rng to use
      * @param serial_number the serial number the cert will be assigned.
      * @param not_before the starting time for the certificate
      * @param not_after the expiration time for the certificate
      * @return resulting certificate
      */
      X509_Certificate sign_request(const PKCS10_Request& req,
                                    RandomNumberGenerator& rng,
                                    const BigInt& serial_number,
                                    const X509_Time& not_before,
                                    const X509_Time& not_after) const;

      /**
      * Create a new and empty CRL for this CA.
      * @param rng the random number generator to use
      * @param issue_time the issue time (typically system_clock::now)
      * @param next_update the time interval after issue_data within which
      *        a new CRL will be produced.
      * @return new CRL
      */
      X509_CRL new_crl(RandomNumberGenerator& rng,
                       std::chrono::system_clock::time_point issue_time,
                       std::chrono::seconds next_update) const;

      /**
      * Create a new CRL by with additional entries.
      * @param last_crl the last CRL of this CA to add the new entries to
      * @param new_entries contains the new CRL entries to be added to the CRL
      * @param rng the random number generator to use
      * @param issue_time the issue time (typically system_clock::now)
      * @param next_update the time interval after issue_data within which
      *        a new CRL will be produced.
      */
      X509_CRL update_crl(const X509_CRL& last_crl,
                          const std::vector<CRL_Entry>& new_entries,
                          RandomNumberGenerator& rng,
                          std::chrono::system_clock::time_point issue_time,
                          std::chrono::seconds next_update) const;

      /**
      * Create a new and empty CRL for this CA.
      * @param rng the random number generator to use
      * @param next_update the time to set in next update in seconds
      * as the offset from the current time
      * @return new CRL
      */
      X509_CRL new_crl(RandomNumberGenerator& rng, uint32_t next_update = 604800) const;

      /**
      * Create a new CRL by with additional entries.
      * @param last_crl the last CRL of this CA to add the new entries to
      * @param new_entries contains the new CRL entries to be added to the CRL
      * @param rng the random number generator to use
      * @param next_update the time to set in next update in seconds
      * as the offset from the current time
      */
      X509_CRL update_crl(const X509_CRL& last_crl,
                          const std::vector<CRL_Entry>& new_entries,
                          RandomNumberGenerator& rng,
                          uint32_t next_update = 604800) const;

      /**
      * Return the set of extensions that will be used for a certificate.
      *
      * This is a helper method that is used internally. It is also exposed
      * so you can call it directly and then modify the extensions before
      * creating a certificate using X509_CA::make_cert.
      */
      static Extensions choose_extensions(const PKCS10_Request& req,
                                          const X509_Certificate& ca_certificate,
                                          std::string_view hash_fn);

      /**
      * Interface for creating new certificates
      * @param signer a signing object
      * @param rng a random number generator
      * @param sig_algo the signature algorithm identifier
      * @param pub_key the serialized public key
      * @param not_before the start time of the certificate
      * @param not_after the end time of the certificate
      * @param issuer_dn the DN of the issuer
      * @param subject_dn the DN of the subject
      * @param extensions an optional list of certificate extensions
      * @returns newly minted certificate
      */
      static X509_Certificate make_cert(PK_Signer& signer,
                                        RandomNumberGenerator& rng,
                                        const AlgorithmIdentifier& sig_algo,
                                        const std::vector<uint8_t>& pub_key,
                                        const X509_Time& not_before,
                                        const X509_Time& not_after,
                                        const X509_DN& issuer_dn,
                                        const X509_DN& subject_dn,
                                        const Extensions& extensions);

      /**
      * Interface for creating new certificates
      * @param signer a signing object
      * @param rng a random number generator
      * @param serial_number the serial number the cert will be assigned
      * @param sig_algo the signature algorithm identifier
      * @param pub_key the serialized public key
      * @param not_before the start time of the certificate
      * @param not_after the end time of the certificate
      * @param issuer_dn the DN of the issuer
      * @param subject_dn the DN of the subject
      * @param extensions an optional list of certificate extensions
      * @returns newly minted certificate
      */
      static X509_Certificate make_cert(PK_Signer& signer,
                                        RandomNumberGenerator& rng,
                                        const BigInt& serial_number,
                                        const AlgorithmIdentifier& sig_algo,
                                        const std::vector<uint8_t>& pub_key,
                                        const X509_Time& not_before,
                                        const X509_Time& not_after,
                                        const X509_DN& issuer_dn,
                                        const X509_DN& subject_dn,
                                        const Extensions& extensions);

      /**
      * Create a new CA object with custom padding option
      *
      * This is mostly useful for creating RSA-PSS certificates
      *
      * @param ca_certificate the certificate of the CA
      * @param key the private key of the CA
      * @param hash_fn name of a hash function to use for signing
      * @param padding_method name of the signature padding method to use
      * @param rng the random generator to use
      */
      X509_CA(const X509_Certificate& ca_certificate,
              const Private_Key& key,
              std::string_view hash_fn,
              std::string_view padding_method,
              RandomNumberGenerator& rng);

      /**
      * Create a new CA object.
      * @param ca_certificate the certificate of the CA
      * @param key the private key of the CA
      * @param hash_fn name of a hash function to use for signing
      * @param rng the random generator to use
      */
      X509_CA(const X509_Certificate& ca_certificate,
              const Private_Key& key,
              std::string_view hash_fn,
              RandomNumberGenerator& rng) :
            X509_CA(ca_certificate, key, hash_fn, "", rng) {}

      /**
      * Create a new CA object.
      * @param ca_certificate the certificate of the CA
      * @param key the private key of the CA
      * @param opts additional options, e.g. padding, as key value pairs
      * @param hash_fn name of a hash function to use for signing
      * @param rng the random generator to use
      */
      BOTAN_DEPRECATED("Use version taking padding as an explicit arg")

      X509_CA(const X509_Certificate& ca_certificate,
              const Private_Key& key,
              const std::map<std::string, std::string>& opts,
              std::string_view hash_fn,
              RandomNumberGenerator& rng) :
            X509_CA(ca_certificate, key, hash_fn, opts.at("padding"), rng) {}

      X509_CA(const X509_CA&) = delete;
      X509_CA& operator=(const X509_CA&) = delete;

      X509_CA(X509_CA&&) = default;
      X509_CA& operator=(X509_CA&&) = default;

      ~X509_CA();

   private:
      X509_CRL make_crl(const std::vector<CRL_Entry>& entries,
                        uint32_t crl_number,
                        RandomNumberGenerator& rng,
                        std::chrono::system_clock::time_point issue_time,
                        std::chrono::seconds next_update) const;

      AlgorithmIdentifier m_ca_sig_algo;
      X509_Certificate m_ca_cert;
      std::string m_hash_fn;
      std::unique_ptr<PK_Signer> m_signer;
};

}  // namespace Botan

#endif
