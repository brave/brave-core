/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/contexts/brave_ethereum_permission_context.h"

#include <utility>

#include "brave/components/brave_wallet/browser/ethereum_permission_utils.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/permissions/permission_manager.h"
#include "components/permissions/permission_request.h"
#include "components/permissions/permission_request_id.h"
#include "components/permissions/permission_request_manager.h"
#include "components/permissions/permissions_client.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/mojom/permissions_policy/permissions_policy_feature.mojom.h"

namespace permissions {

namespace {

bool IsAccepted(PermissionRequest* request,
                const std::vector<std::string>& accounts) {
  for (const auto& account : accounts) {
    if (base::EndsWith(request->GetOrigin().host_piece(), account,
                       base::CompareCase::INSENSITIVE_ASCII)) {
      return true;
    }
  }

  return false;
}

}  // namespace

BraveEthereumPermissionContext::BraveEthereumPermissionContext(
    content::BrowserContext* browser_context)
    : PermissionContextBase(browser_context,
                            ContentSettingsType::BRAVE_ETHEREUM,
                            blink::mojom::PermissionsPolicyFeature::kNotFound) {
}

BraveEthereumPermissionContext::~BraveEthereumPermissionContext() = default;

bool BraveEthereumPermissionContext::IsRestrictedToSecureOrigins() const {
  return true;
}

void BraveEthereumPermissionContext::RequestPermission(
    content::WebContents* web_contents,
    const PermissionRequestID& id,
    const GURL& requesting_frame,
    bool user_gesture,
    BrowserPermissionCallback callback) {
  const std::string id_str = id.ToString();
  GURL requesting_origin = requesting_frame.GetOrigin();
  std::string origin;

  // Parse address list from the requesting frame and save it to the map when
  // it is the first time seeing this request ID.
  auto addr_queue_it = request_address_queues_.find(id_str);
  std::queue<std::string> address_queue;
  bool is_new_id = addr_queue_it == request_address_queues_.end();
  if (!brave_wallet::ParseRequestingOrigin(
          requesting_origin, &origin, is_new_id ? &address_queue : nullptr)) {
    GURL embedding_origin = web_contents->GetLastCommittedURL().GetOrigin();
    NotifyPermissionSet(id, requesting_origin, embedding_origin,
                        std::move(callback), /*persist=*/false,
                        CONTENT_SETTING_BLOCK, /*is_one_time=*/false);
    return;
  }
  if (is_new_id) {
    addr_queue_it =
        request_address_queues_.insert(std::make_pair(id_str, address_queue))
            .first;
  }

  // Overwrite the requesting_frame URL for each sub-request with one address
  // at a time from the map.
  auto& addr_queue = addr_queue_it->second;
  DCHECK(!addr_queue.empty());
  GURL sub_request_origin;
  brave_wallet::GetSubRequestOrigin(GURL(origin), addr_queue.front(),
                                    &sub_request_origin);
  addr_queue.pop();
  if (addr_queue.empty())
    request_address_queues_.erase(addr_queue_it);
  PermissionContextBase::RequestPermission(web_contents, id, sub_request_origin,
                                           user_gesture, std::move(callback));
}

// static
void BraveEthereumPermissionContext::AcceptOrCancel(
    const std::vector<std::string>& accounts,
    content::WebContents* web_contents) {
  PermissionRequestManager* manager =
      PermissionRequestManager::FromWebContents(web_contents);
  DCHECK(manager);

  std::vector<PermissionRequest*> allowed_requests;
  std::vector<PermissionRequest*> cancelled_requests;
  for (PermissionRequest* request : manager->Requests()) {
    if (IsAccepted(request, accounts)) {
      allowed_requests.push_back(request);
    } else {
      cancelled_requests.push_back(request);
    }
  }

  manager->AcceptDenyCancel(allowed_requests, std::vector<PermissionRequest*>(),
                            cancelled_requests);
}

// static
void BraveEthereumPermissionContext::Cancel(
    content::WebContents* web_contents) {
  PermissionRequestManager* manager =
      PermissionRequestManager::FromWebContents(web_contents);
  DCHECK(manager);

  // Dismiss all requests.
  manager->Closing();
}

// static
void BraveEthereumPermissionContext::RequestPermissions(
    content::RenderFrameHost* rfh,
    const std::vector<std::string>& addresses,
    base::OnceCallback<void(const std::vector<ContentSetting>&)> callback) {
  if (!rfh) {
    std::move(callback).Run(std::vector<ContentSetting>());
    return;
  }

  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  // Fail the request came from 3p origin.
  if (web_contents->GetMainFrame()->GetLastCommittedURL().GetOrigin() !=
      rfh->GetLastCommittedURL().GetOrigin()) {
    std::move(callback).Run(std::vector<ContentSetting>());
    return;
  }

  permissions::PermissionManager* permission_manager =
      permissions::PermissionsClient::Get()->GetPermissionManager(
          web_contents->GetBrowserContext());
  if (!permission_manager) {
    std::move(callback).Run(std::vector<ContentSetting>());
    return;
  }

  // To support ethereum permission to be per Ethereum account per site, we map
  // each account address to one ethereum permission request. And the requests
  // will have different origins which includes the address information. Here
  // we first get a concatenated origin to include information of all wallet
  // addresses, then adjust the origin of each request later in the process
  // because PermissionManager::RequestPermissions only accepts a single origin
  // parameter to be passes in.
  GURL origin;
  if (!brave_wallet::GetConcatOriginFromWalletAddresses(
          rfh->GetLastCommittedURL().GetOrigin(), addresses, &origin)) {
    std::move(callback).Run(std::vector<ContentSetting>());
    return;
  }

  std::vector<ContentSettingsType> types(addresses.size(),
                                         ContentSettingsType::BRAVE_ETHEREUM);
  permission_manager->RequestPermissions(types, rfh, origin,
                                         rfh->HasTransientUserActivation(),
                                         std::move(callback));
}

// static
void BraveEthereumPermissionContext::GetAllowedAccounts(
    content::RenderFrameHost* rfh,
    const std::vector<std::string>& addresses,
    base::OnceCallback<void(bool, const std::vector<std::string>&)> callback) {
  if (!rfh) {
    std::move(callback).Run(false, std::vector<std::string>());
    return;
  }

  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  // Fail the request came from 3p origin.
  if (web_contents->GetMainFrame()->GetLastCommittedURL().GetOrigin() !=
      rfh->GetLastCommittedURL().GetOrigin()) {
    std::move(callback).Run(false, std::vector<std::string>());
    return;
  }

  permissions::PermissionManager* permission_manager =
      permissions::PermissionsClient::Get()->GetPermissionManager(
          web_contents->GetBrowserContext());
  if (!permission_manager) {
    std::move(callback).Run(false, std::vector<std::string>());
    return;
  }

  std::vector<std::string> allowed_accounts;
  GURL origin = rfh->GetLastCommittedURL().GetOrigin();
  for (const auto& address : addresses) {
    GURL sub_request_origin;
    bool success =
        brave_wallet::GetSubRequestOrigin(origin, address, &sub_request_origin);
    DCHECK(success);

    PermissionResult result = permission_manager->GetPermissionStatusForFrame(
        ContentSettingsType::BRAVE_ETHEREUM, rfh, sub_request_origin);
    if (result.content_setting == CONTENT_SETTING_ALLOW) {
      allowed_accounts.push_back(address);
    }
  }

  std::move(callback).Run(true, allowed_accounts);
}

}  // namespace permissions
