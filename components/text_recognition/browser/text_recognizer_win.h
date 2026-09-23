/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TEXT_RECOGNITION_BROWSER_TEXT_RECOGNIZER_WIN_H_
#define BRAVE_COMPONENTS_TEXT_RECOGNITION_BROWSER_TEXT_RECOGNIZER_WIN_H_

#include <windows.graphics.imaging.h>
#include <windows.media.ocr.h>
#include <wrl/client.h>

#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "services/shape_detection/public/mojom/textdetection.mojom.h"

// Referred shape_detection::TextDetectionImplWin.
namespace text_recognition {

class TextRecognizerWin {
 public:
  TextRecognizerWin(
      Microsoft::WRL::ComPtr<ABI::Windows::Media::Ocr::IOcrEngine> ocr_engine,
      Microsoft::WRL::ComPtr<
          ABI::Windows::Graphics::Imaging::ISoftwareBitmapStatics>
          bitmap_factory);

  TextRecognizerWin(const TextRecognizerWin&) = delete;
  TextRecognizerWin& operator=(const TextRecognizerWin&) = delete;

  ~TextRecognizerWin();

  void Detect(
      const SkBitmap& bitmap,
      base::OnceCallback<void(const std::pair<bool, std::vector<std::string>>&)>
          callback);

 private:
  Microsoft::WRL::ComPtr<ABI::Windows::Media::Ocr::IOcrEngine> ocr_engine_;
  Microsoft::WRL::ComPtr<
      ABI::Windows::Graphics::Imaging::ISoftwareBitmapStatics>
      bitmap_factory_;
  base::OnceCallback<void(const std::pair<bool, std::vector<std::string>>&)>
      recognize_text_callback_;

  HRESULT BeginDetect(const SkBitmap& bitmap);
  std::vector<shape_detection::mojom::TextDetectionResultPtr>
  BuildTextDetectionResult(
      Microsoft::WRL::ComPtr<ABI::Windows::Media::Ocr::IOcrResult> ocr_result);
  void OnTextDetected(
      Microsoft::WRL::ComPtr<ABI::Windows::Graphics::Imaging::ISoftwareBitmap>
          win_bitmap,
      Microsoft::WRL::ComPtr<ABI::Windows::Media::Ocr::IOcrResult> ocr_result);

  base::WeakPtrFactory<TextRecognizerWin> weak_factory_{this};
};

}  // namespace text_recognition

#endif  // BRAVE_COMPONENTS_TEXT_RECOGNITION_BROWSER_TEXT_RECOGNIZER_WIN_H_
