/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/password_strength_meter/password_strength_meter.h"

#include <string>
#include <utility>

#include "base/no_destructor.h"
#include "components/password_manager/core/browser/ui/weak_check_utility.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

namespace password_strength_meter {

namespace {

class PasswordStrengthMeterHandler
    : public mojom::PasswordStrengthMeterHandler {
 public:
  static PasswordStrengthMeterHandler& GetInstance() {
    static base::NoDestructor<PasswordStrengthMeterHandler> instance;
    return *instance;
  }

  PasswordStrengthMeterHandler(const PasswordStrengthMeterHandler&) = delete;
  PasswordStrengthMeterHandler& operator=(const PasswordStrengthMeterHandler&) =
      delete;

  ~PasswordStrengthMeterHandler() override = default;

  void GetPasswordStrength(
      const std::string& password,
      mojom::PasswordStrengthMeterHandler::GetPasswordStrengthCallback callback)
      override {
    std::move(callback).Run(password_manager::GetPasswordStrength(password));
  }

  void Add(mojo::PendingReceiver<mojom::PasswordStrengthMeterHandler>
               pending_receiver) {
    receivers_.Add(this, std::move(pending_receiver));
  }

 private:
  friend class base::NoDestructor<PasswordStrengthMeterHandler>;

  PasswordStrengthMeterHandler() = default;

  mojo::ReceiverSet<mojom::PasswordStrengthMeterHandler> receivers_;
};

}  // namespace

void BindInterface(mojo::PendingReceiver<mojom::PasswordStrengthMeterHandler>
                       pending_receiver) {
  PasswordStrengthMeterHandler::GetInstance().Add(std::move(pending_receiver));
}

}  // namespace password_strength_meter
