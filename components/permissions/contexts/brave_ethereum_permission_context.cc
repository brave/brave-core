/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/contexts/brave_ethereum_permission_context.h"

#include <utility>

#include "base/no_destructor.h"
#include "brave/components/brave_wallet/browser/ethereum_permission_utils.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/permissions/permission_request_id.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/mojom/permissions_policy/permissions_policy_feature.mojom.h"
#include "third_party/re2/src/re2/re2.h"

namespace {
constexpr char kAddrPattern[] = "addr%3D(0x[[:xdigit:]]{40})";
}  // namespace

namespace permissions {

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
  static const base::NoDestructor<re2::RE2> kAddrRegex(kAddrPattern);
  const std::string id_str = id.ToString();
  GURL requesting_origin = requesting_frame.GetOrigin();
  std::string origin;

  // Validate input.
  if (!requesting_origin.is_valid() ||
      !brave_wallet::ParseRequestingOrigin(requesting_origin, false, &origin,
                                           nullptr)) {
    GURL embedding_origin = web_contents->GetLastCommittedURL().GetOrigin();
    NotifyPermissionSet(id, requesting_origin, embedding_origin,
                        std::move(callback), /*persist=*/false,
                        CONTENT_SETTING_BLOCK, /*is_one_time=*/false);
    return;
  }

  // Parse address list from the requesting frame and save it to the map when
  // it is the first time seeing this request ID.
  if (request_address_queues_.find(id_str) == request_address_queues_.end()) {
    re2::StringPiece input(requesting_frame.spec());
    std::string match;

    request_address_queues_[id_str] = std::queue<std::string>();
    while (re2::RE2::FindAndConsume(&input, *kAddrRegex, &match)) {
      request_address_queues_[id_str].push(match);
    }
  }

  // Overwrite the requesting_frame URL for each sub-request with one address
  // at a time from the map.
  DCHECK(!request_address_queues_[id_str].empty());
  GURL sub_request_origin;
  brave_wallet::GetSubRequestOrigin(GURL(origin),
                                    request_address_queues_[id_str].front(),
                                    &sub_request_origin);
  request_address_queues_[id_str].pop();
  if (request_address_queues_[id_str].empty())
    request_address_queues_.erase(id_str);
  PermissionContextBase::RequestPermission(web_contents, id, sub_request_origin,
                                           user_gesture, std::move(callback));
}

}  // namespace permissions
