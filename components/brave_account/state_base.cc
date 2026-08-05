/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/state_base.h"

#include <utility>

#include "base/check.h"
#include "brave/components/brave_account/mojom/change_password.mojom.h"
#include "brave/components/brave_account/mojom/login.mojom.h"
#include "brave/components/brave_account/mojom/register.mojom.h"
#include "brave/components/brave_account/mojom/reset_password.mojom.h"
#include "brave/components/brave_account/state_internal.h"

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

StateBase::StateBase(AddObserverCallback add_observer)
    : add_observer_(std::move(add_observer)) {
  CHECK(add_observer_);
}

StateBase::~StateBase() = default;

void StateBase::AddObserver(
    mojo::PendingRemote<mojom::AuthenticationObserver> observer) {
  add_observer_.Run(std::move(observer));
}

void StateBase::ChangePasswordStep1(const std::string& email,
                                    ChangePasswordStep1Callback callback) {
  std::move(callback).Run(
      MakeCalledInWrongStateError<mojom::ChangePasswordError>());
}

void StateBase::ChangePasswordStep2(const std::string& code,
                                    ChangePasswordStep2Callback callback) {
  std::move(callback).Run(
      MakeCalledInWrongStateError<mojom::ChangePasswordError>());
}

void StateBase::ChangePasswordStep3(const std::string& blinded_message,
                                    ChangePasswordStep3Callback callback) {
  std::move(callback).Run(
      MakeCalledInWrongStateError<mojom::ChangePasswordError>());
}

void StateBase::ChangePasswordStep4(const std::string& serialized_record,
                                    ChangePasswordStep4Callback callback) {
  std::move(callback).Run(
      MakeCalledInWrongStateError<mojom::ChangePasswordError>());
}

void StateBase::GetServiceToken(mojom::Service,
                                GetServiceTokenCallback callback) {
  std::move(callback).Run(
      MakeCalledInWrongStateError<mojom::GetServiceTokenError>());
}

void StateBase::LogOut() {}

void StateBase::LoginStep1(mojom::Service initiating_service,
                           const std::string& email,
                           const std::string& serialized_ke1,
                           LoginStep1Callback callback) {
  std::move(callback).Run(MakeCalledInWrongStateError<mojom::LoginError>());
}

void StateBase::LoginStep2(const std::string& encrypted_login_token,
                           const std::string& client_mac,
                           LoginStep2Callback callback) {
  std::move(callback).Run(MakeCalledInWrongStateError<mojom::LoginError>());
}

void StateBase::RegisterStep1(mojom::Service initiating_service,
                              const std::string& email,
                              const std::string& blinded_message,
                              RegisterStep1Callback callback) {
  std::move(callback).Run(MakeCalledInWrongStateError<mojom::RegisterError>());
}

void StateBase::RegisterStep2(const std::string& encrypted_verification_token,
                              const std::string& serialized_record,
                              RegisterStep2Callback callback) {
  std::move(callback).Run(MakeCalledInWrongStateError<mojom::RegisterError>());
}

void StateBase::RegisterStep3(const std::string& code,
                              RegisterStep3Callback callback) {
  std::move(callback).Run(MakeCalledInWrongStateError<mojom::RegisterError>());
}

void StateBase::ResetPasswordStep1(const std::string& email,
                                   ResetPasswordStep1Callback callback) {
  std::move(callback).Run(
      MakeCalledInWrongStateError<mojom::ResetPasswordError>());
}

void StateBase::ResetPasswordStep2(const std::string& code,
                                   ResetPasswordStep2Callback callback) {
  std::move(callback).Run(
      MakeCalledInWrongStateError<mojom::ResetPasswordError>());
}

void StateBase::ResetPasswordStep3(const std::string& blinded_message,
                                   ResetPasswordStep3Callback callback) {
  std::move(callback).Run(
      MakeCalledInWrongStateError<mojom::ResetPasswordError>());
}

void StateBase::ResetPasswordStep4(const std::string& serialized_record,
                                   const std::string& email,
                                   ResetPasswordStep4Callback callback) {
  std::move(callback).Run(
      MakeCalledInWrongStateError<mojom::ResetPasswordError>());
}

}  // namespace brave_account
