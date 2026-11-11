/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MISC_METRICS_PAGE_METRICS_TAB_HELPER_H_
#define BRAVE_BROWSER_MISC_METRICS_PAGE_METRICS_TAB_HELPER_H_

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"
#include "ui/base/page_transition_types.h"

namespace content {
class BrowserContext;
class MediaSession;
class NavigationHandle;
class WebContents;
}  // namespace content

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace misc_metrics {

class MediaSessionMetricsImpl;
class PageMetrics;

// Records page and media session metrics for a tab.
class PageMetricsTabHelper : public tabs::ContentsObservingTabFeature {
 public:
  explicit PageMetricsTabHelper(tabs::TabInterface& tab);
  ~PageMetricsTabHelper() override;

  PageMetricsTabHelper(const PageMetricsTabHelper&) = delete;
  PageMetricsTabHelper& operator=(const PageMetricsTabHelper&) = delete;

 private:
  // tabs::ContentsObservingTabFeature:
  void OnDiscardContents(tabs::TabInterface* tab,
                         content::WebContents* old_contents,
                         content::WebContents* new_contents) override;

  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void MediaSessionCreated(content::MediaSession* media_session) override;

  // Stops tracking the media session of the observed contents, if any.
  void ReleaseMediaSession();

  bool IsRelevantNavigationEvent(content::NavigationHandle* navigation_handle);
  bool IsPrivateWindowEvent();
  void MaybeRecordNavigationSource(ui::PageTransition transition,
                                   bool is_reload);

  raw_ptr<content::BrowserContext> browser_context_;
  raw_ptr<PageMetrics> page_metrics_ = nullptr;
  raw_ptr<MediaSessionMetricsImpl> media_session_metrics_ = nullptr;
  raw_ptr<content::MediaSession> media_session_ = nullptr;
};

}  // namespace misc_metrics

#endif  // BRAVE_BROWSER_MISC_METRICS_PAGE_METRICS_TAB_HELPER_H_
