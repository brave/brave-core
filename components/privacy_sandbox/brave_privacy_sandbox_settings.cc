/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/privacy_sandbox/brave_privacy_sandbox_settings.h"

BravePrivacySandboxSettings::BravePrivacySandboxSettings() = default;

BravePrivacySandboxSettings::~BravePrivacySandboxSettings() = default;

// PrivacySandboxSettings:

bool BravePrivacySandboxSettings::IsEventReportingDestinationAttested(
    const url::Origin& destination_origin) const {
  return false;
}
