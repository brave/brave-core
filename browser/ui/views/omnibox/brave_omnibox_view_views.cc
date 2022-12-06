/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/omnibox/brave_omnibox_view_views.h"

#include <utility>

#include "brave/app/brave_command_ids.h"
#include "brave/browser/url_sanitizer/url_sanitizer_service_factory.h"
#include "brave/components/url_sanitizer/browser/url_sanitizer_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/omnibox/browser/omnibox_edit_controller.h"
#include "components/omnibox/browser/omnibox_edit_model.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"

BraveOmniboxViewViews::BraveOmniboxViewViews(
    OmniboxEditController* controller,
    std::unique_ptr<OmniboxClient> client,
    bool popup_window_mode,
    LocationBarView* location_bar,
    const gfx::FontList& font_list)
    : OmniboxViewViews(controller,
                       std::move(client),
                       popup_window_mode,
                       location_bar,
                       font_list) {}

BraveOmniboxViewViews::~BraveOmniboxViewViews() = default;

bool BraveOmniboxViewViews::SelectedTextIsURL() {
  GURL url;
  bool write_url = false;
  std::u16string selected_text = GetSelectedText();
  model()->AdjustTextForCopy(GetSelectedRange().GetMin(), &selected_text, &url,
                             &write_url);
  return write_url;
}
