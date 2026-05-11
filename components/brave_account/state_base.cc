/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/state_base.h"

#include <utility>

#include "base/check.h"
#include "brave/components/brave_account/encryption.h"
#include "brave/components/brave_account/state_internal.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_account {

using internal::MakeCalledInWrongStateError;

void StateBase::AddReceiver(
    mojo::PendingReceiver<mojom::Authentication> receiver) {
  receivers_.Add(this, std::move(receiver));
}

std::vector<mojo::PendingReceiver<mojom::Authentication>>
StateBase::TakeReceivers() {
  return receivers_.TakeReceivers();
}

StateBase::StateBase(
    AccountStatePrefs& account_state_prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const os_crypt_async::Encryptor& encryptor,
    AddObserverCallback add_observer)
    : account_state_prefs_(account_state_prefs),
      url_loader_factory_(std::move(url_loader_factory)),
      encryptor_(encryptor),
      add_observer_(std::move(add_observer)) {
  CHECK(url_loader_factory_);
  CHECK(add_observer_);
}

StateBase::~StateBase() = default;

std::string StateBase::Encrypt(const std::string& plain_text) const {
  return internal::Encrypt(*encryptor_, plain_text);
}

std::string StateBase::Decrypt(const std::string& base64) const {
  return internal::Decrypt(*encryptor_, base64);
}

void StateBase::AddObserver(
    mojo::PendingRemote<mojom::AuthenticationObserver> observer) {
  add_observer_.Run(std::move(observer));
}

void StateBase::RegisterInitialize(mojom::Service,
                                   const std::string&,
                                   const std::string&,
                                   RegisterInitializeCallback callback) {
  std::move(callback).Run(MakeCalledInWrongStateError<mojom::RegisterError>());
}

void StateBase::RegisterFinalize(const std::string&,
                                 const std::string&,
                                 RegisterFinalizeCallback callback) {
  std::move(callback).Run(MakeCalledInWrongStateError<mojom::RegisterError>());
}

void StateBase::RegisterVerify(const std::string&,
                               RegisterVerifyCallback callback) {
  std::move(callback).Run(MakeCalledInWrongStateError<mojom::RegisterError>());
}

void StateBase::ResendConfirmationEmail(
    ResendConfirmationEmailCallback callback) {
  std::move(callback).Run(
      MakeCalledInWrongStateError<mojom::ResendConfirmationEmailError>());
}

void StateBase::CancelRegistration() {}

void StateBase::LoginInitialize(mojom::Service,
                                const std::string&,
                                const std::string&,
                                LoginInitializeCallback callback) {
  std::move(callback).Run(MakeCalledInWrongStateError<mojom::LoginError>());
}

void StateBase::LoginFinalize(const std::string&,
                              const std::string&,
                              LoginFinalizeCallback callback) {
  std::move(callback).Run(MakeCalledInWrongStateError<mojom::LoginError>());
}

void StateBase::LogOut() {}

void StateBase::GetServiceToken(mojom::Service,
                                GetServiceTokenCallback callback) {
  std::move(callback).Run(
      MakeCalledInWrongStateError<mojom::GetServiceTokenError>());
}

void StateBase::RemoveRequestHandle(
    std::list<endpoint_client::RequestHandle>::iterator slot) {
  in_flight_.erase(slot);
}

}  // namespace brave_account
