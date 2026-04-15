// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TEXT_FILE_EXTRACTOR_H_
#define BRAVE_BROWSER_AI_CHAT_TEXT_FILE_EXTRACTOR_H_

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ai_chat/file_text_extractor_base.h"

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
