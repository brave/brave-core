/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_GEOLOCATION_BRAVE_GEOLOCATION_PERMISSION_TAB_HELPER_H_
#define BRAVE_BROWSER_UI_GEOLOCATION_BRAVE_GEOLOCATION_PERMISSION_TAB_HELPER_H_

#include <memory>

#include "base/functional/callback.h"
#include "brave/components/brave_geolocation_permission/common/brave_geolocation_permission.mojom.h"
#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"
#include "content/public/browser/render_frame_host_receiver_set.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"

namespace tabs {
class TabInterface;
}  // namespace tabs

// Records whether the page asked for high accuracy geolocation, so that the
// permission prompt can tell the user. Owned by BraveTabFeatures.
class BraveGeolocationPermissionTabHelper final
    : public tabs::ContentsObservingTabFeature,
      public geolocation::mojom::BraveGeolocationPermission {
 public:
  DECLARE_USER_DATA(BraveGeolocationPermissionTabHelper);

  static BraveGeolocationPermissionTabHelper* From(tabs::TabInterface* tab);

  explicit BraveGeolocationPermissionTabHelper(tabs::TabInterface& tab);
  ~BraveGeolocationPermissionTabHelper() override;

  static void BindBraveGeolocationPermission(
      mojo::PendingAssociatedReceiver<
          geolocation::mojom::BraveGeolocationPermission> receiver,
      content::RenderFrameHost* rfh);

  // geolocation::mojom::BraveGeolocationPermission overrides:
  void SetEnableHighAccuracy(bool high_accuracy) override;

  bool enable_high_accuracy() const { return enable_high_accuracy_; }

  // Sets a callback that runs, with the value, each time a frame reports
  // whether it asked for high accuracy.
  void SetEnableHighAccuracyCallbackForTesting(
      base::RepeatingCallback<void(bool)> callback);

 private:
  // content::WebContentsObserver overrides:
  void PrimaryPageChanged(content::Page& page) override;

  // tabs::ContentsObservingTabFeature overrides:
  void OnDiscardContents(tabs::TabInterface* tab,
                         content::WebContents* old_contents,
                         content::WebContents* new_contents) override;

  // Bound to the tab's current contents.
  std::unique_ptr<content::RenderFrameHostReceiverSet<
      geolocation::mojom::BraveGeolocationPermission>>
      brave_geolocation_permission_receivers_;

  bool enable_high_accuracy_ = false;

  base::RepeatingCallback<void(bool)>
      enable_high_accuracy_callback_for_testing_;

  ui::ScopedUnownedUserData<BraveGeolocationPermissionTabHelper>
      scoped_unowned_user_data_;
};

#endif  // BRAVE_BROWSER_UI_GEOLOCATION_BRAVE_GEOLOCATION_PERMISSION_TAB_HELPER_H_
