/*
* Public Key Interface
* (C) 1999-2010 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_PUBKEY_H_
#define BOTAN_PUBKEY_H_

#include <botan/asn1_obj.h>
#include <botan/pk_keys.h>
#include <botan/pk_ops_fwd.h>
#include <botan/symkey.h>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace Botan {

class RandomNumberGenerator;

/**
* Public Key Encryptor
* This is the primary interface for public key encryption
*/
class BOTAN_PUBLIC_API(2, 0) PK_Encryptor {
   public:
      /**
      * Encrypt a message.
      * @param in the message as a byte array
      * @param length the length of the above byte array
      * @param rng the random number source to use
      * @return encrypted message
      */
      std::vector<uint8_t> encrypt(const uint8_t in[], size_t length, RandomNumberGenerator& rng) const {
         return enc(in, length, rng);
      }

      /**
      * Encrypt a message.
      * @param in the message
      * @param rng the random number source to use
      * @return encrypted message
      */
      std::vector<uint8_t> encrypt(std::span<const uint8_t> in, RandomNumberGenerator& rng) const {
         return enc(in.data(), in.size(), rng);
      }

      /**
      * Return the maximum allowed message size in bytes.
      * @return maximum message size in bytes
      */
      virtual size_t maximum_input_size() const = 0;

      /**
      * Return an upper bound on the ciphertext length
      */
      virtual size_t ciphertext_length(size_t ctext_len) const = 0;

      PK_Encryptor() = default;
      virtual ~PK_Encryptor() = default;

      PK_Encryptor(const PK_Encryptor&) = delete;
      PK_Encryptor& operator=(const PK_Encryptor&) = delete;

      PK_Encryptor(PK_Encryptor&&) noexcept = default;
      PK_Encryptor& operator=(PK_Encryptor&&) noexcept = default;

   private:
      virtual std::vector<uint8_t> enc(const uint8_t[], size_t, RandomNumberGenerator&) const = 0;
};

/**
* Public Key Decryptor
*/
class BOTAN_PUBLIC_API(2, 0) PK_Decryptor {
   public:
      /**
      * Decrypt a ciphertext, throwing an exception if the input
      * seems to be invalid (eg due to an accidental or malicious
      * error in the ciphertext).
      *
      * @param in the ciphertext as a byte array
      * @param length the length of the above byte array
      * @return decrypted message
      */
      secure_vector<uint8_t> decrypt(const uint8_t in[], size_t length) const;

      /**
      * Same as above, but taking a vector
      * @param in the ciphertext
      * @return decrypted message
      */
      secure_vector<uint8_t> decrypt(std::span<const uint8_t> in) const { return decrypt(in.data(), in.size()); }

      /**
      * Decrypt a ciphertext. If the ciphertext is invalid (eg due to
      * invalid padding) or is not the expected length, instead
      * returns a random string of the expected length. Use to avoid
      * oracle attacks, especially against PKCS #1 v1.5 decryption.
      */
      secure_vector<uint8_t> decrypt_or_random(const uint8_t in[],
                                               size_t length,
                                               size_t expected_pt_len,
                                               RandomNumberGenerator& rng) const;

      /**
      * Decrypt a ciphertext. If the ciphertext is invalid (eg due to
      * invalid padding) or is not the expected length, instead
      * returns a random string of the expected length. Use to avoid
      * oracle attacks, especially against PKCS #1 v1.5 decryption.
      *
      * Additionally checks (also in const time) that:
      *    contents[required_content_offsets[i]] == required_content_bytes[i]
      * for 0 <= i < required_contents
      *
      * Used for example in TLS, which encodes the client version in
      * the content bytes: if there is any timing variation the version
      * check can be used as an oracle to recover the key.
      */
      secure_vector<uint8_t> decrypt_or_random(const uint8_t in[],
                                               size_t length,
                                               size_t expected_pt_len,
                                               RandomNumberGenerator& rng,
                                               const uint8_t required_content_bytes[],
                                               const uint8_t required_content_offsets[],
                                               size_t required_contents) const;

      /**
      * Return an upper bound on the plaintext length for a particular
      * ciphertext input length
      */
      virtual size_t plaintext_length(size_t ctext_len) const = 0;

      PK_Decryptor() = default;
      virtual ~PK_Decryptor() = default;

      PK_Decryptor(const PK_Decryptor&) = delete;
      PK_Decryptor& operator=(const PK_Decryptor&) = delete;

      PK_Decryptor(PK_Decryptor&&) noexcept = default;
      PK_Decryptor& operator=(PK_Decryptor&&) noexcept = default;

   private:
      virtual secure_vector<uint8_t> do_decrypt(uint8_t& valid_mask, const uint8_t in[], size_t in_len) const = 0;
};

/**
* Public Key Signer. Use the sign_message() functions for small
* messages. Use multiple calls update() to process large messages and
* generate the signature by finally calling signature().
*/
class BOTAN_PUBLIC_API(2, 0) PK_Signer final {
   public:
      /**
      * Construct a PK Signer.
      * @param key the key to use inside this signer
      * @param rng the random generator to use
      * @param padding the padding/hash to use, eg "EMSA_PKCS1(SHA-256)"
      * @param format the signature format to use
      * @param provider the provider to use
      */
      PK_Signer(const Private_Key& key,
                RandomNumberGenerator& rng,
                std::string_view padding,
                Signature_Format format = Signature_Format::Standard,
                std::string_view provider = "");

      ~PK_Signer();

      PK_Signer(const PK_Signer&) = delete;
      PK_Signer& operator=(const PK_Signer&) = delete;

      PK_Signer(PK_Signer&&) noexcept;
      PK_Signer& operator=(PK_Signer&&) noexcept;

      /**
      * Sign a message all in one go
      * @param in the message to sign as a byte array
      * @param length the length of the above byte array
      * @param rng the rng to use
      * @return signature
      */
      std::vector<uint8_t> sign_message(const uint8_t in[], size_t length, RandomNumberGenerator& rng) {
         this->update(in, length);
         return this->signature(rng);
      }

      /**
      * Sign a message.
      * @param in the message to sign
      * @param rng the rng to use
      * @return signature
      */
      std::vector<uint8_t> sign_message(std::span<const uint8_t> in, RandomNumberGenerator& rng) {
         return sign_message(in.data(), in.size(), rng);
      }

      /**
      * Add a message part (single byte).
      * @param in the byte to add
      */
      void update(uint8_t in) { update(&in, 1); }

      /**
      * Add a message part.
      * @param in the message part to add as a byte array
      * @param length the length of the above byte array
      */
      void update(const uint8_t in[], size_t length);

      /**
      * Add a message part.
      * @param in the message part to add
      */
      void update(std::span<const uint8_t> in) { update(in.data(), in.size()); }

      /**
      * Add a message part.
      * @param in the message part to add
      */
      void update(std::string_view in);

      /**
      * Get the signature of the so far processed message (provided by the
      * calls to update()).
      * @param rng the rng to use
      * @return signature of the total message
      */
      std::vector<uint8_t> signature(RandomNumberGenerator& rng);

      /**
      * Set the output format of the signature.
      * @param format the signature format to use
      */
      void set_output_format(Signature_Format format) { m_sig_format = format; }

      /**
      * Return an upper bound on the length of the signatures this
      * PK_Signer will produce
      */
      size_t signature_length() const;

      /**
      * Return an AlgorithmIdentifier appropriate for identifying the signature
      * method being generated by this PK_Signer. Throws an exception if this
      * is not available for the current signature scheme.
      */
      AlgorithmIdentifier algorithm_identifier() const;

      /**
      * Return the hash function which is being used to create signatures.
      * This should never return an empty string however it may return a string
      * which does not map directly to a hash function, in particular if "Raw"
      * (unhashed) encoding is being used.
      */
      std::string hash_function() const;

   private:
      std::unique_ptr<PK_Ops::Signature> m_op;
      Signature_Format m_sig_format;
      size_t m_parts, m_part_size;
};

/**
* Public Key Verifier. Use the verify_message() functions for small
* messages. Use multiple calls update() to process large messages and
* verify the signature by finally calling check_signature().
*/
class BOTAN_PUBLIC_API(2, 0) PK_Verifier final {
   public:
      /**
      * Construct a PK Verifier.
      * @param pub_key the public key to verify against
      * @param padding the padding/hash to use (eg "EMSA_PKCS1(SHA-256)")
      * @param format the signature format to use
      * @param provider the provider to use
      */
      PK_Verifier(const Public_Key& pub_key,
                  std::string_view padding,
                  Signature_Format format = Signature_Format::Standard,
                  std::string_view provider = "");

      /**
      * Construct a PK Verifier (X.509 specific)
      *
      * This constructor will attempt to decode signature_format relative
      * to the public key provided. If they seem to be inconsistent or
      * otherwise unsupported, a Decoding_Error is thrown.
      *
      * @param pub_key the public key to verify against
      * @param signature_algorithm the supposed signature algorithm
      * @param provider the provider to use
      */
      PK_Verifier(const Public_Key& pub_key,
                  const AlgorithmIdentifier& signature_algorithm,
                  std::string_view provider = "");

      ~PK_Verifier();

      PK_Verifier(const PK_Verifier&) = delete;
      PK_Verifier& operator=(const PK_Verifier&) = delete;

      PK_Verifier(PK_Verifier&&) noexcept;
      PK_Verifier& operator=(PK_Verifier&&) noexcept;

      /**
      * Verify a signature.
      * @param msg the message that the signature belongs to, as a byte array
      * @param msg_length the length of the above byte array msg
      * @param sig the signature as a byte array
      * @param sig_length the length of the above byte array sig
      * @return true if the signature is valid
      */
      bool verify_message(const uint8_t msg[], size_t msg_length, const uint8_t sig[], size_t sig_length);

      /**
      * Verify a signature.
      * @param msg the message that the signature belongs to
      * @param sig the signature
      * @return true if the signature is valid
      */
      bool verify_message(std::span<const uint8_t> msg, std::span<const uint8_t> sig) {
         return verify_message(msg.data(), msg.size(), sig.data(), sig.size());
      }

      /**
      * Add a message part (single byte) of the message corresponding to the
      * signature to be verified.
      * @param in the byte to add
      */
      void update(uint8_t in) { update(&in, 1); }

      /**
      * Add a message part of the message corresponding to the
      * signature to be verified.
      * @param msg_part the new message part as a byte array
      * @param length the length of the above byte array
      */
      void update(const uint8_t msg_part[], size_t length);

      /**
      * Add a message part of the message corresponding to the
      * signature to be verified.
      * @param in the new message part
      */
      void update(std::span<const uint8_t> in) { update(in.data(), in.size()); }

      /**
      * Add a message part of the message corresponding to the
      * signature to be verified.
      */
      void update(std::string_view in);

      /**
      * Check the signature of the buffered message, i.e. the one build
      * by successive calls to update.
      * @param sig the signature to be verified as a byte array
      * @param length the length of the above byte array
      * @return true if the signature is valid, false otherwise
      */
      bool check_signature(const uint8_t sig[], size_t length);

      /**
      * Check the signature of the buffered message, i.e. the one build
      * by successive calls to update.
      * @param sig the signature to be verified
      * @return true if the signature is valid, false otherwise
      */
      bool check_signature(std::span<const uint8_t> sig) { return check_signature(sig.data(), sig.size()); }

      /**
      * Set the format of the signatures fed to this verifier.
      * @param format the signature format to use
      */
      void set_input_format(Signature_Format format);

      /**
      * Return the hash function which is being used to verify signatures.
      * This should never return an empty string however it may return a string
      * which does not map directly to a hash function, in particular if "Raw"
      * (unhashed) encoding is being used.
      */
      std::string hash_function() const;

   private:
      std::unique_ptr<PK_Ops::Verification> m_op;
      Signature_Format m_sig_format;
      size_t m_parts, m_part_size;
};

/**
* Object used for key agreement
*/
class BOTAN_PUBLIC_API(2, 0) PK_Key_Agreement final {
   public:
      /**
      * Construct a PK Key Agreement.
      * @param key the key to use
      * @param rng the random generator to use
      * @param kdf name of the KDF to use (or 'Raw' for no KDF)
      * @param provider the algo provider to use (or empty for default)
      */
      PK_Key_Agreement(const Private_Key& key,
                       RandomNumberGenerator& rng,
                       std::string_view kdf,
                       std::string_view provider = "");

      ~PK_Key_Agreement();

      PK_Key_Agreement(const PK_Key_Agreement&) = delete;
      PK_Key_Agreement& operator=(const PK_Key_Agreement&) = delete;

      PK_Key_Agreement(PK_Key_Agreement&&) noexcept;
      PK_Key_Agreement& operator=(PK_Key_Agreement&&) noexcept;

      /**
      * Perform Key Agreement Operation
      * @param key_len the desired key output size (ignored if "Raw" KDF is used)
      * @param in the other parties key
      * @param in_len the length of in in bytes
      * @param params extra derivation params
      * @param params_len the length of params in bytes
      */
      SymmetricKey derive_key(
         size_t key_len, const uint8_t in[], size_t in_len, const uint8_t params[], size_t params_len) const;

      /**
      * Perform Key Agreement Operation
      * @param key_len the desired key output size (ignored if "Raw" KDF is used)
      * @param in the other parties key
      * @param params extra derivation params
      * @param params_len the length of params in bytes
      */
      SymmetricKey derive_key(size_t key_len,
                              std::span<const uint8_t> in,
                              const uint8_t params[],
                              size_t params_len) const {
         return derive_key(key_len, in.data(), in.size(), params, params_len);
      }

      /**
      * Perform Key Agreement Operation
      * @param key_len the desired key output size (ignored if "Raw" KDF is used)
      * @param in the other parties key
      * @param in_len the length of in in bytes
      * @param params extra derivation params
      */
      SymmetricKey derive_key(size_t key_len, const uint8_t in[], size_t in_len, std::string_view params = "") const;

      /**
      * Perform Key Agreement Operation
      * @param key_len the desired key output size (ignored if "Raw" KDF is used)
      * @param in the other parties key
      * @param params extra derivation params
      */
      SymmetricKey derive_key(size_t key_len, const std::span<const uint8_t> in, std::string_view params = "") const;

      /**
      * Return the underlying size of the value that is agreed.
      * If derive_key is called with a length of 0 with a "Raw"
      * KDF, it will return a value of this size.
      */
      size_t agreed_value_size() const;

   private:
      std::unique_ptr<PK_Ops::Key_Agreement> m_op;
};

/**
* Encryption using a standard message recovery algorithm like RSA or
* ElGamal, paired with an encoding scheme like OAEP.
*/
class BOTAN_PUBLIC_API(2, 0) PK_Encryptor_EME final : public PK_Encryptor {
   public:
      size_t maximum_input_size() const override;

      /**
      * Construct an instance.
      * @param key the key to use inside the encryptor
      * @param rng the RNG to use
      * @param padding the message encoding scheme to use (eg "OAEP(SHA-256)")
      * @param provider the provider to use
      */
      PK_Encryptor_EME(const Public_Key& key,
                       RandomNumberGenerator& rng,
                       std::string_view padding,
                       std::string_view provider = "");

      ~PK_Encryptor_EME() override;

      PK_Encryptor_EME(const PK_Encryptor_EME&) = delete;
      PK_Encryptor_EME& operator=(const PK_Encryptor_EME&) = delete;

      PK_Encryptor_EME(PK_Encryptor_EME&&) noexcept;
      PK_Encryptor_EME& operator=(PK_Encryptor_EME&&) noexcept;

      /**
      * Return an upper bound on the ciphertext length for a particular
      * plaintext input length
      */
      size_t ciphertext_length(size_t ptext_len) const override;

   private:
      std::vector<uint8_t> enc(const uint8_t[], size_t, RandomNumberGenerator& rng) const override;

      std::unique_ptr<PK_Ops::Encryption> m_op;
};

/**
* Decryption with an MR algorithm and an EME.
*/
class BOTAN_PUBLIC_API(2, 0) PK_Decryptor_EME final : public PK_Decryptor {
   public:
      /**
      * Construct an instance.
      * @param key the key to use inside the decryptor
      * @param rng the random generator to use
      * @param eme the EME to use
      * @param provider the provider to use
      */
      PK_Decryptor_EME(const Private_Key& key,
                       RandomNumberGenerator& rng,
                       std::string_view eme,
                       std::string_view provider = "");

      size_t plaintext_length(size_t ptext_len) const override;

      ~PK_Decryptor_EME() override;

      PK_Decryptor_EME(const PK_Decryptor_EME&) = delete;
      PK_Decryptor_EME& operator=(const PK_Decryptor_EME&) = delete;

      PK_Decryptor_EME(PK_Decryptor_EME&&) noexcept;
      PK_Decryptor_EME& operator=(PK_Decryptor_EME&&) noexcept;

   private:
      secure_vector<uint8_t> do_decrypt(uint8_t& valid_mask, const uint8_t in[], size_t in_len) const override;

      std::unique_ptr<PK_Ops::Decryption> m_op;
};

/**
* Result of a key encapsulation operation.
*/
class KEM_Encapsulation final {
   public:
      KEM_Encapsulation(std::vector<uint8_t> encapsulated_shared_key, secure_vector<uint8_t> shared_key) :
            m_encapsulated_shared_key(std::move(encapsulated_shared_key)), m_shared_key(std::move(shared_key)) {}

      /**
      * @returns the encapsulated shared secret (encrypted with the public key)
      */
      const std::vector<uint8_t>& encapsulated_shared_key() const { return m_encapsulated_shared_key; }

      /**
      * @returns the plaintext shared secret
      */
      const secure_vector<uint8_t>& shared_key() const { return m_shared_key; }

      /**
       * @returns the pair (encapsulated key, key) extracted from @p kem
       */
      static std::pair<std::vector<uint8_t>, secure_vector<uint8_t>> destructure(KEM_Encapsulation&& kem) {
         return std::make_pair(std::exchange(kem.m_encapsulated_shared_key, {}), std::exchange(kem.m_shared_key, {}));
      }

   private:
      friend class PK_KEM_Encryptor;

      KEM_Encapsulation(size_t encapsulated_size, size_t shared_key_size) :
            m_encapsulated_shared_key(encapsulated_size), m_shared_key(shared_key_size) {}

   private:
      std::vector<uint8_t> m_encapsulated_shared_key;
      secure_vector<uint8_t> m_shared_key;
};

/**
* Public Key Key Encapsulation Mechanism Encryption.
*/
class BOTAN_PUBLIC_API(2, 0) PK_KEM_Encryptor final {
   public:
      /**
      * Construct an instance.
      * @param key the key to encrypt to
      * @param kem_param additional KEM parameters
      * @param provider the provider to use
      */
      PK_KEM_Encryptor(const Public_Key& key, std::string_view kem_param = "", std::string_view provider = "");

      /**
      * Construct an instance.
      * @param key the key to encrypt to
      * @param rng the RNG to use
      * @param kem_param additional KEM parameters
      * @param provider the provider to use
      */
      BOTAN_DEPRECATED("Use constructor that does not take RNG")
      PK_KEM_Encryptor(const Public_Key& key,
                       RandomNumberGenerator& rng,
                       std::string_view kem_param = "",
                       std::string_view provider = "") :
            PK_KEM_Encryptor(key, kem_param, provider) {
         BOTAN_UNUSED(rng);
      }

      ~PK_KEM_Encryptor();

      PK_KEM_Encryptor(const PK_KEM_Encryptor&) = delete;
      PK_KEM_Encryptor& operator=(const PK_KEM_Encryptor&) = delete;

      PK_KEM_Encryptor(PK_KEM_Encryptor&&) noexcept;
      PK_KEM_Encryptor& operator=(PK_KEM_Encryptor&&) noexcept;

      /**
      * Return the length of the shared key returned by this KEM
      *
      * If this KEM was used with a KDF, then it will always return
      * exactly the desired key length, because the output of the KEM
      * will be hashed by the KDF.
      *
      * However if the KEM was used with "Raw" kdf, to request the
      * algorithmic output of the KEM directly, then the desired key
      * length will be ignored and a bytestring that depends on the
      * algorithm is returned
      *
      * @param desired_shared_key_len is the requested length
      */
      size_t shared_key_length(size_t desired_shared_key_len) const;

      /**
      * Return the length in bytes of encapsulated keys returned by this KEM
      */
      size_t encapsulated_key_length() const;

      /**
      * Generate a shared key for data encryption.
      *
      * @param rng                    the RNG to use
      * @param desired_shared_key_len desired size of the shared key in bytes for the KDF
      *                               (ignored if no KDF is used)
      * @param salt                   a salt value used in the KDF
      *                               (ignored if no KDF is used)
      *
      * @returns a struct with both the shared secret and its encapsulation
      */
      KEM_Encapsulation encrypt(RandomNumberGenerator& rng,
                                size_t desired_shared_key_len = 32,
                                std::span<const uint8_t> salt = {}) {
         std::vector<uint8_t> encapsulated_shared_key(encapsulated_key_length());
         secure_vector<uint8_t> shared_key(shared_key_length(desired_shared_key_len));

         encrypt(std::span{encapsulated_shared_key}, std::span{shared_key}, rng, desired_shared_key_len, salt);
         return KEM_Encapsulation(std::move(encapsulated_shared_key), std::move(shared_key));
      }

      /**
      * Generate a shared key for data encryption.
      * @param out_encapsulated_key   the generated encapsulated key
      * @param out_shared_key         the generated shared key
      * @param rng                    the RNG to use
      * @param desired_shared_key_len desired size of the shared key in bytes
      *                               (ignored if no KDF is used)
      * @param salt                   a salt value used in the KDF
      *                               (ignored if no KDF is used)
      */
      void encrypt(secure_vector<uint8_t>& out_encapsulated_key,
                   secure_vector<uint8_t>& out_shared_key,
                   RandomNumberGenerator& rng,
                   size_t desired_shared_key_len = 32,
                   std::span<const uint8_t> salt = {}) {
         out_encapsulated_key.resize(encapsulated_key_length());
         out_shared_key.resize(shared_key_length(desired_shared_key_len));
         encrypt(std::span{out_encapsulated_key}, std::span{out_shared_key}, rng, desired_shared_key_len, salt);
      }

      /**
      * Generate a shared key for data encryption.
      * @param out_encapsulated_key   the generated encapsulated key
      * @param out_shared_key         the generated shared key
      * @param rng                    the RNG to use
      * @param desired_shared_key_len desired size of the shared key in bytes
      *                               (ignored if no KDF is used)
      * @param salt                   a salt value used in the KDF
      *                               (ignored if no KDF is used)
      */
      void encrypt(std::span<uint8_t> out_encapsulated_key,
                   std::span<uint8_t> out_shared_key,
                   RandomNumberGenerator& rng,
                   size_t desired_shared_key_len = 32,
                   std::span<const uint8_t> salt = {});

      BOTAN_DEPRECATED("use overload with salt as std::span<>")
      void encrypt(secure_vector<uint8_t>& out_encapsulated_key,
                   secure_vector<uint8_t>& out_shared_key,
                   size_t desired_shared_key_len,
                   RandomNumberGenerator& rng,
                   const uint8_t salt[],
                   size_t salt_len) {
         this->encrypt(out_encapsulated_key, out_shared_key, rng, desired_shared_key_len, {salt, salt_len});
      }

      BOTAN_DEPRECATED("use overload where rng comes after the out-paramters")
      void encrypt(secure_vector<uint8_t>& out_encapsulated_key,
                   secure_vector<uint8_t>& out_shared_key,
                   size_t desired_shared_key_len,
                   RandomNumberGenerator& rng,
                   std::span<const uint8_t> salt = {}) {
         out_encapsulated_key.resize(encapsulated_key_length());
         out_shared_key.resize(shared_key_length(desired_shared_key_len));
         encrypt(out_encapsulated_key, out_shared_key, rng, desired_shared_key_len, salt);
      }

   private:
      std::unique_ptr<PK_Ops::KEM_Encryption> m_op;
};

/**
* Public Key Key Encapsulation Mechanism Decryption.
*/
class BOTAN_PUBLIC_API(2, 0) PK_KEM_Decryptor final {
   public:
      /**
      * Construct an instance.
      * @param key the key to use inside the decryptor
      * @param rng the RNG to use
      * @param kem_param additional KEM parameters
      * @param provider the provider to use
      */
      PK_KEM_Decryptor(const Private_Key& key,
                       RandomNumberGenerator& rng,
                       std::string_view kem_param = "",
                       std::string_view provider = "");

      ~PK_KEM_Decryptor();
      PK_KEM_Decryptor(const PK_KEM_Decryptor&) = delete;
      PK_KEM_Decryptor& operator=(const PK_KEM_Decryptor&) = delete;

      PK_KEM_Decryptor(PK_KEM_Decryptor&&) noexcept;
      PK_KEM_Decryptor& operator=(PK_KEM_Decryptor&&) noexcept;

      /**
      * Return the length of the shared key returned by this KEM
      *
      * If this KEM was used with a KDF, then it will always return
      * exactly the desired key length, because the output of the KEM
      * will be hashed by the KDF.
      *
      * However if the KEM was used with "Raw" kdf, to request the
      * algorithmic output of the KEM directly, then the desired key
      * length will be ignored and a bytestring that depends on the
      * algorithm is returned
      *
      * @param desired_shared_key_len is the requested length.
      */
      size_t shared_key_length(size_t desired_shared_key_len) const;

      /**
      * Return the length of the encapsulated key expected by this KEM
      */
      size_t encapsulated_key_length() const;

      /**
      * Decrypts the shared key for data encryption.
      *
      * @param out_shared_key         the generated shared key
      * @param encap_key              the encapsulated key
      * @param desired_shared_key_len desired size of the shared key in bytes
      *                               (ignored if no KDF is used)
      * @param salt                   a salt value used in the KDF
      *                               (ignored if no KDF is used)
      */
      void decrypt(std::span<uint8_t> out_shared_key,
                   std::span<const uint8_t> encap_key,
                   size_t desired_shared_key_len = 32,
                   std::span<const uint8_t> salt = {});

      /**
      * Decrypts the shared key for data encryption.
      *
      * @param encap_key              the encapsulated key
      * @param encap_key_len          size of the encapsulated key in bytes
      * @param desired_shared_key_len desired size of the shared key in bytes
      *                               (ignored if no KDF is used)
      * @param salt                   a salt value used in the KDF
      *                               (ignored if no KDF is used)
      * @param salt_len               size of the salt value in bytes
      *                               (ignored if no KDF is used)
      *
      * @return the shared data encryption key
      */
      secure_vector<uint8_t> decrypt(const uint8_t encap_key[],
                                     size_t encap_key_len,
                                     size_t desired_shared_key_len,
                                     const uint8_t salt[] = nullptr,
                                     size_t salt_len = 0) {
         secure_vector<uint8_t> shared_key(shared_key_length(desired_shared_key_len));
         decrypt(shared_key, {encap_key, encap_key_len}, desired_shared_key_len, {salt, salt_len});
         return shared_key;
      }

      /**
      * Decrypts the shared key for data encryption.
      *
      * @param encap_key              the encapsulated key
      * @param desired_shared_key_len desired size of the shared key in bytes
      *                               (ignored if no KDF is used)
      * @param salt                   a salt value used in the KDF
      *                               (ignored if no KDF is used)
      *
      * @return the shared data encryption key
      */
      secure_vector<uint8_t> decrypt(std::span<const uint8_t> encap_key,
                                     size_t desired_shared_key_len = 32,
                                     std::span<const uint8_t> salt = {}) {
         secure_vector<uint8_t> shared_key(shared_key_length(desired_shared_key_len));
         decrypt(shared_key, encap_key, desired_shared_key_len, salt);
         return shared_key;
      }

   private:
      std::unique_ptr<PK_Ops::KEM_Decryption> m_op;
};

}  // namespace Botan

#endif
