/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TEXT_RECOGNITION_BROWSER_TEXT_RECOGNITION_H_
#define BRAVE_COMPONENTS_TEXT_RECOGNITION_BROWSER_TEXT_RECOGNITION_H_

#include <string>
#include <utility>
#include <vector>

#include "base/component_export.h"
#include "base/functional/callback_forward.h"
#include "build/build_config.h"

class SkBitmap;

namespace text_recognition {

#if BUILDFLAG(IS_MAC)
// Returns a pair with a boolean value for supported if the extraction is
// supported and a vector ofrecognized texts from |image| synchronously.
COMPONENT_EXPORT(TEXT_RECOGNITION_BROWSER)
std::pair<bool, std::vector<std::string>> GetTextFromImage(
    const SkBitmap& image);
#endif

#if BUILDFLAG(IS_WIN)
COMPONENT_EXPORT(TEXT_RECOGNITION_BROWSER)
std::vector<std::string> GetAvailableRecognizerLanguages();

// Recognized text is delivered by running |callback_run_on_ui_thread| on UI
// thread. On failure, |callback| runs with false and an empty set.
COMPONENT_EXPORT(TEXT_RECOGNITION_BROWSER)
void GetTextFromImage(
    const std::string& lang_code,
    const SkBitmap& image,
    base::OnceCallback<void(const std::pair<bool, std::vector<std::string>>&)>
        callback_run_on_ui_thread);
#endif

}  // namespace text_recognition

#endif  // BRAVE_COMPONENTS_TEXT_RECOGNITION_BROWSER_TEXT_RECOGNITION_H_
