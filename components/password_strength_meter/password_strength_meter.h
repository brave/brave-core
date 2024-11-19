/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PASSWORD_STRENGTH_METER_PASSWORD_STRENGTH_METER_H_
#define BRAVE_COMPONENTS_PASSWORD_STRENGTH_METER_PASSWORD_STRENGTH_METER_H_

#include <string>

#include "base/component_export.h"

namespace password_strength_meter {

// Returns password strength on a scale from 0 to 100.
COMPONENT_EXPORT(PASSWORD_STRENGTH_METER)
int GetPasswordStrength(const std::string& password);

}  // namespace password_strength_meter

#endif  // BRAVE_COMPONENTS_PASSWORD_STRENGTH_METER_PASSWORD_STRENGTH_METER_H_
