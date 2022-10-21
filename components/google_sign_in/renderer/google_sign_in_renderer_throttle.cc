/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/google_sign_in/renderer/google_sign_in_renderer_throttle.h"

#include <iostream>
#include <utility>
#include <vector>

#include "extensions/common/url_pattern.h"

#include "brave/components/permissions/brave_permission_manager.h"
#include "components/permissions/permission_manager.h"
#include "components/permissions/permission_request.h"
#include "components/permissions/permission_request_id.h"
#include "components/permissions/permission_request_manager.h"
#include "components/permissions/permissions_client.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"

#include "base/feature_list.h"
#include "content/public/renderer/render_frame.h"
#include "services/network/public/cpp/resource_request.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"
#include "third_party/blink/public/mojom/fetch/fetch_api_request.mojom-shared.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "url/gurl.h"
#include "url/origin.h"

using blink::URLLoaderThrottle;

namespace google_sign_in {

namespace {
constexpr char kGoogleAuthPattern[] = "https://accounts.google.com/*";
// constexpr char kFirebaseContentSettingsPattern[] =
//     "https://[*.]firebaseapp.com/*";
constexpr char kFirebaseUrlPattern[] = "https://*.firebaseapp.com/*";
}  // namespace

GoogleSignInRendererThrottle::GoogleSignInRendererThrottle() = default;

bool IsGoogleAuthUrl(const GURL& gurl) {
  static const std::vector<URLPattern> auth_login_patterns({
      URLPattern(URLPattern::SCHEME_HTTPS, kGoogleAuthPattern),
      URLPattern(URLPattern::SCHEME_HTTPS, kFirebaseUrlPattern),
  });
  return std::any_of(
      auth_login_patterns.begin(), auth_login_patterns.end(),
      [&gurl](URLPattern pattern) { return pattern.MatchesURL(gurl); });
}

std::unique_ptr<blink::URLLoaderThrottle>
GoogleSignInRendererThrottle::MaybeCreateThrottleFor(
    int render_frame_id,
    const blink::WebURLRequest& request) {
  const GURL request_url = static_cast<GURL>(request.Url());
  const auto request_initiator_url =
      static_cast<url::Origin>(request.RequestorOrigin()).GetURL();

  if (!request_initiator_url.is_valid() || !request_url.is_valid()) {
    return nullptr;
  }
  if (!IsGoogleAuthUrl(request_url) || IsGoogleAuthUrl(request_initiator_url)) {
    // We don't want to prompt the user to add a permission for
    // accounts.google.com to access accounts.google.com!
    return nullptr;
  }
  std::cout << "In GoogleSignInRendererThrottle::MaybeCreateThrottleFor"
            << "\n";
  std::cout << "Request url is: " << request_url << "\n";
  std::cout << "RequestContextType is "
            << static_cast<int>(request.GetRequestContext()) << "\n";
  std::cout << "request_originator_url is: " << request_initiator_url << "\n";

  content::RenderFrame* render_frame =
      content::RenderFrame::FromRoutingID(render_frame_id);
  if (!render_frame || !render_frame->IsMainFrame()) {
    std::cout << "Couldn't get render frame! Erroring out"
              << "\n";
    return nullptr;
  }

  std::cout << "Creating throttle!"
            << "\n";
  auto throttle = std::make_unique<GoogleSignInRendererThrottle>();

  return throttle;
}

GoogleSignInRendererThrottle::~GoogleSignInRendererThrottle() = default;


const char* GoogleSignInRendererThrottle::NameForLoggingWillStartRequest() {
  return "GoogleSignInRendererThrottle";
}

void GoogleSignInRendererThrottle::WillStartRequest(
    network::ResourceRequest* request,
    bool* defer) {
  std::cout << "In GoogleSignInRendererThrottle::WillStartRequest"
            << "\n";

  const GURL request_url = request->url;
  const GURL request_initiator_url = request->request_initiator->GetURL();

  DCHECK(request);
  DCHECK(request->request_initiator);
  DCHECK(!IsGoogleAuthUrl(request_initiator_url));
  DCHECK(IsGoogleAuthUrl(request_url));
  std::cout << "Cancelling request for request_url: " << request_url
            << "\nrequest_initiator_url: " << request_initiator_url << "\n";
  delegate_->CancelWithError(net::ERR_ABORTED);
}

}  // namespace google_sign_in
