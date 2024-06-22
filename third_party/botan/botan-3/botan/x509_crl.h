/*
* X.509 CRL
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_X509_CRL_H_
#define BOTAN_X509_CRL_H_

#include <botan/asn1_obj.h>
#include <botan/pkix_enums.h>
#include <botan/x509_obj.h>
#include <vector>

namespace Botan {

class Extensions;
class X509_Certificate;
class X509_DN;

struct CRL_Entry_Data;
struct CRL_Data;

/**
* This class represents CRL entries
*/
class BOTAN_PUBLIC_API(2, 0) CRL_Entry final : public ASN1_Object {
   public:
      void encode_into(DER_Encoder&) const override;
      void decode_from(BER_Decoder&) override;

      /**
      * Get the serial number of the certificate associated with this entry.
      * @return certificate's serial number
      */
      const std::vector<uint8_t>& serial_number() const;

      /**
      * Get the revocation date of the certificate associated with this entry
      * @return certificate's revocation date
      */
      const X509_Time& expire_time() const;

      /**
      * Get the entries reason code
      * @return reason code
      */
      CRL_Code reason_code() const;

      /**
      * Get the extensions on this CRL entry
      */
      const Extensions& extensions() const;

      /**
      * Create uninitialized CRL_Entry object
      */
      CRL_Entry() = default;

      /**
      * Construct an CRL entry.
      * @param cert the certificate to revoke
      * @param reason the reason code to set in the entry
      */
      CRL_Entry(const X509_Certificate& cert, CRL_Code reason = CRL_Code::Unspecified);

   private:
      friend class X509_CRL;

      const CRL_Entry_Data& data() const;

      std::shared_ptr<CRL_Entry_Data> m_data;
};

/**
* Test two CRL entries for equality in all fields.
*/
BOTAN_PUBLIC_API(2, 0) bool operator==(const CRL_Entry&, const CRL_Entry&);

/**
* Test two CRL entries for inequality in at least one field.
*/
BOTAN_PUBLIC_API(2, 0) bool operator!=(const CRL_Entry&, const CRL_Entry&);

/**
* This class represents X.509 Certificate Revocation Lists (CRLs).
*/
class BOTAN_PUBLIC_API(2, 0) X509_CRL final : public X509_Object {
   public:
      /**
      * Check if this particular certificate is listed in the CRL
      */
      bool is_revoked(const X509_Certificate& cert) const;

      /**
      * Get the entries of this CRL in the form of a vector.
      * @return vector containing the entries of this CRL.
      */
      const std::vector<CRL_Entry>& get_revoked() const;

      /**
      * Get the issuer DN of this CRL.
      * @return CRLs issuer DN
      */
      const X509_DN& issuer_dn() const;

      /**
      * @return extension data for this CRL
      */
      const Extensions& extensions() const;

      /**
      * Get the AuthorityKeyIdentifier of this CRL.
      * @return this CRLs AuthorityKeyIdentifier
      */
      const std::vector<uint8_t>& authority_key_id() const;

      /**
      * Get the serial number of this CRL.
      * @return CRLs serial number
      */
      uint32_t crl_number() const;

      /**
      * Get the CRL's thisUpdate value.
      * @return CRLs thisUpdate
      */
      const X509_Time& this_update() const;

      /**
      * Get the CRL's nextUpdate value.
      * @return CRLs nextdUpdate
      */
      const X509_Time& next_update() const;

      /**
      * Get the CRL's issuing distribution point
      */
      BOTAN_DEPRECATED("Use issuing_distribution_points") std::string crl_issuing_distribution_point() const;

      /**
      * Get the CRL's issuing distribution points
      *
      * See https://www.rfc-editor.org/rfc/rfc5280#section-5.2.5
      */
      std::vector<std::string> issuing_distribution_points() const;

      /**
      * Create an uninitialized CRL object. Any attempts to access
      * this object will throw an exception.
      */
      X509_CRL() = default;

      /**
      * Construct a CRL from a data source.
      * @param source the data source providing the DER or PEM encoded CRL.
      */
      X509_CRL(DataSource& source);

#if defined(BOTAN_TARGET_OS_HAS_FILESYSTEM)
      /**
      * Construct a CRL from a file containing the DER or PEM encoded CRL.
      * @param filename the name of the CRL file
      */
      X509_CRL(std::string_view filename);
#endif

      /**
      * Construct a CRL from a binary vector
      * @param vec the binary (DER) representation of the CRL
      */
      X509_CRL(const std::vector<uint8_t>& vec);

      /**
      * Construct a CRL
      * @param issuer issuer of this CRL
      * @param thisUpdate valid from
      * @param nextUpdate valid until
      * @param revoked entries to be included in the CRL
      */
      X509_CRL(const X509_DN& issuer,
               const X509_Time& thisUpdate,
               const X509_Time& nextUpdate,
               const std::vector<CRL_Entry>& revoked);

   private:
      std::string PEM_label() const override;

      std::vector<std::string> alternate_PEM_labels() const override;

      void force_decode() override;

      const CRL_Data& data() const;

      std::shared_ptr<CRL_Data> m_data;
};

}  // namespace Botan

#endif
