/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/omnibox/brave_omnibox_result_view.h"

#include <memory>

#include "base/time/time.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/omnibox/brave_omnibox_popup_view_views.h"
#include "brave/browser/ui/views/omnibox/brave_search_conversion_promotion_view.h"
#include "brave/components/omnibox/browser/leo_provider.h"
#include "brave/components/omnibox/browser/promotion_utils.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/views/omnibox/omnibox_popup_view_views.h"
#include "chrome/browser/ui/views/omnibox/omnibox_suggestion_button_row_view.h"
#include "components/omnibox/browser/autocomplete_controller.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"
#include "components/omnibox/browser/autocomplete_result.h"
#include "components/omnibox/browser/omnibox_controller.h"
#include "components/omnibox/browser/omnibox_edit_model.h"
#include "components/omnibox/browser/omnibox_popup_selection.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/window_open_disposition.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/view_class_properties.h"

BraveOmniboxResultView::~BraveOmniboxResultView() = default;

void BraveOmniboxResultView::ResetChildren() {
  if (brave_search_promotion_view_) {
    RemoveChildViewT(brave_search_promotion_view_);
    brave_search_promotion_view_ = nullptr;
  }

  // Reset children visibility. Their visibility could be configured later
  // based on |match_| and the current input.
  // Reset upstream's layout manager.
  SetLayoutManager(std::make_unique<views::FillLayout>());
  for (auto& child : children()) {
    child->SetVisible(true);
  }
}

void BraveOmniboxResultView::SetMatch(const AutocompleteMatch& match) {
  ResetChildren();
  OmniboxResultView::SetMatch(match);

  UpdateForBraveSearchConversion();
  UpdateForLeoMatch();
}

void BraveOmniboxResultView::OnSelectionStateChanged() {
  OmniboxResultView::OnSelectionStateChanged();

  HandleSelectionStateChangedForPromotionView();
}

gfx::Image BraveOmniboxResultView::GetIcon() const {
  if (LeoProvider::IsMatchFromLeoProvider(match_)) {
    // As Leo icon has gradient color, we can't use vector icon because it lacks
    // of gradient color.
    return ui::ResourceBundle::GetSharedInstance().GetImageNamed(
        IDR_LEO_FAVICON);
  }
  return OmniboxResultView::GetIcon();
}

void BraveOmniboxResultView::OnThemeChanged() {
  OmniboxResultView::OnThemeChanged();
  UpdateForLeoMatch();
}

void BraveOmniboxResultView::OpenMatch() {
  popup_view_->model()->OpenSelection(OmniboxPopupSelection(model_index_),
                                      base::TimeTicks::Now());
}

void BraveOmniboxResultView::RefreshOmniboxResult() {
  auto* controller = popup_view_->controller()->autocomplete_controller();

  // To refresh autocomplete result, start again with current input.
  controller->Start(controller->input());
}

BraveOmniboxPopupViewViews* BraveOmniboxResultView::GetPopupView() {
  return static_cast<BraveOmniboxPopupViewViews*>(popup_view_);
}

void BraveOmniboxResultView::HandleSelectionStateChangedForPromotionView() {
  if (brave_search_promotion_view_ && IsBraveSearchPromotionMatch(match_)) {
    brave_search_promotion_view_->OnSelectionStateChanged(
        GetMatchSelected() &&
        popup_view_->GetSelection().state == OmniboxPopupSelection::NORMAL);
  }
}

void BraveOmniboxResultView::UpdateForBraveSearchConversion() {
  if (!IsBraveSearchPromotionMatch(match_)) {
    return;
  }

  // Hide upstream children and show our promotion view.
  // It'll be the only visible child view.
  for (auto& child : children()) {
    child->SetVisible(false);
  }

  // To have proper size for promotion view.
  SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kVertical);

  CHECK(!brave_search_promotion_view_);
  auto* controller = popup_view_->controller()->autocomplete_controller();
  auto* prefs = controller->autocomplete_provider_client()->GetPrefs();
  brave_search_promotion_view_ =
      AddChildView(std::make_unique<BraveSearchConversionPromotionView>(
          this, g_browser_process->local_state(), prefs,
          popup_view_->controller()->client()->GetTemplateURLService()));

  brave_search_promotion_view_->SetTypeAndInput(
      GetConversionTypeFromMatch(match_),
      popup_view_->controller()->autocomplete_controller()->input().text());
  HandleSelectionStateChangedForPromotionView();
}

void BraveOmniboxResultView::UpdateForLeoMatch() {
  if (LeoProvider::IsMatchFromLeoProvider(match_)) {
    constexpr int kLeoMatchPadding = 4;
    SetProperty(views::kMarginsKey, gfx::Insets().set_top(kLeoMatchPadding));
    if (auto* cp = GetColorProvider()) {
      SetBorder(views::CreatePaddedBorder(
          views::CreateSolidSidedBorder(
              gfx::Insets().set_top(1),
              cp->GetColor(kColorBraveOmniboxResultViewSeparator)),
          gfx::Insets().set_top(kLeoMatchPadding)));
    }
  } else {
    ClearProperty(views::kMarginsKey);
    SetBorder(nullptr);
  }
}

void BraveOmniboxResultView::OnPaintBackground(gfx::Canvas* canvas) {
  gfx::ScopedCanvas scoped_canvas(canvas);
  if (LeoProvider::IsMatchFromLeoProvider(match_)) {
    // Clip upper padding
    canvas->ClipRect(GetContentsBounds());
  }

  OmniboxResultView::OnPaintBackground(canvas);
}

BEGIN_METADATA(BraveOmniboxResultView)
END_METADATA
