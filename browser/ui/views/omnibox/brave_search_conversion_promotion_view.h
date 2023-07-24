/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_SEARCH_CONVERSION_PROMOTION_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_SEARCH_CONVERSION_PROMOTION_VIEW_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_search_conversion/types.h"
#include "chrome/browser/ui/views/omnibox/omnibox_mouse_enter_exit_handler.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

class BraveOmniboxResultView;
class PrefService;

namespace views {
class Background;
class Label;
}  // namespace views

class BraveSearchConversionPromotionView : public views::View {
 public:
  METADATA_HEADER(BraveSearchConversionPromotionView);

  BraveSearchConversionPromotionView(BraveOmniboxResultView* result_view,
                                     PrefService* local_state,
                                     PrefService* profile_prefs);
  BraveSearchConversionPromotionView(
      const BraveSearchConversionPromotionView&) = delete;
  BraveSearchConversionPromotionView& operator=(
      const BraveSearchConversionPromotionView&) = delete;
  ~BraveSearchConversionPromotionView() override;

  void SetTypeAndInput(brave_search_conversion::ConversionType type,
                       const std::u16string& input);
  void OnSelectionStateChanged(bool selected);

  // views::View overrides:
  gfx::Size CalculatePreferredSize() const override;
  void OnThemeChanged() override;

 private:
  void ConfigureForButtonType();
  void UpdateButtonTypeState();
  void ConfigureForBannerType();
  void UpdateBannerTypeState();
  void ResetChildrenVisibility();
  void UpdateHoverState();
  void UpdateState();
  void OpenMatch();
  void Dismiss();
  void MaybeLater();
  int GetBannerTypeTitleStringResourceId();
  int GetBannerTypeDescStringResourceId();
  SkColor GetCloseButtonColor() const;

  // Gives buttton type background based on selected or hovered state.
  std::unique_ptr<views::Background> GetButtonTypeBackground();

  raw_ptr<BraveOmniboxResultView> result_view_ = nullptr;

  // Children for button or banner type promotion.
  // Promotion view is implemented w/o using existing omnibox view controls
  // because our promotion view's layout, bg and text colors are slightly
  // different. |button_type_container_| is child view that holds whole UI for
  // buton type and |banner_type_container_| is for banner type. When current
  // promotion type is button, |button_type_container_| is only visible.
  // Otherwise, |banner_type_container_| is only visible view.

  // Children for button type promotion.
  raw_ptr<views::View> button_type_container_ = nullptr;
  raw_ptr<views::View> button_type_selection_indicator_ = nullptr;
  raw_ptr<views::Label> button_type_description_ = nullptr;
  raw_ptr<views::Label> button_type_contents_input_ = nullptr;
  raw_ptr<views::Label> append_for_input_ = nullptr;

  // Children for banner type promotion.
  raw_ptr<views::View> banner_type_container_ = nullptr;
  raw_ptr<views::Label> banner_type_description_ = nullptr;

  brave_search_conversion::ConversionType type_ =
      brave_search_conversion::ConversionType::kNone;
  bool selected_ = false;
  std::u16string input_;

  // Keeps track of mouse-enter and mouse-exit events of child Views.
  OmniboxMouseEnterExitHandler mouse_enter_exit_handler_;

  raw_ptr<PrefService> local_state_ = nullptr;
  raw_ptr<PrefService> profile_prefs_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_SEARCH_CONVERSION_PROMOTION_VIEW_H_
