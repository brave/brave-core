/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_DRM_TAB_HELPER_H_
#define BRAVE_BROWSER_BRAVE_DRM_TAB_HELPER_H_

#include "base/scoped_observation.h"
#include "brave/components/brave_drm/brave_drm.mojom.h"
#include "components/component_updater/component_updater_service.h"
#include "content/public/browser/render_frame_host_receiver_set.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

// Reacts to DRM content detected on the renderer side.
class BraveDrmTabHelper final
    : public content::WebContentsObserver,
      public content::WebContentsUserData<BraveDrmTabHelper>,
      public brave_drm::mojom::BraveDRM,
      public component_updater::ComponentUpdateService::Observer {
 public:
  explicit BraveDrmTabHelper(content::WebContents* contents);
  ~BraveDrmTabHelper() override;

  static void BindBraveDRM(
      mojo::PendingAssociatedReceiver<brave_drm::mojom::BraveDRM> receiver,
      content::RenderFrameHost* rfh);

  bool ShouldShowWidevineOptIn() const;

  // content::WebContentsObserver
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;

  // blink::mojom::BraveDRM
  void OnWidevineKeySystemAccessRequest() override;

  // component_updater::ComponentUpdateService::Observer
  void OnEvent(const update_client::CrxUpdateItem& item) override;

  WEB_CONTENTS_USER_DATA_KEY_DECL();

 private:
  content::RenderFrameHostReceiverSet<brave_drm::mojom::BraveDRM>
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
};

#endif  // BRAVE_BROWSER_BRAVE_DRM_TAB_HELPER_H_
