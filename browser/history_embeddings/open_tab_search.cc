// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/open_tab_search.h"

#include <numeric>
#include <optional>
#include <utility>

#include "base/check_op.h"
#include "base/containers/flat_map.h"
#include "base/containers/map_util.h"
#include "base/containers/to_vector.h"
#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/cancelable_task_tracker.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/history_embeddings/content/history_embeddings_service.h"
#include "components/history_embeddings/core/history_embeddings_search.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace history_embeddings {

namespace {

// Same predicate as `TabSearchPageHandler_ChromiumImpl::ShouldTrackBrowser`.
// Uses `BrowserWindowInterface*` rather than `Browser*` because
// `Browser`'s definition lives in `//chrome/browser/ui:ui`, which this
// shared util cannot depend on (that target already depends on this util
// via the chromium_src override, creating a cycle).
bool ShouldTrackBrowser(Profile* profile, BrowserWindowInterface* browser) {
  return browser->GetProfile() == profile &&
         browser->GetType() == BrowserWindowInterface::TYPE_NORMAL;
}

// Walks `profile`'s browser windows and returns HTTP(S) tabs from
// regular-type windows tracked by the tab_search feature.
std::vector<OpenTabInfo> SnapshotOpenTabs(Profile* profile) {
  std::vector<OpenTabInfo> tabs;
  GlobalBrowserCollection::GetInstance()->ForEach(
      [profile, &tabs](BrowserWindowInterface* browser_window) {
        if (!ShouldTrackBrowser(profile, browser_window)) {
          return true;
        }
        TabStripModel* tab_strip_model = browser_window->GetTabStripModel();
        if (!tab_strip_model) {
          return true;
        }
        std::vector<int> indices(tab_strip_model->count());
        std::iota(indices.begin(), indices.end(), 0);
        for (tabs::TabInterface* tab :
             tab_strip_model->GetTabsAtIndices(indices)) {
          if (!tab) {
            continue;
          }
          content::WebContents* contents = tab->GetContents();
          if (!contents) {
            continue;
          }
          GURL url = contents->GetLastCommittedURL();
          if (!url.SchemeIsHTTPOrHTTPS()) {
            continue;
          }
          std::string title = base::UTF16ToUTF8(contents->GetTitle());
          if (title.empty()) {
            title = url.host();
          }
          tabs.push_back(
              {tab->GetHandle().raw_value(), std::move(title), std::move(url)});
        }
        return true;
      });
  return tabs;
}

// Emits the matched tabs by joining the URLID→tabs index against the scored
// URL rows. Multiple open tabs may share a URL (and therefore a URLID), so
// each ranked row can produce more than one tab.
void DispatchRankedTabs(
    RankedOpenTabsCallback& callback,
    const base::flat_map<history::URLID, std::vector<OpenTabInfo>>&
        tabs_by_url_id,
    SearchResult result) {
  std::vector<OpenTabInfo> ranked;
  for (const auto& row : result.scored_url_rows) {
    if (auto* matched =
            base::FindOrNull(tabs_by_url_id, row.scored_url.url_id)) {
      ranked.insert(ranked.end(), matched->begin(), matched->end());
    }
  }
  std::move(callback).Run(std::move(ranked));
}

void OnUrlIdsResolved(std::vector<OpenTabInfo> tabs,
                      std::string query,
                      HistoryEmbeddingsSearch* embeddings_search,
                      RankedOpenTabsCallback callback,
                      std::optional<std::vector<history::URLID>> url_ids) {
  // HistoryService returned no result (e.g. shutdown / cancellation).
  if (!url_ids) {
    std::move(callback).Run({});
    return;
  }
  CHECK_EQ(tabs.size(), url_ids->size());
  std::vector<history::URLID> url_id_filter;
  base::flat_map<history::URLID, std::vector<OpenTabInfo>> tabs_by_url_id;
  url_id_filter.reserve(url_ids->size());
  for (size_t i = 0; i < url_ids->size(); ++i) {
    if ((*url_ids)[i] == 0) {
      continue;
    }
    // Only add each URLID once to the filter; duplicate ids in the SQL
    // `IN (?, ?, ...)` list are harmless but wasteful.
    auto& matched = tabs_by_url_id[(*url_ids)[i]];
    if (matched.empty()) {
      url_id_filter.push_back((*url_ids)[i]);
    }
    matched.push_back(std::move(tabs[i]));
  }
  // None of the open tabs have a corresponding URLID in history yet, so the
  // embeddings search would have nothing to score against.
  if (url_id_filter.empty()) {
    std::move(callback).Run({});
    return;
  }
  const size_t count = url_id_filter.size();
  embeddings_search->Search(
      /*previous_search_result=*/nullptr, query,
      /*time_range_start=*/std::nullopt, count,
      /*skip_answering=*/true, std::move(url_id_filter),
      base::BindRepeating(&DispatchRankedTabs,
                          base::OwnedRef(std::move(callback)),
                          std::move(tabs_by_url_id)));
}

}  // namespace

void SearchOpenTabsByContent(Profile* profile,
                             history::HistoryService* history_service,
                             HistoryEmbeddingsSearch* embeddings_search,
                             std::string query,
                             RankedOpenTabsCallback callback,
                             base::CancelableTaskTracker* task_tracker) {
  std::vector<OpenTabInfo> tabs = SnapshotOpenTabs(profile);
  // No tracked tabs to rank against — `SnapshotOpenTabs` only keeps
  // tracked-browser HTTP(S) tabs, so non-normal windows, other profiles and
  // incognito don't reach here.
  if (tabs.empty()) {
    std::move(callback).Run({});
    return;
  }
  // Sequence the read-from-`tabs` (for URLs) before the move-of-`tabs` into
  // the callback bind. Inlining the projection as a sibling argument leaves
  // the order of evaluation unspecified and lets the move win on some
  // toolchains.
  const std::vector<GURL> urls = base::ToVector(tabs, &OpenTabInfo::url);
  history_service->QueryUrlIds(
      urls,
      base::BindOnce(&OnUrlIdsResolved, std::move(tabs), std::move(query),
                     embeddings_search, std::move(callback)),
      task_tracker);
}

}  // namespace history_embeddings
