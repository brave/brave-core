/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/geolocation/brave_geolocation_permission_tab_helper.h"

#include <utility>

#include "base/check_is_test.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"

DEFINE_USER_DATA(BraveGeolocationPermissionTabHelper);

// static
BraveGeolocationPermissionTabHelper* BraveGeolocationPermissionTabHelper::From(
    tabs::TabInterface* tab) {
  return tab ? Get(tab->GetUnownedUserDataHost()) : nullptr;
}

BraveGeolocationPermissionTabHelper::BraveGeolocationPermissionTabHelper(
    tabs::TabInterface& tab)
    : tabs::ContentsObservingTabFeature(tab),
      brave_geolocation_permission_receivers_(
          std::make_unique<content::RenderFrameHostReceiverSet<
              geolocation::mojom::BraveGeolocationPermission>>(
              tab.GetContents(),
              this)),
      scoped_unowned_user_data_(tab.GetUnownedUserDataHost(), *this) {}

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

  auto* tab_helper = BraveGeolocationPermissionTabHelper::From(
      tabs::TabInterface::MaybeGetFromContents(web_contents));
  if (!tab_helper) {
    return;
  }
  tab_helper->brave_geolocation_permission_receivers_->Bind(
      rfh, std::move(receiver));
}

void BraveGeolocationPermissionTabHelper::SetEnableHighAccuracy(
    bool enable_high_accuracy) {
  enable_high_accuracy_ = enable_high_accuracy;
  if (enable_high_accuracy_callback_for_testing_) {
    CHECK_IS_TEST();
    enable_high_accuracy_callback_for_testing_.Run(enable_high_accuracy);
  }
}

void BraveGeolocationPermissionTabHelper::
    SetEnableHighAccuracyCallbackForTesting(
        base::RepeatingCallback<void(bool)> callback) {
  enable_high_accuracy_callback_for_testing_ = std::move(callback);
}

void BraveGeolocationPermissionTabHelper::PrimaryPageChanged(
    content::Page& page) {
  enable_high_accuracy_ = false;
}

void BraveGeolocationPermissionTabHelper::OnDiscardContents(
    tabs::TabInterface* tab,
    content::WebContents* old_contents,
    content::WebContents* new_contents) {
  // The receivers and the high accuracy bit belong to the old contents' frames.
  brave_geolocation_permission_receivers_ =
      std::make_unique<content::RenderFrameHostReceiverSet<
          geolocation::mojom::BraveGeolocationPermission>>(new_contents, this);
  enable_high_accuracy_ = false;
  tabs::ContentsObservingTabFeature::OnDiscardContents(tab, old_contents,
                                                       new_contents);
}
