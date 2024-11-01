/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_RESULT_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_RESULT_VIEW_H_

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/views/omnibox/omnibox_result_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BraveSearchConversionPromotionView;
class BraveOmniboxPopupViewViews;

// This will render brave specific matches such as the braver search conversion
// promotion.
class BraveOmniboxResultView : public OmniboxResultView {
  METADATA_HEADER(BraveOmniboxResultView, OmniboxResultView)
 public:
  using OmniboxResultView::OmniboxResultView;
  BraveOmniboxResultView(const BraveOmniboxResultView&) = delete;
  BraveOmniboxResultView& operator=(const BraveOmniboxResultView&) = delete;
  ~BraveOmniboxResultView() override;

  void OpenMatch();
  void RefreshOmniboxResult();
  BraveOmniboxPopupViewViews* GetPopupView();

  // OmniboxResultView overrides:
  void SetMatch(const AutocompleteMatch& match) override;
  void OnSelectionStateChanged() override;
  gfx::Image GetIcon() const override;
  void OnThemeChanged() override;
  void OnPaintBackground(gfx::Canvas* canvas) override;

 private:
  void ResetChildren();
  void UpdateForBraveSearchConversion();
  void HandleSelectionStateChangedForPromotionView();
  void UpdateForLeoMatch();

  // Brave search conversion promotion
  raw_ptr<BraveSearchConversionPromotionView> brave_search_promotion_view_ =
      nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_RESULT_VIEW_H_
