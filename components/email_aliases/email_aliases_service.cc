/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/email_aliases/email_aliases_service.h"

#include <utility>

#include "base/check.h"
#include "base/feature_list.h"
#include "brave/components/email_aliases/email_aliases.mojom.h"
#include "brave/components/email_aliases/features.h"

namespace email_aliases {

EmailAliasesService::EmailAliasesService() {
  CHECK(base::FeatureList::IsEnabled(email_aliases::kEmailAliases));
}

EmailAliasesService::~EmailAliasesService() = default;

void EmailAliasesService::Shutdown() {
  receivers_.Clear();
  observers_.Clear();
}

void EmailAliasesService::BindInterface(
    mojo::PendingReceiver<mojom::EmailAliasesService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void EmailAliasesService::RequestAuthentication(
    const std::string& auth_email,
    RequestAuthenticationCallback callback) {
  // TODO: Implement authentication logic
  std::move(callback).Run("Not implemented");
}

void EmailAliasesService::CancelAuthenticationOrLogout(
    CancelAuthenticationOrLogoutCallback callback) {
  // TODO: Implement logout logic
  std::move(callback).Run();
}

void EmailAliasesService::GenerateAlias(GenerateAliasCallback callback) {
  // TODO: Implement alias generation logic
  mojom::GenerateAliasResultPtr result =
      mojom::GenerateAliasResult::NewErrorMessage("Not implemented");
  std::move(callback).Run(std::move(result));
}

void EmailAliasesService::UpdateAlias(const std::string& alias_email,
                                      const std::optional<std::string>& note,
                                      UpdateAliasCallback callback) {
  // TODO: Implement alias update logic
  std::move(callback).Run("Not implemented");
}

void EmailAliasesService::DeleteAlias(const std::string& alias_email,
                                      DeleteAliasCallback callback) {
  // TODO: Implement alias deletion logic
  std::move(callback).Run("Not implemented");
}

void EmailAliasesService::AddObserver(
    mojo::PendingRemote<mojom::EmailAliasesServiceObserver> observer) {
  observers_.Add(std::move(observer));
}

}  // namespace email_aliases
