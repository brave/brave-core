/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_NAVIGATION_H_
#define BRAVE_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_NAVIGATION_H_

#include "base/memory/raw_ref.h"
#include "brave/components/brave_origin/brave_origin_service.h"

class Profile;

namespace brave_origin {

// Delegate that opens the Origin settings page when a purchase is detected.
// Constructed with the Profile so it can locate the browser window.
class BraveOriginNavigationDelegate : public BraveOriginService::Delegate {
 public:
  explicit BraveOriginNavigationDelegate(Profile& profile);
  ~BraveOriginNavigationDelegate() override;

  void OpenOriginSettings() override;
  mojo::PendingRemote<skus::mojom::SkusService> GetSkusService() override;

 private:
  const raw_ref<Profile> profile_;
};

}  // namespace brave_origin

#endif  // BRAVE_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_NAVIGATION_H_
