// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TEXT_FILE_EXTRACTOR_H_
#define BRAVE_BROWSER_AI_CHAT_TEXT_FILE_EXTRACTOR_H_

#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ai_chat/file_text_extractor_base.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace ai_chat {

// Extracts text from a file by loading it in a hidden background WebContents.
// Chromium's renderer handles MIME sniffing and renders the file content,
// then text is extracted via document.body.innerText.
//
// The extractor should be kept alive until the callback fires.
class TextFileExtractor : public FileTextExtractorBase {
 public:
  TextFileExtractor();
  ~TextFileExtractor() override;

  // Use an existing file path directly (e.g. from file picker).
  void ExtractText(content::BrowserContext* browser_context,
                   const base::FilePath& file_path,
                   ExtractTextCallback callback);

  // Write bytes to a temp file first (e.g. from drag-and-drop).
  // |original_extension| is preserved for MIME type detection.
  void ExtractText(content::BrowserContext* browser_context,
                   std::vector<uint8_t> file_bytes,
                   const base::FilePath::StringType& original_extension,
                   ExtractTextCallback callback);

 private:
  FRIEND_TEST_ALL_PREFIXES(TextFileExtractorTest,
                           OnTextExtracted_StripsViewSourcePrefix);
  FRIEND_TEST_ALL_PREFIXES(TextFileExtractorTest,
                           OnTextExtracted_StripsLeadingTabs);
  FRIEND_TEST_ALL_PREFIXES(TextFileExtractorTest, OnTextExtracted_EmptyFile);
  FRIEND_TEST_ALL_PREFIXES(TextFileExtractorTest,
                           OnTextExtracted_NonStringReturnsNullopt);

  // FileTextExtractorBase:
  GURL GetLoadURL(const base::FilePath& file_path) const override;
  void OnDocumentReady() override;

  void OnTextExtracted(base::Value result);

  base::WeakPtrFactory<TextFileExtractor> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_TEXT_FILE_EXTRACTOR_H_
