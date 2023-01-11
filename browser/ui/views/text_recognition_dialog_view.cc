/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/text_recognition_dialog_view.h"

#include <memory>

#include "base/bind.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/text_recognition/browser/text_recognition.h"
#include "brave/grit/brave_generated_resources.h"
#include "components/constrained_window/constrained_window_views.h"
#include "content/public/browser/web_contents_user_data.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"
#include "ui/views/window/dialog_client_view.h"

namespace {

// Tracks whether text recognition dialog is active or not for WebContents.
class TextRecognitionDialogTracker
    : public content::WebContentsUserData<TextRecognitionDialogTracker>,
      public views::WidgetObserver {
 public:
  TextRecognitionDialogTracker(const TextRecognitionDialogTracker&) = delete;
  TextRecognitionDialogTracker& operator=(const TextRecognitionDialogTracker&) =
      delete;
  ~TextRecognitionDialogTracker() override = default;

  void SetActiveDialog(views::Widget* widget) {
    DCHECK(!active_dialog_);
    active_dialog_ = widget;
    active_dialog_->AddObserver(this);
  }

  views::Widget* active_dialog() { return active_dialog_; }

 private:
  friend class content::WebContentsUserData<TextRecognitionDialogTracker>;
  explicit TextRecognitionDialogTracker(content::WebContents* web_contents)
      : content::WebContentsUserData<TextRecognitionDialogTracker>(
            *web_contents) {}

  // views::WidgetObserver overrides
  void OnWidgetClosing(views::Widget* widget) override {
    DCHECK_EQ(active_dialog_, widget);
    DCHECK(active_dialog_->HasObserver(this));
    active_dialog_->RemoveObserver(this);
    active_dialog_ = nullptr;
  }

  raw_ptr<views::Widget> active_dialog_ = nullptr;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

WEB_CONTENTS_USER_DATA_KEY_IMPL(TextRecognitionDialogTracker);

}  // namespace

namespace brave {

void ShowTextRecognitionDialog(content::WebContents* web_contents,
                               const SkBitmap& image) {
  // Use existing dialog instead of creating multiple dialog.
  // Dialog will have lastly recognizied text from image from same tab.
  TextRecognitionDialogTracker::CreateForWebContents(web_contents);
  auto* dialog_tracker =
      TextRecognitionDialogTracker::FromWebContents(web_contents);

  if (auto* active_dialog = dialog_tracker->active_dialog()) {
    TextRecognitionDialogView* text_recognition_dialog =
        static_cast<TextRecognitionDialogView*>(
            active_dialog->widget_delegate());
    text_recognition_dialog->StartExtractingText(image);
    return;
  }

  auto* new_dialog = constrained_window::ShowWebModalDialogViews(
      new TextRecognitionDialogView(image), web_contents);
  dialog_tracker->SetActiveDialog(new_dialog);
  new_dialog->Show();
}

}  // namespace brave

TextRecognitionDialogView::TextRecognitionDialogView(const SkBitmap& image) {
  SetModalType(ui::MODAL_TYPE_CHILD);
  SetButtons(ui::DIALOG_BUTTON_OK);
  SetButtonLabel(ui::DIALOG_BUTTON_OK,
                 brave_l10n::GetLocalizedResourceUTF16String(
                     IDS_TEXT_RECOGNITION_DIALOG_CLOSE_BUTTON));
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

  StartExtractingText(image);
}

TextRecognitionDialogView::~TextRecognitionDialogView() = default;

void TextRecognitionDialogView::StartExtractingText(const SkBitmap& image) {
  if (image.empty()) {
    UpdateContents({});
    return;
  }

  header_label_->SetText(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_TEXT_RECOGNITION_DIALOG_HEADER_IN_PROGRESS));

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&text_recognition::GetTextFromImage, image),
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
        IDS_TEXT_RECOGNITION_DIALOG_HEADER_FAILED));
    return;
  }

  header_label_->SetText(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_TEXT_RECOGNITION_DIALOG_HEADER_COMPLETE));

  // Treat each string in |text| as a separated line string.
  const auto unified_string = base::UTF8ToUTF16(base::JoinString(text, "\n"));
  ui::ScopedClipboardWriter(ui::ClipboardBuffer::kCopyPaste)
      .WriteText(unified_string);

  if (!scroll_view_) {
    scroll_view_ = AddChildView(std::make_unique<views::ScrollView>());
    scroll_view_->SetProperty(views::kMarginsKey, gfx::Insets::VH(0, 10));
    scroll_view_->ClipHeightTo(0, 350);
  }
  auto* label =
      scroll_view_->SetContents(std::make_unique<views::Label>(unified_string));
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetSelectable(true);
  label->SetMultiLine(true);
}

void TextRecognitionDialogView::AdjustWidgetSize() {
  GetWidget()->SetSize(GetDialogClientView()->GetPreferredSize());
}

BEGIN_METADATA(TextRecognitionDialogView, views::DialogDelegateView)
END_METADATA
