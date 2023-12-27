/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_CREDENTIAL_MANAGER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_CREDENTIAL_MANAGER_H_

#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
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
      base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
          skus_service_getter,
      PrefService* prefs_service);

  AIChatCredentialManager(const AIChatCredentialManager&) = delete;
  AIChatCredentialManager& operator=(const AIChatCredentialManager&) = delete;
  ~AIChatCredentialManager();

  void GetPremiumStatus(mojom::PageHandler::GetPremiumStatusCallback callback);

  void FetchPremiumCredential(
      base::OnceCallback<void(std::optional<CredentialCacheEntry> credential)>
          callback);

  void PutCredentialInCache(CredentialCacheEntry credential);

 private:
  bool EnsureMojoConnected();

  void OnMojoConnectionError();

  void OnCredentialSummary(
      mojom::PageHandler::GetPremiumStatusCallback callback,
      const std::string& domain,
      const bool credential_in_cache,
      const std::string& summary_string);

  void OnGetPremiumStatus(
      base::OnceCallback<void(std::optional<CredentialCacheEntry> credential)>
          callback,
      mojom::PremiumStatus,
      mojom::PremiumInfoPtr);

  void OnPrepareCredentialsPresentation(
      base::OnceCallback<void(std::optional<CredentialCacheEntry> credential)>
          callback,
      const std::string& domain,
      const std::string& credential_as_cookie);

  base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
      skus_service_getter_;
  mojo::Remote<skus::mojom::SkusService> skus_service_;
  raw_ptr<PrefService> prefs_service_ = nullptr;

  base::WeakPtrFactory<AIChatCredentialManager> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_CREDENTIAL_MANAGER_H_
