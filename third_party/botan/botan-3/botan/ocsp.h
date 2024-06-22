/*
* OCSP
* (C) 2012 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_OCSP_H_
#define BOTAN_OCSP_H_

#include <botan/asn1_obj.h>
#include <botan/bigint.h>
#include <botan/pkix_types.h>
#include <botan/x509cert.h>

#include <chrono>
#include <optional>

namespace Botan {

class Certificate_Store;

namespace OCSP {

class BOTAN_PUBLIC_API(2, 0) CertID final : public ASN1_Object {
   public:
      CertID() = default;

      CertID(const X509_Certificate& issuer, const BigInt& subject_serial);

      bool is_id_for(const X509_Certificate& issuer, const X509_Certificate& subject) const;

      void encode_into(DER_Encoder& to) const override;

      void decode_from(BER_Decoder& from) override;

      const std::vector<uint8_t>& issuer_key_hash() const { return m_issuer_key_hash; }

   private:
      AlgorithmIdentifier m_hash_id;
      std::vector<uint8_t> m_issuer_dn_hash;
      std::vector<uint8_t> m_issuer_key_hash;
      BigInt m_subject_serial;
};

class BOTAN_PUBLIC_API(2, 0) SingleResponse final : public ASN1_Object {
   public:
      const CertID& certid() const { return m_certid; }

      size_t cert_status() const { return m_cert_status; }

      X509_Time this_update() const { return m_thisupdate; }

      X509_Time next_update() const { return m_nextupdate; }

      void encode_into(DER_Encoder& to) const override;

      void decode_from(BER_Decoder& from) override;

   private:
      CertID m_certid;
      size_t m_cert_status = 2;  // unknown
      X509_Time m_thisupdate;
      X509_Time m_nextupdate;
};

/**
* An OCSP request.
*/
class BOTAN_PUBLIC_API(2, 0) Request final {
   public:
      /**
      * Create an OCSP request.
      * @param issuer_cert issuer certificate
      * @param subject_cert subject certificate
      */
      Request(const X509_Certificate& issuer_cert, const X509_Certificate& subject_cert);

      Request(const X509_Certificate& issuer_cert, const BigInt& subject_serial);

      /**
      * @return BER-encoded OCSP request
      */
      std::vector<uint8_t> BER_encode() const;

      /**
      * @return Base64-encoded OCSP request
      */
      std::string base64_encode() const;

      /**
      * @return issuer certificate
      */
      const X509_Certificate& issuer() const { return m_issuer; }

      /**
      * @return subject certificate
      */
      const X509_Certificate& subject() const { throw Not_Implemented("Method have been deprecated"); }

      const std::vector<uint8_t>& issuer_key_hash() const { return m_certid.issuer_key_hash(); }

   private:
      X509_Certificate m_issuer;
      CertID m_certid;
};

/**
* OCSP response status.
*
* see https://tools.ietf.org/html/rfc6960#section-4.2.1
*/
enum class Response_Status_Code {
   Successful = 0,
   Malformed_Request = 1,
   Internal_Error = 2,
   Try_Later = 3,
   Sig_Required = 5,
   Unauthorized = 6
};

/**
* OCSP response.
*
* Note this class is only usable as an OCSP client
*/
class BOTAN_PUBLIC_API(2, 0) Response final {
   public:
      /**
      * Create a fake OCSP response from a given status code.
      * @param status the status code the check functions will return
      */
      Response(Certificate_Status_Code status);

      /**
      * Parses an OCSP response.
      * @param response_bits response bits received
      */
      Response(const std::vector<uint8_t>& response_bits) : Response(response_bits.data(), response_bits.size()) {}

      /**
      * Parses an OCSP response.
      * @param response_bits response bits received
      * @param response_bits_len length of response in bytes
      */
      Response(const uint8_t response_bits[], size_t response_bits_len);

      /**
      * Find the certificate that signed this OCSP response from all possible
      * candidates and taking the attached certificates into account.
      *
      * @param issuer_certificate is the issuer of the certificate in question
      * @param trusted_ocsp_responders optionally, a certificate store containing
      *        additionally trusted responder certificates
      *
      * @return the certificate that signed this response or std::nullopt if not found
      */
      std::optional<X509_Certificate> find_signing_certificate(
         const X509_Certificate& issuer_certificate, const Certificate_Store* trusted_ocsp_responders = nullptr) const;

      /**
      * Check signature of the OCSP response.
      *
      * Note: It is the responsibility of the caller to verify that signing
      *       certificate is trustworthy and authorized to do so.
      *
      * @param signing_certificate the certificate that signed this response
      *                            (@sa Response::find_signing_certificate).
      *
      * @return status code indicating the validity of the signature
      */
      Certificate_Status_Code verify_signature(const X509_Certificate& signing_certificate) const;

      /**
      * @return the status of the response
      */
      Response_Status_Code status() const { return m_status; }

      /**
      * @return the time this OCSP response was supposedly produced at
      */
      const X509_Time& produced_at() const { return m_produced_at; }

      /**
      * @return DN of signer, if provided in response (may be empty)
      */
      const X509_DN& signer_name() const { return m_signer_name; }

      /**
      * @return key hash, if provided in response (may be empty)
      */
      const std::vector<uint8_t>& signer_key_hash() const { return m_key_hash; }

      const std::vector<uint8_t>& raw_bits() const { return m_response_bits; }

      /**
       * Searches the OCSP response for issuer and subject certificate.
       * @param issuer issuer certificate
       * @param subject subject certificate
       * @param ref_time the reference time
       * @param max_age the maximum age the response should be considered valid
       *                if next_update is not set
       * @return OCSP status code, possible values:
       *         CERT_IS_REVOKED,
       *         OCSP_NOT_YET_VALID,
       *         OCSP_HAS_EXPIRED,
       *         OCSP_IS_TOO_OLD,
       *         OCSP_RESPONSE_GOOD,
       *         OCSP_BAD_STATUS,
       *         OCSP_CERT_NOT_LISTED
       */
      Certificate_Status_Code status_for(
         const X509_Certificate& issuer,
         const X509_Certificate& subject,
         std::chrono::system_clock::time_point ref_time = std::chrono::system_clock::now(),
         std::chrono::seconds max_age = std::chrono::seconds::zero()) const;

      /**
       * @return the certificate chain, if provided in response
       */
      const std::vector<X509_Certificate>& certificates() const { return m_certs; }

      /**
      * @return the dummy response if this is a 'fake' OCSP response otherwise std::nullopt
      */
      std::optional<Certificate_Status_Code> dummy_status() const { return m_dummy_response_status; }

   private:
      bool is_issued_by(const X509_Certificate& candidate) const;

   private:
      Response_Status_Code m_status = Response_Status_Code::Internal_Error;
      std::vector<uint8_t> m_response_bits;
      X509_Time m_produced_at;
      X509_DN m_signer_name;
      std::vector<uint8_t> m_key_hash;
      std::vector<uint8_t> m_tbs_bits;
      AlgorithmIdentifier m_sig_algo;
      std::vector<uint8_t> m_signature;
      std::vector<X509_Certificate> m_certs;

      std::vector<SingleResponse> m_responses;

      std::optional<Certificate_Status_Code> m_dummy_response_status;
};

#if defined(BOTAN_HAS_HTTP_UTIL)

/**
* Makes an online OCSP request via HTTP and returns the (unverified) OCSP response.
* @param issuer issuer certificate
* @param subject_serial the subject's serial number
* @param ocsp_responder the OCSP responder to query
* @param timeout a timeout on the HTTP request
* @return OCSP response
*/
BOTAN_PUBLIC_API(3, 0)
Response online_check(const X509_Certificate& issuer,
                      const BigInt& subject_serial,
                      std::string_view ocsp_responder,
                      std::chrono::milliseconds timeout = std::chrono::milliseconds(3000));

/**
* Makes an online OCSP request via HTTP and returns the (unverified) OCSP response.
* @param issuer issuer certificate
* @param subject subject certificate
* @param timeout a timeout on the HTTP request
* @return OCSP response
*/
BOTAN_PUBLIC_API(3, 0)
Response online_check(const X509_Certificate& issuer,
                      const X509_Certificate& subject,
                      std::chrono::milliseconds timeout = std::chrono::milliseconds(3000));

#endif

}  // namespace OCSP

}  // namespace Botan

#endif
