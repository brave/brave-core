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

#include <assert.h>
#include <fcntl.h>
#include "third_party/boringssl/src/include/openssl/ecdh.h"
#include "third_party/boringssl/src/include/openssl/err.h"
#include "third_party/boringssl/src/include/openssl/hmac.h"
#include "third_party/boringssl/src/include/openssl/pem.h"
#include "third_party/boringssl/src/include/openssl/rand.h"
#include <stdint.h>
#include <stdio.h>

#include <cstring>
#include <iostream>
#include <string>

#include "prochlo_crypto.h"

namespace prochlo {

bool Crypto::load_analyzer_key(const std::string& keyfile) {
  public_analyzer_key_ = load_public_key(keyfile);
  return public_analyzer_key_ != nullptr;
}

bool Crypto::load_shuffler_key(const std::string& keyfile) {
  public_shuffler_key_ = load_public_key(keyfile);
  return public_shuffler_key_ != nullptr;
}

EVP_PKEY* Crypto::load_public_key(const std::string& keyfile) {
  FILE* fp = fopen(keyfile.c_str(), "r");
  if (fp == nullptr) {
    //warn("fopen()");
    return nullptr;
  }

  EVP_PKEY* key;
  key = PEM_read_PUBKEY(fp, NULL, NULL, NULL);
  fclose(fp);

  if (key == nullptr) {
    ERR_print_errors_fp(stderr);
  }
  return key;
}

Crypto::Crypto()
  : public_shuffler_key_(nullptr), public_analyzer_key_(nullptr) {
  // Pedantically check that we have the same endianness everywhere
//  uint32_t number = 1;
//  assert(reinterpret_cast<uint8_t*>(&number)[0] == 1);

  // Ensure the AES128-GCM default nonce length is kNonceLength
  assert(EVP_CIPHER_iv_length(EVP_aes_128_gcm()) == kNonceLength);
}

Crypto::~Crypto() {
  if (public_shuffler_key_ != nullptr) {
    EVP_PKEY_free(public_shuffler_key_);
  }
  if (public_analyzer_key_ != nullptr) {
    EVP_PKEY_free(public_analyzer_key_);
  }
}

Crypto::ProchlomationToAnalyzerItemEncryption::
  ProchlomationToAnalyzerItemEncryption(EVP_PKEY* peer_key,
    const Prochlomation& prochlomation,
    AnalyzerItem* analyzer_item)
  : Encryption(peer_key),
  prochlomation(prochlomation),
  analyzer_item(analyzer_item) {}

uint8_t* Crypto::ProchlomationToAnalyzerItemEncryption::ToPublicKey() {
  return analyzer_item->client_public_key;
}

const char* Crypto::ProchlomationToAnalyzerItemEncryption::TypeString() {
  return "Prochlomation->AnalyzerItem";
}

uint8_t* Crypto::ProchlomationToAnalyzerItemEncryption::ToNonce() {
  return analyzer_item->nonce;
}

uint8_t* Crypto::ProchlomationToAnalyzerItemEncryption::ToTag() {
  return analyzer_item->tag;
}

bool Crypto::ProchlomationToAnalyzerItemEncryption::StreamDataForEncryption(
  EVP_CIPHER_CTX* ctx) {
  // Stream the proclomation to the cipher and write out the ciphertext.
  uint8_t* next_byte = nullptr;
  size_t ciphertext_byte_count = 0;
  int32_t out_length;

  // First the metric
  next_byte = &analyzer_item->ciphertext[ciphertext_byte_count];
  const uint8_t* to_metric =
    reinterpret_cast<const uint8_t*>(&prochlomation.metric);
  if (EVP_EncryptUpdate(ctx, next_byte, &out_length, to_metric,
    sizeof(prochlomation.metric)) != 1) {
    //warn("Couldn't encrypt metric with AES128-GCM.");
    ERR_print_errors_fp(stderr);
    return false;
  }
  ciphertext_byte_count += out_length;

  // And the data
  next_byte = &analyzer_item->ciphertext[ciphertext_byte_count];
  const uint8_t* to_data = prochlomation.data;
  if (EVP_EncryptUpdate(ctx, next_byte, &out_length, to_data,
    kProchlomationDataLength) != 1) {
    //warn("Couldn't encrypt data with AES128-GCM.");
    ERR_print_errors_fp(stderr);
    return false;
  }
  ciphertext_byte_count += out_length;
  assert(ciphertext_byte_count == kProchlomationCiphertextLength);
  return true;
}

Crypto::PlainShufflerItemToShufflerItemEncryption::
  PlainShufflerItemToShufflerItemEncryption(
    EVP_PKEY* peer_key, const PlainShufflerItem& plain_shuffler_item,
    ShufflerItem* shuffler_item)
  : Encryption(peer_key),
  plain_shuffler_item(plain_shuffler_item),
  shuffler_item(shuffler_item) {}

uint8_t* Crypto::PlainShufflerItemToShufflerItemEncryption::ToPublicKey() {
  return shuffler_item->client_public_key;
}

const char* Crypto::PlainShufflerItemToShufflerItemEncryption::TypeString() {
  return "PlainShufflerItem->ShufflerItem";
}

uint8_t* Crypto::PlainShufflerItemToShufflerItemEncryption::ToNonce() {
  return shuffler_item->nonce;
}

uint8_t* Crypto::PlainShufflerItemToShufflerItemEncryption::ToTag() {
  return shuffler_item->tag;
}

bool Crypto::PlainShufflerItemToShufflerItemEncryption::StreamDataForEncryption(
  EVP_CIPHER_CTX* ctx) {
  // Stream the PlainShufflerItem to the CIPHER and write out the ciphertext.
  uint8_t* next_byte = nullptr;
  size_t ciphertext_byte_count = 0;
  int32_t out_length;

  // First the analyzer item (i.e., its innards).
  next_byte = &shuffler_item->ciphertext[ciphertext_byte_count];
  const uint8_t* to_analyzer_item_ciphertext =
    plain_shuffler_item.analyzer_item.ciphertext;
  if (EVP_EncryptUpdate(ctx, next_byte, &out_length,
    to_analyzer_item_ciphertext,
    kProchlomationCiphertextLength) != 1) {
    //warn("Couldn't encrypt analyzer item ciphertext with AES128-GCM.");
    ERR_print_errors_fp(stderr);
    return false;
  }
  ciphertext_byte_count += out_length;

  next_byte = &shuffler_item->ciphertext[ciphertext_byte_count];
  const uint8_t* to_analyzer_item_tag = plain_shuffler_item.analyzer_item.tag;
  if (EVP_EncryptUpdate(ctx, next_byte, &out_length, to_analyzer_item_tag,
    kTagLength) != 1) {
    //warn("Couldn't encrypt analyzer item tag with AES128-GCM.");
    ERR_print_errors_fp(stderr);
    return false;
  }
  ciphertext_byte_count += out_length;

  next_byte = &shuffler_item->ciphertext[ciphertext_byte_count];
  const uint8_t* to_analyzer_item_nonce =
    plain_shuffler_item.analyzer_item.nonce;
  if (EVP_EncryptUpdate(ctx, next_byte, &out_length, to_analyzer_item_nonce,
    kNonceLength) != 1) {
    //warn("Couldn't encrypt analyzer item nonce with AES128-GCM.");
    ERR_print_errors_fp(stderr);
    return false;
  }
  ciphertext_byte_count += out_length;

  next_byte = &shuffler_item->ciphertext[ciphertext_byte_count];
  const uint8_t* to_analyzer_item_client_public_key =
    plain_shuffler_item.analyzer_item.client_public_key;
  if (EVP_EncryptUpdate(ctx, next_byte, &out_length,
    to_analyzer_item_client_public_key,
    kPublicKeyLength) != 1) {
    //warn("Couldn't encrypt analyzer item client public key with AES128-GCM.");
    ERR_print_errors_fp(stderr);
    return false;
  }
  ciphertext_byte_count += out_length;

  // And now finish with the crowd ID
  next_byte = &shuffler_item->ciphertext[ciphertext_byte_count];
  const uint8_t* to_crowd_id = plain_shuffler_item.crowd_id;
  if (EVP_EncryptUpdate(ctx, next_byte, &out_length, to_crowd_id,
    kCrowdIdLength) != 1) {
    //warn("Couldn't encrypt crowd IDwith AES128-GCM.");
    ERR_print_errors_fp(stderr);
    return false;
  }
  ciphertext_byte_count += out_length;

  assert(ciphertext_byte_count == kPlainShufflerItemLength);
  return true;
}

bool Crypto::MakeEncryptedMessage(Encryption* encryption) {
  EVP_PKEY* my_key = nullptr;
  EVP_PKEY* peer_key = encryption->ToPeerKey();

  do {  // Using BoringSSL's scoped EVP_PKEY pointers would be a lot more
        // exciting here that the do {} while(false) kludge.
    if (!GenerateKeyPair(peer_key, &my_key, encryption->ToPublicKey())) {
      //warn("Couldn't generate an ephemeral keypair during %s message creation.",
      //  encryption->TypeString());
      break;
    }

    uint8_t symmetric_key[kSymmetricKeyLength];
    if (!DeriveSecretSymmetricKey(my_key, peer_key, symmetric_key)) {
      //warn("Couldn't generate a symmetric key during %s message creation.",
      //  encryption->TypeString());
      break;
    }

    if (!Encrypt(symmetric_key, encryption)) {
      //warn("Couldn't encrypt for %s.", encryption->TypeString());
      break;
    }

    if (my_key != nullptr) {
      EVP_PKEY_free(my_key);
    }
    return true;
  } while (false);

  if (my_key != nullptr) {
    EVP_PKEY_free(my_key);
  }
  return false;
}

bool Crypto::EncryptForAnalyzer(const Prochlomation& prochlomation,
  AnalyzerItem* analyzer_item) {
  ProchlomationToAnalyzerItemEncryption encryption(
    public_analyzer_key_, prochlomation, analyzer_item);
  return MakeEncryptedMessage(&encryption);
}

bool Crypto::EncryptForShuffler(const PlainShufflerItem& plain_shuffler_item,
  ShufflerItem* shuffler_item) {
  PlainShufflerItemToShufflerItemEncryption encryption(
    public_shuffler_key_, plain_shuffler_item, shuffler_item);
  return MakeEncryptedMessage(&encryption);
}

bool Crypto::GenerateKeyPair(EVP_PKEY* peer_public_key, EVP_PKEY** key_out,
  uint8_t* binary_key) {
  assert(peer_public_key != nullptr);
  assert(key_out != nullptr);
  assert(binary_key != nullptr);
  assert(*key_out == nullptr);

  EVP_PKEY_CTX* ctx = nullptr;
  EVP_PKEY* key = nullptr;
  BIO* bio = NULL;

  do {
    // Generate a key based on the peer's key parameters.
    ctx = EVP_PKEY_CTX_new(peer_public_key, /*e=*/nullptr);
    if (ctx == nullptr) {
      //warn("Couldn't create an EVP_PKEY_CTX.");
      ERR_print_errors_fp(stderr);
      break;
    }

    if (EVP_PKEY_keygen_init(ctx) != 1) {
      //warn("Couldn't initialize the key-pair generation.");
      ERR_print_errors_fp(stderr);
      break;
    }

    if (EVP_PKEY_keygen(ctx, &key) != 1) {
      //warn("Couldn't generate a key pair.");
      ERR_print_errors_fp(stderr);
      break;
    }

    // Serialize the key.
    bio = BIO_new(BIO_s_mem());
    if (bio == nullptr) {
      //warn("Couldn't allocate an OpenSSL buffer.");
      ERR_print_errors_fp(stderr);
      break;
    }

    if (i2d_PUBKEY_bio(bio, key) != 1) {
      //warn("Couldn't serialize a key pair.");
      ERR_print_errors_fp(stderr);
      break;
    }

    uint8_t* serialized_buffer = nullptr;
    size_t serialized_key_length = BIO_get_mem_data(bio, (char **)&serialized_buffer);
    // We'd better have provisioned enough space for the serialized public key.
    assert(serialized_key_length <= kPublicKeyLength);

    // Now write the results out (they OpenSSL key and the serialized key)
    memcpy(binary_key, serialized_buffer, serialized_key_length);
    *key_out = key;

    // Successful return.
    EVP_PKEY_CTX_free(ctx);
    BIO_free(bio);
    return true;
  } while (false);

  // Unsuccessful return.
  *key_out = nullptr;
  if (key != nullptr) {
    EVP_PKEY_free(key);
  }
  if (ctx != nullptr) {
    EVP_PKEY_CTX_free(ctx);
  }
  if (bio != nullptr) {
    BIO_free(bio);
  }
  return false;
}

bool Crypto::DeriveSecretSymmetricKey(EVP_PKEY* local_key,
  EVP_PKEY* peer_public_key,
  uint8_t* secret_key) {
  assert(local_key != nullptr);
  assert(peer_public_key != nullptr);
  assert(secret_key != nullptr);

  EVP_PKEY_CTX* ctx = nullptr;

  do {
    ctx = EVP_PKEY_CTX_new(local_key, /*e=*/nullptr);
    if (ctx == nullptr) {
      //warn("Couldn't create an EVP_PKEY_CTX for secret derivation.");
      ERR_print_errors_fp(stderr);
      break;
    }

    if (EVP_PKEY_derive_init(ctx) != 1) {
      //warn("Couldn't initiate a secret derivation.");
      ERR_print_errors_fp(stderr);
      break;
    }

    if (EVP_PKEY_derive_set_peer(ctx, peer_public_key) != 1) {
      //warn("Couldn't set the public key of my peer for a secret derivation.");
      ERR_print_errors_fp(stderr);
      break;
    }

    size_t derived_secret_length = 0;
    if (EVP_PKEY_derive(ctx, nullptr, &derived_secret_length) != 1) {
      //warn("Couldn't find the length of the derived secret.");
      ERR_print_errors_fp(stderr);
      break;
    }
    assert(derived_secret_length <= kSharedSecretLength);
    uint8_t derived_secret[kSharedSecretLength];

    if (EVP_PKEY_derive(ctx, derived_secret, &derived_secret_length) != 1) {
      //warn("Couldn't derive a shared secret.");
      ERR_print_errors_fp(stderr);
      break;
    }

    // Now turn it into a key, using a HKDF.
    // 1. Extract
    uint8_t expansion[kSharedSecretExpansionLength];
    // Zero it out, don't use a fancy salt.
    memset(expansion, 0, kSharedSecretExpansionLength);
    // First HMAC the shared secret with the expansion as the key (initially
    // zero).
    uint32_t hmac_length;
    uint8_t* hmac = HMAC(EVP_sha256(),
      /* key = */ expansion, kSharedSecretExpansionLength,
      /* d = */ derived_secret, derived_secret_length,
      /* md = */ expansion, &hmac_length);
    if (hmac == nullptr) {
      //warn("Couldn't HMAC the derived secret.");
      ERR_print_errors_fp(stderr);
      break;
    }
    assert(hmac_length == kSharedSecretExpansionLength);
    // Now HMAC the previous HMAC result with itself as a key, and some
    // well-defined additional data (namely, 1).
    uint8_t one = 1;
    hmac = HMAC(EVP_sha256(),
      /* key = */ expansion, kSharedSecretExpansionLength,
      /* d = */ &one, sizeof(one),  // arbitrary choice
      /* md = */ expansion, /* md_len= */ nullptr);  // No need to
                                                     // obtain the
                                                     // length of the
                                                     // md yet again.
    if (hmac == nullptr) {
      //warn("Couldn't HMAC to expand the symmetric key.");
      ERR_print_errors_fp(stderr);
      break;
    }
    // Now we have good key material in |expansion|. Strip it down to the
    // keysize of AES128.
    assert(kSharedSecretExpansionLength > kSymmetricKeyLength);
    memcpy(secret_key, expansion, kSymmetricKeyLength);

    EVP_PKEY_CTX_free(ctx);
    return true;
  } while (false);

  if (ctx != nullptr) {
    EVP_PKEY_CTX_free(ctx);
  }
  return false;
}

bool Crypto::Encrypt(const uint8_t* symmetric_key, Encryption* encryption) {
  assert(encryption != nullptr);

  EVP_CIPHER_CTX* ctx = nullptr;
  do {
    ctx = EVP_CIPHER_CTX_new();
    if (ctx == nullptr) {
      //warn("Couldn't create a new EVP_CIPHER_CTX.");
      ERR_print_errors_fp(stderr);
      break;
    }

    EVP_CIPHER_CTX_init(ctx);

    // Set up a random nonce
    if (RAND_bytes(encryption->ToNonce(), kNonceLength) != 1) {
      //warn("Couldn't generate random nonce.");
      ERR_print_errors_fp(stderr);
      break;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(),
      /* impl= */ nullptr, symmetric_key,
      /* iv= */ encryption->ToNonce()) != 1) {
      //warn("Couldn't initialize for AES128-GCM encryption.");
      ERR_print_errors_fp(stderr);
      break;
    }

    if (!encryption->StreamDataForEncryption(ctx)) {
      //warn("Couldn't stream data for %s AES128-GCM encryption.",
      //encryption->TypeString());
      break;
    }

    // Now finalize to obtain the tag. We should have no pending ciphertext data
    // at this point.
    int32_t out_length;
    if (EVP_EncryptFinal_ex(ctx, /* out= */ nullptr, &out_length) != 1) {
      //warn("Couldn't finalize the prochlomation encryption.");
      ERR_print_errors_fp(stderr);
      break;
    }
    assert(out_length == 0);

    // We have filled in the ciphertext. Now we also need to fill in the tag.
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, kTagLength,
      encryption->ToTag()) != 1) {
      //warn("Couldn't obtain the AEAD tag from the prochlomation encryption.");
      ERR_print_errors_fp(stderr);
      break;
    }

    EVP_CIPHER_CTX_free(ctx);
    return true;
  } while (false);

  if (ctx != nullptr) {
    EVP_CIPHER_CTX_free(ctx);
  }
  return false;
}

} // namespace prochlo
