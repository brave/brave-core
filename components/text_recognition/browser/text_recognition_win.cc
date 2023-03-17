/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/text_recognition/browser/text_recognition.h"

#include <windows.foundation.collections.h>
#include <windows.globalization.h>
#include <windows.graphics.imaging.h>
#include <windows.media.ocr.h>
#include <wrl/client.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/win/core_winrt_util.h"
#include "base/win/scoped_hstring.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/text_recognition/browser/text_recognizer_win.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "services/shape_detection/detection_utils_win.h"
#include "services/shape_detection/public/mojom/textdetection.mojom.h"

using ABI::Windows::Foundation::Collections::IVectorView;
using ABI::Windows::Globalization::ILanguage;
using ABI::Windows::Globalization::ILanguageFactory;
using ABI::Windows::Globalization::Language;
using ABI::Windows::Graphics::Imaging::ISoftwareBitmap;
using ABI::Windows::Graphics::Imaging::ISoftwareBitmapStatics;
using ABI::Windows::Media::Ocr::IOcrEngine;
using ABI::Windows::Media::Ocr::IOcrEngineStatics;
using base::win::GetActivationFactory;
using base::win::ScopedHString;
using Microsoft::WRL::ComPtr;

// Referred shape_detection::TextDetectionImplWin.
namespace text_recognition {

namespace {

void RunOnUIThread(
    base::OnceCallback<void(const std::vector<std::string>&)> callback,
    const std::vector<std::string>& result = {}) {
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), result));
}

}  // namespace

void GetTextFromImage(const std::string& language_code,
                      const SkBitmap& image,
                      base::OnceCallback<void(const std::vector<std::string>&)>
                          callback_run_on_ui_thread) {
  CHECK(!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  // Loads functions dynamically at runtime to prevent library dependencies.
  if (!base::win::ResolveCoreWinRTDelayload()) {
    VLOG(2) << "Failed loading functions from combase.dll";
    RunOnUIThread(std::move(callback_run_on_ui_thread));
    return;
  }

  ComPtr<IOcrEngineStatics> engine_factory;
  HRESULT hr = GetActivationFactory<IOcrEngineStatics,
                                    RuntimeClass_Windows_Media_Ocr_OcrEngine>(
      &engine_factory);
  if (FAILED(hr)) {
    VLOG(2) << "IOcrEngineStatics factory failed: "
            << logging::SystemErrorCodeToString(hr);
    RunOnUIThread(std::move(callback_run_on_ui_thread));
    return;
  }

  ComPtr<ILanguageFactory> language_factory;
  hr = GetActivationFactory<ILanguageFactory,
                            RuntimeClass_Windows_Globalization_Language>(
      &language_factory);
  if (FAILED(hr)) {
    VLOG(2) << "ILanguage factory failed: "
            << logging::SystemErrorCodeToString(hr);
    RunOnUIThread(std::move(callback_run_on_ui_thread));
    return;
  }

  ScopedHString language_hstring = ScopedHString::Create(language_code);
  if (!language_hstring.is_valid()) {
    RunOnUIThread(std::move(callback_run_on_ui_thread));
    return;
  }

  ComPtr<ILanguage> language;
  hr = language_factory->CreateLanguage(language_hstring.get(), &language);
  if (FAILED(hr)) {
    VLOG(2) << "Create language failed: "
            << logging::SystemErrorCodeToString(hr);
    RunOnUIThread(std::move(callback_run_on_ui_thread));
    return;
  }

  boolean is_supported = false;
  hr = engine_factory->IsLanguageSupported(language.Get(), &is_supported);
  if (FAILED(hr) || !is_supported) {
    RunOnUIThread(std::move(callback_run_on_ui_thread));
    return;
  }

  ComPtr<IOcrEngine> ocr_engine;
  hr = engine_factory->TryCreateFromLanguage(language.Get(), &ocr_engine);
  if (FAILED(hr)) {
    VLOG(2) << "Create engine failed from language: "
            << logging::SystemErrorCodeToString(hr);
    RunOnUIThread(std::move(callback_run_on_ui_thread));
    return;
  }

  ComPtr<ISoftwareBitmapStatics> bitmap_factory;
  hr = GetActivationFactory<
      ISoftwareBitmapStatics,
      RuntimeClass_Windows_Graphics_Imaging_SoftwareBitmap>(&bitmap_factory);
  if (FAILED(hr)) {
    VLOG(2) << "ISoftwareBitmapStatics factory failed: "
            << logging::SystemErrorCodeToString(hr);
    RunOnUIThread(std::move(callback_run_on_ui_thread));
    return;
  }

  auto recognizer = std::make_unique<TextRecognizerWin>(
      std::move(ocr_engine), std::move(bitmap_factory));
  auto* recognizer_ptr = recognizer.get();
  recognizer_ptr->Detect(
      image, base::BindOnce(
                 [](std::unique_ptr<TextRecognizerWin> recognizer,
                    base::OnceCallback<void(const std::vector<std::string>&)>
                        callback_run_on_ui_thread,
                    std::vector<shape_detection::mojom::TextDetectionResultPtr>
                        result) {
                   std::vector<std::string> detected_string;
                   for (const auto& re : result) {
                     detected_string.push_back(re->raw_value);
                   }
                   RunOnUIThread(std::move(callback_run_on_ui_thread),
                                 detected_string);
                 },
                 std::move(recognizer), std::move(callback_run_on_ui_thread)));
}

base::flat_set<std::string> GetAvailableRecognizerLanguages() {
  // Loads functions dynamically at runtime to prevent library dependencies.
  if (!base::win::ResolveCoreWinRTDelayload()) {
    VLOG(2) << "Failed loading functions from combase.dll";
    return {};
  }

  ComPtr<IOcrEngineStatics> engine_factory;
  HRESULT hr = GetActivationFactory<IOcrEngineStatics,
                                    RuntimeClass_Windows_Media_Ocr_OcrEngine>(
      &engine_factory);
  if (FAILED(hr)) {
    VLOG(2) << "IOcrEngineStatics factory failed: "
            << logging::SystemErrorCodeToString(hr);
    return {};
  }

  ComPtr<IVectorView<Language*>> available_languages;
  hr = engine_factory->get_AvailableRecognizerLanguages(&available_languages);
  if (FAILED(hr)) {
    VLOG(2) << "Fetch available languages failed: "
            << logging::SystemErrorCodeToString(hr);
    return {};
  }

  uint32_t lang_count;
  hr = available_languages->get_Size(&lang_count);
  if (FAILED(hr)) {
    VLOG(2) << "get_Size failed: " << logging::SystemErrorCodeToString(hr);
    return {};
  }

  base::flat_set<std::string> available_language_codes;
  for (uint32_t i = 0; i < lang_count; ++i) {
    ComPtr<ILanguage> lang;
    hr = available_languages->GetAt(i, &lang);
    if (FAILED(hr)) {
      continue;
    }

    HSTRING text;
    hr = lang->get_LanguageTag(&text);
    if (FAILED(hr)) {
      continue;
    }

    available_language_codes.insert(
        brave_l10n::GetISOLanguageCode(ScopedHString(text).GetAsUTF8()));
  }

  return available_language_codes;
}

}  // namespace text_recognition
