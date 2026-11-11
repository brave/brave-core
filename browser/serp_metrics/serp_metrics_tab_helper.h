/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_TAB_HELPER_H_
#define BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_TAB_HELPER_H_

#include <memory>
#include <optional>

#include "base/memory/raw_ref.h"
#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"
#include "components/search_engines/search_engine_type.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"
#include "url/gurl.h"

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace serp_metrics {

class SerpMetrics;

// Records searches made in a tab. Owned by BraveTabFeatures.
class SerpMetricsTabHelper final : public tabs::ContentsObservingTabFeature {
 public:
  DECLARE_USER_DATA(SerpMetricsTabHelper);

  static SerpMetricsTabHelper* From(tabs::TabInterface* tab);

  // Returns nullptr when the feature is disabled or the profile has no
  // SerpMetricsService, e.g. off the record.
  static std::unique_ptr<SerpMetricsTabHelper> MaybeCreate(
      tabs::TabInterface& tab);

  SerpMetricsTabHelper(tabs::TabInterface& tab, SerpMetrics& serp_metrics);
  SerpMetricsTabHelper(const SerpMetricsTabHelper&) = delete;
  SerpMetricsTabHelper& operator=(const SerpMetricsTabHelper&) = delete;

  ~SerpMetricsTabHelper() override;

 private:
  bool IsSameSerpAsLastRecorded(const GURL& url) const;

  void MaybeClassifyAndRecordSearchEngineForUrl(const GURL& url);
  void RecordSearchEngine(SearchEngineType search_engine_type);

  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  const raw_ref<SerpMetrics> serp_metrics_;

  std::optional<GURL> last_recorded_serp_url_;

  ui::ScopedUnownedUserData<SerpMetricsTabHelper> scoped_unowned_user_data_;
};

}  // namespace serp_metrics

#endif  // BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_TAB_HELPER_H_
