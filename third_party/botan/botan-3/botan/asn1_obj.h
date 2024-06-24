/*
* (C) 1999-2007,2018,2020 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_ASN1_OBJECT_TYPES_H_
#define BOTAN_ASN1_OBJECT_TYPES_H_

#include <botan/exceptn.h>
#include <botan/secmem.h>
#include <chrono>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Botan {

class BER_Decoder;
class DER_Encoder;

/**
* ASN.1 Class Tags
*/
enum class ASN1_Class : uint32_t {
   Universal = 0b0000'0000,
   Application = 0b0100'0000,
   ContextSpecific = 0b1000'0000,
   Private = 0b1100'0000,

   Constructed = 0b0010'0000,
   ExplicitContextSpecific = Constructed | ContextSpecific,

   NoObject = 0xFF00
};

/**
* ASN.1 Type Tags
*/
enum class ASN1_Type : uint32_t {
   Eoc = 0x00,
   Boolean = 0x01,
   Integer = 0x02,
   BitString = 0x03,
   OctetString = 0x04,
   Null = 0x05,
   ObjectId = 0x06,
   Enumerated = 0x0A,
   Sequence = 0x10,
   Set = 0x11,

   Utf8String = 0x0C,
   NumericString = 0x12,
   PrintableString = 0x13,
   TeletexString = 0x14,
   Ia5String = 0x16,
   VisibleString = 0x1A,
   UniversalString = 0x1C,
   BmpString = 0x1E,

   UtcTime = 0x17,
   GeneralizedTime = 0x18,

   NoObject = 0xFF00,
};

inline bool intersects(ASN1_Class x, ASN1_Class y) {
   return static_cast<uint32_t>(x) & static_cast<uint32_t>(y);
}

inline ASN1_Type operator|(ASN1_Type x, ASN1_Type y) {
   return static_cast<ASN1_Type>(static_cast<uint32_t>(x) | static_cast<uint32_t>(y));
}

inline ASN1_Class operator|(ASN1_Class x, ASN1_Class y) {
   return static_cast<ASN1_Class>(static_cast<uint32_t>(x) | static_cast<uint32_t>(y));
}

inline uint32_t operator|(ASN1_Type x, ASN1_Class y) {
   return static_cast<uint32_t>(x) | static_cast<uint32_t>(y);
}

inline uint32_t operator|(ASN1_Class x, ASN1_Type y) {
   return static_cast<uint32_t>(x) | static_cast<uint32_t>(y);
}

std::string BOTAN_UNSTABLE_API asn1_tag_to_string(ASN1_Type type);
std::string BOTAN_UNSTABLE_API asn1_class_to_string(ASN1_Class type);

/**
* Basic ASN.1 Object Interface
*/
class BOTAN_PUBLIC_API(2, 0) ASN1_Object {
   public:
      /**
      * Encode whatever this object is into to
      * @param to the DER_Encoder that will be written to
      */
      virtual void encode_into(DER_Encoder& to) const = 0;

      /**
      * Decode whatever this object is from from
      * @param from the BER_Decoder that will be read from
      */
      virtual void decode_from(BER_Decoder& from) = 0;

      /**
      * Return the encoding of this object. This is a convenience
      * method when just one object needs to be serialized. Use
      * DER_Encoder for complicated encodings.
      */
      std::vector<uint8_t> BER_encode() const;

      ASN1_Object() = default;
      ASN1_Object(const ASN1_Object&) = default;
      ASN1_Object& operator=(const ASN1_Object&) = default;
      virtual ~ASN1_Object() = default;
};

/**
* BER Encoded Object
*/
class BOTAN_PUBLIC_API(2, 0) BER_Object final {
   public:
      BER_Object() : m_type_tag(ASN1_Type::NoObject), m_class_tag(ASN1_Class::Universal) {}

      BER_Object(const BER_Object& other) = default;

      BER_Object& operator=(const BER_Object& other) = default;

      BER_Object(BER_Object&& other) = default;

      BER_Object& operator=(BER_Object&& other) = default;

      bool is_set() const { return m_type_tag != ASN1_Type::NoObject; }

      uint32_t tagging() const { return type_tag() | class_tag(); }

      ASN1_Type type_tag() const { return m_type_tag; }

      ASN1_Class class_tag() const { return m_class_tag; }

      ASN1_Type type() const { return m_type_tag; }

      ASN1_Class get_class() const { return m_class_tag; }

      const uint8_t* bits() const { return m_value.data(); }

      size_t length() const { return m_value.size(); }

      std::span<const uint8_t> data() const { return std::span{m_value}; }

      void assert_is_a(ASN1_Type type_tag, ASN1_Class class_tag, std::string_view descr = "object") const;

      bool is_a(ASN1_Type type_tag, ASN1_Class class_tag) const;

      bool is_a(int type_tag, ASN1_Class class_tag) const;

   private:
      ASN1_Type m_type_tag;
      ASN1_Class m_class_tag;
      secure_vector<uint8_t> m_value;

      friend class BER_Decoder;

      void set_tagging(ASN1_Type type_tag, ASN1_Class class_tag);

      uint8_t* mutable_bits(size_t length) {
         m_value.resize(length);
         return m_value.data();
      }
};

/*
* ASN.1 Utility Functions
*/
class DataSource;

namespace ASN1 {

std::vector<uint8_t> put_in_sequence(const std::vector<uint8_t>& val);
std::vector<uint8_t> put_in_sequence(const uint8_t bits[], size_t len);
std::string to_string(const BER_Object& obj);

/**
* Heuristics tests; is this object possibly BER?
* @param src a data source that will be peeked at but not modified
*/
bool maybe_BER(DataSource& src);

}  // namespace ASN1

/**
* General BER Decoding Error Exception
*/
class BOTAN_PUBLIC_API(2, 0) BER_Decoding_Error : public Decoding_Error {
   public:
      explicit BER_Decoding_Error(std::string_view);
};

/**
* Exception For Incorrect BER Taggings
*/
class BOTAN_PUBLIC_API(2, 0) BER_Bad_Tag final : public BER_Decoding_Error {
   public:
      BER_Bad_Tag(std::string_view msg, uint32_t tagging);
};

/**
* This class represents ASN.1 object identifiers.
*/
class BOTAN_PUBLIC_API(2, 0) OID final : public ASN1_Object {
   public:
      /**
      * Create an uninitialied OID object
      */
      explicit OID() = default;

      /**
      * Construct an OID from a string.
      * @param str a string in the form "a.b.c" etc., where a,b,c are integers
      *
      * Note: it is currently required that each integer fit into 32 bits
      */
      explicit OID(std::string_view str);

      /**
      * Initialize an OID from a sequence of integer values
      */
      explicit OID(std::initializer_list<uint32_t> init);

      /**
      * Initialize an OID from a vector of integer values
      */
      BOTAN_DEPRECATED("Use another contructor") explicit OID(std::vector<uint32_t>&& init);

      /**
      * Construct an OID from a string.
      * @param str a string in the form "a.b.c" etc., where a,b,c are numbers
      *        or any known OID name (for example "RSA" or "X509v3.SubjectKeyIdentifier")
      */
      static OID from_string(std::string_view str);

      /**
      * Construct an OID from a name
      * @param name any known OID name (for example "RSA" or "X509v3.SubjectKeyIdentifier")
      */
      static std::optional<OID> from_name(std::string_view name);

      /**
      * Register a new OID in the internal table
      */
      static void register_oid(const OID& oid, std::string_view name);

      void encode_into(DER_Encoder&) const override;
      void decode_from(BER_Decoder&) override;

      /**
      * Find out whether this OID is empty
      * @return true is no OID value is set
      */
      bool empty() const { return m_id.empty(); }

      /**
      * Find out whether this OID has a value
      * @return true is this OID has a value
      */
      bool has_value() const { return !empty(); }

      /**
      * Get this OID as a dotted-decimal string
      * @return string representing this OID
      */
      std::string to_string() const;

      /**
      * If there is a known name associated with this OID, return that.
      * Otherwise return the result of to_string
      */
      std::string to_formatted_string() const;

      /**
      * If there is a known name associated with this OID, return that.
      * Otherwise return the empty string.
      */
      std::string human_name_or_empty() const;

      /**
      * Return true if the OID in *this is registered in the internal
      * set of constants as a known OID.
      */
      bool registered_oid() const;

      /**
      * Compare two OIDs.
      * @return true if they are equal, false otherwise
      */
      bool operator==(const OID& other) const { return m_id == other.m_id; }

      /**
      * Get this OID as list (vector) of its components.
      * @return vector representing this OID
      */
      BOTAN_DEPRECATED("Do not access the integer values, use eg to_string")
      const std::vector<uint32_t>& get_components() const {
         return m_id;
      }

      BOTAN_DEPRECATED("Do not access the integer values, use eg to_string")
      const std::vector<uint32_t>& get_id() const {
         return m_id;
      }

   private:
      std::vector<uint32_t> m_id;
};

inline std::ostream& operator<<(std::ostream& out, const OID& oid) {
   out << oid.to_string();
   return out;
}

/**
* Compare two OIDs.
* @param a the first OID
* @param b the second OID
* @return true if a is not equal to b
*/
inline bool operator!=(const OID& a, const OID& b) {
   return !(a == b);
}

/**
* Compare two OIDs.
* @param a the first OID
* @param b the second OID
* @return true if a is lexicographically smaller than b
*/
bool BOTAN_PUBLIC_API(2, 0) operator<(const OID& a, const OID& b);

/**
* Time (GeneralizedTime/UniversalTime)
*/
class BOTAN_PUBLIC_API(2, 0) ASN1_Time final : public ASN1_Object {
   public:
      /// DER encode a ASN1_Time
      void encode_into(DER_Encoder&) const override;

      // Decode a BER encoded ASN1_Time
      void decode_from(BER_Decoder&) override;

      /// Return an internal string representation of the time
      std::string to_string() const;

      /// Returns a human friendly string replesentation of no particular formatting
      std::string readable_string() const;

      /// Return if the time has been set somehow
      bool time_is_set() const;

      ///  Compare this time against another
      int32_t cmp(const ASN1_Time& other) const;

      /// Create an invalid ASN1_Time
      ASN1_Time() = default;

      /// Create a ASN1_Time from a time point
      explicit ASN1_Time(const std::chrono::system_clock::time_point& time);

      /// Create an ASN1_Time from string
      ASN1_Time(std::string_view t_spec);

      /// Create an ASN1_Time from string and a specified tagging (Utc or Generalized)
      ASN1_Time(std::string_view t_spec, ASN1_Type tag);

      /// Returns a STL timepoint object
      std::chrono::system_clock::time_point to_std_timepoint() const;

      /// Return time since epoch
      uint64_t time_since_epoch() const;

   private:
      void set_to(std::string_view t_spec, ASN1_Type type);
      bool passes_sanity_check() const;

      uint32_t m_year = 0;
      uint32_t m_month = 0;
      uint32_t m_day = 0;
      uint32_t m_hour = 0;
      uint32_t m_minute = 0;
      uint32_t m_second = 0;
      ASN1_Type m_tag = ASN1_Type::NoObject;
};

/*
* Comparison Operations
*/
bool BOTAN_PUBLIC_API(2, 0) operator==(const ASN1_Time&, const ASN1_Time&);
bool BOTAN_PUBLIC_API(2, 0) operator!=(const ASN1_Time&, const ASN1_Time&);
bool BOTAN_PUBLIC_API(2, 0) operator<=(const ASN1_Time&, const ASN1_Time&);
bool BOTAN_PUBLIC_API(2, 0) operator>=(const ASN1_Time&, const ASN1_Time&);
bool BOTAN_PUBLIC_API(2, 0) operator<(const ASN1_Time&, const ASN1_Time&);
bool BOTAN_PUBLIC_API(2, 0) operator>(const ASN1_Time&, const ASN1_Time&);

typedef ASN1_Time X509_Time;

/**
* ASN.1 string type
* This class normalizes all inputs to a UTF-8 std::string
*/
class BOTAN_PUBLIC_API(2, 0) ASN1_String final : public ASN1_Object {
   public:
      void encode_into(DER_Encoder&) const override;
      void decode_from(BER_Decoder&) override;

      ASN1_Type tagging() const { return m_tag; }

      const std::string& value() const { return m_utf8_str; }

      size_t size() const { return value().size(); }

      bool empty() const { return m_utf8_str.empty(); }

      /**
      * Return true iff this is a tag for a known string type we can handle.
      */
      static bool is_string_type(ASN1_Type tag);

      bool operator==(const ASN1_String& other) const { return value() == other.value(); }

      friend bool operator<(const ASN1_String& a, const ASN1_String& b) { return a.value() < b.value(); }

      explicit ASN1_String(std::string_view utf8 = "");
      ASN1_String(std::string_view utf8, ASN1_Type tag);

   private:
      std::vector<uint8_t> m_data;
      std::string m_utf8_str;
      ASN1_Type m_tag;
};

/**
* Algorithm Identifier
*/
class BOTAN_PUBLIC_API(2, 0) AlgorithmIdentifier final : public ASN1_Object {
   public:
      enum Encoding_Option { USE_NULL_PARAM, USE_EMPTY_PARAM };

      void encode_into(DER_Encoder&) const override;
      void decode_from(BER_Decoder&) override;

      AlgorithmIdentifier() = default;

      AlgorithmIdentifier(const OID& oid, Encoding_Option enc);
      AlgorithmIdentifier(std::string_view oid_name, Encoding_Option enc);

      AlgorithmIdentifier(const OID& oid, const std::vector<uint8_t>& params);
      AlgorithmIdentifier(std::string_view oid_name, const std::vector<uint8_t>& params);

      const OID& oid() const { return m_oid; }

      const std::vector<uint8_t>& parameters() const { return m_parameters; }

      BOTAN_DEPRECATED("Use AlgorithmIdentifier::oid") const OID& get_oid() const { return m_oid; }

      BOTAN_DEPRECATED("Use AlgorithmIdentifier::parameters") const std::vector<uint8_t>& get_parameters() const {
         return m_parameters;
      }

      bool parameters_are_null() const;

      bool parameters_are_empty() const { return m_parameters.empty(); }

      bool parameters_are_null_or_empty() const { return parameters_are_empty() || parameters_are_null(); }

      bool empty() const { return m_oid.empty() && m_parameters.empty(); }

   private:
      OID m_oid;
      std::vector<uint8_t> m_parameters;
};

/*
* Comparison Operations
*/
bool BOTAN_PUBLIC_API(2, 0) operator==(const AlgorithmIdentifier&, const AlgorithmIdentifier&);
bool BOTAN_PUBLIC_API(2, 0) operator!=(const AlgorithmIdentifier&, const AlgorithmIdentifier&);

}  // namespace Botan

#endif
