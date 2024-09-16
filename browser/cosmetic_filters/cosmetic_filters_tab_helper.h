// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_COSMETIC_FILTERS_COSMETIC_FILTERS_TAB_HELPER_H_
#define BRAVE_BROWSER_COSMETIC_FILTERS_COSMETIC_FILTERS_TAB_HELPER_H_

#include <string>

#include "brave/components/cosmetic_filters/common/cosmetic_filters.mojom.h"
#include "content/public/browser/render_frame_host_receiver_set.h"
#include "content/public/browser/web_contents_user_data.h"

namespace cosmetic_filters {
// A tab helper to communicate with instances of CosmeticFiltersJSHandler.
// Currently it's created on demand and used for Content Picker feature.
class CosmeticFiltersTabHelper
    : public content::WebContentsUserData<CosmeticFiltersTabHelper>,
      public mojom::CosmeticFiltersHandler {
 public:
  CosmeticFiltersTabHelper(const CosmeticFiltersTabHelper&) = delete;
  CosmeticFiltersTabHelper& operator=(const CosmeticFiltersTabHelper&) = delete;
  ~CosmeticFiltersTabHelper() override;

  static void LaunchContentPicker(content::WebContents* web_contents);

  static void BindCosmeticFiltersHandler(
      content::RenderFrameHost* rfh,
      mojo::PendingAssociatedReceiver<mojom::CosmeticFiltersHandler> receiver);

 private:
  void AddSiteCosmeticFilter(const std::string& filter) override;
  void ManageCustomFilters() override;

  friend class content::WebContentsUserData<CosmeticFiltersTabHelper>;

  explicit CosmeticFiltersTabHelper(content::WebContents* web_contents);

  content::RenderFrameHostReceiverSet<mojom::CosmeticFiltersHandler> receivers_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};
}  // namespace cosmetic_filters
#endif  // BRAVE_BROWSER_COSMETIC_FILTERS_COSMETIC_FILTERS_TAB_HELPER_H_
