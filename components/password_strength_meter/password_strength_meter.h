/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PASSWORD_STRENGTH_METER_PASSWORD_STRENGTH_METER_H_
#define BRAVE_COMPONENTS_PASSWORD_STRENGTH_METER_PASSWORD_STRENGTH_METER_H_

#include <string>

#include "brave/components/password_strength_meter/mojom/password_strength_meter.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace password_strength_meter {

class PasswordStrengthMeterHandler
    : public mojom::PasswordStrengthMeterHandler {
 public:
  explicit PasswordStrengthMeterHandler(
      mojo::PendingReceiver<mojom::PasswordStrengthMeterHandler> handler);
  ~PasswordStrengthMeterHandler() override;

  void GetPasswordStrength(
      const std::string& password,
      mojom::PasswordStrengthMeterHandler::GetPasswordStrengthCallback callback)
      override;

 private:
  mojo::Receiver<mojom::PasswordStrengthMeterHandler> handler_;
};

}  // namespace password_strength_meter

#endif  // BRAVE_COMPONENTS_PASSWORD_STRENGTH_METER_PASSWORD_STRENGTH_METER_H_
