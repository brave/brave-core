// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_E2EE_PROCESSOR_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_E2EE_PROCESSOR_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/engine/e2ee/lib.rs.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"

namespace api_request_helper {
class APIRequestHelper;
class APIRequestResult;
}  // namespace api_request_helper

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

class E2EEProcessor {
 public:
  using FetchModelAttestationCallback =
      base::OnceCallback<void(std::optional<mojom::APIError>)>;

  using ClientSecretKeyBox = rust::Box<ClientSecretKey>;
  using EncryptCallback = base::RepeatingCallback<std::optional<std::string>(
      const base::ListValue&)>;
  using DecryptCallback =
      base::RepeatingCallback<std::optional<std::string>(const std::string&)>;

  struct ClientKeyPair {
    ClientKeyPair(std::string public_key_hex, ClientSecretKeyBox secret_key);
    ~ClientKeyPair();

    std::string public_key_hex;
    ClientSecretKeyBox secret_key;
  };

  explicit E2EEProcessor(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  E2EEProcessor(const E2EEProcessor&) = delete;
  E2EEProcessor& operator=(const E2EEProcessor&) = delete;
  virtual ~E2EEProcessor();

 protected:
  // For use by test subclasses that do not need network functionality.
  E2EEProcessor();

 public:
  virtual void ClearCachedModelAttestations();

  // Fetches the model attestation from GET /v1/models/{model_name}/attestation.
  // Resolves immediately from cache if a valid entry exists. On success, caches
  // the result and invokes |callback| with nullopt. On failure, invokes
  // |callback| with the error.
  virtual void FetchModelAttestation(const std::string& model_name,
                                     FetchModelAttestationCallback callback);

  // Generates a new client keypair. Returns a hex-encoded public key
  // alongside an owned box containing the secret key.
  virtual ClientKeyPair GenerateClientKeyPair();

  // Returns a callback that JSON-serializes a ListValue, encrypts it with the
  // cached public key for |model_name|, and returns hex-encoded ciphertext.
  // Returns empty string on error.
  virtual EncryptCallback CreateEncryptCallback(const std::string& model_name);

  // Returns a callback that hex-decodes its input, decrypts it using |key|,
  // and returns the plaintext. Returns empty optional on error. The caller must
  // ensure |key| outlives the returned callback.
  virtual DecryptCallback CreateDecryptCallback(const ClientSecretKey* key);

  void SetAPIRequestHelperForTesting(
      std::unique_ptr<api_request_helper::APIRequestHelper> api_helper);

 private:
  struct Attestation {
    explicit Attestation(std::vector<uint8_t> model_public_key);
    ~Attestation();

    std::vector<uint8_t> model_public_key;
    base::TimeTicks cached_at;
  };

  void OnFetchModelAttestationComplete(
      const std::string& model_name,
      FetchModelAttestationCallback callback,
      api_request_helper::APIRequestResult result);

  absl::flat_hash_map<std::string, std::unique_ptr<Attestation>>
      attestation_cache_;
  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;

  base::WeakPtrFactory<E2EEProcessor> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_E2EE_PROCESSOR_H_
