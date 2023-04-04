/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TEXT_RECOGNITION_BROWSER_TEXT_RECOGNITION_H_
#define BRAVE_COMPONENTS_TEXT_RECOGNITION_BROWSER_TEXT_RECOGNITION_H_

#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/functional/callback_forward.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/single_thread_task_runner.h"
#include "build/build_config.h"

class SkBitmap;

namespace text_recognition {

#if BUILDFLAG(IS_MAC)
// Returns recognized texts from |image| synchronously.
COMPONENT_EXPORT(TEXT_RECOGNITION_BROWSER)
std::vector<std::string> GetTextFromImage(const SkBitmap& image);
#endif

#if BUILDFLAG(IS_WIN)
COMPONENT_EXPORT(TEXT_RECOGNITION_BROWSER)
std::vector<std::string> GetAvailableRecognizerLanguages();

// Recognized text is delivered by running |callback_run_on_ui_thread| on UI
// thread. On failure, |callback| runs with empty set.
// Return false when text recognization is not supported.
COMPONENT_EXPORT(TEXT_RECOGNITION_BROWSER)
bool GetTextFromImage(const std::string& lang_code,
                      const SkBitmap& image,
                      scoped_refptr<base::SingleThreadTaskRunner> reply_runner,
                      base::OnceCallback<void(const std::vector<std::string>&)>
                          callback_run_on_ui_thread);
#endif

}  // namespace text_recognition

#endif  // BRAVE_COMPONENTS_TEXT_RECOGNITION_BROWSER_TEXT_RECOGNITION_H_
