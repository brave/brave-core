// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_LOCAL_AI_FAST_VLM_TAB_CONTENT_EXTRACTOR_H_
#define BRAVE_BROWSER_LOCAL_AI_FAST_VLM_TAB_CONTENT_EXTRACTOR_H_

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace content {
class WebContents;
}

namespace local_ai {

// Helper class to extract visual content from tabs using FastVLM service
// This replaces text-based content extraction with screenshot + AI description
class FastVLMTabContentExtractor {
 public:
  FastVLMTabContentExtractor();
  ~FastVLMTabContentExtractor();

  FastVLMTabContentExtractor(const FastVLMTabContentExtractor&) = delete;
  FastVLMTabContentExtractor& operator=(const FastVLMTabContentExtractor&) =
      delete;

  // Extract visual content from a web contents using screenshot + FastVLM
  // Callback returns (tab_index, visual_description)
  using ExtractContentCallback =
      base::OnceCallback<void(int, const std::string&)>;

  static void ExtractVisualContent(content::WebContents* web_contents,
                                   int tab_index,
                                   ExtractContentCallback callback);

 private:
  static void OnSurfaceCopied(content::WebContents* web_contents,
                              int tab_index,
                              ExtractContentCallback callback,
                              const SkBitmap& bitmap);

  static void OnFastVLMAnalysisComplete(int tab_index,
                                        ExtractContentCallback callback,
                                        bool success,
                                        const std::string& description);
};

}  // namespace local_ai

#endif  // BRAVE_BROWSER_LOCAL_AI_FAST_VLM_TAB_CONTENT_EXTRACTOR_H_
