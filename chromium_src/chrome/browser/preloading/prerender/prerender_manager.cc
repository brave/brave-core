/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/preloading/prerender/prerender_manager.h"

namespace internal {
const char kHistogramPrerenderPredictionStatusDefaultSearchEngine[] =
    "Prerender.Experimental.PredictionStatus.DefaultSearchEngine";
const char kHistogramPrerenderPredictionStatusDirectUrlInput[] =
    "Prerender.Experimental.PredictionStatus.DirectUrlInput";
}  // namespace internal

PrerenderManager::~PrerenderManager() = default;

base::WeakPtr<content::PrerenderHandle>
PrerenderManager::StartPrerenderDirectUrlInput(
    const GURL& prerendering_url,
    content::PreloadingAttempt& preloading_attempt) {
  return nullptr;
}

base::WeakPtr<content::PrerenderHandle>
PrerenderManager::StartPrerenderBookmark(const GURL& prerendering_url) {
  return nullptr;
}

void PrerenderManager::StopPrerenderBookmark(
    base::WeakPtr<content::PrerenderHandle> prerender_handle) {}

base::WeakPtr<content::PrerenderHandle>
PrerenderManager::StartPrerenderNewTabPage(
    const GURL& prerendering_url,
    content::PreloadingPredictor predictor) {
  return nullptr;
}

void PrerenderManager::StopPrerenderNewTabPage(
    base::WeakPtr<content::PrerenderHandle> prerender_handle) {}

void PrerenderManager::StartPrerenderSearchResult(
    const GURL& canonical_search_url,
    const GURL& prerendering_url,
    base::WeakPtr<content::PreloadingAttempt> preloading_attempt) {}

void PrerenderManager::StopPrerenderSearchResult(
    const GURL& canonical_search_url) {}

bool PrerenderManager::HasSearchResultPagePrerendered() const {
  return false;
}

base::WeakPtr<PrerenderManager> PrerenderManager::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

const GURL PrerenderManager::GetPrerenderCanonicalSearchURLForTesting() const {
  return GURL();
}

PrerenderManager::PrerenderManager(content::WebContents* web_contents)
    : content::WebContentsUserData<PrerenderManager>(*web_contents) {}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PrerenderManager);
