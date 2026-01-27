/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_WALLET_PERMISSION_CONTEXT_H_
#define BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_WALLET_PERMISSION_CONTEXT_H_

#include <optional>
#include <string>
#include <vector>

#include "components/permissions/content_setting_permission_context_base.h"
#include "components/permissions/permission_request_id.h"
#include "components/permissions/request_type.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"
#include "url/origin.h"

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace brave_wallet::mojom {
enum class PermissionLifetimeOption;
}

namespace permissions {

class BraveWalletPermissionContext
    : public ContentSettingPermissionContextBase {
 public:
  using RequestWalletPermissionsCallback =
      base::OnceCallback<void(std::vector<std::string> allowed_addresses)>;

  explicit BraveWalletPermissionContext(
      content::BrowserContext* browser_context,
      ContentSettingsType content_settings_type);
  ~BraveWalletPermissionContext() override;

  BraveWalletPermissionContext(const BraveWalletPermissionContext&) = delete;
  BraveWalletPermissionContext& operator=(const BraveWalletPermissionContext&) =
      delete;

  static void RequestWalletPermissions(
      const std::vector<std::string>& addresses,
      blink::PermissionType permission,
      content::RenderFrameHost* rfh,
      RequestWalletPermissionsCallback callback);
  static bool HasRequestsInProgress(content::RenderFrameHost* rfh,
                                    permissions::RequestType request_type);
  static void AcceptOrCancel(
      const std::vector<std::string>& accounts,
      brave_wallet::mojom::PermissionLifetimeOption option,
      content::WebContents* web_contents);
  static void Cancel(content::WebContents* web_contents);

  static std::optional<std::vector<std::string>> GetAllowedAccounts(
      blink::PermissionType permission,
      content::RenderFrameHost* rfh,
      const std::vector<std::string>& addresses);

  // We will only check global setting and setting per origin since we won't
  // write block rule per address on an origin.
  static bool IsPermissionDenied(blink::PermissionType permission,
                                 content::BrowserContext* context,
                                 const url::Origin& origin);

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
  static void ResetAllPermissions(content::BrowserContext* context);

  static std::vector<std::string> GetWebSitesWithPermission(
      blink::PermissionType permission,
      content::BrowserContext* context);
  static bool ResetWebSitePermission(blink::PermissionType permission,
                                     content::BrowserContext* context,
                                     const std::string& formed_website);

 protected:
  bool IsRestrictedToSecureOrigins() const override;
};

}  // namespace permissions

#endif  // BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_WALLET_PERMISSION_CONTEXT_H_
