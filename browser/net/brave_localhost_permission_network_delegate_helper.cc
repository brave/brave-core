/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_localhost_permission_network_delegate_helper.h"

#include <array>
#include <iostream>
#include <string>
#include <vector>
#include <utility>

#include "base/containers/fixed_flat_set.h"
#include "base/strings/string_split.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_shields/browser/brave_farbling_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/language/core/browser/language_prefs.h"
#include "components/language/core/browser/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "net/base/net_errors.h"

namespace brave {

int OnBeforeURLRequest_LocalhostPermissionWork(
    const ResponseCallback& next_callback,
    std::scoped_refptr<BraveRequestInfo> ctx) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Don't try to overwrite an already set URL by another delegate (adblock/tp)
  if (!ctx->new_url_spec.empty() || !ctx->mock_data_url.empty()) {
    return net::OK;
  }

  Profile* profile = Profile::FromBrowserContext(ctx->browser_context);
  DCHECK(profile);
  HostContentSettingsMap* content_settings =
      HostContentSettingsMapFactory::GetForProfile(profile);
  DCHECK(content_settings);

  auto* contents =
      content::WebContents::FromFrameTreeNodeId(ctx->frame_tree_node_id);
  if (!contents) {
    return net::OK;
  }

  const auto& request_initiator_url = ctx->initiator_url;
  const auto& request_url = ctx->request_url;

  if (!request_initiator_url.is_valid() || !request_url.is_valid() ||
      !net::IsLocalhost(request_url)) {
    return net::OK;
  }

  std::cerr << "XYZZY request_initiator_url is : " << request_initiator_url
            << "\n";
  std::cerr << "XYZZY request URL is : " << request_url << "\n";

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
      return net::ERR_ABORTED;
    }

    case blink::mojom::PermissionStatus::ASK: {
      CreateLocalhostPermissionRequest(
          permission_controller, contents->GetPrimaryMainFrame(),
          request_initiator_url, next_callback,
          base::BindOnce(&OnPermissionRequestStatus, next_callback,
                         contents->GetController().GetPendingEntry(), contents,
                         request_initiator_url));
      return net::ERR_IO_PENDING;
    }
  }

  return net::OK;
}

void CreateLocalhostPermissionRequest(
    content::PermissionControllerDelegate* permission_controller,
    content::RenderFrameHost* rfh,
    const GURL& request_initiator_url,
    base::OnceCallback<void(const std::vector<blink::mojom::PermissionStatus>&)>
        callback) {
  if (!rfh->IsDocumentOnLoadCompletedInMainFrame()) {
    return;
  }
  permission_controller->RequestPermissionsFromCurrentDocument(
      {blink::PermissionType::BRAVE_LOCALHOST_ACCESS}, rfh, true,
      std::move(callback));
}

void OnPermissionRequestStatus(
    const brave::ResponseCallback& next_callback,
    content::NavigationEntry* pending_entry,
    content::WebContents* contents,
    const GURL& request_initiator_url,
    const std::vector<blink::mojom::PermissionStatus>& permission_statuses) {
  DCHECK_EQ(1u, permission_statuses.size());
  // Check if current pending navigation is the one we started out with.
  // TODO(ssahib): consider deleting?
  if (pending_entry != contents->GetController().GetPendingEntry()) {
    return;
  }
  // Now that we have complete the permission request, resume navigation.

  if (!next_callback.is_null() &&
      permission_statuses[0] == blink::mojom::PermissionStatus::GRANTED) {
    next_callback.Run();
  }

  // TODO(ssahib): What if permission_statuses[0] ==
  // blink::mojom::PermissionStatus::DENIED?
  std::cerr << "XYZZY permission was denied for " << request_initiator_url
            << std::endl;
}

}  // namespace brave
