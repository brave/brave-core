/*
* Discrete Logarithm Group
* (C) 1999-2008,2018 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_DL_PARAM_H_
#define BOTAN_DL_PARAM_H_

#include <botan/bigint.h>
#include <string_view>

namespace Botan {

class Montgomery_Params;
class DL_Group_Data;

enum class DL_Group_Source {
   Builtin,
   RandomlyGenerated,
   ExternalSource,
};

/**
* The DL group encoding format variants.
*/
enum class DL_Group_Format {
   ANSI_X9_42,
   ANSI_X9_57,
   PKCS_3,

   DSA_PARAMETERS = ANSI_X9_57,
   DH_PARAMETERS = ANSI_X9_42,
   ANSI_X9_42_DH_PARAMETERS = ANSI_X9_42,
   PKCS3_DH_PARAMETERS = PKCS_3
};

/**
* This class represents discrete logarithm groups. It holds a prime
* modulus p, a generator g, and (optionally) a prime q which is a
* factor of (p-1). In most cases g generates the order-q subgroup.
*/
class BOTAN_PUBLIC_API(2, 0) DL_Group final {
   public:
      /**
      * Determine the prime creation for DL groups.
      */
      enum PrimeType { Strong, Prime_Subgroup, DSA_Kosherizer };

      using Format = DL_Group_Format;

      /**
      * Construct a DL group with uninitialized internal value.
      * Use this constructor is you wish to set the groups values
      * from a DER or PEM encoded group.
      */
      DL_Group() = default;

      /**
      * Construct a DL group that is registered in the configuration.
      * @param name the name of the group, for example "modp/ietf/3072"
      *
      * @warning This constructor also accepts PEM inputs. This behavior is
      * deprecated and will be removed in a future major release. Instead
      * use DL_Group_from_PEM function
      */
      explicit DL_Group(std::string_view name);

      /*
      * Read a PEM representation
      */
      static DL_Group DL_Group_from_PEM(std::string_view pem);

      /**
      * Create a new group randomly.
      * @param rng the random number generator to use
      * @param type specifies how the creation of primes p and q shall
      * be performed. If type=Strong, then p will be determined as a
      * safe prime, and q will be chosen as (p-1)/2. If
      * type=Prime_Subgroup and qbits = 0, then the size of q will be
      * determined according to the estimated difficulty of the DL
      * problem. If type=DSA_Kosherizer, DSA primes will be created.
      * @param pbits the number of bits of p
      * @param qbits the number of bits of q. Leave it as 0 to have
      * the value determined according to pbits.
      */
      DL_Group(RandomNumberGenerator& rng, PrimeType type, size_t pbits, size_t qbits = 0);

      /**
      * Create a DSA group with a given seed.
      * @param rng the random number generator to use
      * @param seed the seed to use to create the random primes
      * @param pbits the desired bit size of the prime p
      * @param qbits the desired bit size of the prime q.
      */
      DL_Group(RandomNumberGenerator& rng, const std::vector<uint8_t>& seed, size_t pbits = 1024, size_t qbits = 0);

      /**
      * Create a DL group.
      * @param p the prime p
      * @param g the base g
      */
      DL_Group(const BigInt& p, const BigInt& g);

      /**
      * Create a DL group.
      * @param p the prime p
      * @param q the prime q
      * @param g the base g
      */
      DL_Group(const BigInt& p, const BigInt& q, const BigInt& g);

      /**
      * Decode a BER-encoded DL group param
      */
      DL_Group(const uint8_t ber[], size_t ber_len, DL_Group_Format format);

      /**
      * Decode a BER-encoded DL group param
      */
      template <typename Alloc>
      DL_Group(const std::vector<uint8_t, Alloc>& ber, DL_Group_Format format) :
            DL_Group(ber.data(), ber.size(), format) {}

      /**
      * Get the prime p.
      * @return prime p
      */
      const BigInt& get_p() const;

      /**
      * Get the prime q, returns zero if q is not used
      * @return prime q
      */
      const BigInt& get_q() const;

      /**
      * Get the base g.
      * @return base g
      */
      const BigInt& get_g() const;

      /**
      * Perform validity checks on the group.
      * @param rng the rng to use
      * @param strong whether to perform stronger by lengthier tests
      * @return true if the object is consistent, false otherwise
      */
      bool verify_group(RandomNumberGenerator& rng, bool strong = true) const;

      /**
      * Verify a public element, ie check if y = g^x for some x.
      *
      * This is not a perfect test. It verifies that 1 < y < p and (if q is set)
      * that y is in the subgroup of size q.
      */
      bool verify_public_element(const BigInt& y) const;

      /**
      * Verify a private element
      *
      * Specifically this checks that x is > 1 and < p, and additionally if
      * q is set then x must be < q
      */
      bool verify_private_element(const BigInt& x) const;

      /**
      * Verify a pair of elements y = g^x
      *
      * This verifies that 1 < x,y < p and that y=g^x mod p
      */
      bool verify_element_pair(const BigInt& y, const BigInt& x) const;

      /**
      * Encode this group into a string using PEM encoding.
      * @param format the encoding format
      * @return string holding the PEM encoded group
      */
      std::string PEM_encode(DL_Group_Format format) const;

      /**
      * Encode this group into a string using DER encoding.
      * @param format the encoding format
      * @return string holding the DER encoded group
      */
      std::vector<uint8_t> DER_encode(DL_Group_Format format) const;

      /**
      * Reduce an integer modulo p
      * @return x % p
      */
      BigInt mod_p(const BigInt& x) const;

      /**
      * Multiply and reduce an integer modulo p
      * @return (x*y) % p
      */
      BigInt multiply_mod_p(const BigInt& x, const BigInt& y) const;

      /**
      * Return the inverse of x mod p
      */
      BigInt inverse_mod_p(const BigInt& x) const;

      /**
      * Reduce an integer modulo q
      * Throws if q is unset on this DL_Group
      * @return x % q
      */
      BigInt mod_q(const BigInt& x) const;

      /**
      * Multiply and reduce an integer modulo q
      * Throws if q is unset on this DL_Group
      * @return (x*y) % q
      */
      BigInt multiply_mod_q(const BigInt& x, const BigInt& y) const;

      /**
      * Multiply and reduce an integer modulo q
      * Throws if q is unset on this DL_Group
      * @return (x*y*z) % q
      */
      BigInt multiply_mod_q(const BigInt& x, const BigInt& y, const BigInt& z) const;

      /**
      * Square and reduce an integer modulo q
      * Throws if q is unset on this DL_Group
      * @return (x*x) % q
      */
      BigInt square_mod_q(const BigInt& x) const;

      /**
      * Return the inverse of x mod q
      * Throws if q is unset on this DL_Group
      */
      BigInt inverse_mod_q(const BigInt& x) const;

      /**
      * Modular exponentiation
      *
      * @warning this function leaks the size of x via the number of
      * loop iterations. Use the version taking the maximum size to
      * avoid this.
      *
      * @return (g^x) % p
      */
      BigInt power_g_p(const BigInt& x) const;

      /**
      * Modular exponentiation
      * @param x the exponent
      * @param max_x_bits x is assumed to be at most this many bits long.
      *
      * @return (g^x) % p
      */
      BigInt power_g_p(const BigInt& x, size_t max_x_bits) const;

      /**
      * Modular exponentiation
      * @param b the base
      * @param x the exponent
      * @param max_x_bits x is assumed to be at most this many bits long.
      *
      * @return (b^x) % p
      */
      BigInt power_b_p(const BigInt& b, const BigInt& x, size_t max_x_bits) const;

      /**
      * Modular exponentiation
      * @param b the base
      * @param x the exponent
      *
      * @return (b^x) % p
      */
      BigInt power_b_p(const BigInt& b, const BigInt& x) const;

      /**
      * Multi-exponentiate
      * Return (g^x * y^z) % p
      */
      BigInt multi_exponentiate(const BigInt& x, const BigInt& y, const BigInt& z) const;

      /**
      * Return parameters for Montgomery reduction/exponentiation mod p
      */
      std::shared_ptr<const Montgomery_Params> monty_params_p() const;

      /**
      * Return the size of p in bits
      * Same as get_p().bits()
      */
      size_t p_bits() const;

      /**
      * Return the size of p in bytes
      * Same as get_p().bytes()
      */
      size_t p_bytes() const;

      /**
      * Return the size of q in bits
      * Same as get_q().bits()
      * Throws if q is unset
      */
      size_t q_bits() const;

      /**
      * Return the size of q in bytes
      * Same as get_q().bytes()
      * Throws if q is unset
      */
      size_t q_bytes() const;

      /**
      * Return if the q value is set
      */
      bool has_q() const;

      /**
      * Return size in bits of a secret exponent
      *
      * This attempts to balance between the attack costs of NFS
      * (which depends on the size of the modulus) and Pollard's rho
      * (which depends on the size of the exponent).
      *
      * It may vary over time for a particular group, if the attack
      * costs change.
      */
      size_t exponent_bits() const;

      /**
      * Return an estimate of the strength of this group against
      * discrete logarithm attacks (eg NFS). Warning: since this only
      * takes into account known attacks it is by necessity an
      * overestimate of the actual strength.
      */
      size_t estimated_strength() const;

      /**
      * Decode a DER/BER encoded group into this instance.
      * @param ber a vector containing the DER/BER encoded group
      * @param format the format of the encoded group
      *
      * @warning avoid this. Instead use the DL_Group constructor
      */
      void BER_decode(const std::vector<uint8_t>& ber, DL_Group_Format format);

      DL_Group_Source source() const;

      /*
      * For internal use only
      */
      static std::shared_ptr<DL_Group_Data> DL_group_info(std::string_view name);

   private:
      static std::shared_ptr<DL_Group_Data> load_DL_group_info(const char* p_str, const char* q_str, const char* g_str);

      static std::shared_ptr<DL_Group_Data> load_DL_group_info(const char* p_str, const char* g_str);

      static std::shared_ptr<DL_Group_Data> BER_decode_DL_group(const uint8_t data[],
                                                                size_t data_len,
                                                                DL_Group_Format format,
                                                                DL_Group_Source source);

      const DL_Group_Data& data() const;
      std::shared_ptr<DL_Group_Data> m_data;
};

}  // namespace Botan

#endif
