/*
* Low Level Types
* (C) 1999-2007 Jack Lloyd
* (C) 2015 Simon Warta (Kullo GmbH)
* (C) 2016 René Korthaus, Rohde & Schwarz Cybersecurity
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TYPES_H_
#define BOTAN_TYPES_H_

#include <botan/assert.h>    // IWYU pragma: export
#include <botan/build.h>     // IWYU pragma: export
#include <botan/compiler.h>  // IWYU pragma: export
#include <cstddef>           // IWYU pragma: export
#include <cstdint>           // IWYU pragma: export
#include <memory>            // IWYU pragma: export

namespace Botan {

/**
* @mainpage Botan Crypto Library API Reference
*
* <dl>
* <dt>Abstract Base Classes<dd>
*        BlockCipher, HashFunction, KDF, MessageAuthenticationCode, RandomNumberGenerator,
*        StreamCipher, SymmetricAlgorithm, AEAD_Mode, Cipher_Mode, XOF
* <dt>Public Key Interface Classes<dd>
*        PK_Key_Agreement, PK_Signer, PK_Verifier, PK_Encryptor, PK_Decryptor, PK_KEM_Encryptor, PK_KEM_Decryptor
* <dt>Authenticated Encryption Modes<dd>
*        @ref CCM_Mode "CCM", @ref ChaCha20Poly1305_Mode "ChaCha20Poly1305", @ref EAX_Mode "EAX",
*        @ref GCM_Mode "GCM", @ref OCB_Mode "OCB", @ref SIV_Mode "SIV"
* <dt>Block Ciphers<dd>
*        @ref aria.h "ARIA", @ref aes.h "AES", @ref Blowfish, @ref camellia.h "Camellia", @ref Cascade_Cipher "Cascade",
*        @ref CAST_128 "CAST-128", @ref CAST_128 DES, @ref TripleDES "3DES",
*        @ref GOST_28147_89 "GOST 28147-89", IDEA, Kuznyechik, Lion, Noekeon, SEED, Serpent, SHACAL2, SM4,
*        @ref Threefish_512 "Threefish", Twofish
* <dt>Stream Ciphers<dd>
*        ChaCha, @ref CTR_BE "CTR", OFB, RC4, Salsa20
* <dt>Hash Functions<dd>
*        BLAKE2b, @ref GOST_34_11 "GOST 34.11", @ref Keccak_1600 "Keccak", MD4, MD5, @ref RIPEMD_160 "RIPEMD-160",
*        @ref SHA_1 "SHA-1", @ref SHA_224 "SHA-224", @ref SHA_256 "SHA-256", @ref SHA_384 "SHA-384",
*        @ref SHA_512 "SHA-512", @ref Skein_512 "Skein-512", SM3, Streebog, Whirlpool
* <dt>Non-Cryptographic Checksums<dd>
*        Adler32, CRC24, CRC32
* <dt>Message Authentication Codes<dd>
*        @ref BLAKE2bMAC "BLAKE2b", CMAC, HMAC, KMAC, Poly1305, SipHash, ANSI_X919_MAC
* <dt>Random Number Generators<dd>
*        AutoSeeded_RNG, HMAC_DRBG, Processor_RNG, System_RNG
* <dt>Key Derivation<dd>
*        HKDF, @ref KDF1 "KDF1 (IEEE 1363)", @ref KDF1_18033 "KDF1 (ISO 18033-2)", @ref KDF2 "KDF2 (IEEE 1363)",
*        @ref sp800_108.h "SP800-108", @ref SP800_56C "SP800-56C", @ref PKCS5_PBKDF2 "PBKDF2 (PKCS#5)"
* <dt>Password Hashing<dd>
*        @ref argon2.h "Argon2", @ref scrypt.h "scrypt", @ref bcrypt.h "bcrypt", @ref passhash9.h "passhash9"
* <dt>Public Key Cryptosystems<dd>
*        @ref dlies.h "DLIES", @ref ecies.h "ECIES", @ref elgamal.h "ElGamal",
*        @ref rsa.h "RSA", @ref mceliece.h "McEliece", @ref sm2.h "SM2"
* <dt>Key Encapsulation Mechanisms<dd>
*        @ref frodokem.h "FrodoKEM", @ref kyber.h "Kyber", @ref rsa.h "RSA"
* <dt>Public Key Signature Schemes<dd>
*        @ref dsa.h "DSA", @ref dilithium.h "Dilithium", @ref ecdsa.h "ECDSA", @ref ecgdsa.h "ECGDSA",
*        @ref eckcdsa.h "ECKCDSA", @ref gost_3410.h "GOST 34.10-2001", @ref hss_lms.h "HSS/LMS", @ref sm2.h "SM2",
         @ref sphincsplus.h "SPHINCS+", @ref xmss.h "XMSS"
* <dt>Key Agreement<dd>
*        @ref dh.h "DH", @ref ecdh.h "ECDH"
* <dt>Compression<dd>
*        @ref bzip2.h "bzip2", @ref lzma.h "lzma", @ref zlib.h "zlib"
* <dt>TLS<dd>
*        TLS::Client, TLS::Server, TLS::Policy, TLS::Protocol_Version, TLS::Callbacks, TLS::Ciphersuite,
*        TLS::Session, TLS::Session_Summary, TLS::Session_Manager, Credentials_Manager
* <dt>X.509<dd>
*        X509_Certificate, X509_CRL, X509_CA, Certificate_Extension, PKCS10_Request, X509_Cert_Options,
*        Certificate_Store, Certificate_Store_In_SQL, Certificate_Store_In_SQLite
* <dt>eXtendable Output Functions<dd>
*        @ref SHAKE_XOF "SHAKE"
* </dl>
*/

using std::int32_t;
using std::int64_t;
using std::size_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::uint8_t;

#if !defined(BOTAN_IS_BEING_BUILT)
/*
* These typedefs are no longer used within the library headers
* or code. They are kept only for compatability with software
* written against older versions.
*/
using byte = std::uint8_t;
using u16bit = std::uint16_t;
using u32bit = std::uint32_t;
using u64bit = std::uint64_t;
using s32bit = std::int32_t;
#endif

#if(BOTAN_MP_WORD_BITS == 32)
typedef uint32_t word;
#elif(BOTAN_MP_WORD_BITS == 64)
typedef uint64_t word;
#else
   #error BOTAN_MP_WORD_BITS must be 32 or 64
#endif

#if defined(__SIZEOF_INT128__) && defined(BOTAN_TARGET_CPU_HAS_NATIVE_64BIT)
   #define BOTAN_TARGET_HAS_NATIVE_UINT128

// GCC complains if this isn't marked with __extension__
__extension__ typedef unsigned __int128 uint128_t;
#endif

/*
* Should this assert fail on your system please contact the developers
* for assistance in porting.
*/
static_assert(sizeof(std::size_t) == 8 || sizeof(std::size_t) == 4, "This platform has an unexpected size for size_t");

}  // namespace Botan

#endif
