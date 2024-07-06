/*
* PK Key Types
* (C) 1999-2007,2018 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_PK_KEYS_H_
#define BOTAN_PK_KEYS_H_

#include <botan/asn1_obj.h>
#include <botan/pk_ops_fwd.h>
#include <botan/secmem.h>

#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace Botan {

class BigInt;
class RandomNumberGenerator;

/**
* Enumeration specifying the signature format.
*
* This is mostly used for requesting DER encoding of ECDSA signatures;
* most other algorithms only support "standard".
*/
enum class Signature_Format {
   Standard,
   DerSequence,

   IEEE_1363 BOTAN_DEPRECATED("Use Standard") = Standard,
   DER_SEQUENCE BOTAN_DEPRECATED("Use DerSequence") = DerSequence,
};

/**
* Enumeration of possible operations a public key could be used for.
*
* It is possible to query if a key supports a particular operation
* type using Asymmetric_Key::supports_operation()
*/
enum class PublicKeyOperation {
   Encryption,
   Signature,
   KeyEncapsulation,
   KeyAgreement,
};

class Private_Key;

/**
* An interface for objects that are keys in public key algorithms
*
* This is derived for both public and private keys
*/
class BOTAN_PUBLIC_API(3, 0) Asymmetric_Key {
   public:
      virtual ~Asymmetric_Key() = default;

      /**
      * Get the name of the underlying public key scheme.
      * @return name of the public key scheme
      */
      virtual std::string algo_name() const = 0;

      /**
      * Return the estimated strength of the underlying key against
      * the best currently known attack. Note that this ignores anything
      * but pure attacks against the key itself and do not take into
      * account padding schemes, usage mistakes, etc which might reduce
      * the strength. However it does suffice to provide an upper bound.
      *
      * @return estimated strength in bits
      */
      virtual size_t estimated_strength() const = 0;

      /**
      * Get the OID of the underlying public key scheme.
      * @return OID of the public key scheme
      */
      virtual OID object_identifier() const;

      /**
      * Access an algorithm specific field
      *
      * If the field is not known for this algorithm, an Invalid_Argument is
      * thrown. The interpretation of the result requires knowledge of which
      * algorithm is involved. For instance for RSA "p" represents one of the
      * secret primes, while for DSA "p" is the public prime.
      *
      * Some algorithms may not implement this method at all.
      *
      * This is primarily used to implement the FFI botan_pubkey_get_field
      * and botan_privkey_get_field functions.
      */
      virtual const BigInt& get_int_field(std::string_view field) const;

      /**
      * Return true if this key could be used for the specified type
      * of operation.
      */
      virtual bool supports_operation(PublicKeyOperation op) const = 0;

      /**
       * Generate another (cryptographically independent) key pair using the
       * same algorithm parameters as this key. This is most useful for algorithms
       * that support PublicKeyOperation::KeyAgreement to generate a fitting ephemeral
       * key pair. For other key types it might throw Not_Implemented.
       */
      virtual std::unique_ptr<Private_Key> generate_another(RandomNumberGenerator& rng) const = 0;
};

/*
* Public Key Base Class.
*/
class BOTAN_PUBLIC_API(2, 0) Public_Key : public virtual Asymmetric_Key {
   public:
      /**
      * Return an integer value best approximating the length of the
      * primary security parameter. For example for RSA this will be
      * the size of the modulus, for ECDSA the size of the ECC group,
      * and for McEliece the size of the code will be returned.
      */
      virtual size_t key_length() const = 0;

      /**
      * Deprecated version of object_identifier
      */
      BOTAN_DEPRECATED("Use object_identifier") OID get_oid() const { return this->object_identifier(); }

      /*
      * Test the key values for consistency.
      * @param rng rng to use
      * @param strong whether to perform strong and lengthy version
      * of the test
      * @return true if the test is passed
      */
      virtual bool check_key(RandomNumberGenerator& rng, bool strong) const = 0;

      /**
      * @return X.509 AlgorithmIdentifier for this key
      */
      virtual AlgorithmIdentifier algorithm_identifier() const = 0;

      /**
      * @return binary public key bits, with no additional encoding
      *
      * For key agreements this is an alias for PK_Key_Agreement_Key::public_value.
      *
      * Note: some algorithms (for example RSA) do not have an obvious encoding
      * for this value due to having many different values, and thus throw
      * Not_Implemented when invoking this method.
      */
      virtual std::vector<uint8_t> raw_public_key_bits() const = 0;

      /**
      * @return BER encoded public key bits
      */
      virtual std::vector<uint8_t> public_key_bits() const = 0;

      /**
      * @return X.509 subject key encoding for this key object
      */
      std::vector<uint8_t> subject_public_key() const;

      /**
       * @return Hash of the subject public key
       */
      std::string fingerprint_public(std::string_view alg = "SHA-256") const;

      // Internal or non-public declarations follow

      /**
      * Returns more than 1 if the output of this algorithm
      * (ciphertext, signature) should be treated as more than one
      * value. This is used for algorithms like DSA and ECDSA, where
      * the (r,s) output pair can be encoded as either a plain binary
      * list or a TLV tagged DER encoding depending on the protocol.
      *
      * This function is public but applications should have few
      * reasons to ever call this.
      *
      * @return number of message parts
      */
      virtual size_t message_parts() const { return 1; }

      /**
      * Returns how large each of the message parts refered to
      * by message_parts() is
      *
      * This function is public but applications should have few
      * reasons to ever call this.
      *
      * @return size of the message parts in bits
      */
      virtual size_t message_part_size() const { return 0; }

      virtual Signature_Format default_x509_signature_format() const {
         return (this->message_parts() >= 2) ? Signature_Format::DerSequence : Signature_Format::Standard;
      }

      /**
      * This is an internal library function exposed on key types.
      * In almost all cases applications should use wrappers in pubkey.h
      *
      * Return an encryption operation for this key/params or throw
      *
      * @param rng a random number generator. The PK_Op may maintain a
      * reference to the RNG and use it many times. The rng must outlive
      * any operations which reference it.
      * @param params additional parameters
      * @param provider the provider to use
      */
      virtual std::unique_ptr<PK_Ops::Encryption> create_encryption_op(RandomNumberGenerator& rng,
                                                                       std::string_view params,
                                                                       std::string_view provider) const;

      /**
      * This is an internal library function exposed on key types.
      * In almost all cases applications should use wrappers in pubkey.h
      *
      * Return a KEM encryption operation for this key/params or throw
      *
      * @param params additional parameters
      * @param provider the provider to use
      */
      virtual std::unique_ptr<PK_Ops::KEM_Encryption> create_kem_encryption_op(std::string_view params,
                                                                               std::string_view provider) const;

      /**
      * This is an internal library function exposed on key types.
      * In all cases applications should use wrappers in pubkey.h
      *
      * Return a verification operation for this key/params or throw
      * @param params additional parameters
      * @param provider the provider to use
      */
      virtual std::unique_ptr<PK_Ops::Verification> create_verification_op(std::string_view params,
                                                                           std::string_view provider) const;

      /**
      * This is an internal library function exposed on key types.
      * In all cases applications should use wrappers in pubkey.h
      *
      * Return a verification operation for this combination of key and
      * signature algorithm or throw.
      *
      * @param signature_algorithm is the X.509 algorithm identifier encoding the padding
      * scheme and hash hash function used in the signature if applicable.
      *
      * @param provider the provider to use
      */
      virtual std::unique_ptr<PK_Ops::Verification> create_x509_verification_op(
         const AlgorithmIdentifier& signature_algorithm, std::string_view provider) const;
};

/**
* Private Key Base Class
*/
class BOTAN_PUBLIC_API(2, 0) Private_Key : public virtual Public_Key {
   public:
      /**
      * @return BER encoded private key bits
      */
      virtual secure_vector<uint8_t> private_key_bits() const = 0;

      /**
      * @return binary private key bits, with no additional encoding
      *
      * Note: some algorithms (for example RSA) do not have an obvious encoding
      * for this value due to having many different values, and thus not implement
      * this function. The default implementation throws Not_Implemented
      */
      virtual secure_vector<uint8_t> raw_private_key_bits() const;

      /**
      * Allocate a new object for the public key associated with this
      * private key.
      *
      * @return public key
      */
      virtual std::unique_ptr<Public_Key> public_key() const = 0;

      /**
      * @return PKCS #8 private key encoding for this key object
      */
      secure_vector<uint8_t> private_key_info() const;

      /**
      * @return PKCS #8 AlgorithmIdentifier for this key
      * Might be different from the X.509 identifier, but normally is not
      */
      virtual AlgorithmIdentifier pkcs8_algorithm_identifier() const { return algorithm_identifier(); }

      /**
      * Indicates if this key is stateful, ie that performing a private
      * key operation requires updating the key storage.
      */
      virtual bool stateful_operation() const { return false; }

      /**
       * @brief Retrieves the number of remaining operations if this is a stateful private key.
       *
       * @returns the number of remaining operations or std::nullopt if not applicable.
       */
      virtual std::optional<uint64_t> remaining_operations() const { return std::nullopt; }

      // Internal or non-public declarations follow

      /**
       * @return Hash of the PKCS #8 encoding for this key object
       */
      std::string fingerprint_private(std::string_view alg) const;

      /**
      * This is an internal library function exposed on key types.
      * In all cases applications should use wrappers in pubkey.h
      *
      * Return an decryption operation for this key/params or throw
      *
      * @param rng a random number generator. The PK_Op may maintain a
      * reference to the RNG and use it many times. The rng must outlive
      * any operations which reference it.
      * @param params additional parameters
      * @param provider the provider to use
      *
      */
      virtual std::unique_ptr<PK_Ops::Decryption> create_decryption_op(RandomNumberGenerator& rng,
                                                                       std::string_view params,
                                                                       std::string_view provider) const;

      /**
      * This is an internal library function exposed on key types.
      * In all cases applications should use wrappers in pubkey.h
      *
      * Return a KEM decryption operation for this key/params or throw
      *
      * @param rng a random number generator. The PK_Op may maintain a
      * reference to the RNG and use it many times. The rng must outlive
      * any operations which reference it.
      * @param params additional parameters
      * @param provider the provider to use
      */
      virtual std::unique_ptr<PK_Ops::KEM_Decryption> create_kem_decryption_op(RandomNumberGenerator& rng,
                                                                               std::string_view params,
                                                                               std::string_view provider) const;

      /**
      * This is an internal library function exposed on key types.
      * In all cases applications should use wrappers in pubkey.h
      *
      * Return a signature operation for this key/params or throw
      *
      * @param rng a random number generator. The PK_Op may maintain a
      * reference to the RNG and use it many times. The rng must outlive
      * any operations which reference it.
      * @param params additional parameters
      * @param provider the provider to use
      */
      virtual std::unique_ptr<PK_Ops::Signature> create_signature_op(RandomNumberGenerator& rng,
                                                                     std::string_view params,
                                                                     std::string_view provider) const;

      /**
      * This is an internal library function exposed on key types.
      * In all cases applications should use wrappers in pubkey.h
      *
      * Return a key agreement operation for this key/params or throw
      *
      * @param rng a random number generator. The PK_Op may maintain a
      * reference to the RNG and use it many times. The rng must outlive
      * any operations which reference it.
      * @param params additional parameters
      * @param provider the provider to use
      */
      virtual std::unique_ptr<PK_Ops::Key_Agreement> create_key_agreement_op(RandomNumberGenerator& rng,
                                                                             std::string_view params,
                                                                             std::string_view provider) const;
};

/**
* PK Secret Value Derivation Key
*/
class BOTAN_PUBLIC_API(2, 0) PK_Key_Agreement_Key : public virtual Private_Key {
   public:
      /*
      * @return public component of this key
      */
      virtual std::vector<uint8_t> public_value() const = 0;
};

std::string BOTAN_PUBLIC_API(2, 4) create_hex_fingerprint(const uint8_t bits[], size_t len, std::string_view hash_name);

inline std::string create_hex_fingerprint(std::span<const uint8_t> vec, std::string_view hash_name) {
   return create_hex_fingerprint(vec.data(), vec.size(), hash_name);
}

}  // namespace Botan

#endif
