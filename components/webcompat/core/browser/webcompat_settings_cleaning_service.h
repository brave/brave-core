// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_WEBCOMPAT_CORE_BROWSER_WEBCOMPAT_SETTINGS_CLEANING_SERVICE_H_
#define BRAVE_COMPONENTS_WEBCOMPAT_CORE_BROWSER_WEBCOMPAT_SETTINGS_CLEANING_SERVICE_H_

#include "brave/components/webcompat/content/browser/webcompat_exceptions_observer.h"
#include "components/content_settings/core/common/content_settings_types.h"

class HostContentSettingsMap;

namespace webcompat {

class WebcompatSettingsCleaningService : public WebcompatExceptionsObserver {
 public:
  WebcompatSettingsCleaningService();
  void Add(HostContentSettingsMap* settings_map);
  void OnWebcompatRulesUpdated() override;
  static WebcompatSettingsCleaningService* CreateInstance();
  static void AddSettingsMap(HostContentSettingsMap* settings_map);

 protected:
  ~WebcompatSettingsCleaningService() override;

 private:
  void RemoveRedundantWebcompatSettingsByType(
      HostContentSettingsMap* settings_map,
      ContentSettingsType settings_type);

  base::WeakPtrFactory<WebcompatSettingsCleaningService> weak_factory_{this};
};

}  // namespace webcompat

#endif  // BRAVE_COMPONENTS_WEBCOMPAT_CORE_BROWSER_WEBCOMPAT_SETTINGS_CLEANING_SERVICE_H_
