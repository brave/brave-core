/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EPHEMERAL_STORAGE_PUPPETEER_STORAGE_MANAGER_H_
#define BRAVE_BROWSER_EPHEMERAL_STORAGE_PUPPETEER_STORAGE_MANAGER_H_

#include "base/memory/raw_ptr.h"
#include "base/unguessable_token.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "url/origin.h"

namespace content {
class RenderFrameHost;
class WebContents;
}  // namespace content

namespace ephemeral_storage {

class EphemeralStorageService;

// Manages storage isolation for frames with puppeteer permissions
class PuppeteerStorageManager : public content::WebContentsObserver,
                               public content::WebContentsUserData<PuppeteerStorageManager> {
 public:
  explicit PuppeteerStorageManager(content::WebContents* web_contents);
  ~PuppeteerStorageManager() override;

  // Check and apply puppeteer storage isolation
  void CheckAndApplyPuppeteerStorage(content::RenderFrameHost* frame_host);

  // Grant puppeteer permission for testing/development
  void GrantPuppeteerPermissionForTesting(const url::Origin& origin);

 protected:
  // WebContentsObserver overrides
  void RenderFrameCreated(content::RenderFrameHost* render_frame_host) override;
  void DidFinishNavigation(content::NavigationHandle* navigation_handle) override;

 private:
  friend class content::WebContentsUserData<PuppeteerStorageManager>;

  bool HasPuppeteerPermission(const url::Origin& origin) const;
  void ConfigureStoragePartition(content::RenderFrameHost* frame_host,
                                const base::UnguessableToken& storage_token);

  raw_ptr<HostContentSettingsMap> host_content_settings_map_;
  raw_ptr<EphemeralStorageService> ephemeral_storage_service_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace ephemeral_storage

#endif  // BRAVE_BROWSER_EPHEMERAL_STORAGE_PUPPETEER_STORAGE_MANAGER_H_