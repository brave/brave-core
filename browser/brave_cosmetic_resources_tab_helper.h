/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_COSMETIC_RESOURCES_TAB_HELPER_H_
#define BRAVE_BROWSER_BRAVE_COSMETIC_RESOURCES_TAB_HELPER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/content/browser/cosmetic_filters_observer.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class BraveCosmeticResourcesTabHelper
    : public content::CosmeticFiltersObserver,
      public content::WebContentsObserver,
      public content::WebContentsUserData<BraveCosmeticResourcesTabHelper> {
 public:
  BraveCosmeticResourcesTabHelper(const BraveCosmeticResourcesTabHelper&) =
      delete;
  void operator=(const BraveCosmeticResourcesTabHelper&) = delete;

  explicit BraveCosmeticResourcesTabHelper(content::WebContents* contents);
  ~BraveCosmeticResourcesTabHelper() override;

  // content::WebContentsObserver overrides:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void ResourceLoadComplete(
      content::RenderFrameHost* render_frame_host,
      const content::GlobalRequestID& request_id,
      const blink::mojom::ResourceLoadInfo& resource_load_info) override;

  // content::CosmeticFiltersObserver overrides:
  void ApplyHiddenClassIdSelectors(
      content::RenderFrameHost* render_frame_host,
      const std::vector<std::string>& classes,
      const std::vector<std::string>& ids) override;

  WEB_CONTENTS_USER_DATA_KEY_DECL();

 private:
  void ProcessURL(content::RenderFrameHost* render_frame_host,
                  const GURL& url,
                  const bool do_non_scriptlets);

  void GetUrlCosmeticResourcesOnUI(content::GlobalFrameRoutingId frame_id,
                                   const std::string& url,
                                   bool do_non_scriptlets,
                                   std::unique_ptr<base::ListValue> resources);
  void CSSRulesRoutine(const std::string& url,
                       base::DictionaryValue* resources_dict,
                       content::GlobalFrameRoutingId frame_id);

  void GetHiddenClassIdSelectorsOnUI(
      content::GlobalFrameRoutingId frame_id,
      const GURL& url,
      std::unique_ptr<base::ListValue> selectors);

  std::vector<std::string> exceptions_;
  bool enabled_1st_party_cf_filtering_;

  base::WeakPtrFactory<BraveCosmeticResourcesTabHelper> weak_factory_;
};

#endif  // BRAVE_BROWSER_BRAVE_COSMETIC_RESOURCES_TAB_HELPER_H_
