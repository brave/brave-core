/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/text_recognition_dialog_view.h"

#include <algorithm>
#include <memory>

#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/text_recognition/browser/text_recognition.h"
#include "brave/grit/brave_generated_resources.h"
#include "components/constrained_window/constrained_window_views.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_client_view.h"

namespace brave {

void ShowTextRecognitionDialog(content::WebContents* web_contents,
                               const SkBitmap& image) {
  constrained_window::ShowWebModalDialogViews(
      new TextRecognitionDialogView(image), web_contents)
      ->Show();
}

}  // namespace brave

TextRecognitionDialogView::TextRecognitionDialogView(const SkBitmap& image) {
  SetModalType(ui::MODAL_TYPE_CHILD);
  SetButtons(ui::DIALOG_BUTTON_OK);
  SetButtonLabel(ui::DIALOG_BUTTON_OK,
                 brave_l10n::GetLocalizedResourceUTF16String(
                     IDS_TEXT_RECOG_DIALOG_CLOSE_BUTTON));
  SetShowCloseButton(false);

  SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kVertical)
      .SetMainAxisAlignment(views::LayoutAlignment::kStart)
      .SetInteriorMargin(gfx::Insets::TLBR(24, 26, 0, 26));

  header_label_ = AddChildView(std::make_unique<views::Label>());
  const int size_diff = 14 - views::Label::GetDefaultFontList().GetFontSize();
  header_label_->SetFontList(
      views::Label::GetDefaultFontList()
          .DeriveWithSizeDelta(size_diff)
          .DeriveWithWeight(gfx::Font::Weight::SEMIBOLD));
  header_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header_label_->SetProperty(views::kMarginsKey,
                             gfx::Insets::TLBR(0, 0, 10, 0));

  if (image.empty()) {
    UpdateContents({});
    return;
  }

  StartExtractingText(image);
}

TextRecognitionDialogView::~TextRecognitionDialogView() = default;

void TextRecognitionDialogView::StartExtractingText(const SkBitmap& image) {
  header_label_->SetText(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_TEXT_RECOG_DIALOG_HEADER_IN_PROGRESS));

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&GetTextFromImage, image),
      base::BindOnce(&TextRecognitionDialogView::OnGetTextFromImage,
                     weak_factory_.GetWeakPtr()));
}

void TextRecognitionDialogView::OnGetTextFromImage(
    const std::vector<std::string>& text) {
  UpdateContents(text);
  AdjustWidgetSize();
}

void TextRecognitionDialogView::UpdateContents(
    const std::vector<std::string>& text) {
  if (text.empty()) {
    header_label_->SetText(brave_l10n::GetLocalizedResourceUTF16String(
        IDS_TEXT_RECOG_DIALOG_HEADER_FAILED));
    return;
  }

  header_label_->SetText(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_TEXT_RECOG_DIALOG_HEADER_COMPLETE));

  // Treat each string in |text| as a separated line string.
  const char* const delimiter = "\n";
  std::ostringstream unified;
  std::copy(text.begin(), text.end(),
            std::ostream_iterator<std::string>(unified, delimiter));
  std::string unified_string = unified.str();

  ui::ScopedClipboardWriter(ui::ClipboardBuffer::kCopyPaste)
      .WriteText(base::UTF8ToUTF16(unified_string));

  auto* scroll_view = AddChildView(std::make_unique<views::ScrollView>());
  scroll_view->SetProperty(views::kMarginsKey, gfx::Insets::VH(0, 10));
  scroll_view->ClipHeightTo(0, 350);
  auto* label = scroll_view->SetContents(
      std::make_unique<views::Label>(base::UTF8ToUTF16(unified_string)));

  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetSelectable(true);
  label->SetMultiLine(true);
}

void TextRecognitionDialogView::AdjustWidgetSize() {
  GetWidget()->SetSize(GetDialogClientView()->GetPreferredSize());
}

BEGIN_METADATA(TextRecognitionDialogView, views::DialogDelegateView)
END_METADATA
