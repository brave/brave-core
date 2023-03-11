// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GOOGLE_SIGN_IN_PERMISSION_GOOGLE_SIGN_IN_PERMISSION_UTIL_H_
#define BRAVE_COMPONENTS_GOOGLE_SIGN_IN_PERMISSION_GOOGLE_SIGN_IN_PERMISSION_UTIL_H_

#include <string>
#include <vector>

#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "services/network/public/cpp/resource_request.h"
#include "third_party/blink/public/mojom/permissions/permission_status.mojom-shared.h"
#include "url/gurl.h"

namespace google_sign_in_permission {

const ContentSettingsPattern& GetFirebaseAuthPattern();
const ContentSettingsPattern& GetGoogleAuthPattern();

bool IsGoogleAuthRelatedRequest(const GURL& request_url,
                                const GURL& request_initiator_url);
// Check if feature flag is enabled.
bool IsGoogleSignInFeatureEnabled();
GURL GetRequestInitiatingUrlFromRequest(
    const network::ResourceRequest& request);
bool CanCreateWindow(content::RenderFrameHost* opener,
                     const GURL& opener_url,
                     const GURL& target_url);
bool GetPermissionAndMaybeCreatePrompt(
    content::WebContents* contents,
    const GURL& request_initiator_url,
    bool* defer,
    base::OnceCallback<void(const std::vector<blink::mojom::PermissionStatus>&)>
        permission_result_callback);

}  // namespace google_sign_in_permission

#endif  // BRAVE_COMPONENTS_GOOGLE_SIGN_IN_PERMISSION_GOOGLE_SIGN_IN_PERMISSION_UTIL_H_
