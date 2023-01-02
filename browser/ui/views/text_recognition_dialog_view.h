/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TEXT_RECOGNITION_DIALOG_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_TEXT_RECOGNITION_DIALOG_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/window/dialog_delegate.h"

class SkBitmap;

namespace views {
class Label;
}  // namespace views

class TextRecognitionDialogView : public views::DialogDelegateView {
 public:
  METADATA_HEADER(TextRecognitionDialogView);

  explicit TextRecognitionDialogView(const SkBitmap& image);
  TextRecognitionDialogView(const TextRecognitionDialogView&) = delete;
  TextRecognitionDialogView& operator=(const TextRecognitionDialogView&) =
      delete;
  ~TextRecognitionDialogView() override;

 private:
  void StartExtractingText(const SkBitmap& image);
  void OnGetTextFromImage(const std::vector<std::string>& text);

  // Show |text| in this dialog and copy it to clipboard.
  void UpdateContents(const std::vector<std::string>& text);
  void AdjustWidgetSize();

  raw_ptr<views::Label> header_label_ = nullptr;
  base::WeakPtrFactory<TextRecognitionDialogView> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TEXT_RECOGNITION_DIALOG_VIEW_H_
