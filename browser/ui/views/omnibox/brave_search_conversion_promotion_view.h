/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_SEARCH_CONVERSION_PROMOTION_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_SEARCH_CONVERSION_PROMOTION_VIEW_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_search_conversion/types.h"
#include "chrome/browser/ui/views/omnibox/omnibox_mouse_enter_exit_handler.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

class BraveOmniboxResultView;
class PrefService;
class TemplateURLService;

namespace views {
class Background;
class Label;
}  // namespace views

class BraveSearchConversionPromotionView : public views::View {
  METADATA_HEADER(BraveSearchConversionPromotionView, views::View)
 public:
  BraveSearchConversionPromotionView(BraveOmniboxResultView* result_view,
                                     PrefService* local_state,
                                     PrefService* profile_prefs,
                                     TemplateURLService* template_url_service);
  BraveSearchConversionPromotionView(
      const BraveSearchConversionPromotionView&) = delete;
  BraveSearchConversionPromotionView& operator=(
      const BraveSearchConversionPromotionView&) = delete;
  ~BraveSearchConversionPromotionView() override;

  void SetTypeAndInput(brave_search_conversion::ConversionType type,
                       const std::u16string& input);
  void OnSelectionStateChanged(bool selected);

  // views::View overrides:
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;

 private:
  void ConfigureForBannerType();
  void UpdateState();
  void UpdateHoverState();
  void OpenMatch();
  void SetBraveAsDefault();
  void Dismiss();
  void MaybeLater();
  int GetBannerTypeTitleStringResourceId();
  int GetBannerTypeDescStringResourceId();
  SkColor GetCloseButtonColor() const;
  int GetOverallHorizontalMarginAroundDescription() const;
  std::unique_ptr<views::View> GetPrimaryButton();
  std::unique_ptr<views::View> GetSecondaryButton();
  void OnPrimaryButtonPressed();
  void OnSecondaryButtonPressed();

  // true when this is for ddg conversion promotion.
  bool UseDDG() const;

  // false if we don't have sufficient space.
  // Only renders title & description in that situation.
  bool ShouldDrawGraphic() const;

  raw_ptr<BraveOmniboxResultView> result_view_ = nullptr;

  // Children for button or banner type promotion.
  // Promotion view is implemented w/o using existing omnibox view controls
  // because our promotion view's layout, bg and text colors are slightly
  // different. |banner_type_container_| is for banner type.

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
  raw_ref<TemplateURLService> template_url_service_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_SEARCH_CONVERSION_PROMOTION_VIEW_H_
