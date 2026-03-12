// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_TAB_HELPER_H_
#define BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_TAB_HELPER_H_

#include "base/memory/raw_ptr.h"
#include "ios/web/public/web_state_observer.h"
#include "ios/web/public/web_state_user_data.h"

class GURL;

namespace base {
class Value;
}

namespace web {
class NavigationContext;
class WebState;
}  // namespace web

namespace brave_ads {

class AdsService;

class AdsTabHelper : public web::WebStateUserData<AdsTabHelper>,
                     public web::WebStateObserver {
 public:
  AdsTabHelper(const AdsTabHelper&) = delete;
  AdsTabHelper& operator=(const AdsTabHelper&) = delete;
  ~AdsTabHelper() override;

  static void MaybeCreateForWebState(web::WebState* web_state);

  void NotifyTabDidStartPlayingMedia();
  void NotifyTabDidStopPlayingMedia();

  // web::WebStateObserver
  void WasShown(web::WebState* web_state) override;
  void WasHidden(web::WebState* web_state) override;
  void DidStartNavigation(web::WebState* web_state,
                          web::NavigationContext* navigation_context) override;
  void DidRedirectNavigation(
      web::WebState* web_state,
      web::NavigationContext* navigation_context) override;
  void DidFinishNavigation(web::WebState* web_state,
                           web::NavigationContext* navigation_context) override;
  void PageLoaded(
      web::WebState* web_state,
      web::PageLoadCompletionStatus load_completion_status) override;
  void WebStateDestroyed(web::WebState* web_state) override;

 private:
  friend class web::WebStateUserData<AdsTabHelper>;

  AdsTabHelper(web::WebState* web_state, AdsService* ads_service);

  void MaybeNotifyTabDidChange();
  void MaybeNotifyTabDidLoad();
  void OnVisibilityChanged(bool is_visible);
  void MaybeNotifyTabHtmlContentDidChange();
  void MaybeNotifyTabTextContentDidChange();
  bool UserHasOptedInToNotificationAds() const;
  bool UserHasJoinedBraveRewards() const;
  bool ShouldNotifyTabContentDidChange() const;
  void OnMaybeNotifyTabTextContentDidChange(
      const std::vector<GURL>& redirect_chain,
      const base::Value* value);
  void OnMaybeNotifyTabHtmlContentDidChange(
      const std::vector<GURL>& redirect_chain,
      const base::Value* value);

  raw_ptr<web::WebState> web_state_;
  int32_t tab_id_;
  raw_ref<AdsService> ads_service_;

  std::vector<GURL> redirect_chain_;
  std::optional<int> http_status_code_;
  bool was_restored_ = false;
  bool is_new_navigation_ = false;
  bool is_web_state_visible_ = false;

  base::WeakPtrFactory<AdsTabHelper> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_TAB_HELPER_H_
