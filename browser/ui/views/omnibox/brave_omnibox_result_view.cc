/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/omnibox/brave_omnibox_result_view.h"

#include <memory>

#include "base/time/time.h"
#include "brave/browser/ui/views/omnibox/brave_search_conversion_promotion_view.h"
#include "brave/components/omnibox/browser/promotion_utils.h"
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
#include "ui/base/window_open_disposition.h"
#include "ui/views/controls/button/image_button.h"

BraveOmniboxResultView::~BraveOmniboxResultView() = default;

void BraveOmniboxResultView::ResetChildrenVisibility() {
  // Reset children visibility. Their visibility could be configured later
  // based on |match_| and the current input.
  // NOTE: The first child in the result box is supposed to be the
  // `suggestion_container_`, which used to be stored as a data member.
  children().front()->SetVisible(true);
  button_row_->SetVisible(true);
  if (brave_search_promotion_view_) {
    brave_search_promotion_view_->SetVisible(false);
  }
}

void BraveOmniboxResultView::SetMatch(const AutocompleteMatch& match) {
  ResetChildrenVisibility();
  OmniboxResultView::SetMatch(match);

  if (IsBraveSearchPromotionMatch(match)) {
    UpdateForBraveSearchConversion();
  }
}

void BraveOmniboxResultView::OnSelectionStateChanged() {
  OmniboxResultView::OnSelectionStateChanged();

  HandleSelectionStateChangedForPromotionView();
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

void BraveOmniboxResultView::HandleSelectionStateChangedForPromotionView() {
  if (brave_search_promotion_view_ && IsBraveSearchPromotionMatch(match_)) {
    brave_search_promotion_view_->OnSelectionStateChanged(
        GetMatchSelected() &&
        popup_view_->GetSelection().state == OmniboxPopupSelection::NORMAL);
  }
}

void BraveOmniboxResultView::UpdateForBraveSearchConversion() {
  DCHECK(IsBraveSearchPromotionMatch(match_));

  // Hide upstream children and show our promotion view.
  // NOTE: The first child in the result box is supposed to be the
  // `suggestion_container_`, which used to be stored as a data member.
  children().front()->SetVisible(false);
  button_row_->SetVisible(false);

  if (!brave_search_promotion_view_) {
    auto* controller = popup_view_->controller()->autocomplete_controller();
    auto* prefs = controller->autocomplete_provider_client()->GetPrefs();
    brave_search_promotion_view_ =
        AddChildView(std::make_unique<BraveSearchConversionPromotionView>(
            this, g_browser_process->local_state(), prefs));
  }

  brave_search_promotion_view_->SetVisible(true);
  brave_search_promotion_view_->SetTypeAndInput(
      GetConversionTypeFromMatch(match_),
      popup_view_->controller()->autocomplete_controller()->input().text());
}

BEGIN_METADATA(BraveOmniboxResultView, OmniboxResultView)
END_METADATA
