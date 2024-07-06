/*
* SSL/TLS Protocol Constants
* (C) 2004-2010 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_PROTOCOL_MAGIC_H_
#define BOTAN_TLS_PROTOCOL_MAGIC_H_

#include <vector>

#include <botan/types.h>

//BOTAN_FUTURE_INTERNAL_HEADER(tls_magic.h)

namespace Botan::TLS {

/**
* Protocol Constants for SSL/TLS
*
* TODO: this should not be an enum
*/
enum Size_Limits : size_t {
   TLS_HEADER_SIZE = 5,
   DTLS_HEADER_SIZE = TLS_HEADER_SIZE + 8,

   // The "TLSInnerPlaintext" length, i.e. the maximum amount of plaintext
   // application data that can be transmitted in a single TLS record.
   MAX_PLAINTEXT_SIZE = 16 * 1024,

   MAX_COMPRESSED_SIZE = MAX_PLAINTEXT_SIZE + 1024,
   MAX_CIPHERTEXT_SIZE = MAX_COMPRESSED_SIZE + 1024,

   // RFC 8446 5.2:
   //   This limit is derived from the maximum TLSInnerPlaintext length of 2^14
   //   octets + 1 octet for ContentType + the maximum AEAD expansion of 255
   //   octets.
   MAX_AEAD_EXPANSION_SIZE_TLS13 = 255,
   MAX_CIPHERTEXT_SIZE_TLS13 = MAX_PLAINTEXT_SIZE + MAX_AEAD_EXPANSION_SIZE_TLS13 + 1
};

enum class Connection_Side {
   Client = 1,
   Server = 2,

   CLIENT BOTAN_DEPRECATED("Use Connection_Side::Client") = Client,
   SERVER BOTAN_DEPRECATED("Use Connection_Side::Server") = Server,
};

enum class Handshake_Type {
   HelloRequest = 0,
   ClientHello = 1,
   ServerHello = 2,
   HelloVerifyRequest = 3,
   NewSessionTicket = 4,  // RFC 5077

   EndOfEarlyData = 5,       // RFC 8446 (TLS 1.3)
   EncryptedExtensions = 8,  // RFC 8446 (TLS 1.3)

   Certificate = 11,
   ServerKeyExchange = 12,
   CertificateRequest = 13,
   ServerHelloDone = 14,
   CertificateVerify = 15,
   ClientKeyExchange = 16,
   Finished = 20,

   CertificateUrl = 21,
   CertificateStatus = 22,

   KeyUpdate = 24,  // RFC 8446 (TLS 1.3)

   HelloRetryRequest = 253,  // Not a wire value (HRR appears as an ordinary Server Hello)
   HandshakeCCS = 254,       // Not a wire value (TLS 1.3 uses this value for 'message_hash' -- RFC 8446 4.4.1)
   None = 255                // Null value
};

BOTAN_TEST_API const char* handshake_type_to_string(Handshake_Type t);

using Transcript_Hash = std::vector<uint8_t>;

}  // namespace Botan::TLS

#endif
