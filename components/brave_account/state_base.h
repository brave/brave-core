/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_STATE_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_STATE_BASE_H_

#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

namespace brave_account {

// `StateBase` implements `mojom::Authentication`. Derived states override
// only the methods valid in their state; the rest inherit the default
// wrong-state response (client error with `kCalledInWrongState`).
//
// The `mojom::Authentication` methods that carry real logic are implemented by
// flow helpers (`Login`, `LogOut`, `Register`, ...), each a member of the state
// whose overrides it serves.
//
// `AddObserver()` is the one method that is not state-scoped; `StateBase`
// forwards it to the service via `AddObserverCallback` so observer
// subscriptions outlive any single state.
//
// The rules that govern pref writes, callback ordering, and response-callback
// `WeakPtr`s live on `FlowBase`, since the flows are what implement them.
class StateBase : public mojom::Authentication {
 public:
  StateBase(const StateBase&) = delete;
  StateBase& operator=(const StateBase&) = delete;

  void AddReceiver(mojo::PendingReceiver<mojom::Authentication> receiver);

  std::vector<mojo::PendingReceiver<mojom::Authentication>> TakeReceivers();

 protected:
  using AddObserverCallback = base::RepeatingCallback<void(
      mojo::PendingRemote<mojom::AuthenticationObserver>)>;

  explicit StateBase(AddObserverCallback add_observer);

  ~StateBase() override;

 private:
  void AddObserver(
      mojo::PendingRemote<mojom::AuthenticationObserver> observer) final;

  void ChangePasswordStep1(const std::string& email,
                           ChangePasswordStep1Callback callback) override;

  void ChangePasswordStep2(const std::string& code,
                           ChangePasswordStep2Callback callback) override;

  void ChangePasswordStep3(const std::string& blinded_message,
                           ChangePasswordStep3Callback callback) override;

  void ChangePasswordStep4(const std::string& serialized_record,
                           ChangePasswordStep4Callback callback) override;

  void GetServiceToken(mojom::Service service,
                       GetServiceTokenCallback callback) override;

  void LogOut() override;

  void LoginStep1(mojom::Service initiating_service,
                  const std::string& email,
                  const std::string& serialized_ke1,
                  LoginStep1Callback callback) override;

  void LoginStep2(const std::string& encrypted_login_token,
                  const std::string& client_mac,
                  LoginStep2Callback callback) override;

  void RegisterStep1(mojom::Service initiating_service,
                     const std::string& email,
                     const std::string& blinded_message,
                     RegisterStep1Callback callback) override;

  void RegisterStep2(const std::string& encrypted_verification_token,
                     const std::string& serialized_record,
                     RegisterStep2Callback callback) override;

  void RegisterStep3(const std::string& code,
                     RegisterStep3Callback callback) override;

  void ResetPasswordStep1(const std::string& email,
                          ResetPasswordStep1Callback callback) override;

  void ResetPasswordStep2(const std::string& code,
                          ResetPasswordStep2Callback callback) override;

  void ResetPasswordStep3(const std::string& blinded_message,
                          ResetPasswordStep3Callback callback) override;

  void ResetPasswordStep4(const std::string& serialized_record,
                          const std::string& email,
                          ResetPasswordStep4Callback callback) override;

  const AddObserverCallback add_observer_;
  mojo::ReceiverSet<mojom::Authentication> receivers_;
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_STATE_BASE_H_
