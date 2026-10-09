/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PERMISSIONS_BRAVE_WALLET_PERMISSION_CONTEXT_H_
#define BRAVE_BROWSER_PERMISSIONS_BRAVE_WALLET_PERMISSION_CONTEXT_H_

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

// Drives the wallet connect prompt and reports permission status. The accounts
// an origin may actually see are held by BraveWalletAccountChooserContext; the
// BRAVE_ETHEREUM/SOLANA/CARDANO content settings this context is registered
// for serve only as ask/block guards, mirroring USB_GUARD.
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

  // Raises a single prompt for `origin`. The reply lists whichever of
  // `addresses` ended up granted, which the user chooses in the panel.
  static void RequestWalletPermissions(
      const std::vector<std::string>& addresses,
      blink::PermissionType permission,
      const url::Origin& origin,
      content::RenderFrameHost* rfh,
      RequestWalletPermissionsCallback callback);
  static bool HasRequestsInProgress(content::RenderFrameHost* rfh,
                                    permissions::RequestType request_type);
  // Grants `accounts` for the prompt's origin with the lifetime `option`
  // names, then resolves the pending request. An empty `accounts` cancels.
  static void AcceptOrCancel(
      const std::vector<std::string>& accounts,
      brave_wallet::mojom::PermissionLifetimeOption option,
      content::WebContents* web_contents);
  static void Cancel(content::WebContents* web_contents);

  static std::optional<std::vector<std::string>> GetAllowedAccounts(
      blink::PermissionType permission,
      content::RenderFrameHost* rfh,
      const std::vector<std::string>& addresses);

  // True when the origin's guard is blocked, which hides every grant it has.
  static bool IsPermissionDenied(blink::PermissionType permission,
                                 content::BrowserContext* context,
                                 const url::Origin& origin);

  // Grants without an expiry. Used by the Android connect flow and tests.
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
  static void ResetPermissionsForAccount(
      content::BrowserContext* context,
      ContentSettingsType content_settings_type,
      const std::string& account);
  static void ResetAllPermissions(content::BrowserContext* context);

  // Origin specs that hold at least one granted account.
  static std::vector<std::string> GetWebSitesWithPermission(
      blink::PermissionType permission,
      content::BrowserContext* context);
  static bool ResetWebSitePermission(blink::PermissionType permission,
                                     content::BrowserContext* context,
                                     const std::string& formed_website);

 protected:
  bool IsRestrictedToSecureOrigins() const override;

  // ContentSettingPermissionContextBase:
  //
  // Status is left to the base class, which reads the guard and so yields Ask
  // or Block. A single origin-scoped setting cannot express which accounts are
  // granted, and reporting Allow here would make PermissionContextBase resolve
  // a request without prompting, breaking "connect one more account".
  // RequestWalletPermissions short-circuits instead.
  void UpdateSetting(const PermissionRequestData& request_data,
                     const PermissionSetting& setting,
                     bool is_one_time) override;
};

}  // namespace permissions

#endif  // BRAVE_BROWSER_PERMISSIONS_BRAVE_WALLET_PERMISSION_CONTEXT_H_
