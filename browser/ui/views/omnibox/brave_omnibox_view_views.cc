/* Copyright (c) 2023 The Brave Authors. All rights reserved.
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

namespace {
void BraveUpdateContextMenu(ui::SimpleMenuModel* menu_contents, GURL url) {
  if (!url.SchemeIsHTTPOrHTTPS())
    return;
  absl::optional<size_t> copy_position =
      menu_contents->GetIndexOfCommandId(views::Textfield::kCopy);
  if (!copy_position)
    return;
  menu_contents->InsertItemWithStringIdAt(
      copy_position.value() + 1, IDC_COPY_CLEAN_LINK, IDS_COPY_CLEAN_LINK);
}
void BraveCopyCleanURL(Profile* profile,
                       OmniboxEditModel* model,
                       int sel_min,
                       std::u16string selected_text) {
  GURL url;
  bool write_url = false;
  model->AdjustTextForCopy(sel_min, &selected_text, &url, &write_url);
  if (!write_url || !url.is_valid())
    return;
  GURL sanitized_url =
      brave::URLSanitizerServiceFactory::GetForBrowserContext(profile)
          ->SanitizeURL(url);
  ui::ScopedClipboardWriter scoped_clipboard_writer(
      ui::ClipboardBuffer::kCopyPaste);
  scoped_clipboard_writer.WriteText(base::UTF8ToUTF16(sanitized_url.spec()));
}

}  // namespace
BraveOmniboxViewViews::~BraveOmniboxViewViews() = default;

bool BraveOmniboxViewViews::SelectedTextIsURL() {
  GURL url;
  bool write_url = false;
  std::u16string selected_text = GetSelectedText();
  model()->AdjustTextForCopy(GetSelectedRange().GetMin(), &selected_text, &url,
                             &write_url);
  return write_url;
}

bool BraveOmniboxViewViews::IsCleanLinkCommand(int command_id) const {
  return command_id == IDC_COPY_CLEAN_LINK;
}

void BraveOmniboxViewViews::OnSanitizedCopy(
    ui::ClipboardBuffer clipboard_buffer) {
  ExecuteCommand(IDC_COPY_CLEAN_LINK, 0);
}

void BraveOmniboxViewViews::ExecuteCommand(int command_id, int event_flags) {
  if (command_id == IDC_COPY_CLEAN_LINK) {
    BraveCopyCleanURL(location_bar_view_->profile(), model(),
                      GetSelectedRange().GetMin(), GetSelectedText());
    return;
  }
  OmniboxViewViews::ExecuteCommand(command_id, event_flags);
}

void BraveOmniboxViewViews::UpdateContextMenu(
    ui::SimpleMenuModel* menu_contents) {
  OmniboxViewViews::UpdateContextMenu(menu_contents);
  if (model()->CurrentTextIsURL() && !GetSelectedText().empty()) {
    BraveUpdateContextMenu(menu_contents,
                           controller()->GetLocationBarModel()->GetURL());
  }
}
