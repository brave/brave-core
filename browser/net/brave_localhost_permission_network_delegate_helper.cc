// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/net/brave_localhost_permission_network_delegate_helper.h"

#include <vector>

#include "brave/browser/brave_browser_process.h"
#include "brave/components/localhost_permission/localhost_permission_component.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/permission_controller.h"
#include "content/public/browser/web_contents.h"
#include "net/base/url_util.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"

namespace brave {

void OnPermissionRequestStatus(
    content::FrameTreeNodeId frame_tree_node_id,
    const std::vector<blink::mojom::PermissionStatus>& permission_statuses) {
  DCHECK_EQ(1u, permission_statuses.size());
  // Once permission status has been updated, reload the page.
  // We do this so as to let the user know that they should retry
  // an action. Also just makes state management easier.
  auto* contents =
      content::WebContents::FromFrameTreeNodeId(frame_tree_node_id);
  if (contents &&
      permission_statuses[0] == blink::mojom::PermissionStatus::GRANTED) {
    contents->GetController().Reload(content::ReloadType::NORMAL, true);
  }
}

bool IsLocalhostRequest(const GURL& request_url,
                        const GURL& request_initiator_url) {
  return net::IsLocalhost(request_url) &&
         !net::IsLocalhost(request_initiator_url);
}

// Assumes that the caller has verified that
// the request is valid and for a localhost subresource.
// If no WebContents, then we can't prompt for permission;
// use existing content setting.
int HandleLocalhostRequestsWithNoWebContents(
    const GURL& request_initiator_url,
    content::BrowserContext* browser_context) {
  auto* settings_map =
      HostContentSettingsMapFactory::GetForProfile(browser_context);
  auto setting_for_url = settings_map->GetContentSetting(
      request_initiator_url, GURL(),
      ContentSettingsType::BRAVE_LOCALHOST_ACCESS);
  switch (setting_for_url) {
    case ContentSetting::CONTENT_SETTING_ALLOW: {
      return net::OK;
    }
    default: {
      return net::ERR_ACCESS_DENIED;
    }
  }
}

int OnBeforeURLRequest_LocalhostPermissionWork(
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  // If feature is disabled, return.
  auto* localhost_permission_component =
      g_brave_browser_process->localhost_permission_component();
  if (!localhost_permission_component) {
    return net::OK;
  }

  // If request is already blocked by adblock, return.
  if (ctx->blocked_by == kAdBlocked) {
    return net::OK;
  }

  const bool is_web_socket_request = ctx->request_url.SchemeIsWSOrWSS();
  const bool is_valid_subresource_request =
      ctx->resource_type != blink::mojom::ResourceType::kMainFrame &&
      ctx->resource_type != BraveRequestInfo::kInvalidResourceType;

  // Only throttle valid subresource requests + WebSockets.
  // https://github.com/brave/brave-browser/issues/26302
  if (!is_web_socket_request && !is_valid_subresource_request) {
    return net::OK;
  }

  const auto& request_initiator_url = ctx->initiator_url;
  const auto& request_url = ctx->request_url;

  const bool is_request_url_valid =
      request_url.is_valid() && !request_url.is_empty();
  const bool is_request_initiator_url_valid =
      request_initiator_url.is_valid() && !request_initiator_url.is_empty() &&
      request_initiator_url.has_host();

  // If the following info isn't available, then there's not much we can do.
  if (!is_request_url_valid || !is_request_initiator_url_valid) {
    return net::OK;
  }

  // We don't want to block requests from extensions, because
  // we don't currently do that via adblock.
  if (request_initiator_url.SchemeIs(content_settings::kExtensionScheme)) {
    return net::OK;
  }

  if (!IsLocalhostRequest(request_url, request_initiator_url)) {
    return net::OK;
  }

  auto* contents =
      content::WebContents::FromFrameTreeNodeId(ctx->frame_tree_node_id);

  if (!contents) {
    return HandleLocalhostRequestsWithNoWebContents(request_initiator_url,
                                                    ctx->browser_context);
  }

  auto* permission_controller =
      contents->GetBrowserContext()->GetPermissionController();
  auto current_status =
      permission_controller->GetPermissionStatusForCurrentDocument(
          blink::PermissionType::BRAVE_LOCALHOST_ACCESS,
          /* rfh */ contents->GetPrimaryMainFrame());

  switch (current_status) {
    case blink::mojom::PermissionStatus::GRANTED: {
      return net::OK;
    }

    case blink::mojom::PermissionStatus::DENIED: {
      return net::ERR_ACCESS_DENIED;
    }

    case blink::mojom::PermissionStatus::ASK: {
      // Check if website is allowed to ask for permission.
      if (localhost_permission_component->CanAskForLocalhostPermission(
              request_initiator_url)) {
        permission_controller->RequestPermissionsFromCurrentDocument(
            /* rfh */ contents->GetPrimaryMainFrame(),
            content::PermissionRequestDescription(
                blink::PermissionType::BRAVE_LOCALHOST_ACCESS,
                /*user_gesture*/ true),
            base::BindOnce(&OnPermissionRequestStatus,
                           ctx->frame_tree_node_id));
      }
      return net::ERR_ACCESS_DENIED;
    }
  }
  return net::OK;
}

}  // namespace brave
