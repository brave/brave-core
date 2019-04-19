/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_TAB_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_TAB_HELPER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/media_player_id.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class Browser;

namespace dom_distiller {

class DistillerPage;

namespace proto {
class DomDistillerResult;
}  // namespace proto

}  // namespace dom_distiller

namespace brave_ads {

class AdsService;

class AdsTabHelper : public content::WebContentsObserver,
#if !defined(OS_ANDROID)
                     public BrowserListObserver,
#endif
                     public content::WebContentsUserData<AdsTabHelper> {
 public:
  AdsTabHelper(content::WebContents*);
  ~AdsTabHelper() override;

 private:
  friend class content::WebContentsUserData<AdsTabHelper>;

  void TabUpdated();

  // content::WebContentsObserver overrides.
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DocumentOnLoadCompletedInMainFrame() override;
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  void DidAttachInterstitialPage() override;
  void MediaStartedPlaying(const MediaPlayerInfo& video_type,
                           const content::MediaPlayerId& id) override;
  void MediaStoppedPlaying(
      const MediaPlayerInfo& video_type,
      const content::MediaPlayerId& id,
      WebContentsObserver::MediaStoppedReason reason) override;
  void OnVisibilityChanged(content::Visibility visibility) override;
  void WebContentsDestroyed() override;

#if !defined(OS_ANDROID)
  // BrowserListObserver overrides
  void OnBrowserSetLastActive(Browser* browser) override;
  void OnBrowserNoLongerActive(Browser* browser) override;
#endif

  void OnWebContentsDistillationDone(
      const GURL& url,
      std::unique_ptr<dom_distiller::DistillerPage>,
      std::unique_ptr<dom_distiller::proto::DomDistillerResult> result,
      bool distillation_successful);

  SessionID tab_id_;
  AdsService* ads_service_;  // NOT OWNED
  bool is_active_;
  bool is_browser_active_;
  bool run_distiller_;

  base::WeakPtrFactory<AdsTabHelper> weak_factory_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
  DISALLOW_COPY_AND_ASSIGN(AdsTabHelper);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_TAB_HELPER_H_
