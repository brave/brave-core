/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_CONTENT_SETTINGS_BRAVE_CONTENT_SETTINGS_MANAGER_IMPL_H_
#define BRAVE_BROWSER_CONTENT_SETTINGS_BRAVE_CONTENT_SETTINGS_MANAGER_IMPL_H_

#include "chrome/browser/content_settings/content_settings_manager_impl.h"

class HostContentSettingsMap;

namespace chrome {

class BraveContentSettingsManagerImpl : public ContentSettingsManagerImpl {
 public:
  ~BraveContentSettingsManagerImpl() override;

  static void Create(
      content::RenderProcessHost* render_process_host,
      mojo::PendingReceiver<mojom::ContentSettingsManager> receiver);

  // mojom::ContentSettingsManager methods:
  void Clone(
      mojo::PendingReceiver<mojom::ContentSettingsManager> receiver) override;
  void AllowStorageAccess(int32_t render_frame_id,
                          StorageType storage_type,
                          const url::Origin& origin,
                          const GURL& site_for_cookies,
                          const url::Origin& top_frame_origin,
                          base::OnceCallback<void(bool)> callback) override;

 private:
  explicit BraveContentSettingsManagerImpl(
      content::RenderProcessHost* render_process_host);
  explicit BraveContentSettingsManagerImpl(
      const BraveContentSettingsManagerImpl& other);

  bool ShouldStoreState(int render_frame_id,
                        const url::Origin& origin,
                        const url::Origin& top_origin);

  HostContentSettingsMap* host_content_settings_map_;
};

}  // namespace chrome

#endif  // BRAVE_BROWSER_CONTENT_SETTINGS_BRAVE_CONTENT_SETTINGS_MANAGER_IMPL_H_
