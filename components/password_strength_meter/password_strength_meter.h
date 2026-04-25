/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PASSWORD_STRENGTH_METER_PASSWORD_STRENGTH_METER_H_
#define BRAVE_COMPONENTS_PASSWORD_STRENGTH_METER_PASSWORD_STRENGTH_METER_H_

#include "brave/components/password_strength_meter/password_strength_meter.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace password_strength_meter {

void BindInterface(
    mojo::PendingReceiver<mojom::PasswordStrengthMeter> pending_receiver);

}  // namespace password_strength_meter

#endif  // BRAVE_COMPONENTS_PASSWORD_STRENGTH_METER_PASSWORD_STRENGTH_METER_H_
