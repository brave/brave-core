// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_HISTORY_EMBEDDINGS_TAB_HELPER_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_HISTORY_EMBEDDINGS_TAB_HELPER_H_

#include <memory>
#include <optional>

#include "base/callback_list.h"
#include "base/memory/weak_ptr.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/time/time.h"
#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"
#include "components/history/core/browser/history_types.h"
#include "url/gurl.h"

namespace history {
class HistoryService;
}

namespace passage_embeddings {
class PassageEmbedderModelObserver;
}

namespace tabs {
class TabInterface;
}

namespace brave_history_embeddings {

class HistoryEmbeddingsService;

// Hands each committed history visit to `HistoryEmbeddingsService`, so the
// passages it later computes for the page are stored against that visit.
// Follows the tab across discards, which swap its WebContents.
class HistoryEmbeddingsTabHelper : public tabs::ContentsObservingTabFeature {
 public:
  // Returns null when the profile has no embeddings service.
  static std::unique_ptr<HistoryEmbeddingsTabHelper> MaybeCreate(
      tabs::TabInterface& tab);

  explicit HistoryEmbeddingsTabHelper(tabs::TabInterface& tab);
  ~HistoryEmbeddingsTabHelper() override;

  HistoryEmbeddingsTabHelper(const HistoryEmbeddingsTabHelper&) = delete;
  HistoryEmbeddingsTabHelper& operator=(const HistoryEmbeddingsTabHelper&) =
      delete;

  // Called right after submitting a new navigation for `web_contents()` to
  // HistoryService. Allows close coordination with History's conception of the
  // visit lifetime.
  // Virtual for testing.
  virtual void OnUpdatedHistoryForNavigation(int64_t navigation_id,
                                             bool is_in_primary_main_frame,
                                             base::Time timestamp,
                                             const GURL& url);

  // content::WebContentsObserver:
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  void DidFailLoad(content::RenderFrameHost* render_frame_host,
                   const GURL& validated_url,
                   int error_code) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void PrimaryMainFrameRenderProcessGone(
      base::TerminationStatus status) override;

 private:
  // tabs::ContentsObservingTabFeature:
  void OnDiscardContents(tabs::TabInterface* tab,
                         content::WebContents* old_contents,
                         content::WebContents* new_contents) override;

  // Subscribes to the history visits `HistoryTabHelper` records for the
  // observed WebContents.
  void ObserveHistoryTabHelper();

  // Drops any pending visit, and the service's record of one, for the
  // observed WebContents.
  void ResetVisit();

  // Callback for `ExtractPassages()`. It's in a member method to enable
  // cancellation via `weak_factory_`.
  void UpdateEmbeddingsServiceWithHistoryData(
      history::QueryURLAndVisitsResult result);

  // Invalidates weak pointers and cancels any pending history lookup callbacks.
  void CancelHistoryLookup();

  // Helper functions to return the embeddings and history services.
  // `GetHistoryClustersService()` may return nullptr (in tests).
  HistoryEmbeddingsService* GetHistoryEmbeddingsService();
  passage_embeddings::PassageEmbedderModelObserver*
  GetPassageEmbedderModelObserver();
  // `GetHistoryService()` may return nullptr.
  history::HistoryService* GetHistoryService();

  // Data saved from the `HistoryTabHelper` call to
  // `OnUpdatedHistoryForNavigation` which happens in `DidFinishNavigation`
  // and precedes `DidFinishLoad`.
  std::optional<int64_t> history_navigation_id_;
  std::optional<base::Time> history_visit_time_;
  std::optional<GURL> history_url_;

  // Task tracker for calls for the history service.
  base::CancelableTaskTracker task_tracker_;

  base::CallbackListSubscription history_tab_helper_subscription_;

  // This factory frequently invalidates existing weak pointers to cancel
  // delayed passage extraction.
  base::WeakPtrFactory<HistoryEmbeddingsTabHelper> extraction_weak_ptr_factory_{
      this};

  // A standard WeakPtrFactory with lifetime equal to the object itself.
  base::WeakPtrFactory<HistoryEmbeddingsTabHelper> weak_ptr_factory_{this};
};

}  // namespace brave_history_embeddings

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_HISTORY_EMBEDDINGS_TAB_HELPER_H_
