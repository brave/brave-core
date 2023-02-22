/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_localhost_permission_network_delegate_helper.h"

#include <array>
#include <string>
#include <utility>
#include <vector>

#include "brave/browser/brave_browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/permission_controller_delegate.h"
#include "content/public/browser/web_contents.h"
#include "net/base/net_errors.h"
#include "net/base/url_util.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"

namespace brave {

void OnPermissionRequestStatus(
    content::WebContents* contents,
    const GURL& request_initiator_url,
    const std::vector<blink::mojom::PermissionStatus>& permission_statuses) {
  DCHECK_EQ(1u, permission_statuses.size());
  if (contents &&
      permission_statuses[0] == blink::mojom::PermissionStatus::GRANTED) {
    contents->GetController().Reload(content::ReloadType::NORMAL, true);
  }
}

bool IsLocalhostRequest(const GURL& request_url,
                        const GURL& request_initiator_url) {
  return request_initiator_url.is_valid() && request_url.is_valid() &&
         net::IsLocalhost(request_url) &&
         !net::IsLocalhost(request_initiator_url);
}

int OnBeforeURLRequest_LocalhostPermissionWork(
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
      
  auto* contents =
      content::WebContents::FromFrameTreeNodeId(ctx->frame_tree_node_id);
  if (!contents) {
    return net::OK;
  }

  const auto& request_initiator_url = ctx->initiator_url;
  const auto& request_url = ctx->request_url;

  if (!IsLocalhostRequest(request_url, request_initiator_url)) {
    return net::OK;
  }

  content::PermissionControllerDelegate* permission_controller =
      contents->GetBrowserContext()->GetPermissionControllerDelegate();
  auto current_status =
      permission_controller->GetPermissionStatusForCurrentDocument(
          blink::PermissionType::BRAVE_LOCALHOST_ACCESS,
          contents->GetPrimaryMainFrame());

  switch (current_status) {
    case blink::mojom::PermissionStatus::GRANTED: {
      return net::OK;
    }

    case blink::mojom::PermissionStatus::DENIED: {
      return net::ERR_ACCESS_DENIED;
    }

    case blink::mojom::PermissionStatus::ASK: {
      permission_controller->RequestPermissionsFromCurrentDocument(
          {blink::PermissionType::BRAVE_LOCALHOST_ACCESS},
          contents->GetPrimaryMainFrame(), true,
          base::BindOnce(&OnPermissionRequestStatus, contents,
                         request_initiator_url));

      return net::ERR_ACCESS_DENIED;
    }
  }

  return net::OK;
}

}  // namespace brave
