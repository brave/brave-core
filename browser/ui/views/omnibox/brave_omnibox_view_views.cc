/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/omnibox/brave_omnibox_view_views.h"

#include "base/feature_list.h"
#include "brave/browser/ui/toolbar/brave_location_bar_model_delegate.h"
#include "brave/common/url_constants.h"
#include "components/omnibox/browser/omnibox_edit_model.h"
#include "content/public/common/url_constants.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "url/gurl.h"

namespace {
// Copied from omnibox_view_views.cc
constexpr base::Feature kOmniboxCanCopyHyperlinksToClipboard{
    "OmniboxCanCopyHyperlinksToClipboard", base::FEATURE_ENABLED_BY_DEFAULT};

bool ShouldAdjust(const base::string16& original,
                  const base::string16& adjusted) {
  // Only adjust scheme is changed from brave to chrome.
  if (GURL(original).scheme() == kBraveUIScheme &&
      GURL(adjusted).scheme() == content::kChromeUIScheme) {
    return true;
  }

  return false;
}

void ReplaceChromeSchemeToBrave(GURL* url) {
  DCHECK_EQ(content::kChromeUIScheme, url->scheme());
  GURL::Replacements replacements;
  replacements.SetSchemeStr(kBraveUIScheme);
  *url = url->ReplaceComponents(replacements);
}

}  // namespace

void BraveOmniboxViewViews::OnAfterCutOrCopy(
    ui::ClipboardBuffer clipboard_buffer) {
  ui::Clipboard* cb = ui::Clipboard::GetForCurrentThread();
  base::string16 selected_text;
  cb->ReadText(clipboard_buffer, &selected_text);
  const base::string16 original_selected_text = selected_text;

  GURL url;
  bool write_url = false;
  model()->AdjustTextForCopy(GetSelectedRange().GetMin(), &selected_text, &url,
                             &write_url);

  if (ShouldAdjust(original_selected_text, selected_text)) {
    BraveLocationBarModelDelegate::FormattedStringFromURL(url, &selected_text);
    ReplaceChromeSchemeToBrave(&url);
  }

  ui::ScopedClipboardWriter scoped_clipboard_writer(clipboard_buffer);
  scoped_clipboard_writer.WriteText(selected_text);

  if (write_url &&
      base::FeatureList::IsEnabled(kOmniboxCanCopyHyperlinksToClipboard)) {
    scoped_clipboard_writer.WriteHyperlink(selected_text, url.spec());
  }
}
