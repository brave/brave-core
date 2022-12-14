// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PERMISSIONS_GOOGLE_SIGN_IN_GOOGLE_SIGN_IN_PERMISSION_UTIL_H_
#define BRAVE_COMPONENTS_PERMISSIONS_GOOGLE_SIGN_IN_GOOGLE_SIGN_IN_PERMISSION_UTIL_H_

#include <string>
#include <vector>

#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/permission_controller_delegate.h"
#include "content/public/browser/web_contents.h"
#include "services/network/public/cpp/resource_request.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

namespace permissions {

constexpr char kGoogleAuthPattern[] =
    "https://accounts.google.com/o/oauth2/auth/*";
constexpr char kFirebasePattern[] = "https://[*.]firebaseapp.com/__/auth/*";

ContentSettingsPattern GetFirebaseAuthPattern();
ContentSettingsPattern GetGoogleAuthPattern();

bool IsGoogleAuthRelatedRequest(const GURL& request_url,
                                const GURL& request_initiator_url);
// Check if feature flag is enabled.
bool IsGoogleSignInFeatureEnabled();
// Check if user preference is enabled (default ON). Caller should make sure
// feature flag is enabled.
bool IsGoogleSignInPrefEnabled(PrefService* prefs);
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

}  // namespace permissions

#endif  // BRAVE_COMPONENTS_PERMISSIONS_GOOGLE_SIGN_IN_GOOGLE_SIGN_IN_PERMISSION_UTIL_H_
