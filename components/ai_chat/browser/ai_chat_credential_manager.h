/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_BROWSER_AI_CHAT_CREDENTIAL_MANAGER_H_
#define BRAVE_COMPONENTS_AI_CHAT_BROWSER_AI_CHAT_CREDENTIAL_MANAGER_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote_set.h"

namespace ai_chat {

// Interfaces with the SKUs SDK to provide APIs to check and fetch Leo
// premium credentials.
class AIChatCredentialManager {
 public:
  AIChatCredentialManager(
      base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
          skus_service_getter);

  AIChatCredentialManager(const AIChatCredentialManager&) = delete;
  AIChatCredentialManager& operator=(const AIChatCredentialManager&) = delete;
  ~AIChatCredentialManager();

  using UserHasValidPremiumCredentialCallback =
      base::OnceCallback<void(bool success)>;

  using FetchPremiumCredentialCallback = base::OnceCallback<void(
      absl::optional<std::string> credential_as_cookie)>;

  void UserHasValidPremiumCredential(
      UserHasValidPremiumCredentialCallback callback);

  void FetchPremiumCredential(FetchPremiumCredentialCallback callback);

 private:
  void EnsureMojoConnected();

  void OnMojoConnectionError();

  void OnCredentialSummary(UserHasValidPremiumCredentialCallback callback,
                           const std::string& domain,
                           const std::string& summary_string);

  void OnUserHasValidPremiumCredential(FetchPremiumCredentialCallback callback,
                                       bool result);

  void OnPrepareCredentialsPresentation(
      FetchPremiumCredentialCallback callback,
      const std::string& domain,
      const std::string& credential_as_cookie);

  base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
      skus_service_getter_;
  mojo::Remote<skus::mojom::SkusService> skus_service_;
  base::WeakPtrFactory<AIChatCredentialManager> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_BROWSER_AI_CHAT_CREDENTIAL_MANAGER_H_
