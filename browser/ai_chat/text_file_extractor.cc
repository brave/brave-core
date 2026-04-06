// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/text_file_extractor.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "net/base/filename_util.h"
#include "third_party/blink/public/strings/grit/blink_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace ai_chat {

TextFileExtractor::TextFileExtractor() = default;

TextFileExtractor::~TextFileExtractor() = default;

void TextFileExtractor::ExtractText(content::BrowserContext* browser_context,
                                    const base::FilePath& file_path,
                                    ExtractTextCallback callback) {
  CHECK(!callback_) << "ExtractText called while extraction in progress";
  callback_ = std::move(callback);
  LoadInWebContents(browser_context, file_path);
}

void TextFileExtractor::ExtractText(
    content::BrowserContext* browser_context,
    std::vector<uint8_t> file_bytes,
    const base::FilePath::StringType& original_extension,
    ExtractTextCallback callback) {
  CHECK(!callback_) << "ExtractText called while extraction in progress";
  callback_ = std::move(callback);
  WriteTempFileAndLoad(browser_context, std::move(file_bytes),
                       original_extension);
}

GURL TextFileExtractor::GetLoadURL(const base::FilePath& file_path) const {
  // Use view-source: to prevent HTML/XHTML from being rendered (which could
  // execute scripts or load external resources). The source is displayed as
  // raw text in the view-source document.
  return GURL(base::StrCat({content::kViewSourceScheme, ":",
                            net::FilePathToFileURL(file_path).spec()}));
}

void TextFileExtractor::OnDocumentReady() {
  auto* rfh = GetWebContents()->GetPrimaryMainFrame();
  if (!rfh) {
    Finish(std::nullopt);
    return;
  }

  rfh->ExecuteJavaScriptInIsolatedWorld(
      u"document.body.innerText",
      base::BindOnce(&TextFileExtractor::OnTextExtracted,
                     weak_ptr_factory_.GetWeakPtr()),
      ISOLATED_WORLD_ID_BRAVE_INTERNAL);
}

void TextFileExtractor::OnTextExtracted(base::Value result) {
  if (result.is_string()) {
    // view-source: innerText starts with the localized "Line wrap" label
    // followed by a newline, and indents each line with a leading tab.
    // Strip the label prefix and the per-line tabs.
    // For non-empty files the prefix is "Line wrap\n\t", for empty files
    // it's just "Line wrap" with no trailing newline or tab.
    std::string text = std::move(result).TakeString();
    std::string line_wrap_label =
        l10n_util::GetStringUTF8(IDS_VIEW_SOURCE_LINE_WRAP);
    if (auto rest = base::RemovePrefix(text, line_wrap_label)) {
      text = std::string(base::RemovePrefix(*rest, "\n\t").value_or(*rest));
    }
    base::ReplaceSubstringsAfterOffset(&text, 0, "\n\t", "\n");
    Finish(std::move(text));
  } else {
    DVLOG(1) << "TextFileExtractor: JS returned non-string result";
    Finish(std::nullopt);
  }
}

}  // namespace ai_chat
