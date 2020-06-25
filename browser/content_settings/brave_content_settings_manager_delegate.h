/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_CONTENT_SETTINGS_BRAVE_CONTENT_SETTINGS_MANAGER_DELEGATE_H_
#define BRAVE_BROWSER_CONTENT_SETTINGS_BRAVE_CONTENT_SETTINGS_MANAGER_DELEGATE_H_

#include <memory>

#include "chrome/browser/content_settings/content_settings_manager_delegate.h"

namespace chrome {

class BraveContentSettingsManagerDelegate
    : public ContentSettingsManagerDelegate {
 public:
  ~BraveContentSettingsManagerDelegate() override;

 private:
  // content_settings::ContentSettingsManagerImpl::Delegate:
  bool AllowStorageAccess(
      int render_process_id,
      int render_frame_id,
      content_settings::mojom::ContentSettingsManager::StorageType storage_type,
      const GURL& url,
      bool allowed,
      base::OnceCallback<void(bool)>* callback) override;
  std::unique_ptr<Delegate> Clone() override;
};

}  // namespace chrome

#endif  // BRAVE_BROWSER_CONTENT_SETTINGS_BRAVE_CONTENT_SETTINGS_MANAGER_DELEGATE_H_
