/*
* ECC Domain Parameters
*
* (C) 2007 Falko Strenzke, FlexSecure GmbH
*     2008-2010 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_ECC_DOMAIN_PARAMETERS_H_
#define BOTAN_ECC_DOMAIN_PARAMETERS_H_

#include <botan/asn1_obj.h>
#include <botan/ec_point.h>
#include <memory>
#include <set>
#include <span>

namespace Botan {

/**
* This class represents elliptic curce domain parameters
*/
enum class EC_Group_Encoding {
   Explicit,
   ImplicitCA,
   NamedCurve,

   EC_DOMPAR_ENC_EXPLICIT = Explicit,
   EC_DOMPAR_ENC_IMPLICITCA = ImplicitCA,
   EC_DOMPAR_ENC_OID = NamedCurve
};

enum class EC_Group_Source {
   Builtin,
   ExternalSource,
};

class EC_Group_Data;
class EC_Group_Data_Map;

/**
* Class representing an elliptic curve
*
* The internal representation is stored in a shared_ptr, so copying an
* EC_Group is inexpensive.
*/
class BOTAN_PUBLIC_API(2, 0) EC_Group final {
   public:
      /**
      * Construct elliptic curve from the specified parameters
      *
      * @param p the elliptic curve p
      * @param a the elliptic curve a param
      * @param b the elliptic curve b param
      * @param base_x the x coordinate of the base point
      * @param base_y the y coordinate of the base point
      * @param order the order of the base point
      * @param cofactor the cofactor
      * @param oid an optional OID used to identify this curve
      *
      * @warning support for cofactors > 1 is deprecated and will be removed
      *
      * @warning support for prime fields > 521 bits is deprecated and
      * will be removed.
      *
      * @warning Support for explicitly encoded curve parameters is deprecated.
      * An OID must be assigned.
      */
      BOTAN_DEPRECATED("Use alternate constructor")
      EC_Group(const BigInt& p,
               const BigInt& a,
               const BigInt& b,
               const BigInt& base_x,
               const BigInt& base_y,
               const BigInt& order,
               const BigInt& cofactor,
               const OID& oid = OID());

      /**
      * Construct elliptic curve from the specified parameters
      *
      * Unlike the deprecated constructor, this constructor requires:
      *  - That p is at most 521 bits
      *  - That an object identifier is provided
      *  - That the cofactor is 1 (this is implicit due to not having
      *    cofactor argument)
      *  - That the curve is at least plausibly valid (for example
      *    it checks that p is prime). It does not fully verify
      *    the group since certain things cannot be checked until
      *    the EC_Group object is constructed.
      *
      * @warning use only elliptic curve parameters that you trust
      *
      * @param oid an object identifier used to identify this curve
      * @param p the elliptic curve prime (at most 521 bits)
      * @param a the elliptic curve a param
      * @param b the elliptic curve b param
      * @param base_x the x coordinate of the group generator
      * @param base_y the y coordinate of the group generator
      * @param order the order of the group
      */
      EC_Group(const OID& oid,
               const BigInt& p,
               const BigInt& a,
               const BigInt& b,
               const BigInt& base_x,
               const BigInt& base_y,
               const BigInt& order);

      /**
      * Decode a BER encoded ECC domain parameter set
      * @param ber the bytes of the BER encoding
      */
      explicit EC_Group(std::span<const uint8_t> ber);

      BOTAN_DEPRECATED("Use EC_Group(std::span)")
      EC_Group(const uint8_t ber[], size_t ber_len) : EC_Group(std::span{ber, ber_len}) {}

      /**
      * Create an EC domain by OID (or throw if unknown)
      * @param oid the OID of the EC domain to create
      */
      BOTAN_DEPRECATED("Use EC_Group::from_OID") explicit EC_Group(const OID& oid) { *this = EC_Group::from_OID(oid); }

      /**
      * Create an EC domain from PEM encoding (as from PEM_encode), or
      * from an OID name (eg "secp256r1", or "1.2.840.10045.3.1.7")
      * @param pem_or_oid PEM-encoded data, or an OID
      *
      * @warning Support for PEM in this function is deprecated. Use
      * EC_Group::from_PEM or EC_Group::from_OID or EC_Group::from_name
      */
      BOTAN_DEPRECATED("Use EC_Group::from_{name,OID,PEM}") explicit EC_Group(std::string_view pem_or_oid);

      /**
      * Initialize an EC group from the PEM/ASN.1 encoding
      */
      static EC_Group from_PEM(std::string_view pem);

      /**
      * Initialize an EC group from a group named by an object identifier
      */
      static EC_Group from_OID(const OID& oid);

      /**
      * Initialize an EC group from a group common name (eg "secp256r1")
      */
      static EC_Group from_name(std::string_view name);

      BOTAN_DEPRECATED("Use EC_Group::from_PEM") static EC_Group EC_Group_from_PEM(std::string_view pem) {
         return EC_Group::from_PEM(pem);
      }

      /**
      * Create an uninitialized EC_Group
      */
      EC_Group();

      ~EC_Group();

      EC_Group(const EC_Group&);
      EC_Group(EC_Group&&) = default;

      EC_Group& operator=(const EC_Group&);
      EC_Group& operator=(EC_Group&&) = default;

      /**
      * Create the DER encoding of this domain
      * @param form of encoding to use
      * @returns bytes encododed as DER
      */
      std::vector<uint8_t> DER_encode(EC_Group_Encoding form) const;

      /**
      * Return the PEM encoding (always in explicit form)
      * @return string containing PEM data
      */
      std::string PEM_encode() const;

      /**
      * Return the size of p in bits (same as get_p().bits())
      */
      size_t get_p_bits() const;

      /**
      * Return the size of p in bits (same as get_p().bytes())
      */
      size_t get_p_bytes() const;

      /**
      * Return the size of group order in bits (same as get_order().bits())
      */
      size_t get_order_bits() const;

      /**
      * Return the size of the group order in bytes (same as get_order().bytes())
      */
      size_t get_order_bytes() const;

      /**
      * Check if y is a plausible point on the curve
      *
      * In particular, checks that it is a point on the curve, not infinity,
      * and that it has order matching the group.
      */
      bool verify_public_element(const EC_Point& y) const;

      /**
      * Return the OID of these domain parameters
      * @result the OID
      */
      const OID& get_curve_oid() const;

      /**
      * Return the prime modulus of the field
      */
      const BigInt& get_p() const;

      /**
      * Return the a parameter of the elliptic curve equation
      */
      const BigInt& get_a() const;

      /**
      * Return the b parameter of the elliptic curve equation
      */
      const BigInt& get_b() const;

      /**
      * Return group base point
      * @result base point
      */
      const EC_Point& get_base_point() const;

      /**
      * Return the x coordinate of the base point
      */
      const BigInt& get_g_x() const;

      /**
      * Return the y coordinate of the base point
      */
      const BigInt& get_g_y() const;

      /**
      * Return the order of the base point
      * @result order of the base point
      */
      const BigInt& get_order() const;

      /**
      * Return the cofactor
      * @result the cofactor
      */
      const BigInt& get_cofactor() const;

      /**
      * Multi exponentiate. Not constant time.
      * @return base_point*x + pt*y
      */
      EC_Point point_multiply(const BigInt& x, const EC_Point& pt, const BigInt& y) const;

      /**
      * Blinded point multiplication, attempts resistance to side channels
      * @param k the scalar
      * @param rng a random number generator
      * @param ws a temp workspace
      * @return base_point*k
      */
      EC_Point blinded_base_point_multiply(const BigInt& k, RandomNumberGenerator& rng, std::vector<BigInt>& ws) const;

      /**
      * Blinded point multiplication, attempts resistance to side channels
      * Returns just the x coordinate of the point
      *
      * @param k the scalar
      * @param rng a random number generator
      * @param ws a temp workspace
      * @return x coordinate of base_point*k
      */
      BigInt blinded_base_point_multiply_x(const BigInt& k, RandomNumberGenerator& rng, std::vector<BigInt>& ws) const;

      /**
      * Blinded point multiplication, attempts resistance to side channels
      * @param point input point
      * @param k the scalar
      * @param rng a random number generator
      * @param ws a temp workspace
      * @return point*k
      */
      EC_Point blinded_var_point_multiply(const EC_Point& point,
                                          const BigInt& k,
                                          RandomNumberGenerator& rng,
                                          std::vector<BigInt>& ws) const;

      /**
      * Return a random scalar ie an integer in [1,order)
      */
      BigInt random_scalar(RandomNumberGenerator& rng) const;

      /**
      * Hash onto the curve.
      * For some curve types no mapping is currently available, in this
      * case this function will throw an exception.
      *
      * @param hash_fn the hash function to use (typically "SHA-256" or "SHA-512")
      * @param input the input to hash
      * @param input_len length of input in bytes
      * @param domain_sep a domain seperator
      * @param domain_sep_len length of domain_sep in bytes
      * @param random_oracle if the mapped point must be uniform (use
               "true" here unless you know what you are doing)
      */
      EC_Point hash_to_curve(std::string_view hash_fn,
                             const uint8_t input[],
                             size_t input_len,
                             const uint8_t domain_sep[],
                             size_t domain_sep_len,
                             bool random_oracle = true) const;

      /**
      * Hash onto the curve.
      * For some curve types no mapping is currently available, in this
      * case this function will throw an exception.
      *
      * @param hash_fn the hash function to use (typically "SHA-256" or "SHA-512")
      * @param input the input to hash
      * @param input_len length of input in bytes
      * @param domain_sep a domain seperator
      * @param random_oracle if the mapped point must be uniform (use
               "true" here unless you know what you are doing)
      */
      EC_Point hash_to_curve(std::string_view hash_fn,
                             const uint8_t input[],
                             size_t input_len,
                             std::string_view domain_sep,
                             bool random_oracle = true) const;

      /**
      * OS2ECP (Octet String To Elliptic Curve Point)
      *
      * Deserialize an encoded point. Verifies that the point is on the curve.
      */
      EC_Point OS2ECP(const uint8_t bits[], size_t len) const;

      EC_Point OS2ECP(std::span<const uint8_t> encoded_point) const {
         return this->OS2ECP(encoded_point.data(), encoded_point.size());
      }

      bool initialized() const { return (m_data != nullptr); }

      /**
       * Verify EC_Group domain
       * @returns true if group is valid. false otherwise
       */
      bool verify_group(RandomNumberGenerator& rng, bool strong = false) const;

      bool operator==(const EC_Group& other) const;

      EC_Group_Source source() const;

      /**
      * Return true if this EC_Group was derived from an explicit encoding
      *
      * Explicit encoding of groups is deprecated; when support for explicit curves
      * is removed in a future major release, this function will also be removed.
      */
      bool used_explicit_encoding() const { return m_explicit_encoding; }

      /**
      * Return a set of known named EC groups
      */
      static const std::set<std::string>& known_named_groups();

      // Everything below here will be removed in a future release:

      /**
      * Return if a == -3 mod p
      */
      BOTAN_DEPRECATED("Deprecated no replacement") bool a_is_minus_3() const;

      /**
      * Return if a == 0 mod p
      */
      BOTAN_DEPRECATED("Deprecated no replacement") bool a_is_zero() const;

      /*
      * Reduce x modulo the order
      */
      BOTAN_DEPRECATED("Deprecated no replacement") BigInt mod_order(const BigInt& x) const;

      /*
      * Return inverse of x modulo the order
      */
      BOTAN_DEPRECATED("Deprecated no replacement") BigInt inverse_mod_order(const BigInt& x) const;

      /*
      * Reduce (x*x) modulo the order
      */
      BOTAN_DEPRECATED("Deprecated no replacement") BigInt square_mod_order(const BigInt& x) const;

      /*
      * Reduce (x*y) modulo the order
      */
      BOTAN_DEPRECATED("Deprecated no replacement") BigInt multiply_mod_order(const BigInt& x, const BigInt& y) const;

      /*
      * Reduce (x*y*z) modulo the order
      */
      BOTAN_DEPRECATED("Deprecated no replacement")
      BigInt multiply_mod_order(const BigInt& x, const BigInt& y, const BigInt& z) const;

      /*
      * Return x^3 modulo the order
      */
      BOTAN_DEPRECATED("Deprecated no replacement") BigInt cube_mod_order(const BigInt& x) const;

      /**
      * Return a point on this curve with the affine values x, y
      */
      BOTAN_DEPRECATED("Deprecated - use OS2ECP") EC_Point point(const BigInt& x, const BigInt& y) const;

      /**
      * Return the zero (or infinite) point on this curve
      */
      BOTAN_DEPRECATED("Deprecated no replacement") EC_Point zero_point() const;

      BOTAN_DEPRECATED("Just serialize the point and check") size_t point_size(EC_Point_Format format) const;

      /*
      * For internal use only
      */
      static std::shared_ptr<EC_Group_Data> EC_group_info(const OID& oid);

      /*
      * For internal use only
      */
      static size_t clear_registered_curve_data();

      /*
      * For internal use only
      */
      static OID EC_group_identity_from_order(const BigInt& order);

   private:
      static EC_Group_Data_Map& ec_group_data();

      EC_Group(std::shared_ptr<EC_Group_Data>&& data);

      static std::pair<std::shared_ptr<EC_Group_Data>, bool> BER_decode_EC_group(std::span<const uint8_t> ber,
                                                                                 EC_Group_Source source);

      static std::shared_ptr<EC_Group_Data> load_EC_group_info(const char* p,
                                                               const char* a,
                                                               const char* b,
                                                               const char* g_x,
                                                               const char* g_y,
                                                               const char* order,
                                                               const OID& oid);

      // Member data
      const EC_Group_Data& data() const;
      std::shared_ptr<EC_Group_Data> m_data;
      bool m_explicit_encoding = false;
};

inline bool operator!=(const EC_Group& lhs, const EC_Group& rhs) {
   return !(lhs == rhs);
}

}  // namespace Botan

#endif
