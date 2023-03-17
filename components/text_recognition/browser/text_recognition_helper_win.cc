/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/text_recognition/browser/text_recognition_helper_win.h"

#include <utility>

#include "base/logging.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/text_recognition/browser/text_recognition.h"
#include "content/public/browser/browser_thread.h"

namespace text_recognition {

TextRecognitionHelperWin::TextRecognitionHelperWin() = default;
TextRecognitionHelperWin::~TextRecognitionHelperWin() = default;

void TextRecognitionHelperWin::GetTextFromImage(
    const SkBitmap& image,
    base::OnceCallback<void(const std::vector<std::string>&)> callback) {
  callback_ = std::move(callback);
  image_ = image;

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&GetAvailableRecognizerLanguages),
      base::BindOnce(&TextRecognitionHelperWin::OnGetAvailableLanguages,
                     weak_factory_.GetWeakPtr()));
}

void TextRecognitionHelperWin::SetResultText(
    const std::vector<std::string>& text) {
  if (text.empty()) {
    return;
  }

  if (best_result_.empty()) {
    best_result_ = text;
  }

  // Determine which one is more better result between |text| and
  // |best_result_|. We choose one that has more longer detected string.
  int text_string_size = 0;
  for (const auto& s : text) {
    text_string_size += s.size();
  }

  int best_result_string_size = 0;
  for (const auto& s : best_result_) {
    best_result_string_size += s.size();
  }

  if (text_string_size > best_result_string_size) {
    best_result_ = text;
  }
}

void TextRecognitionHelperWin::OnGetTextFromImage(
    const std::vector<std::string>& text) {
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  remained_language_detection_request_count_--;

  SetResultText(text);

  if (remained_language_detection_request_count_ == 0) {
    std::move(callback_).Run(best_result_);
  }
}

void TextRecognitionHelperWin::OnGetAvailableLanguages(
    const base::flat_set<std::string>& supported_languages) {
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  if (supported_languages.empty()) {
    std::move(callback_).Run({});
    return;
  }

  remained_language_detection_request_count_ = supported_languages.size();

  scoped_refptr<base::SequencedTaskRunner> task_runner =
      base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});

  for (const auto& lang : supported_languages) {
    task_runner->PostTask(
        FROM_HERE,
        base::BindOnce(
            &::text_recognition::GetTextFromImage, lang, image_,
            base::BindOnce(&TextRecognitionHelperWin::OnGetTextFromImage,
                           weak_factory_.GetWeakPtr())));
  }
}

}  // namespace text_recognition
