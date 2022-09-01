/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/omnibox/omnibox_view_views.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/url_sanitizer/url_sanitizer_service_factory.h"
#include "brave/components/url_sanitizer/browser/url_sanitizer_service.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/translate/chrome_translate_client.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_view_views.h"
#include "chrome/browser/ui/views/send_tab_to_self/send_tab_to_self_bubble_controller.h"
#include "chrome/grit/generated_resources.h"
#include "components/omnibox/browser/omnibox_edit_model.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/views/controls/textfield/textfield.h"

namespace {

void BraveUpdateContextMenu(ui::SimpleMenuModel* menu_contents) {
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

int GetSearchEnginesID() {
  return IDS_MANAGE_SEARCH_ENGINES_AND_SITE_SEARCH;
}

}  // namespace

#define ShowBubble                                                     \
  ShowBubble();                                                        \
  return;                                                              \
  case IDC_COPY_CLEAN_LINK:                                            \
    BraveCopyCleanURL(location_bar_view_->profile(), model(),          \
                      GetSelectedRange().GetMin(), GetSelectedText()); \
    GetSelectedText

#undef IDS_MANAGE_SEARCH_ENGINES_AND_SITE_SEARCH
#define IDS_MANAGE_SEARCH_ENGINES_AND_SITE_SEARCH GetSearchEnginesID()); \
  if (model()->CurrentTextIsURL() && !GetSelectedText().empty())         \
    BraveUpdateContextMenu(menu_contents
#include "src/chrome/browser/ui/views/omnibox/omnibox_view_views.cc"
#undef IDS_MANAGE_SEARCH_ENGINES_AND_SITE_SEARCH
#define IDS_MANAGE_SEARCH_ENGINES_AND_SITE_SEARCH GetSearchEnginesID()

#undef ShowBubble
