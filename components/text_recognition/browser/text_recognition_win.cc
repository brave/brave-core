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

#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/scoped_blocking_call.h"
#include "base/win/core_winrt_util.h"
#include "base/win/scoped_hstring.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/text_recognition/browser/text_recognizer_win.h"

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

void GetTextFromImage(
    const std::string& language_code,
    const SkBitmap& image,
    base::OnceCallback<void(const std::pair<bool, std::vector<std::string>>&)>
        callback_run_on_ui_thread) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);

  ComPtr<IOcrEngineStatics> engine_factory;
  HRESULT hr = GetActivationFactory<IOcrEngineStatics,
                                    RuntimeClass_Windows_Media_Ocr_OcrEngine>(
      &engine_factory);
  if (FAILED(hr)) {
    VLOG(2) << "IOcrEngineStatics factory failed: "
            << logging::SystemErrorCodeToString(hr);
    std::move(callback_run_on_ui_thread).Run({false, {}});
    return;
  }

  ComPtr<ILanguageFactory> language_factory;
  hr = GetActivationFactory<ILanguageFactory,
                            RuntimeClass_Windows_Globalization_Language>(
      &language_factory);
  if (FAILED(hr)) {
    VLOG(2) << "ILanguage factory failed: "
            << logging::SystemErrorCodeToString(hr);
    std::move(callback_run_on_ui_thread).Run({false, {}});
    return;
  }

  ComPtr<IOcrEngine> ocr_engine;
  if (language_code.empty()) {
    hr = engine_factory->TryCreateFromUserProfileLanguages(&ocr_engine);
    if (FAILED(hr)) {
      VLOG(2) << "Create engine failed from language: "
              << logging::SystemErrorCodeToString(hr);
      std::move(callback_run_on_ui_thread).Run({false, {}});
      return;
    }
  } else {
    ScopedHString language_hstring = ScopedHString::Create(language_code);
    if (!language_hstring.is_valid()) {
      VLOG(2) << "Got invalid language code";
      std::move(callback_run_on_ui_thread).Run({false, {}});
      return;
    }

    ComPtr<ILanguage> language;
    hr = language_factory->CreateLanguage(language_hstring.get(), &language);
    if (FAILED(hr)) {
      VLOG(2) << "Create language failed: "
              << logging::SystemErrorCodeToString(hr);
      std::move(callback_run_on_ui_thread).Run({false, {}});
      return;
    }

    boolean is_supported = false;
    hr = engine_factory->IsLanguageSupported(language.Get(), &is_supported);
    if (FAILED(hr) || !is_supported) {
      std::move(callback_run_on_ui_thread).Run({false, {}});
      return;
    }

    hr = engine_factory->TryCreateFromLanguage(language.Get(), &ocr_engine);
    if (FAILED(hr)) {
      VLOG(2) << "Create engine failed from language: "
              << logging::SystemErrorCodeToString(hr);
      std::move(callback_run_on_ui_thread).Run({false, {}});
      return;
    }
  }

  ComPtr<ISoftwareBitmapStatics> bitmap_factory;
  hr = GetActivationFactory<
      ISoftwareBitmapStatics,
      RuntimeClass_Windows_Graphics_Imaging_SoftwareBitmap>(&bitmap_factory);
  if (FAILED(hr)) {
    VLOG(2) << "ISoftwareBitmapStatics factory failed: "
            << logging::SystemErrorCodeToString(hr);
    std::move(callback_run_on_ui_thread).Run({false, {}});
    return;
  }

  auto recognizer = std::make_unique<TextRecognizerWin>(
      std::move(ocr_engine), std::move(bitmap_factory));
  auto* recognizer_ptr = recognizer.get();
  recognizer_ptr->Detect(
      image, base::BindOnce(
                 [](std::unique_ptr<TextRecognizerWin> recognizer,
                    base::OnceCallback<void(
                        const std::pair<bool, std::vector<std::string>>&)>
                        callback_run_on_ui_thread,
                    const std::pair<bool, std::vector<std::string>>& result) {
                   std::move(callback_run_on_ui_thread).Run(result);
                 },
                 std::move(recognizer), std::move(callback_run_on_ui_thread)));
}

std::vector<std::string> GetAvailableRecognizerLanguages() {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);

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

  std::vector<std::string> language_codes;
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

    const auto code =
        brave_l10n::GetISOLanguageCode(ScopedHString(text).GetAsUTF8());
    if (base::Contains(language_codes, code)) {
      continue;
    }

    language_codes.push_back(code);
  }

  WCHAR name[127]{};
  ULONG nameLen = std::size(name);
  ULONG langs = -1;
  if (GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &langs,
                                  reinterpret_cast<PZZWSTR>(&name), &nameLen)) {
    std::string default_language_code =
        brave_l10n::GetISOLanguageCode(base::WideToUTF8(name));
    // Try to locate default language at 0.
    const auto iter = base::ranges::find(language_codes, default_language_code);
    if (iter != language_codes.begin() && iter != language_codes.end()) {
      std::iter_swap(language_codes.begin(), iter);
    }
  }

  return language_codes;
}

}  // namespace text_recognition
