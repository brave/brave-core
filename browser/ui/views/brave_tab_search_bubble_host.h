/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_TAB_SEARCH_BUBBLE_HOST_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_TAB_SEARCH_BUBBLE_HOST_H_

#include <optional>

#include "chrome/browser/ui/views/tab_search_bubble_host.h"

class BraveTabSearchBubbleHost : public TabSearchBubbleHost {
 public:
  using TabSearchBubbleHost::TabSearchBubbleHost;
  ~BraveTabSearchBubbleHost() override = default;

  void SetBubbleArrow(views::BubbleBorder::Arrow arrow);

  // TabSearchBubbleHost:
  bool ShowTabSearchBubble(
      bool triggered_by_keyboard_shortcut = false,
      tab_search::mojom::TabSearchSection section =
          tab_search::mojom::TabSearchSection::kNone,
      tab_search::mojom::TabOrganizationFeature organization_feature =
          tab_search::mojom::TabOrganizationFeature::kNone) override;

 private:
  std::optional<views::BubbleBorder::Arrow> arrow_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_TAB_SEARCH_BUBBLE_HOST_H_
