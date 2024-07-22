/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEBCOMPAT_CONTENT_BROWSER_WEBCOMPAT_EXCEPTIONS_SERVICE_H_
#define BRAVE_COMPONENTS_WEBCOMPAT_CONTENT_BROWSER_WEBCOMPAT_EXCEPTIONS_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/synchronization/lock.h"
#include "base/thread_annotations.h"
#include "base/values.h"
#include "brave/components/brave_component_updater/browser/local_data_files_observer.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"

namespace webcompat {

using content_settings::mojom::ContentSettingsType;

using PatternsByWebcompatTypeMap = base::flat_map<ContentSettingsType, std::vector<ContentSettingsPattern>>;

// The WebcompatExceptionsService loads a list of site-specific webcompat
// exceptions from the Brave Local Data component and provides these exceptions
// as needed. GetPatterns can be called by any thread, because
// HostContentSettingsMap requires it.
class WebcompatExceptionsService
    : public brave_component_updater::LocalDataFilesObserver {
 public:
  explicit WebcompatExceptionsService(
      brave_component_updater::LocalDataFilesService* local_data_files_service);

  // implementation of brave_component_updater::LocalDataFilesObserver
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

  ~WebcompatExceptionsService() override;
  static WebcompatExceptionsService* CreateInstance(
      brave_component_updater::LocalDataFilesService* local_data_files_service);
  static WebcompatExceptionsService* GetInstance();
  // Callable from any thread; needed for functions like
  // HostContentSettingsMap::GetContentSetting(...)
  std::vector<ContentSettingsPattern> GetPatterns(
      ContentSettingsType webcompat_type);
  void SetRulesForTesting(PatternsByWebcompatTypeMap patterns_by_webcompat_type);

 private:
  void LoadWebcompatExceptions(const base::FilePath& install_dir);
  // Use around accesses to |patterns_by_webcompat_type_| to guarantee
  // thread safety.
  void SetRules(PatternsByWebcompatTypeMap patterns_by_webcompat_type);
  base::Lock lock_;
  PatternsByWebcompatTypeMap patterns_by_webcompat_type_ GUARDED_BY(lock_);
  base::WeakPtrFactory<WebcompatExceptionsService> weak_factory_{this};
};

}  // namespace webcompat

#endif  // BRAVE_COMPONENTS_WEBCOMPAT_CONTENT_BROWSER_WEBCOMPAT_EXCEPTIONS_SERVICE_H_
