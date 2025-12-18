/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_query_metrics/search_query_metrics_tab_helper.h"

#include "base/check.h"
#include "base/logging.h"
#include "brave/browser/search_query_metrics/search_query_metrics_service_factory.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/search_query_metrics/search_engine/search_engine_util.h"
#include "brave/components/search_query_metrics/search_query_metrics_entry_point_type.h"
#include "brave/components/search_query_metrics/search_query_metrics_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/renderer_host/chrome_navigation_ui_data.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/search_engines/search_engine_utils.h"
#include "components/search_engines/template_url_service.h"
#include "components/sessions/content/session_tab_helper.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace metrics {

namespace {

bool IsAllowedToSendUsagePings() {
  return g_browser_process->local_state()->GetBoolean(kStatsReportingEnabled);
}

void LogPageTransitionForDebugging(ui::PageTransition page_transition) {
  DVLOG(9) << "[METRIC] Page transition for debugging:";

  if (ui::PageTransitionCoreTypeIs(page_transition, ui::PAGE_TRANSITION_LINK)) {
    DVLOG(9) << "  PAGE_TRANSITION_LINK";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_TYPED)) {
    DVLOG(9) << "  PAGE_TRANSITION_TYPED";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_AUTO_BOOKMARK)) {
    DVLOG(9) << "  PAGE_TRANSITION_AUTO_BOOKMARK";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_AUTO_SUBFRAME)) {
    DVLOG(9) << "  PAGE_TRANSITION_AUTO_SUBFRAME";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_MANUAL_SUBFRAME)) {
    DVLOG(9) << "  PAGE_TRANSITION_MANUAL_SUBFRAME";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_GENERATED)) {
    DVLOG(9) << "  PAGE_TRANSITION_GENERATED";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_AUTO_TOPLEVEL)) {
    DVLOG(9) << "  PAGE_TRANSITION_AUTO_TOPLEVEL";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_FORM_SUBMIT)) {
    DVLOG(9) << "  PAGE_TRANSITION_FORM_SUBMIT";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_RELOAD)) {
    DVLOG(9) << "  PAGE_TRANSITION_RELOAD";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_KEYWORD)) {
    DVLOG(9) << "  PAGE_TRANSITION_KEYWORD";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_KEYWORD_GENERATED)) {
    DVLOG(9) << "  PAGE_TRANSITION_KEYWORD_GENERATED";
  }

  if ((ui::PageTransitionGetQualifier(page_transition) &
       ui::PAGE_TRANSITION_BLOCKED) != 0) {
    DVLOG(9) << "  PAGE_TRANSITION_BLOCKED";
  }

  if ((ui::PageTransitionGetQualifier(page_transition) &
       ui::PAGE_TRANSITION_FORWARD_BACK) != 0) {
    DVLOG(9) << "  PAGE_TRANSITION_FORWARD_BACK";
  }

  if ((ui::PageTransitionGetQualifier(page_transition) &
       ui::PAGE_TRANSITION_FROM_ADDRESS_BAR) != 0) {
    DVLOG(9) << "  PAGE_TRANSITION_FROM_ADDRESS_BAR";
  }

  if ((ui::PageTransitionGetQualifier(page_transition) &
       ui::PAGE_TRANSITION_HOME_PAGE) != 0) {
    DVLOG(9) << "  PAGE_TRANSITION_HOME_PAGE";
  }

  if ((ui::PageTransitionGetQualifier(page_transition) &
       ui::PAGE_TRANSITION_FROM_API) != 0) {
    DVLOG(9) << "  PAGE_TRANSITION_FROM_API";
  }

  if ((ui::PageTransitionGetQualifier(page_transition) &
       ui::PAGE_TRANSITION_CHAIN_START) != 0) {
    DVLOG(9) << "  PAGE_TRANSITION_CHAIN_START";
  }

  if ((ui::PageTransitionGetQualifier(page_transition) &
       ui::PAGE_TRANSITION_CHAIN_END) != 0) {
    DVLOG(9) << "  PAGE_TRANSITION_CHAIN_END";
  }

  if ((ui::PageTransitionGetQualifier(page_transition) &
       ui::PAGE_TRANSITION_CLIENT_REDIRECT) != 0) {
    DVLOG(9) << "  PAGE_TRANSITION_CLIENT_REDIRECT";
  }

  if ((ui::PageTransitionGetQualifier(page_transition) &
       ui::PAGE_TRANSITION_SERVER_REDIRECT) != 0) {
    DVLOG(9) << "  PAGE_TRANSITION_SERVER_REDIRECT";
  }
}

bool IsBookmark(content::NavigationHandle* navigation_handle) {
  CHECK(navigation_handle);

  if (!ui::PageTransitionCoreTypeIs(navigation_handle->GetPageTransition(),
                                    ui::PAGE_TRANSITION_AUTO_BOOKMARK)) {
    return false;
  }

  auto* chrome_navigation_ui_data = static_cast<ChromeNavigationUIData*>(
      navigation_handle->GetNavigationUIData());
  if (!chrome_navigation_ui_data) {
    return false;
  }

  return !!chrome_navigation_ui_data->bookmark_id();
}

}  // namespace

SearchQueryMetricsTabHelper::SearchQueryMetricsTabHelper(
    content::WebContents* const web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<SearchQueryMetricsTabHelper>(*web_contents) {
  const SessionID session_id =
      sessions::SessionTabHelper::IdForTab(web_contents);
  if (!session_id.is_valid()) {
    return;
  }

  Profile* const profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  search_query_metrics_service_ =
      SearchQueryMetricsServiceFactory::GetForProfile(profile);
}

SearchQueryMetricsTabHelper::~SearchQueryMetricsTabHelper() = default;

///////////////////////////////////////////////////////////////////////////////

void SearchQueryMetricsTabHelper::MaybeClassifyAndReport(
    content::NavigationHandle* navigation_handle) {
  CHECK(navigation_handle);

  if (!search_query_metrics_service_) {
    return;
  }

  const ui::PageTransition page_transition =
      navigation_handle->GetPageTransition();

  LogPageTransitionForDebugging(page_transition);

  const GURL& url = navigation_handle->GetURL();
  if (!url.is_valid()) {
    return;
  }

  const bool is_bookmark = IsBookmark(navigation_handle);

  std::optional<SearchQueryMetricsEntryPointType> entry_point_type =
      MaybeClassifyEntryPoint(url, page_transition, is_bookmark);
  if (!entry_point_type) {
    return;
  }
  entry_point_type_ = entry_point_type;

  if (defer_reporting_until_next_search_query_) {
    defer_reporting_until_next_search_query_ = false;
    return;
  }

  search_query_metrics_service_->MaybeReport(url, *entry_point_type);
}

std::optional<SearchQueryMetricsEntryPointType>
SearchQueryMetricsTabHelper::MaybeClassifyEntryPoint(
    const GURL& url,
    ui::PageTransition page_transition,
    bool is_bookmark) {
  CHECK(url.is_valid());

  if (IsSearchEngine(url)) {
    if (is_bookmark) {
      // For supported search engines, bookmark navigations always override the
      // entry point and defer reporting until the next search query.
      defer_reporting_until_next_search_query_ = true;
      DVLOG(6) << "[METRIC] Classified as bookmark (Deferred until next query)";
      return SearchQueryMetricsEntryPointType::kBookmark;
    }

    if (is_initial_navigation_) {
      if (entry_point_type_ == SearchQueryMetricsEntryPointType::kDirect) {
        // For supported search engines, defer the first navigation for direct
        // entry points until the next search query.
        is_initial_navigation_ = false;
        defer_reporting_until_next_search_query_ = true;
        DVLOG(6) << "[METRIC] Classified as direct (Deferred until next query)";
        return entry_point_type_;
      }

      if (entry_point_type_ == SearchQueryMetricsEntryPointType::kTopSite) {
        // For supported search engines, defer the first navigation for top
        // sites entry points until the next search query.
        is_initial_navigation_ = false;
        defer_reporting_until_next_search_query_ = true;
        DVLOG(6)
            << "[METRIC] Classified as top site (Deferred until next query)";
        return entry_point_type_;
      }
    }
  }

  if (!IsSearchEngineResultsPage(url)) {
    // If the URL is not a supported search results page, do not classify it.
    return std::nullopt;
  }

  is_initial_navigation_ = false;

  switch (
      entry_point_type_.value_or(SearchQueryMetricsEntryPointType::kOther)) {
    case SearchQueryMetricsEntryPointType::kBookmark: {
      DVLOG(6) << "[METRIC] Classified as bookmark";
      return entry_point_type_;
    }

    case SearchQueryMetricsEntryPointType::kDirect: {
      DVLOG(6) << "[METRIC] Classified as direct";
      return entry_point_type_;
    }

    case SearchQueryMetricsEntryPointType::kNTP: {
      DVLOG(6) << "[METRIC] Classified as NTP";
      return entry_point_type_;
    }

    case SearchQueryMetricsEntryPointType::kOmniboxHistory: {
      DVLOG(6) << "[METRIC] Classified as omnibox history";
      return entry_point_type_;
    }

    case SearchQueryMetricsEntryPointType::kOmniboxSuggestion: {
      DVLOG(6) << "[METRIC] Classified as omnibox suggestion";
      return entry_point_type_;
    }

    case SearchQueryMetricsEntryPointType::kOmniboxSearch: {
      DVLOG(6) << "[METRIC] Classified as omnibox search";
      return entry_point_type_;
    }

    case SearchQueryMetricsEntryPointType::kQuickSearch: {
      DVLOG(6) << "[METRIC] Classified as quick search";
      return entry_point_type_;
    }

    case SearchQueryMetricsEntryPointType::kShortcut: {
      DVLOG(6) << "[METRIC] Classified as shortcut";
      return entry_point_type_;
    }

    case SearchQueryMetricsEntryPointType::kTopSite: {
      DVLOG(6) << "[METRIC] Classified as top site";
      return entry_point_type_;
    }

    default: {
      // Fall through to derive the entry point from page transition or default
      // classification when no explicit entry point is set.
      break;
    }
  }

  if (!entry_point_type_ &&
      (ui::PageTransitionCoreTypeIs(page_transition,
                                    ui::PAGE_TRANSITION_FORM_SUBMIT) ||
       ui::PageTransitionCoreTypeIs(page_transition,
                                    ui::PAGE_TRANSITION_LINK))) {
    // For supported search engines, restored tabs do not have an explicit entry
    // point, so classify form submissions and link navigations as direct.
    DVLOG(6) << "[METRIC] Classified as direct";
    return entry_point_type_.value_or(
        SearchQueryMetricsEntryPointType::kDirect);
  }

  // For supported search engines, default to "Other" when no specific entry
  // point classification applies.
  DVLOG(6) << "[METRIC] Classified as other";
  return SearchQueryMetricsEntryPointType::kOther;
}

// This method is called when a navigation in the main frame or a subframe has
// completed. It indicates that the navigation has finished, but the document
// might still be loading resources.
void SearchQueryMetricsTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!IsAllowedToSendUsagePings()) {
    return;
  }

  if (!navigation_handle->IsInPrimaryMainFrame()) {
    // Ignore navigations not in the primary main frame. Subframe navigations
    // are not top-level user actions.
    return;
  }

  if (!navigation_handle->HasCommitted()) {
    // Ignore navigations that did not fully commit such as aborted, replaced,
    // or error pages. Only committed navigations count as completed actions.
    return;
  }

  if (navigation_handle->GetRestoreType() == content::RestoreType::kRestored) {
    // Ignore navigations restored from session history such as tab restore.
    // These reflect previously saved state and not a new user action.
    return;
  }

  if (!navigation_handle->HasUserGesture() &&
      navigation_handle->IsRendererInitiated()) {
    // Ignore navigations without an explicit user gesture. This avoids
    // processing navigations triggered automatically or by scripts. Some
    // browser initiated navigations return `false` for `HasUserGesture` so we
    // must also check `IsRendererInitiated`. See crbug.com/617904.
    return;
  }

  if (!ui::PageTransitionIsNewNavigation(
          navigation_handle->GetPageTransition())) {
    // Ignore navigations that are not new actions. Back, forward, and reload
    // navigations reuse existing history entries.
    return;
  }

  MaybeClassifyAndReport(navigation_handle);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(SearchQueryMetricsTabHelper);

}  // namespace metrics
