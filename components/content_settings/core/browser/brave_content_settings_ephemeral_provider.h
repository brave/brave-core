/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_EPHEMERAL_PROVIDER_H_
#define BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_EPHEMERAL_PROVIDER_H_

#include <memory>

#include "components/content_settings/core/browser/content_settings_ephemeral_provider.h"

namespace content_settings {

// See the comments of BravePrefProvider.
class BraveEphemeralProvider : public EphemeralProvider {
 public:
  using EphemeralProvider::EphemeralProvider;
  ~BraveEphemeralProvider() override {}

  // Method for clearing flash plugins type.
  void ClearFlashPluginContentSettings();

 private:
  // EphemeralProvider overrides:
  bool SetWebsiteSetting(const ContentSettingsPattern& primary_pattern,
                         const ContentSettingsPattern& secondary_pattern,
                         ContentSettingsType content_type,
                         const ResourceIdentifier& resource_identifier,
                         std::unique_ptr<base::Value>&& value) override;

  DISALLOW_COPY_AND_ASSIGN(BraveEphemeralProvider);
};

}  // namespace content_settings

#endif  // BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_EPHEMERAL_PROVIDER_H_
