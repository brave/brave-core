// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PERMISSIONS_GOOGLE_SIGN_IN_GOOGLE_SIGN_IN_PERMISSION_THROTTLE_H_
#define BRAVE_COMPONENTS_PERMISSIONS_GOOGLE_SIGN_IN_GOOGLE_SIGN_IN_PERMISSION_THROTTLE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"

namespace permissions {

class GoogleSignInPermissionThrottle : public blink::URLLoaderThrottle {
 public:
  GoogleSignInPermissionThrottle(
      const content::WebContents::Getter wc_getter,
      scoped_refptr<HostContentSettingsMap> settings_map);
  ~GoogleSignInPermissionThrottle() override;

  static std::unique_ptr<blink::URLLoaderThrottle> MaybeCreateThrottleFor(
      const network::ResourceRequest& request,
      const content::WebContents::Getter wc_getter,
      HostContentSettingsMap* settings_map);

  GoogleSignInPermissionThrottle(const GoogleSignInPermissionThrottle&) =
      delete;
  GoogleSignInPermissionThrottle& operator=(
      const GoogleSignInPermissionThrottle&) = delete;

  // Implements blink::URLLoaderThrottle:
  void DetachFromCurrentSequence() override;
  void WillStartRequest(network::ResourceRequest* request,
                        bool* defer) override;

 private:
  const content::WebContents::Getter wc_getter_;
  scoped_refptr<HostContentSettingsMap> settings_map_;
  base::WeakPtrFactory<GoogleSignInPermissionThrottle> weak_factory_{this};
};

}  // namespace permissions

#endif  // BRAVE_COMPONENTS_PERMISSIONS_GOOGLE_SIGN_IN_GOOGLE_SIGN_IN_PERMISSION_THROTTLE_H_
