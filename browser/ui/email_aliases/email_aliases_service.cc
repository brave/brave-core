/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/email_aliases/email_aliases_service.h"

#include <algorithm>
#include <set>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "brave/browser/ui/email_aliases/email_aliases.mojom.h"
#include "chrome/browser/profiles/profile.h"

namespace email_aliases {

EmailAliasesService::EmailAliasesService(Profile* profile) : profile_(profile) {
  DCHECK(profile_);
}

EmailAliasesService::~EmailAliasesService() = default;

void EmailAliasesService::Shutdown() {
  receivers_.Clear();
  observers_.clear();
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

void EmailAliasesService::CancelAuthenticationOrLogout() {
  // TODO: Implement logout logic
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
  observers_.push_back(
      mojo::Remote<mojom::EmailAliasesServiceObserver>(std::move(observer)));
}

void EmailAliasesService::RemoveObserver(
    mojo::PendingRemote<mojom::EmailAliasesServiceObserver> observer) {
  mojo::Remote<mojom::EmailAliasesServiceObserver> remote(std::move(observer));
  auto it = std::find_if(observers_.begin(), observers_.end(),
                         [&remote](const auto& observer) {
                           return observer.get() == remote.get();
                         });
  if (it != observers_.end()) {
    observers_.erase(it);
  }
}

}  // namespace email_aliases
