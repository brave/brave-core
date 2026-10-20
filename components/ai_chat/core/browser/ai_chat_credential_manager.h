/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_CREDENTIAL_MANAGER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_CREDENTIAL_MANAGER_H_

#include <optional>
#include <string>
#include <vector>

#include "base/containers/circular_deque.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "build/build_config.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/bindings/remote_set.h"

class PrefService;
namespace mojo {
template <typename Interface>
class PendingRemote;
}  // namespace mojo

namespace ai_chat {

struct CredentialCacheEntry {
  std::string credential;
  base::Time expires_at;
};

// Interfaces with the SKUs SDK to provide APIs to check and fetch Leo
// premium credentials.
class AIChatCredentialManager {
 public:
  using FetchPremiumCredentialCallback =
      base::OnceCallback<void(std::optional<CredentialCacheEntry> credential)>;

  AIChatCredentialManager(
      base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
          skus_service_getter,
      PrefService* prefs_service);

  AIChatCredentialManager(const AIChatCredentialManager&) = delete;
  AIChatCredentialManager& operator=(const AIChatCredentialManager&) = delete;
  virtual ~AIChatCredentialManager();

  virtual void GetPremiumStatus(
      mojom::Service::GetPremiumStatusCallback callback);

  virtual void FetchPremiumCredential(FetchPremiumCredentialCallback callback);

  virtual void PutCredentialInCache(CredentialCacheEntry credential);

#if BUILDFLAG(IS_ANDROID)
  void CreateOrderFromReceipt(
      const std::string& purchase_token,
      const std::string& package,
      const std::string& subscription_id,
      skus::mojom::SkusService::CreateOrderFromReceiptCallback callback);
  void FetchOrderCredentials(
      const std::string& order_id,
      skus::mojom::SkusService::FetchOrderCredentialsCallback callback);
  void RefreshOrder(const std::string& order_id,
                    skus::mojom::SkusService::RefreshOrderCallback callback);
#endif

 private:
  bool EnsureMojoConnected();

  void OnMojoConnectionError();

  void OnCredentialSummary(const std::string& domain,
                           const bool credential_in_cache,
                           skus::mojom::SkusResultPtr summary_result);

  void CompleteGetPremiumStatus(mojom::PremiumStatus status,
                                mojom::PremiumInfoPtr info);

  void OnGetPremiumStatus(mojom::PremiumStatus status,
                          mojom::PremiumInfoPtr info);

  void OnPrepareCredentialsPresentation(
      const std::string& domain,
      skus::mojom::SkusResultPtr credential_as_cookie);

  void ProcessNextFetchPremiumCredential();

  void CompleteFetchPremiumCredential(
      std::optional<CredentialCacheEntry> credential);

  base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
      skus_service_getter_;
  mojo::Remote<skus::mojom::SkusService> skus_service_;
  raw_ptr<PrefService> prefs_service_ = nullptr;

  // Callbacks waiting on the in-flight GetPremiumStatus request. All of them
  // receive the same result.
  std::vector<mojom::Service::GetPremiumStatusCallback>
      pending_premium_status_callbacks_;

  // FetchPremiumCredential requests are processed one at a time since each
  // caller must receive a distinct credential. The front entry is the request
  // currently being processed.
  base::circular_deque<FetchPremiumCredentialCallback>
      pending_fetch_credential_callbacks_;

  base::WeakPtrFactory<AIChatCredentialManager> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_CREDENTIAL_MANAGER_H_
