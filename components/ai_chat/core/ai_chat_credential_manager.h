/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_AI_CHAT_CREDENTIAL_MANAGER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_AI_CHAT_CREDENTIAL_MANAGER_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/common/mojom/ai_chat.mojom.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote_set.h"

class PrefService;

namespace ai_chat {

struct CredentialCacheEntry {
  std::string credential;
  base::Time expires_at;
};

// Interfaces with the SKUs SDK to provide APIs to check and fetch Leo
// premium credentials.
class AIChatCredentialManager {
 public:
  AIChatCredentialManager(
      mojo::PendingRemote<skus::mojom::SkusService> skus_service,
      PrefService* prefs_service);

  AIChatCredentialManager(const AIChatCredentialManager&) = delete;
  AIChatCredentialManager& operator=(const AIChatCredentialManager&) = delete;
  ~AIChatCredentialManager();

  void GetPremiumStatus(
      ai_chat::mojom::PageHandler::GetPremiumStatusCallback callback);

  void FetchPremiumCredential(
      base::OnceCallback<void(absl::optional<CredentialCacheEntry> credential)>
          callback);

  void PutCredentialInCache(CredentialCacheEntry credential);

 private:
  void OnCredentialSummary(
      ai_chat::mojom::PageHandler::GetPremiumStatusCallback callback,
      const std::string& domain,
      const std::string& summary_string);

  void OnGetPremiumStatus(
      base::OnceCallback<void(absl::optional<CredentialCacheEntry> credential)>
          callback,
      ai_chat::mojom::PremiumStatus);

  void OnPrepareCredentialsPresentation(
      base::OnceCallback<void(absl::optional<CredentialCacheEntry> credential)>
          callback,
      const std::string& domain,
      const std::string& credential_as_cookie);

  mojo::Remote<skus::mojom::SkusService> skus_service_;
  raw_ptr<PrefService> prefs_service_ = nullptr;

  base::WeakPtrFactory<AIChatCredentialManager> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_AI_CHAT_CREDENTIAL_MANAGER_H_
