/*
* X.509 Certificate Extensions
* (C) 1999-2007,2012 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_X509_EXTENSIONS_H_
#define BOTAN_X509_EXTENSIONS_H_

#include <botan/pkix_types.h>
#include <set>

namespace Botan {

class X509_Certificate;

namespace Cert_Extension {

static const size_t NO_CERT_PATH_LIMIT = 0xFFFFFFF0;

/**
* Basic Constraints Extension
*/
class BOTAN_PUBLIC_API(2, 0) Basic_Constraints final : public Certificate_Extension {
   public:
      std::unique_ptr<Certificate_Extension> copy() const override {
         return std::make_unique<Basic_Constraints>(m_is_ca, m_path_limit);
      }

      Basic_Constraints(bool ca = false, size_t limit = 0) : m_is_ca(ca), m_path_limit(limit) {}

      bool get_is_ca() const { return m_is_ca; }

      size_t get_path_limit() const;

      static OID static_oid() { return OID("2.5.29.19"); }

      OID oid_of() const override { return static_oid(); }

   private:
      std::string oid_name() const override { return "X509v3.BasicConstraints"; }

      std::vector<uint8_t> encode_inner() const override;
      void decode_inner(const std::vector<uint8_t>&) override;

      bool m_is_ca;
      size_t m_path_limit;
};

/**
* Key Usage Constraints Extension
*/
class BOTAN_PUBLIC_API(2, 0) Key_Usage final : public Certificate_Extension {
   public:
      std::unique_ptr<Certificate_Extension> copy() const override {
         return std::make_unique<Key_Usage>(m_constraints);
      }

      explicit Key_Usage(Key_Constraints c) : m_constraints(c) {}

      explicit Key_Usage() : m_constraints(Key_Constraints::None) {}

      Key_Constraints get_constraints() const { return m_constraints; }

      static OID static_oid() { return OID("2.5.29.15"); }

      OID oid_of() const override { return static_oid(); }

   private:
      std::string oid_name() const override { return "X509v3.KeyUsage"; }

      bool should_encode() const override { return !m_constraints.empty(); }

      std::vector<uint8_t> encode_inner() const override;
      void decode_inner(const std::vector<uint8_t>&) override;

      Key_Constraints m_constraints;
};

/**
* Subject Key Identifier Extension
*/
class BOTAN_PUBLIC_API(2, 0) Subject_Key_ID final : public Certificate_Extension {
   public:
      Subject_Key_ID() = default;

      explicit Subject_Key_ID(const std::vector<uint8_t>& k) : m_key_id(k) {}

      Subject_Key_ID(const std::vector<uint8_t>& public_key, std::string_view hash_fn);

      std::unique_ptr<Certificate_Extension> copy() const override {
         return std::make_unique<Subject_Key_ID>(m_key_id);
      }

      const std::vector<uint8_t>& get_key_id() const { return m_key_id; }

      static OID static_oid() { return OID("2.5.29.14"); }

      OID oid_of() const override { return static_oid(); }

   private:
      std::string oid_name() const override { return "X509v3.SubjectKeyIdentifier"; }

      bool should_encode() const override { return (!m_key_id.empty()); }

      std::vector<uint8_t> encode_inner() const override;
      void decode_inner(const std::vector<uint8_t>&) override;

      std::vector<uint8_t> m_key_id;
};

/**
* Authority Key Identifier Extension
*/
class BOTAN_PUBLIC_API(2, 0) Authority_Key_ID final : public Certificate_Extension {
   public:
      std::unique_ptr<Certificate_Extension> copy() const override {
         return std::make_unique<Authority_Key_ID>(m_key_id);
      }

      Authority_Key_ID() = default;

      explicit Authority_Key_ID(const std::vector<uint8_t>& k) : m_key_id(k) {}

      const std::vector<uint8_t>& get_key_id() const { return m_key_id; }

      static OID static_oid() { return OID("2.5.29.35"); }

      OID oid_of() const override { return static_oid(); }

   private:
      std::string oid_name() const override { return "X509v3.AuthorityKeyIdentifier"; }

      bool should_encode() const override { return (!m_key_id.empty()); }

      std::vector<uint8_t> encode_inner() const override;
      void decode_inner(const std::vector<uint8_t>&) override;

      std::vector<uint8_t> m_key_id;
};

/**
* Subject Alternative Name Extension
*/
class BOTAN_PUBLIC_API(2, 4) Subject_Alternative_Name final : public Certificate_Extension {
   public:
      const AlternativeName& get_alt_name() const { return m_alt_name; }

      static OID static_oid() { return OID("2.5.29.17"); }

      OID oid_of() const override { return static_oid(); }

      std::unique_ptr<Certificate_Extension> copy() const override {
         return std::make_unique<Subject_Alternative_Name>(get_alt_name());
      }

      explicit Subject_Alternative_Name(const AlternativeName& name = AlternativeName()) : m_alt_name(name) {}

   private:
      std::string oid_name() const override { return "X509v3.SubjectAlternativeName"; }

      bool should_encode() const override { return m_alt_name.has_items(); }

      std::vector<uint8_t> encode_inner() const override;
      void decode_inner(const std::vector<uint8_t>&) override;

      AlternativeName m_alt_name;
};

/**
* Issuer Alternative Name Extension
*/
class BOTAN_PUBLIC_API(2, 0) Issuer_Alternative_Name final : public Certificate_Extension {
   public:
      const AlternativeName& get_alt_name() const { return m_alt_name; }

      static OID static_oid() { return OID("2.5.29.18"); }

      OID oid_of() const override { return static_oid(); }

      std::unique_ptr<Certificate_Extension> copy() const override {
         return std::make_unique<Issuer_Alternative_Name>(get_alt_name());
      }

      explicit Issuer_Alternative_Name(const AlternativeName& name = AlternativeName()) : m_alt_name(name) {}

   private:
      std::string oid_name() const override { return "X509v3.IssuerAlternativeName"; }

      bool should_encode() const override { return m_alt_name.has_items(); }

      std::vector<uint8_t> encode_inner() const override;
      void decode_inner(const std::vector<uint8_t>&) override;

      AlternativeName m_alt_name;
};

/**
* Extended Key Usage Extension
*/
class BOTAN_PUBLIC_API(2, 0) Extended_Key_Usage final : public Certificate_Extension {
   public:
      std::unique_ptr<Certificate_Extension> copy() const override {
         return std::make_unique<Extended_Key_Usage>(m_oids);
      }

      Extended_Key_Usage() = default;

      explicit Extended_Key_Usage(const std::vector<OID>& o) : m_oids(o) {}

      const std::vector<OID>& object_identifiers() const { return m_oids; }

      static OID static_oid() { return OID("2.5.29.37"); }

      OID oid_of() const override { return static_oid(); }

   private:
      std::string oid_name() const override { return "X509v3.ExtendedKeyUsage"; }

      bool should_encode() const override { return (!m_oids.empty()); }

      std::vector<uint8_t> encode_inner() const override;
      void decode_inner(const std::vector<uint8_t>&) override;

      std::vector<OID> m_oids;
};

/**
* Name Constraints
*/
class BOTAN_PUBLIC_API(2, 0) Name_Constraints final : public Certificate_Extension {
   public:
      std::unique_ptr<Certificate_Extension> copy() const override {
         return std::make_unique<Name_Constraints>(m_name_constraints);
      }

      Name_Constraints() = default;

      Name_Constraints(const NameConstraints& nc) : m_name_constraints(nc) {}

      void validate(const X509_Certificate& subject,
                    const X509_Certificate& issuer,
                    const std::vector<X509_Certificate>& cert_path,
                    std::vector<std::set<Certificate_Status_Code>>& cert_status,
                    size_t pos) override;

      const NameConstraints& get_name_constraints() const { return m_name_constraints; }

      static OID static_oid() { return OID("2.5.29.30"); }

      OID oid_of() const override { return static_oid(); }

   private:
      std::string oid_name() const override { return "X509v3.NameConstraints"; }

      bool should_encode() const override { return true; }

      std::vector<uint8_t> encode_inner() const override;
      void decode_inner(const std::vector<uint8_t>&) override;

      NameConstraints m_name_constraints;
};

/**
* Certificate Policies Extension
*/
class BOTAN_PUBLIC_API(2, 0) Certificate_Policies final : public Certificate_Extension {
   public:
      std::unique_ptr<Certificate_Extension> copy() const override {
         return std::make_unique<Certificate_Policies>(m_oids);
      }

      Certificate_Policies() = default;

      explicit Certificate_Policies(const std::vector<OID>& o) : m_oids(o) {}

      const std::vector<OID>& get_policy_oids() const { return m_oids; }

      static OID static_oid() { return OID("2.5.29.32"); }

      OID oid_of() const override { return static_oid(); }

      void validate(const X509_Certificate& subject,
                    const X509_Certificate& issuer,
                    const std::vector<X509_Certificate>& cert_path,
                    std::vector<std::set<Certificate_Status_Code>>& cert_status,
                    size_t pos) override;

   private:
      std::string oid_name() const override { return "X509v3.CertificatePolicies"; }

      bool should_encode() const override { return (!m_oids.empty()); }

      std::vector<uint8_t> encode_inner() const override;
      void decode_inner(const std::vector<uint8_t>&) override;

      std::vector<OID> m_oids;
};

/**
* Authority Information Access Extension
*/
class BOTAN_PUBLIC_API(2, 0) Authority_Information_Access final : public Certificate_Extension {
   public:
      std::unique_ptr<Certificate_Extension> copy() const override {
         return std::make_unique<Authority_Information_Access>(m_ocsp_responder, m_ca_issuers);
      }

      Authority_Information_Access() = default;

      explicit Authority_Information_Access(std::string_view ocsp,
                                            const std::vector<std::string>& ca_issuers = std::vector<std::string>()) :
            m_ocsp_responder(ocsp), m_ca_issuers(ca_issuers) {}

      std::string ocsp_responder() const { return m_ocsp_responder; }

      static OID static_oid() { return OID("1.3.6.1.5.5.7.1.1"); }

      OID oid_of() const override { return static_oid(); }

      const std::vector<std::string>& ca_issuers() const { return m_ca_issuers; }

   private:
      std::string oid_name() const override { return "PKIX.AuthorityInformationAccess"; }

      bool should_encode() const override { return (!m_ocsp_responder.empty() || !m_ca_issuers.empty()); }

      std::vector<uint8_t> encode_inner() const override;
      void decode_inner(const std::vector<uint8_t>&) override;

      std::string m_ocsp_responder;
      std::vector<std::string> m_ca_issuers;
};

/**
* CRL Number Extension
*/
class BOTAN_PUBLIC_API(2, 0) CRL_Number final : public Certificate_Extension {
   public:
      std::unique_ptr<Certificate_Extension> copy() const override;

      CRL_Number() : m_has_value(false), m_crl_number(0) {}

      CRL_Number(size_t n) : m_has_value(true), m_crl_number(n) {}

      size_t get_crl_number() const;

      static OID static_oid() { return OID("2.5.29.20"); }

      OID oid_of() const override { return static_oid(); }

   private:
      std::string oid_name() const override { return "X509v3.CRLNumber"; }

      bool should_encode() const override { return m_has_value; }

      std::vector<uint8_t> encode_inner() const override;
      void decode_inner(const std::vector<uint8_t>&) override;

      bool m_has_value;
      size_t m_crl_number;
};

/**
* CRL Entry Reason Code Extension
*/
class BOTAN_PUBLIC_API(2, 0) CRL_ReasonCode final : public Certificate_Extension {
   public:
      std::unique_ptr<Certificate_Extension> copy() const override {
         return std::make_unique<CRL_ReasonCode>(m_reason);
      }

      explicit CRL_ReasonCode(CRL_Code r = CRL_Code::Unspecified) : m_reason(r) {}

      CRL_Code get_reason() const { return m_reason; }

      static OID static_oid() { return OID("2.5.29.21"); }

      OID oid_of() const override { return static_oid(); }

   private:
      std::string oid_name() const override { return "X509v3.ReasonCode"; }

      bool should_encode() const override { return (m_reason != CRL_Code::Unspecified); }

      std::vector<uint8_t> encode_inner() const override;
      void decode_inner(const std::vector<uint8_t>&) override;

      CRL_Code m_reason;
};

/**
* CRL Distribution Points Extension
* todo enforce restrictions from RFC 5280 4.2.1.13
*/
class BOTAN_PUBLIC_API(2, 0) CRL_Distribution_Points final : public Certificate_Extension {
   public:
      class BOTAN_PUBLIC_API(2, 0) Distribution_Point final : public ASN1_Object {
         public:
            void encode_into(DER_Encoder&) const override;
            void decode_from(BER_Decoder&) override;

            explicit Distribution_Point(const AlternativeName& name = AlternativeName()) : m_point(name) {}

            const AlternativeName& point() const { return m_point; }

         private:
            AlternativeName m_point;
      };

      std::unique_ptr<Certificate_Extension> copy() const override {
         return std::make_unique<CRL_Distribution_Points>(m_distribution_points);
      }

      CRL_Distribution_Points() = default;

      explicit CRL_Distribution_Points(const std::vector<Distribution_Point>& points) : m_distribution_points(points) {}

      const std::vector<Distribution_Point>& distribution_points() const { return m_distribution_points; }

      const std::vector<std::string>& crl_distribution_urls() const { return m_crl_distribution_urls; }

      static OID static_oid() { return OID("2.5.29.31"); }

      OID oid_of() const override { return static_oid(); }

   private:
      std::string oid_name() const override { return "X509v3.CRLDistributionPoints"; }

      bool should_encode() const override { return !m_distribution_points.empty(); }

      std::vector<uint8_t> encode_inner() const override;
      void decode_inner(const std::vector<uint8_t>&) override;

      std::vector<Distribution_Point> m_distribution_points;
      std::vector<std::string> m_crl_distribution_urls;
};

/**
* CRL Issuing Distribution Point Extension
* todo enforce restrictions from RFC 5280 5.2.5
*/
class CRL_Issuing_Distribution_Point final : public Certificate_Extension {
   public:
      CRL_Issuing_Distribution_Point() = default;

      explicit CRL_Issuing_Distribution_Point(const CRL_Distribution_Points::Distribution_Point& distribution_point) :
            m_distribution_point(distribution_point) {}

      std::unique_ptr<Certificate_Extension> copy() const override {
         return std::make_unique<CRL_Issuing_Distribution_Point>(m_distribution_point);
      }

      const AlternativeName& get_point() const { return m_distribution_point.point(); }

      static OID static_oid() { return OID("2.5.29.28"); }

      OID oid_of() const override { return static_oid(); }

   private:
      std::string oid_name() const override { return "X509v3.CRLIssuingDistributionPoint"; }

      bool should_encode() const override { return true; }

      std::vector<uint8_t> encode_inner() const override;
      void decode_inner(const std::vector<uint8_t>&) override;

      CRL_Distribution_Points::Distribution_Point m_distribution_point;
};

/**
* OCSP NoCheck Extension
*
* RFC6960 4.2.2.2.1
*    A CA may specify that an OCSP client can trust a responder for the
*    lifetime of the responder's certificate.  The CA does so by
*    including the extension id-pkix-ocsp-nocheck.
*
* In other words: OCSP responder certificates with this extension do not need
*                 to be validated against some revocation info.
*/
class OCSP_NoCheck final : public Certificate_Extension {
   public:
      OCSP_NoCheck() = default;

      std::unique_ptr<Certificate_Extension> copy() const override { return std::make_unique<OCSP_NoCheck>(); }

      static OID static_oid() { return OID("1.3.6.1.5.5.7.48.1.5"); }

      OID oid_of() const override { return static_oid(); }

   private:
      std::string oid_name() const override { return "PKIX.OCSP.NoCheck"; }

      bool should_encode() const override { return true; }

      std::vector<uint8_t> encode_inner() const override { return {}; }

      void decode_inner(const std::vector<uint8_t>&) override;
};

/**
* An unknown X.509 extension
* Will add a failure to the path validation result, if critical
*/
class BOTAN_PUBLIC_API(2, 4) Unknown_Extension final : public Certificate_Extension {
   public:
      Unknown_Extension(const OID& oid, bool critical) : m_oid(oid), m_critical(critical) {}

      std::unique_ptr<Certificate_Extension> copy() const override {
         return std::make_unique<Unknown_Extension>(m_oid, m_critical);
      }

      /**
      * Return the OID of this unknown extension
      */
      OID oid_of() const override { return m_oid; }

      //static_oid not defined for Unknown_Extension

      /**
      * Return the extension contents
      */
      const std::vector<uint8_t>& extension_contents() const { return m_bytes; }

      /**
      * Return if this extension was marked critical
      */
      bool is_critical_extension() const { return m_critical; }

      void validate(const X509_Certificate&,
                    const X509_Certificate&,
                    const std::vector<X509_Certificate>&,
                    std::vector<std::set<Certificate_Status_Code>>& cert_status,
                    size_t pos) override {
         if(m_critical) {
            cert_status.at(pos).insert(Certificate_Status_Code::UNKNOWN_CRITICAL_EXTENSION);
         }
      }

   private:
      std::string oid_name() const override { return ""; }

      bool should_encode() const override { return true; }

      std::vector<uint8_t> encode_inner() const override;
      void decode_inner(const std::vector<uint8_t>&) override;

      OID m_oid;
      bool m_critical;
      std::vector<uint8_t> m_bytes;
};

}  // namespace Cert_Extension

}  // namespace Botan

#endif
