/*
* (C) 1999-2010,2012,2018,2020 Jack Lloyd
* (C) 2007 Yves Jerschow
* (C) 2015 Kai Michaelis
* (C) 2016 René Korthaus, Rohde & Schwarz Cybersecurity
* (C) 2017 Fabian Weissberg, Rohde & Schwarz Cybersecurity
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_PKIX_TYPES_H_
#define BOTAN_PKIX_TYPES_H_

#include <botan/asn1_obj.h>
#include <botan/pkix_enums.h>
#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace Botan {

class X509_Certificate;
class Public_Key;

BOTAN_DEPRECATED("Use Key_Constraints::to_string")

inline std::string key_constraints_to_string(Key_Constraints c) {
   return c.to_string();
}

/**
* Distinguished Name
*/
class BOTAN_PUBLIC_API(2, 0) X509_DN final : public ASN1_Object {
   public:
      X509_DN() = default;

      explicit X509_DN(const std::multimap<OID, std::string>& args) {
         for(const auto& i : args) {
            add_attribute(i.first, i.second);
         }
      }

      explicit X509_DN(const std::multimap<std::string, std::string>& args) {
         for(const auto& i : args) {
            add_attribute(i.first, i.second);
         }
      }

      void encode_into(DER_Encoder&) const override;
      void decode_from(BER_Decoder&) override;

      bool has_field(const OID& oid) const;
      ASN1_String get_first_attribute(const OID& oid) const;

      /*
      * Return the BER encoded data, if any
      */
      const std::vector<uint8_t>& get_bits() const { return m_dn_bits; }

      std::vector<uint8_t> DER_encode() const;

      bool empty() const { return m_rdn.empty(); }

      std::string to_string() const;

      const std::vector<std::pair<OID, ASN1_String>>& dn_info() const { return m_rdn; }

      std::multimap<OID, std::string> get_attributes() const;
      std::multimap<std::string, std::string> contents() const;

      bool has_field(std::string_view attr) const;
      std::vector<std::string> get_attribute(std::string_view attr) const;
      std::string get_first_attribute(std::string_view attr) const;

      void add_attribute(std::string_view key, std::string_view val);

      void add_attribute(const OID& oid, std::string_view val) { add_attribute(oid, ASN1_String(val)); }

      void add_attribute(const OID& oid, const ASN1_String& val);

      static std::string deref_info_field(std::string_view key);

      /**
      * Lookup upper bounds in characters for the length of distinguished name fields
      * as given in RFC 5280, Appendix A.
      *
      * @param oid the oid of the DN to lookup
      * @return the upper bound, or zero if no ub is known to Botan
      */
      static size_t lookup_ub(const OID& oid);

   private:
      std::vector<std::pair<OID, ASN1_String>> m_rdn;
      std::vector<uint8_t> m_dn_bits;
};

bool BOTAN_PUBLIC_API(2, 0) operator==(const X509_DN& dn1, const X509_DN& dn2);
bool BOTAN_PUBLIC_API(2, 0) operator!=(const X509_DN& dn1, const X509_DN& dn2);

/*
The ordering here is arbitrary and may change from release to release.
It is intended for allowing DNs as keys in std::map and similiar containers
*/
bool BOTAN_PUBLIC_API(2, 0) operator<(const X509_DN& dn1, const X509_DN& dn2);

BOTAN_PUBLIC_API(2, 0) std::ostream& operator<<(std::ostream& out, const X509_DN& dn);
BOTAN_PUBLIC_API(2, 0) std::istream& operator>>(std::istream& in, X509_DN& dn);

/**
* Alternative Name
*/
class BOTAN_PUBLIC_API(2, 0) AlternativeName final : public ASN1_Object {
   public:
      void encode_into(DER_Encoder&) const override;
      void decode_from(BER_Decoder&) override;

      /// Create an empty name
      AlternativeName() {}

      /// Add a URI to this AlternativeName
      void add_uri(std::string_view uri);

      /// Add a URI to this AlternativeName
      void add_email(std::string_view addr);

      /// Add a DNS name to this AlternativeName
      void add_dns(std::string_view dns);

      /// Add an "OtherName" identified by object identifier to this AlternativeName
      void add_other_name(const OID& oid, const ASN1_String& value);

      /// Add a directory name to this AlternativeName
      void add_dn(const X509_DN& dn);

      /// Add an IP address to this alternative name
      void add_ipv4_address(uint32_t ipv4);

      /// Return the set of URIs included in this alternative name
      const std::set<std::string>& uris() const { return m_uri; }

      /// Return the set of email addresses included in this alternative name
      const std::set<std::string>& email() const { return m_email; }

      /// Return the set of DNS names included in this alternative name
      const std::set<std::string>& dns() const { return m_dns; }

      /// Return the set of IPv4 addresses included in this alternative name
      const std::set<uint32_t>& ipv4_address() const { return m_ipv4_addr; }

      /// Return the set of "other names" included in this alternative name
      const std::set<std::pair<OID, ASN1_String>>& other_names() const { return m_othernames; }

      /// Return the set of directory names included in this alternative name
      const std::set<X509_DN>& directory_names() const { return m_dn_names; }

      /// Return the total number of names in this AlternativeName
      ///
      /// This only counts names which were parsed, ignoring names which
      /// were of some unknown type
      size_t count() const;

      /// Return true if this has any names set
      bool has_items() const;

      // Old, now deprecated interface follows:
      BOTAN_DEPRECATED("Use AlternativeName::{uris, email, dns, othernames, directory_names}")
      std::multimap<std::string, std::string> contents() const;

      BOTAN_DEPRECATED("Use AlternativeName::{uris, email, dns, othernames, directory_names}.empty()")
      bool has_field(std::string_view attr) const;

      BOTAN_DEPRECATED("Use AlternativeName::{uris, email, dns, othernames, directory_names}")
      std::vector<std::string> get_attribute(std::string_view attr) const;

      BOTAN_DEPRECATED("Use AlternativeName::{uris, email, dns, othernames, directory_names}")
      std::multimap<std::string, std::string, std::less<>> get_attributes() const;

      BOTAN_DEPRECATED("Use AlternativeName::{uris, email, dns, othernames, directory_names}")
      std::string get_first_attribute(std::string_view attr) const;

      BOTAN_DEPRECATED("Use AlternativeName::add_{uri, dns, email, ...}")
      void add_attribute(std::string_view type, std::string_view value);

      BOTAN_DEPRECATED("Use AlternativeName::add_other_name")
      void add_othername(const OID& oid, std::string_view value, ASN1_Type type);

      BOTAN_DEPRECATED("Use AlternativeName::othernames") std::multimap<OID, ASN1_String> get_othernames() const;

      BOTAN_DEPRECATED("Use AlternativeName::directory_names") X509_DN dn() const;

      BOTAN_DEPRECATED("Use plain constructor plus add_{uri,dns,email,ipv4_address}")
      AlternativeName(std::string_view email_addr,
                      std::string_view uri = "",
                      std::string_view dns = "",
                      std::string_view ip_address = "");

   private:
      std::set<std::string> m_dns;
      std::set<std::string> m_uri;
      std::set<std::string> m_email;
      std::set<uint32_t> m_ipv4_addr;
      std::set<X509_DN> m_dn_names;
      std::set<std::pair<OID, ASN1_String>> m_othernames;
};

/**
* Attribute
*/
class BOTAN_PUBLIC_API(2, 0) Attribute final : public ASN1_Object {
   public:
      void encode_into(DER_Encoder& to) const override;
      void decode_from(BER_Decoder& from) override;

      Attribute() = default;
      Attribute(const OID& oid, const std::vector<uint8_t>& params);
      Attribute(std::string_view oid_str, const std::vector<uint8_t>& params);

      const OID& oid() const { return m_oid; }

      const std::vector<uint8_t>& parameters() const { return m_parameters; }

      const OID& object_identifier() const { return m_oid; }

      const std::vector<uint8_t>& get_parameters() const { return m_parameters; }

   private:
      OID m_oid;
      std::vector<uint8_t> m_parameters;
};

/**
* @brief X.509 GeneralName Type
*
* Handles parsing GeneralName types in their BER and canonical string
* encoding. Allows matching GeneralNames against each other using
* the rules laid out in the RFC 5280, sec. 4.2.1.10 (Name Contraints).
*/
class BOTAN_PUBLIC_API(2, 0) GeneralName final : public ASN1_Object {
   public:
      enum MatchResult : int {
         All,
         Some,
         None,
         NotFound,
         UnknownType,
      };

      GeneralName() = default;

      // Encoding is not implemented
      void encode_into(DER_Encoder&) const override;

      void decode_from(BER_Decoder&) override;

      /**
      * @return Type of the name. Can be DN, DNS, IP, RFC822 or URI.
      */
      std::string type() const;

      /**
      * @return The name as string. Format depends on type.
      */
      std::string name() const;

      /**
      * @return true if this name is a type we don't understand
      *
      * Note this returns true also for the case of URIs and email, which we can
      * parse but do not currently implement matching for.
      */
      bool is_unknown_type() const;

      /**
      * Checks whether a given certificate (partially) matches this name.
      * @param cert certificate to be matched
      * @return the match result
      */
      MatchResult matches(const X509_Certificate& cert) const;

   private:
      enum class NameType : uint8_t {
         Unknown = 0,
         RFC822 = 1,
         DNS = 2,
         URI = 3,
         DN = 4,
         IPv4 = 5,
      };

      static constexpr size_t RFC822_IDX = 0;
      static constexpr size_t DNS_IDX = 1;
      static constexpr size_t URI_IDX = 2;
      static constexpr size_t DN_IDX = 3;
      static constexpr size_t IPV4_IDX = 4;

      NameType m_type;
      std::variant<std::string, std::string, std::string, X509_DN, std::pair<uint32_t, uint32_t>> m_names;

      static bool matches_dns(const std::string& name, const std::string& constraint);

      static bool matches_dn(const X509_DN& name, const X509_DN& constraint);
};

std::ostream& operator<<(std::ostream& os, const GeneralName& gn);

/**
* @brief A single Name Constraint
*
* The Name Constraint extension adds a minimum and maximum path
* length to a GeneralName to form a constraint. The length limits
* are not used in PKIX.
*/
class BOTAN_PUBLIC_API(2, 0) GeneralSubtree final : public ASN1_Object {
   public:
      /**
      * Creates an empty name constraint.
      */
      GeneralSubtree() : m_base() {}

      void encode_into(DER_Encoder&) const override;

      void decode_from(BER_Decoder&) override;

      /**
      * @return name
      */
      const GeneralName& base() const { return m_base; }

   private:
      GeneralName m_base;
};

BOTAN_DEPRECATED("Deprecated no replacement") std::ostream& operator<<(std::ostream& os, const GeneralSubtree& gs);

/**
* @brief Name Constraints
*
* Wraps the Name Constraints associated with a certificate.
*/
class BOTAN_PUBLIC_API(2, 0) NameConstraints final {
   public:
      /**
      * Creates an empty name NameConstraints.
      */
      NameConstraints() : m_permitted_subtrees(), m_excluded_subtrees() {}

      /**
      * Creates NameConstraints from a list of permitted and excluded subtrees.
      * @param permitted_subtrees names for which the certificate is permitted
      * @param excluded_subtrees names for which the certificate is not permitted
      */
      NameConstraints(std::vector<GeneralSubtree>&& permitted_subtrees,
                      std::vector<GeneralSubtree>&& excluded_subtrees);

      /**
      * @return permitted names
      */
      const std::vector<GeneralSubtree>& permitted() const { return m_permitted_subtrees; }

      /**
      * @return excluded names
      */
      const std::vector<GeneralSubtree>& excluded() const { return m_excluded_subtrees; }

      // Return true if this certificate is known to be permitted
      bool is_permitted(const X509_Certificate& cert, bool reject_unknown) const;

      // Return true if this certificate is known to be excluded
      bool is_excluded(const X509_Certificate& cert, bool reject_unknown) const;

   private:
      std::vector<GeneralSubtree> m_permitted_subtrees;
      std::vector<GeneralSubtree> m_excluded_subtrees;
      bool m_permitted_contains_unknown;
      bool m_excluded_contains_unknown;
};

/**
* X.509 Certificate Extension
*/
class BOTAN_PUBLIC_API(2, 0) Certificate_Extension {
   public:
      /**
      * @return OID representing this extension
      */
      virtual OID oid_of() const = 0;

      /*
      * @return specific OID name
      * If possible OIDS table should match oid_name to OIDS, ie
      * OID::from_string(ext->oid_name()) == ext->oid_of()
      * Should return empty string if OID is not known
      */
      virtual std::string oid_name() const = 0;

      /**
      * Make a copy of this extension
      * @return copy of this
      */

      virtual std::unique_ptr<Certificate_Extension> copy() const = 0;

      /*
      * Callback visited during path validation.
      *
      * An extension can implement this callback to inspect
      * the path during path validation.
      *
      * If an error occurs during validation of this extension,
      * an appropriate status code shall be added to cert_status.
      *
      * @param subject Subject certificate that contains this extension
      * @param issuer Issuer certificate
      * @param status Certificate validation status codes for subject certificate
      * @param cert_path Certificate path which is currently validated
      * @param pos Position of subject certificate in cert_path
      */
      virtual void validate(const X509_Certificate& subject,
                            const X509_Certificate& issuer,
                            const std::vector<X509_Certificate>& cert_path,
                            std::vector<std::set<Certificate_Status_Code>>& cert_status,
                            size_t pos);

      virtual ~Certificate_Extension() = default;

   protected:
      friend class Extensions;

      virtual bool should_encode() const { return true; }

      virtual std::vector<uint8_t> encode_inner() const = 0;
      virtual void decode_inner(const std::vector<uint8_t>&) = 0;
};

/**
* X.509 Certificate Extension List
*/
class BOTAN_PUBLIC_API(2, 0) Extensions final : public ASN1_Object {
   public:
      /**
      * Look up an object in the extensions, based on OID Returns
      * nullptr if not set, if the extension was either absent or not
      * handled. The pointer returned is owned by the Extensions
      * object.
      * This would be better with an optional<T> return value
      */
      const Certificate_Extension* get_extension_object(const OID& oid) const;

      template <typename T>
      const T* get_extension_object_as(const OID& oid = T::static_oid()) const {
         if(const Certificate_Extension* extn = get_extension_object(oid)) {
            // Unknown_Extension oid_name is empty
            if(extn->oid_name().empty()) {
               return nullptr;
            } else if(const T* extn_as_T = dynamic_cast<const T*>(extn)) {
               return extn_as_T;
            } else {
               throw Decoding_Error("Exception::get_extension_object_as dynamic_cast failed");
            }
         }

         return nullptr;
      }

      /**
      * Return the set of extensions in the order they appeared in the certificate
      * (or as they were added, if constructed)
      */
      const std::vector<OID>& get_extension_oids() const { return m_extension_oids; }

      /**
      * Return true if an extension was set
      */
      bool extension_set(const OID& oid) const;

      /**
      * Return true if an extesion was set and marked critical
      */
      bool critical_extension_set(const OID& oid) const;

      /**
      * Return the raw bytes of the extension
      * Will throw if OID was not set as an extension.
      */
      std::vector<uint8_t> get_extension_bits(const OID& oid) const;

      void encode_into(DER_Encoder&) const override;
      void decode_from(BER_Decoder&) override;

      /**
      * Adds a new extension to the list.
      * @param extn pointer to the certificate extension (Extensions takes ownership)
      * @param critical whether this extension should be marked as critical
      * @throw Invalid_Argument if the extension is already present in the list
      */
      void add(std::unique_ptr<Certificate_Extension> extn, bool critical = false);

      /**
      * Adds a new extension to the list unless it already exists. If the extension
      * already exists within the Extensions object, the extn pointer will be deleted.
      *
      * @param extn pointer to the certificate extension (Extensions takes ownership)
      * @param critical whether this extension should be marked as critical
      * @return true if the object was added false if the extension was already used
      */
      bool add_new(std::unique_ptr<Certificate_Extension> extn, bool critical = false);

      /**
      * Adds an extension to the list or replaces it.
      * @param extn the certificate extension
      * @param critical whether this extension should be marked as critical
      */
      void replace(std::unique_ptr<Certificate_Extension> extn, bool critical = false);

      /**
      * Remove an extension from the list. Returns true if the
      * extension had been set, false otherwise.
      */
      bool remove(const OID& oid);

      /**
      * Searches for an extension by OID and returns the result.
      * Only the known extensions types declared in this header
      * are searched for by this function.
      * @return Copy of extension with oid, nullptr if not found.
      * Can avoid creating a copy by using get_extension_object function
      */
      std::unique_ptr<Certificate_Extension> get(const OID& oid) const;

      /**
      * Searches for an extension by OID and returns the result decoding
      * it to some arbitrary extension type chosen by the application.
      *
      * Only the unknown extensions, that is, extensions types that
      * are not declared in this header, are searched for by this
      * function.
      *
      * @return Pointer to new extension with oid, nullptr if not found.
      */
      template <typename T>
      std::unique_ptr<T> get_raw(const OID& oid) const {
         auto extn_info = m_extension_info.find(oid);

         if(extn_info != m_extension_info.end()) {
            // Unknown_Extension oid_name is empty
            if(extn_info->second.obj().oid_name().empty()) {
               auto ext = std::make_unique<T>();
               ext->decode_inner(extn_info->second.bits());
               return ext;
            }
         }
         return nullptr;
      }

      /**
      * Returns a copy of the list of extensions together with the corresponding
      * criticality flag. All extensions are encoded as some object, falling back
      * to Unknown_Extension class which simply allows reading the bytes as well
      * as the criticality flag.
      */
      std::vector<std::pair<std::unique_ptr<Certificate_Extension>, bool>> extensions() const;

      /**
      * Returns the list of extensions as raw, encoded bytes
      * together with the corresponding criticality flag.
      * Contains all extensions, including any extensions encoded as Unknown_Extension
      */
      std::map<OID, std::pair<std::vector<uint8_t>, bool>> extensions_raw() const;

      Extensions() = default;

      Extensions(const Extensions&) = default;
      Extensions& operator=(const Extensions&) = default;

      Extensions(Extensions&&) = default;
      Extensions& operator=(Extensions&&) = default;

   private:
      static std::unique_ptr<Certificate_Extension> create_extn_obj(const OID& oid,
                                                                    bool critical,
                                                                    const std::vector<uint8_t>& body);

      class Extensions_Info {
         public:
            Extensions_Info(bool critical, std::unique_ptr<Certificate_Extension> ext) :
                  m_obj(std::move(ext)), m_bits(m_obj->encode_inner()), m_critical(critical) {}

            Extensions_Info(bool critical,
                            const std::vector<uint8_t>& encoding,
                            std::unique_ptr<Certificate_Extension> ext) :
                  m_obj(std::move(ext)), m_bits(encoding), m_critical(critical) {}

            bool is_critical() const { return m_critical; }

            const std::vector<uint8_t>& bits() const { return m_bits; }

            const Certificate_Extension& obj() const {
               BOTAN_ASSERT_NONNULL(m_obj.get());
               return *m_obj;
            }

         private:
            std::shared_ptr<Certificate_Extension> m_obj;
            std::vector<uint8_t> m_bits;
            bool m_critical = false;
      };

      std::vector<OID> m_extension_oids;
      std::map<OID, Extensions_Info> m_extension_info;
};

}  // namespace Botan

#endif
