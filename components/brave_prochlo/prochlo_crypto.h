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

#ifndef BRAVE_COMPONENTS_BRAVE_PROCHLO_PROCHLO_CRYPTO_H_
#define BRAVE_COMPONENTS_BRAVE_PROCHLO_PROCHLO_CRYPTO_H_

#include <memory>
#include <string>

#include "third_party/boringssl/src/include/openssl/evp.h"

#include "brave/components/brave_prochlo/prochlo_data.h"

namespace prochlo {

class Crypto {
 public:
  // Load the public key for the Analyzer
  bool load_analyzer_key(const std::string& keyfile);

  // Load the public key for the Analyzer
  bool load_shuffler_key(const std::string& keyfile);

  Crypto();
  ~Crypto();

  bool EncryptForAnalyzer(const Prochlomation& prochlomation,
                          AnalyzerItem* analyzer_item);

  bool EncryptForShuffler(const PlainShufflerItem& plain_shuffler_item,
                          ShufflerItem* shuffler_item);

 private:
  friend class BraveProchloCrypto;

  // A convenient interface for encrypting between pairs of Prochlo messages
  // without producing a separate serialized copy of the input.
  class Encryption {
   public:
    explicit Encryption(EVP_PKEY* peer_key) : peer_key(peer_key) {}

    EVP_PKEY* ToPeerKey() { return peer_key; }

    virtual bool StreamDataForEncryption(EVP_CIPHER_CTX* ctx) = 0;
    virtual uint8_t* ToPublicKey() = 0;
    virtual uint8_t* ToNonce() = 0;
    virtual uint8_t* ToTag() = 0;
    virtual const char* TypeString() = 0;

    EVP_PKEY* peer_key;
  };

  class ProchlomationToAnalyzerItemEncryption : public Crypto::Encryption {
   public:
    ProchlomationToAnalyzerItemEncryption(EVP_PKEY* peer_key,
                                          const Prochlomation& prochlomation,
                                          AnalyzerItem* analyzer_item);
    const Prochlomation& prochlomation;
    AnalyzerItem* analyzer_item;

    uint8_t* ToPublicKey() override;
    const char* TypeString() override;
    uint8_t* ToNonce() override;
    uint8_t* ToTag() override;
    bool StreamDataForEncryption(EVP_CIPHER_CTX* ctx) override;
  };

  class PlainShufflerItemToShufflerItemEncryption : public Crypto::Encryption {
   public:
    PlainShufflerItemToShufflerItemEncryption(
        EVP_PKEY* peer_key,
        const PlainShufflerItem& plain_shuffler_item,
        ShufflerItem* shuffler_item);
    const PlainShufflerItem& plain_shuffler_item;
    ShufflerItem* shuffler_item;

    uint8_t* ToPublicKey() override;
    const char* TypeString() override;
    uint8_t* ToNonce() override;
    uint8_t* ToTag() override;
    bool StreamDataForEncryption(EVP_CIPHER_CTX* ctx) override;
  };

  bool MakeEncryptedMessage(Encryption* encryption);
  bool GenerateKeyPair(EVP_PKEY* peer_public_key,
                       EVP_PKEY** key_out,
                       uint8_t* binary_key);

  bool DeriveSecretSymmetricKey(EVP_PKEY* local_key,
                                EVP_PKEY* peer_public_key,
                                uint8_t* secret_key);
  bool Encrypt(const uint8_t* symmetric_key, Encryption* encryption);

  // Load a public key returning the structure
  EVP_PKEY* load_public_key(const std::string& keyfile);

  EVP_PKEY* public_shuffler_key_;
  EVP_PKEY* public_analyzer_key_;
};

}  // namespace prochlo

#endif  // BRAVE_COMPONENTS_BRAVE_PROCHLO_PROCHLO_CRYPTO_H_
