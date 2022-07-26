/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_WALLET_PERMISSION_CONTEXT_H_
#define BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_WALLET_PERMISSION_CONTEXT_H_

#include <map>
#include <queue>
#include <string>
#include <vector>

#include "components/permissions/permission_context_base.h"
#include "components/permissions/permission_request_id.h"
#include "components/permissions/request_type.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"
#include "url/origin.h"

class GURL;

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace permissions {

class BraveWalletPermissionContext : public PermissionContextBase {
 public:
  // using PermissionContextBase::RequestPermission;
  explicit BraveWalletPermissionContext(
      content::BrowserContext* browser_context,
      ContentSettingsType content_settings_type);
  ~BraveWalletPermissionContext() override;

  BraveWalletPermissionContext(const BraveWalletPermissionContext&) = delete;
  BraveWalletPermissionContext& operator=(const BraveWalletPermissionContext&) =
      delete;

  /**
   * This is called by PermissionManager::RequestPermissions, for each
   * permission request ID, we will parse the requesting_frame URL to get the
   * ethereum address list to be used for each sub-request. Each sub-request
   * will then consume one address from the saved list and call
   * PermissionContextBase::RequestPermission with it.
   */
  void RequestPermission(const PermissionRequestID& id,
                         const GURL& requesting_frame,
                         bool user_gesture,
                         BrowserPermissionCallback callback) override;

  static void RequestPermissions(
      blink::PermissionType permission,
      content::RenderFrameHost* rfh,
      const std::vector<std::string>& addresses,
      base::OnceCallback<
          void(const std::vector<blink::mojom::PermissionStatus>&)> callback);
  static bool HasRequestsInProgress(content::RenderFrameHost* rfh,
                                    permissions::RequestType request_type);

  static void AcceptOrCancel(const std::vector<std::string>& accounts,
                             content::WebContents* web_contents);
  static void Cancel(content::WebContents* web_contents);

  static void GetAllowedAccounts(
      blink::PermissionType permission,
      content::RenderFrameHost* rfh,
      const std::vector<std::string>& addresses,
      base::OnceCallback<void(bool, const std::vector<std::string>&)> callback);

  static bool AddPermission(blink::PermissionType permission,
                            content::BrowserContext* context,
                            const url::Origin& origin,
                            const std::string& account);
  static bool HasPermission(blink::PermissionType permission,
                            content::BrowserContext* context,
                            const url::Origin& origin,
                            const std::string& account,
                            bool* has_permission);
  static bool ResetPermission(blink::PermissionType permission,
                              content::BrowserContext* context,
                              const url::Origin& origin,
                              const std::string& account);

  static std::vector<std::string> GetWebSitesWithPermission(
      blink::PermissionType permission,
      content::BrowserContext* context);
  static bool ResetWebSitePermission(blink::PermissionType permission,
                                     content::BrowserContext* context,
                                     const std::string& formed_website);

 protected:
  bool IsRestrictedToSecureOrigins() const override;

 private:
  std::map<std::string, std::queue<std::string>> request_address_queues_;
};

}  // namespace permissions

#endif  // BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_WALLET_PERMISSION_CONTEXT_H_
