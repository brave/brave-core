// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_WIDEVINE_BRAVE_DRM_TAB_HELPER_H_
#define BRAVE_BROWSER_WIDEVINE_BRAVE_DRM_TAB_HELPER_H_

#include <memory>

#include "base/scoped_observation.h"
#include "brave/components/brave_drm/brave_drm.mojom.h"
#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"
#include "components/component_updater/component_updater_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_frame_host_receiver_set.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"

namespace tabs {
class TabInterface;
}  // namespace tabs

// Reacts to DRM content detected on the renderer side. Owned by
// BraveTabFeatures.
class BraveDrmTabHelper final
    : public tabs::ContentsObservingTabFeature,
      public brave_drm::mojom::BraveDRM,
      public component_updater::ComponentUpdateService::Observer {
 public:
  DECLARE_USER_DATA(BraveDrmTabHelper);

  static BraveDrmTabHelper* From(tabs::TabInterface* tab);

  explicit BraveDrmTabHelper(tabs::TabInterface& tab);
  ~BraveDrmTabHelper() override;

  static void BindBraveDRM(
      mojo::PendingAssociatedReceiver<brave_drm::mojom::BraveDRM> receiver,
      content::RenderFrameHost* rfh);

  bool ShouldShowWidevineOptIn() const;

  // content::WebContentsObserver
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;

  // brave_drm::mojom::BraveDRM
  // Must only be called while a BraveDRM message is being dispatched, as it
  // resolves the sending frame from `brave_drm_receivers_` and forwards to the
  // method below.
  void OnWidevineKeySystemAccessRequest() override;

  // Handles a Widevine key system access request originating from
  // `requesting_frame`. Exposed so that tests can drive this helper without a
  // renderer dispatching a mojo message.
  void OnWidevineKeySystemAccessRequestForFrame(
      content::RenderFrameHost* requesting_frame);

  // component_updater::ComponentUpdateService::Observer
  void OnEvent(const update_client::CrxUpdateItem& item) override;

 private:
  // tabs::ContentsObservingTabFeature
  void OnDiscardContents(tabs::TabInterface* tab,
                         content::WebContents* old_contents,
                         content::WebContents* new_contents) override;

  // Bound to the tab's current contents.
  std::unique_ptr<
      content::RenderFrameHostReceiverSet<brave_drm::mojom::BraveDRM>>
      brave_drm_receivers_;

  // Permission request is done only once during the navigation. If user
  // chooses dismiss/deny, additional request is added again only when new
  // main frame navigation is started.
  bool is_permission_requested_ = false;

  // True if we are notified that a page requested widevine availability.
  bool is_widevine_requested_ = false;

  base::ScopedObservation<component_updater::ComponentUpdateService,
                          component_updater::ComponentUpdateService::Observer>
      observer_{this};

  ui::ScopedUnownedUserData<BraveDrmTabHelper> scoped_unowned_user_data_;
};

#endif  // BRAVE_BROWSER_WIDEVINE_BRAVE_DRM_TAB_HELPER_H_
