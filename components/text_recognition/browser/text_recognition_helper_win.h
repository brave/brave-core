/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TEXT_RECOGNITION_BROWSER_TEXT_RECOGNITION_HELPER_WIN_H_
#define BRAVE_COMPONENTS_TEXT_RECOGNITION_BROWSER_TEXT_RECOGNITION_HELPER_WIN_H_

#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/containers/flat_set.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "third_party/skia/include/core/SkBitmap.h"

class SkBitmap;

namespace text_recognition {

// Request text detection for all available languages, and pick best result
// and deliver it via |callback_|.
class COMPONENT_EXPORT(TEXT_RECOGNITION_BROWSER) TextRecognitionHelperWin {
 public:
  TextRecognitionHelperWin();
  ~TextRecognitionHelperWin();

  TextRecognitionHelperWin(const TextRecognitionHelperWin&) = delete;
  TextRecognitionHelperWin& operator=(const TextRecognitionHelperWin&) = delete;

  void GetTextFromImage(
      const SkBitmap& image,
      base::OnceCallback<void(const std::vector<std::string>&)> callback);

 private:
  void SetResultText(const std::vector<std::string>& text);
  void OnGetTextFromImage(const std::vector<std::string>& text);
  void OnGetAvailableLanguages(const base::flat_set<std::string>& languages);

  // The number of supported language code for text detection from image.
  int remained_language_detection_request_count_ = 0;
  std::vector<std::string> best_result_;
  SkBitmap image_;

  // Called when final result is ready.
  base::OnceCallback<void(const std::vector<std::string>&)> callback_;
  base::WeakPtrFactory<TextRecognitionHelperWin> weak_factory_{this};
};

}  // namespace text_recognition

#endif  // BRAVE_COMPONENTS_TEXT_RECOGNITION_BROWSER_TEXT_RECOGNITION_HELPER_WIN_H_
