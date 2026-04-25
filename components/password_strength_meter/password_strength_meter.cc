/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/password_strength_meter/password_strength_meter.h"

#include <memory>
#include <string>
#include <utility>

#include "components/password_manager/core/browser/ui/weak_check_utility.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"

namespace password_strength_meter {

namespace {

class PasswordStrengthMeterImpl : public mojom::PasswordStrengthMeter {
 public:
  PasswordStrengthMeterImpl() = default;

  PasswordStrengthMeterImpl(const PasswordStrengthMeterImpl&) = delete;
  PasswordStrengthMeterImpl& operator=(const PasswordStrengthMeterImpl&) =
      delete;

  ~PasswordStrengthMeterImpl() override = default;

 private:
  void GetPasswordStrength(
      const std::string& password,
      mojom::PasswordStrengthMeter::GetPasswordStrengthCallback callback)
      override {
    std::move(callback).Run(password_manager::GetPasswordStrength(password));
  }
};

}  // namespace

void BindInterface(
    mojo::PendingReceiver<mojom::PasswordStrengthMeter> pending_receiver) {
  mojo::MakeSelfOwnedReceiver(std::make_unique<PasswordStrengthMeterImpl>(),
                              std::move(pending_receiver));
}

}  // namespace password_strength_meter
