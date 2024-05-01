/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/geolocation/brave_geolocation_permission_tab_helper.h"

#include <utility>

#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

BraveGeolocationPermissionTabHelper::BraveGeolocationPermissionTabHelper(
    content::WebContents* contents)
    : WebContentsObserver(contents),
      content::WebContentsUserData<BraveGeolocationPermissionTabHelper>(
          *contents),
      brave_geolocation_permission_receivers_(contents, this) {}

BraveGeolocationPermissionTabHelper::~BraveGeolocationPermissionTabHelper() =
    default;

// static
void BraveGeolocationPermissionTabHelper::BindBraveGeolocationPermission(
    mojo::PendingAssociatedReceiver<
        geolocation::mojom::BraveGeolocationPermission> receiver,
    content::RenderFrameHost* rfh) {
  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  if (!web_contents) {
    return;
  }

  auto* tab_helper =
      BraveGeolocationPermissionTabHelper::FromWebContents(web_contents);
  if (!tab_helper) {
    return;
  }
  tab_helper->brave_geolocation_permission_receivers_.Bind(rfh,
                                                           std::move(receiver));
}

void BraveGeolocationPermissionTabHelper::PrimaryPageChanged(
    content::Page& page) {
  enable_high_accuracy_ = false;
}

void BraveGeolocationPermissionTabHelper::SetEnableHighAccuracy(
    bool enable_high_accuracy) {
  enable_high_accuracy_ = enable_high_accuracy;
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveGeolocationPermissionTabHelper);
