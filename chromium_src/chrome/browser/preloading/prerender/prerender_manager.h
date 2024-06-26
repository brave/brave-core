/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PRELOADING_PRERENDER_PRERENDER_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PRELOADING_PRERENDER_PRERENDER_MANAGER_H_

// Pulling in the original just to expose PrerenderPredictionStatus enum.
#define PrerenderManager PrerenderManager_ChromiumImpl
#include "src/chrome/browser/preloading/prerender/prerender_manager.h"  // IWYU pragma: export
#undef PrerenderManager

// Completely override PrerendererManager as we don't want to use prerendering.
class PrerenderManager : public content::WebContentsUserData<PrerenderManager> {
 public:
  PrerenderManager(const PrerenderManager&) = delete;
  PrerenderManager& operator=(const PrerenderManager&) = delete;

  ~PrerenderManager() override;

  // Calling this method will lead to the cancellation of the previous prerender
  // if the given `canonical_search_url` differs from the ongoing one's.
  void StartPrerenderSearchResult(
      const GURL& canonical_search_url,
      const GURL& prerendering_url,
      base::WeakPtr<content::PreloadingAttempt> attempt);

  // Cancels the prerender that is prerendering the given
  // `canonical_search_url`.
  // TODO(crbug.com/40214220): Use the creator's address to identify the
  // owner that can cancels the corresponding prerendering?
  void StopPrerenderSearchResult(const GURL& canonical_search_url);

  // The entry of bookmark prerender.
  // Calling this method will return WeakPtr of the started prerender, and lead
  // to the cancellation of the previous prerender if the given url is different
  // from the on-going one. If the url given is already on-going, this function
  // will return the weak pointer to the on-going prerender handle.
  base::WeakPtr<content::PrerenderHandle> StartPrerenderBookmark(
      const GURL& prerendering_url);
  void StopPrerenderBookmark(
      base::WeakPtr<content::PrerenderHandle> prerender_handle);

  // The entry of new tab page prerender.
  // Calling this method will return WeakPtr of the started prerender, and lead
  // to the cancellation of the previous prerender if the given url is different
  // from the on-going one. If the url given is already on-going, this function
  // will return the weak pointer to the on-going prerender handle.
  base::WeakPtr<content::PrerenderHandle> StartPrerenderNewTabPage(
      const GURL& prerendering_url,
      content::PreloadingPredictor predictor);
  void StopPrerenderNewTabPage(
      base::WeakPtr<content::PrerenderHandle> prerender_handle);

  // The entry of direct url input prerender.
  // Calling this method will return WeakPtr of the started prerender, and lead
  // to the cancellation of the previous prerender if the given url is different
  // from the on-going one. If the url given is already on-going, this function
  // will return the weak pointer to the on-going prerender handle.
  // PreloadingAttempt represents the attempt corresponding to this prerender to
  // log the necessary metrics.
  // TODO(crbug.com/40208255): Merge the start method with DSE interface
  // using AutocompleteMatch as the parameter instead of GURL.
  base::WeakPtr<content::PrerenderHandle> StartPrerenderDirectUrlInput(
      const GURL& prerendering_url,
      content::PreloadingAttempt& preloading_attempt);

  // Returns true if the current tab prerendered a search result for omnibox
  // inputs.
  bool HasSearchResultPagePrerendered() const;

  base::WeakPtr<PrerenderManager> GetWeakPtr();

  // Returns the prerendered search terms if search_prerender_task_ exists.
  // Returns empty string otherwise.
  const GURL GetPrerenderCanonicalSearchURLForTesting() const;

 private:
  explicit PrerenderManager(content::WebContents* web_contents);
  friend class content::WebContentsUserData<PrerenderManager>;

  base::WeakPtrFactory<PrerenderManager> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PRELOADING_PRERENDER_PRERENDER_MANAGER_H_
