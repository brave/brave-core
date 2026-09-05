// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_PDF_TEXT_HELPER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_PDF_TEXT_HELPER_H_

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback.h"

namespace content {
class WebContents;
}  // namespace content

namespace ai_chat {

using PdfTextCallback = base::OnceCallback<void(std::optional<std::string>)>;

// Extracts all page text from a PDF loaded in the given WebContents via
// PDFDocumentHelper. Assumes the PDF document has already finished loading.
// Calls |callback| with the joined text or nullopt on failure.
void ExtractTextFromLoadedPdf(content::WebContents* web_contents,
                              PdfTextCallback callback);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_PDF_TEXT_HELPER_H_
