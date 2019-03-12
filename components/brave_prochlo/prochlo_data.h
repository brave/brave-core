// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __LIB_DATA_H__
#define __LIB_DATA_H__

#include <cstdint>
#include <cstddef>

namespace prochlo {

// Nomenclature for message structures:
//
// The client's encoder produces a Proclomation, i.e., an EncoderItem, which it
// wishes to deliver to the Analyzer, via the shuffler.
//
// A ShufflerItem travels from the Client to the Shuffler.
//
// An AnalyzerItem travels from the Shuffler to the Analyzer.
//
// The Shuffler stores the intermediate state of its shuffle on local
// (untrusted) storage, in the shape of an IntermediateShufflerItem.
//
// A Proclomation contains just the type of data (|metric|) and the value. The
// Client encrypts using AES128-GCM, with a key derived from its ephemeral key
// pair and the Analyzer's public key. That's the inner layer of the nested
// encryption, and constitutes the AnalyzerItem.
//
// Next, the Client constructs the outer layer of the nested encryption, by
// adding the value of the crowd id to the AnalyzerItem, and encrypting it,
// again using AES128-GCM, with a key derived from (another) ephemeral key pair,
// and the Shuffler's public key. That's the outer layer of the nested
// encryption, and constitutes the ShufflerItem. This is what the Client
// transmits to the Shuffler.
//
// The StashShuffler shuffles ShufflerItems in at least two rounds, storing
// intermediate results on local (but untrusted) storage. To ensure the shuffle
// is oblivious, the Shuffler decrypts ShufflerItems (as collected from
// Clients), and re-encrypts them unchanged using an ephemeral symmetric key of
// its choosing (again using AES128-GCM). The resulting structure is an
// IntermediateShufflerItem. In addition to the AnalyzerItem and |crowd_id|, and
// IntermediateShufflerItem also contains some StashShuffler metadata (e.g.,
// whether an IntermediateShufflerItem is a dummy).
//
// In a production setting, all of these message structures are represented by
// Protocol Buffers, which are not shown here.
//
// Note that we use struct sizes as the sizes of messages (i.e., sizeof(type)),
// rather than the number of bytes they'd take when marshalled. Due to
// alignment, the former may be larger than the latter.

// Default lengths
#define DATA_LENGTH 64
#define CROWD_ID_LENGTH 8

// Problem-specific lengths, in bytes.
constexpr size_t kProchlomationDataLength = DATA_LENGTH;
constexpr size_t kCrowdIdLength = CROWD_ID_LENGTH;

// Crypto-specific lengths.
constexpr size_t kPublicKeyLength = 91;  // This is the maximum length we devote
                                         // for storing a DER-encoded NIST
                                         // P-256 public key.
constexpr size_t kSharedSecretLength = 256 / 8;  // The length of the derived
                                                 // shared secret from Diffie
                                                 // Hellman key exchange on NIST
                                                 // P-256.
constexpr size_t kSymmetricKeyLength = 128 / 8;  // The length of an AES128 key.
constexpr size_t kSharedSecretExpansionLength = 256 / 8;  // The length of the
                                                          // pseudo-random space
                                                          // used to derive a
                                                          // shared symmetric
                                                          // key from a shared
                                                          // DH secret. It's
                                                          // determined by the
                                                          // length of SHA256.

constexpr size_t kNonceLength = 12;  // The recommended nonce (i.e., IV) length
                                     // for AES128-GCM is 12 bytes.

constexpr size_t kTagLength = 16;  // The maximum tag length for AES128-GCM is
                                   // 16 bytes.

////////////////////////////////////////////////////////////////////////////////
// Prochlomation
////////////////////////////////////////////////////////////////////////////////
// A prochlomation is the plain encoded data that a Client's Encoder generates
// and intends to deliver to an Analyzer.
struct Prochlomation {
  uint64_t metric;
  uint8_t data[kProchlomationDataLength];  // e.g., 64
};
constexpr size_t kProchlomationLength = sizeof(Prochlomation);  // e.g., 72
constexpr size_t kProchlomationCiphertextLength =
  kProchlomationLength;  // The ciphertext is the same length, but it is
                         // augmented by the MAC stored in |tag| below.

////////////////////////////////////////////////////////////////////////////////
// AnalyzerItem
////////////////////////////////////////////////////////////////////////////////
struct EncryptedProchlomation {
  // The result of encrypting a Prochlomation using AES128-GCM is |ciphertext|,
  // with MAC |tag|, starting with the IV in |nonce|.
  uint8_t ciphertext[kProchlomationCiphertextLength];
  uint8_t tag[kTagLength];
  uint8_t nonce[kNonceLength];

  // The key used to produce |ciphertext| is derived from the analyzer's key
  // pair and the client's ephermeral key pair. The public key of the client's
  // key pair is |client_public_key|.
  uint8_t client_public_key[kPublicKeyLength];
};
constexpr size_t kEncryptedProchlomationLength = sizeof(EncryptedProchlomation);

// Aliases for EncryptedProchlomations
typedef EncryptedProchlomation AnalyzerItem;
constexpr size_t kAnalyzerItemLength = kEncryptedProchlomationLength;

////////////////////////////////////////////////////////////////////////////////
// ShufflerItem
////////////////////////////////////////////////////////////////////////////////
// This is the item that the Shuffler handles, and it contains the AnalyzerItem
// and the crowd ID.
struct PlainShufflerItem {
  AnalyzerItem analyzer_item;

  // The crowd ID for the Prochlomation included in |analyzer_item|.
  uint8_t crowd_id[kCrowdIdLength];
};
constexpr size_t kPlainShufflerItemLength = sizeof(PlainShufflerItem);

struct EncryptedPlainShufflerItem {
  // The result of encrypting an PlainShufflerItem using AES128-GCM is
  // |ciphertext|, with MAC |tag|, starting with the IV in |nonce|.
  uint8_t ciphertext[kPlainShufflerItemLength];
  uint8_t tag[kTagLength];
  uint8_t nonce[kNonceLength];

  // The key used to produce |ciphertext| is derived from the shuffler's key
  // pair and the client's ephermeral key pair. The public key of the client's
  // key pair is |client_public_key|. Note that the client may (in fact, might
  // as well) use two different ephemeral key pairs, one for the shuffler and
  // one for the analyzer. So this may not be the same as the
  // |client_public_key| in EncryptedProchlomation.
  uint8_t client_public_key[kPublicKeyLength];
};
constexpr size_t kEncryptedPlainShufflerItemLength =
  sizeof(EncryptedPlainShufflerItem);

// The ShufflerItem is just an EncryptedPlainShufflerItem
typedef EncryptedPlainShufflerItem ShufflerItem;
constexpr size_t kShufflerItemLength = kEncryptedPlainShufflerItemLength;

}  // namespace prochlo

#endif //  __LIB_DATA_H__
