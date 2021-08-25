/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_ETHEREUM_PERMISSION_CONTEXT_H_
#define BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_ETHEREUM_PERMISSION_CONTEXT_H_

#include <map>
#include <queue>
#include <string>
#include <vector>

#include "components/permissions/permission_context_base.h"
#include "components/permissions/permission_request_id.h"

class GURL;

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace permissions {

class BraveEthereumPermissionContext : public PermissionContextBase {
 public:
  // using PermissionContextBase::RequestPermission;
  explicit BraveEthereumPermissionContext(
      content::BrowserContext* browser_context);
  ~BraveEthereumPermissionContext() override;

  BraveEthereumPermissionContext(const BraveEthereumPermissionContext&) =
      delete;
  BraveEthereumPermissionContext& operator=(
      const BraveEthereumPermissionContext&) = delete;

  /**
   * This is called by PermissionManager::RequestPermissions, for each
   * permission request ID, we will parse the requesting_frame URL to get the
   * ethereum address list to be used for each sub-request. Each sub-request
   * will then consume one address from the saved list and call
   * PermissionContextBase::RequestPermission with it.
   */
  void RequestPermission(content::WebContents* web_contents,
                         const PermissionRequestID& id,
                         const GURL& requesting_frame,
                         bool user_gesture,
                         BrowserPermissionCallback callback) override;

  static void RequestPermissions(
      content::RenderFrameHost* rfh,
      const std::vector<std::string>& addresses,
      base::OnceCallback<void(const std::vector<ContentSetting>&)> callback);

  static void AcceptOrCancel(const std::vector<std::string>& accounts,
                             content::WebContents* web_contents);
  static void Cancel(content::WebContents* web_contents);

  static void GetAllowedAccounts(
      content::RenderFrameHost* rfh,
      const std::vector<std::string>& addresses,
      base::OnceCallback<void(bool, const std::vector<std::string>&)> callback);

 protected:
  bool IsRestrictedToSecureOrigins() const override;

 private:
  std::map<std::string, std::queue<std::string>> request_address_queues_;
};

}  // namespace permissions

#endif  // BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_ETHEREUM_PERMISSION_CONTEXT_H_
