/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_SETTINGS_SERVICE_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_SETTINGS_SERVICE_H_

#include "base/memory/raw_ref.h"
#include "brave/components/psst/common/psst_metadata_schema.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/origin.h"

namespace psst {

class PsstSettingsService : public KeyedService {
 public:
  explicit PsstSettingsService(
      HostContentSettingsMap& host_content_settings_map);
  ~PsstSettingsService() override;

  // Saves the PSST metadata for the (origin, user_id) pair with the given
  // details.
  void SetPsstWebsiteSettings(const url::Origin& origin,
                              ConsentStatus consent_status,
                              int script_version,
                              std::string_view user_id,
                              base::Value::List urls_to_skip);

  // Returns the PSST settings for the (origin, user_id) pair if exists
  std::optional<PsstWebsiteSettings> GetPsstWebsiteSettings(
      const url::Origin& origin,
      std::string_view user_id);

  void SetPsstWebsiteSettings(const url::Origin& origin,
                              PsstWebsiteSettings psst_metadata);

 private:
  const raw_ref<HostContentSettingsMap>
      host_content_settings_map_;  // NOT OWNED
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_SETTINGS_SERVICE_H_
