/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/text_recognition/browser/text_recognizer_win.h"

#include <windows.foundation.collections.h>
#include <windows.globalization.h>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/logging.h"
#include "base/win/core_winrt_util.h"
#include "base/win/post_async_results.h"
#include "base/win/scoped_hstring.h"
#include "services/shape_detection/detection_utils_win.h"
#include "services/shape_detection/public/mojom/textdetection.mojom.h"

using ABI::Windows::Foundation::IAsyncOperation;
using ABI::Windows::Foundation::Collections::IVectorView;
using ABI::Windows::Graphics::Imaging::ISoftwareBitmap;
using ABI::Windows::Graphics::Imaging::ISoftwareBitmapStatics;
using ABI::Windows::Media::Ocr::IOcrEngine;
using ABI::Windows::Media::Ocr::IOcrLine;
using ABI::Windows::Media::Ocr::IOcrResult;
using ABI::Windows::Media::Ocr::OcrLine;
using ABI::Windows::Media::Ocr::OcrResult;
using base::win::ScopedHString;
using Microsoft::WRL::ComPtr;

namespace text_recognition {

TextRecognizerWin::TextRecognizerWin(
    ComPtr<IOcrEngine> ocr_engine,
    ComPtr<ISoftwareBitmapStatics> bitmap_factory)
    : ocr_engine_(std::move(ocr_engine)),
      bitmap_factory_(std::move(bitmap_factory)) {
  DCHECK(ocr_engine_);
  DCHECK(bitmap_factory_);
}

TextRecognizerWin::~TextRecognizerWin() = default;

void TextRecognizerWin::Detect(
    const SkBitmap& bitmap,
    base::OnceCallback<void(const std::pair<bool, std::vector<std::string>>&)>
        callback) {
  if (FAILED(BeginDetect(bitmap))) {
    // No detection taking place; run |callback| with an empty array of results.
    std::move(callback).Run({false, {}});
    return;
  }
  // Hold on the callback until AsyncOperation completes.
  recognize_text_callback_ = std::move(callback);
}

HRESULT TextRecognizerWin::BeginDetect(const SkBitmap& bitmap) {
  ComPtr<ISoftwareBitmap> win_bitmap =
      shape_detection::CreateWinBitmapFromSkBitmap(bitmap,
                                                   bitmap_factory_.Get());
  if (!win_bitmap) {
    return E_FAIL;
  }

  // Recognize text asynchronously.
  ComPtr<IAsyncOperation<OcrResult*>> async_op;
  const HRESULT hr = ocr_engine_->RecognizeAsync(win_bitmap.Get(), &async_op);
  if (FAILED(hr)) {
    VLOG(2) << "Recognize text asynchronously failed: "
            << logging::SystemErrorCodeToString(hr);
    return hr;
  }

  // Use WeakPtr to bind the callback so that the once callback will not be run
  // if this object has been already destroyed. |win_bitmap| needs to be kept
  // alive until OnTextDetected().
  return base::win::PostAsyncResults(
      std::move(async_op),
      base::BindOnce(&TextRecognizerWin::OnTextDetected,
                     weak_factory_.GetWeakPtr(), std::move(win_bitmap)));
}

std::vector<shape_detection::mojom::TextDetectionResultPtr>
TextRecognizerWin::BuildTextDetectionResult(ComPtr<IOcrResult> ocr_result) {
  std::vector<shape_detection::mojom::TextDetectionResultPtr> results;
  if (!ocr_result) {
    return results;
  }

  ComPtr<IVectorView<OcrLine*>> ocr_lines;
  HRESULT hr = ocr_result->get_Lines(&ocr_lines);
  if (FAILED(hr)) {
    VLOG(2) << "Get Lines failed: " << logging::SystemErrorCodeToString(hr);
    return results;
  }

  uint32_t count;
  hr = ocr_lines->get_Size(&count);
  if (FAILED(hr)) {
    VLOG(2) << "get_Size failed: " << logging::SystemErrorCodeToString(hr);
    return results;
  }

  results.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    ComPtr<IOcrLine> line;
    hr = ocr_lines->GetAt(i, &line);
    if (FAILED(hr)) {
      break;
    }

    HSTRING text;
    hr = line->get_Text(&text);
    if (FAILED(hr)) {
      break;
    }

    auto result = shape_detection::mojom::TextDetectionResult::New();
    result->raw_value = ScopedHString(text).GetAsUTF8();
    results.push_back(std::move(result));
  }
  return results;
}

// |win_bitmap| is passed here so that it is kept alive until the AsyncOperation
// completes because RecognizeAsync does not hold a reference.
void TextRecognizerWin::OnTextDetected(ComPtr<ISoftwareBitmap> /* win_bitmap */,
                                       ComPtr<IOcrResult> ocr_result) {
  std::vector<std::string> detected_string;
  for (const auto& re : BuildTextDetectionResult(std::move(ocr_result))) {
    detected_string.push_back(re->raw_value);
  }

  std::move(recognize_text_callback_).Run({true, detected_string});
}

}  // namespace text_recognition
