/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/omnibox/omnibox_row_view.h"

#include "brave/browser/ui/views/omnibox/brave_omnibox_result_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_header_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_match_cell_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_popup_view_views.h"
#include "chrome/browser/ui/views/omnibox/omnibox_result_view.h"
#include "components/omnibox/browser/omnibox_controller.h"
#include "components/omnibox/browser/omnibox_edit_model.h"
#include "components/omnibox/browser/omnibox_field_trial.h"
#include "components/omnibox/browser/omnibox_popup_selection.h"

// This substitution is being added to have OmniboxRowView adding a
// BraveOmniboxResultView instance as a child view, in the constructor code.
#define OmniboxResultView BraveOmniboxResultView
#define GetInsets GetInsets_UnUsed
#include "src/chrome/browser/ui/views/omnibox/omnibox_row_view.cc"
#undef GetInsets
#undef OmniboxResultView

gfx::Insets OmniboxRowView::GetInsets() const {
  return gfx::Insets();
}
