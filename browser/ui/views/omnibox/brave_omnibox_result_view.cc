/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/omnibox/brave_omnibox_result_view.h"

#include <memory>

#include "base/time/time.h"
#include "brave/browser/ui/views/omnibox/brave_search_conversion_promotion_view.h"
#include "brave/components/brave_search_conversion/types.h"
#include "brave/components/brave_search_conversion/utils.h"
#include "brave/components/omnibox/browser/promotion_utils.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/views/omnibox/omnibox_popup_view_views.h"
#include "chrome/browser/ui/views/omnibox/omnibox_suggestion_button_row_view.h"
#include "components/omnibox/browser/autocomplete_controller.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"
#include "components/omnibox/browser/autocomplete_result.h"
#include "components/omnibox/browser/omnibox_edit_model.h"
#include "components/omnibox/browser/omnibox_popup_selection.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/window_open_disposition.h"
#include "ui/views/controls/button/image_button.h"

using brave_search_conversion::ConversionType;
using brave_search_conversion::IsBraveSearchConversionFetureEnabled;
using brave_search_conversion::SetDismissed;

BraveOmniboxResultView::~BraveOmniboxResultView() = default;

void BraveOmniboxResultView::ResetChildrenVisibility() {
  // Reset children visibility. Their visibility could be configured later
  // based on |match_| and the current input.
  suggestion_container_->SetVisible(true);
  button_row_->SetVisible(true);
  if (brave_search_promotion_view_) {
    brave_search_promotion_view_->SetVisible(false);
  }
}

void BraveOmniboxResultView::SetMatch(const AutocompleteMatch& match) {
  if (!IsBraveSearchConversionFetureEnabled()) {
    OmniboxResultView::SetMatch(match);
    return;
  }

  ResetChildrenVisibility();
  OmniboxResultView::SetMatch(match);

  UpdateForBraveSearchConversion();
}

void BraveOmniboxResultView::OnSelectionStateChanged() {
  OmniboxResultView::OnSelectionStateChanged();

  HandleSelectionStateChangedForPromotionView();
}

void BraveOmniboxResultView::OpenMatch() {
  model_->OpenSelection(OmniboxPopupSelection(model_index_),
                        base::TimeTicks::Now());
}

void BraveOmniboxResultView::Dismiss() {
  auto* controller = model_->autocomplete_controller();
  auto* prefs = controller->autocomplete_provider_client()->GetPrefs();
  SetDismissed(prefs);

  // To refresh autocomplete result after dismiss, start again with current
  // input. Then, popup gets same autocomplete matches w/o promotion match.
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
  if (!IsBraveSearchPromotionMatch(match_))
    return;

  // Hide upstream children and show our promotion view.
  suggestion_container_->SetVisible(false);
  button_row_->SetVisible(false);

  if (!brave_search_promotion_view_) {
    brave_search_promotion_view_ =
        AddChildView(std::make_unique<BraveSearchConversionPromotionView>(
            this, g_browser_process->local_state()));
  }

  brave_search_promotion_view_->SetVisible(true);
  brave_search_promotion_view_->SetTypeAndInput(
      GetConversionTypeFromMatch(match_),
      model_->autocomplete_controller()->input().text());
}

BEGIN_METADATA(BraveOmniboxResultView, OmniboxResultView)
END_METADATA
