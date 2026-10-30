// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_ANDROID_BRAVE_TAB_FEATURES_H_
#define BRAVE_BROWSER_ANDROID_BRAVE_TAB_FEATURES_H_

#include <memory>

#include "chrome/browser/android/tab_features.h"

class Profile;

namespace ai_chat {
class TabDataWebContentsObserver;
class WebMcpInjector;
}  // namespace ai_chat

namespace content {
class WebContents;
}  // namespace content

namespace image_metadata_stripper {
class FileUploadController;
}  // namespace image_metadata_stripper

namespace tabs {

class ContentsObservingTabFeature;

// This class holds state that is scoped to a tab in Android. It is constructed
// after the WebContents/tab_helpers, and destroyed before.
class BraveTabFeatures : public TabFeatures_Chromium {
 public:
  BraveTabFeatures(content::WebContents* web_contents, Profile* profile);
  ~BraveTabFeatures();
  static BraveTabFeatures* FromTabFeatures(TabFeatures* tab_features);

 private:
  // Records Cloudflare javascript-detection script loads. Null for
  // incognito/guest profiles and when captcha metrics are disabled.
  std::unique_ptr<ContentsObservingTabFeature>
      cloudflare_js_detection_tab_helper_;

  // Records page and media session metrics.
  std::unique_ptr<ContentsObservingTabFeature> page_metrics_tab_helper_;

  // Records searches. Null when SERP metrics are disabled or off the record.
  std::unique_ptr<ContentsObservingTabFeature> serp_metrics_tab_helper_;
  // Notifies the ads service of tab events. Null without Brave Ads.
  std::unique_ptr<ContentsObservingTabFeature> ads_tab_helper_;
  // Handles search result ads. Null without Brave Ads or off the record.
  std::unique_ptr<ContentsObservingTabFeature>
      creative_search_result_ad_tab_helper_;
  // Offers and runs Web Discovery. Null without it or outside regular profiles.
  std::unique_ptr<ContentsObservingTabFeature> web_discovery_tab_helper_;
  // Detects the Rewards creator of the page. Null without Brave Rewards.
  std::unique_ptr<ContentsObservingTabFeature> rewards_tab_helper_;
  // Offers to enable Widevine when the page asks for it. Null without it.
  std::unique_ptr<ContentsObservingTabFeature> drm_tab_helper_;
  // Finds the page's Brave News feeds. Null without Brave News or off the
  // record.
  std::unique_ptr<ContentsObservingTabFeature> brave_news_tab_helper_;

  std::unique_ptr<ai_chat::TabDataWebContentsObserver> tab_data_observer_;
  std::unique_ptr<ai_chat::WebMcpInjector> web_mcp_injector_;

  // The main controller which takes care of stripping metadata from images
  // for the upload flow.
  std::unique_ptr<image_metadata_stripper::FileUploadController>
      stripper_upload_controller_;
};

}  // namespace tabs

#endif  // BRAVE_BROWSER_ANDROID_BRAVE_TAB_FEATURES_H_
