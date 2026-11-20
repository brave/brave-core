/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PRIVACY_SANDBOX_BRAVE_PRIVACY_SANDBOX_SETTINGS_H_
#define BRAVE_COMPONENTS_PRIVACY_SANDBOX_BRAVE_PRIVACY_SANDBOX_SETTINGS_H_

#include "components/privacy_sandbox/privacy_sandbox_settings.h"

class BravePrivacySandboxSettings
    : public privacy_sandbox::PrivacySandboxSettings {
 public:
  BravePrivacySandboxSettings();
  ~BravePrivacySandboxSettings() override;

  // PrivacySandboxSettings:
  bool IsEventReportingDestinationAttested(
      const url::Origin& destination_origin) const override;
};

#endif  // BRAVE_COMPONENTS_PRIVACY_SANDBOX_BRAVE_PRIVACY_SANDBOX_SETTINGS_H_
