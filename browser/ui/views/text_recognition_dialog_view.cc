/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/text_recognition_dialog_view.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/bind_post_task.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "brave/browser/ui/views/text_recognition_dialog_tracker.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/text_recognition/browser/text_recognition.h"
#include "brave/grit/brave_generated_resources.h"
#include "build/build_config.h"
#include "components/constrained_window/constrained_window_views.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/combobox_model.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_client_view.h"

namespace {

constexpr base::TimeDelta kShowResultDelay = base::Milliseconds(400);

#if BUILDFLAG(IS_WIN)
class TargetLanguageComboboxModel : public ui::ComboboxModel {
 public:
  explicit TargetLanguageComboboxModel(
      const std::vector<std::string>& languages)
      : languages_(languages) {}

  TargetLanguageComboboxModel(const TargetLanguageComboboxModel&) = delete;
  TargetLanguageComboboxModel& operator=(const TargetLanguageComboboxModel&) =
      delete;

  ~TargetLanguageComboboxModel() override = default;

  // Overridden from ui::ComboboxModel:
  size_t GetItemCount() const override { return languages_.size(); }

  std::u16string GetItemAt(size_t index) const override {
    return base::UTF8ToUTF16(languages_[index]);
  }

  std::optional<size_t> GetDefaultIndex() const override { return 0; }

 private:
  const std::vector<std::string> languages_;
};
#endif

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
    text_recognition_dialog->set_image(image);
    text_recognition_dialog->StartExtractingText();
    return;
  }

  auto* delegate = new TextRecognitionDialogView(image);
  auto* new_dialog =
      constrained_window::ShowWebModalDialogViews(delegate, web_contents);
  dialog_tracker->SetActiveDialog(new_dialog);
  new_dialog->Show();
}

}  // namespace brave

TextRecognitionDialogView::TextRecognitionDialogView(const SkBitmap& image)
    : image_(image),
      show_result_timer_(FROM_HERE,
                         kShowResultDelay,
                         base::BindRepeating(
                             &TextRecognitionDialogView::OnShowResultTimerFired,
                             base::Unretained(this))) {
  SetModalType(ui::mojom::ModalType::kChild);
  SetButtons(static_cast<int>(ui::mojom::DialogButton::kOk));
  SetButtonLabel(ui::mojom::DialogButton::kOk,
                 brave_l10n::GetLocalizedResourceUTF16String(
                     IDS_TEXT_RECOGNITION_DIALOG_CLOSE_BUTTON));
  SetShowCloseButton(false);

  SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kVertical)
      .SetMainAxisAlignment(views::LayoutAlignment::kStart)
      .SetInteriorMargin(gfx::Insets::TLBR(24, 26, 0, 26));

  header_container_ = AddChildView(std::make_unique<views::View>());
  header_container_->SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetMainAxisAlignment(views::LayoutAlignment::kStart);

  header_label_ =
      header_container_->AddChildView(std::make_unique<views::Label>());
  const int size_diff = 14 - views::Label::GetDefaultFontList().GetFontSize();
  header_label_->SetFontList(
      views::Label::GetDefaultFontList()
          .DeriveWithSizeDelta(size_diff)
          .DeriveWithWeight(gfx::Font::Weight::SEMIBOLD));
  header_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header_label_->SetProperty(views::kMarginsKey,
                             gfx::Insets::TLBR(0, 0, 10, 0));
}

TextRecognitionDialogView::~TextRecognitionDialogView() = default;

void TextRecognitionDialogView::AddedToWidget() {
#if BUILDFLAG(IS_WIN)
  // Start text extracting after getting language list to make code simpler.
  com_task_runner_ =
      base::ThreadPool::CreateCOMSTATaskRunner({base::MayBlock()});
  com_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&text_recognition::GetAvailableRecognizerLanguages),
      base::BindOnce(
          &TextRecognitionDialogView::OnGetAvailableRecognizerLanguages,
          weak_factory_.GetWeakPtr()));
#else
  StartExtractingText();
#endif
}

void TextRecognitionDialogView::StartExtractingText(
    const std::string& language_code) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  result_ = std::nullopt;
  show_result_timer_.Reset();

  if (image_.empty()) {
    show_result_timer_.Stop();
    OnGetTextFromImage({false, {}});
    return;
  }

  // Clear previous text.
  if (scroll_view_) {
    RemoveChildViewT(scroll_view_);
    scroll_view_ = nullptr;
  }

  header_label_->SetText(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_TEXT_RECOGNITION_DIALOG_HEADER_IN_PROGRESS));
  AdjustWidgetSize();

#if BUILDFLAG(IS_MAC)
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&text_recognition::GetTextFromImage, image_),
      base::BindOnce(&TextRecognitionDialogView::OnGetTextFromImage,
                     weak_factory_.GetWeakPtr()));
#endif

#if BUILDFLAG(IS_WIN)
  // Disable till extracting finished.
  if (combobox_) {
    combobox_->SetEnabled(false);
  }

  com_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&text_recognition::GetTextFromImage, language_code, image_,
                     base::BindPostTaskToCurrentDefault(base::BindOnce(
                         &TextRecognitionDialogView::OnGetTextFromImage,
                         weak_factory_.GetWeakPtr()))));
#endif
}

void TextRecognitionDialogView::OnGetTextFromImage(
    const std::pair<bool, std::vector<std::string>>& supported_text) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto& text = supported_text.second;
  if (show_result_timer_.IsRunning()) {
    result_ = text;
    return;
  }

#if BUILDFLAG(IS_WIN)
  // Can choose another language when previous detect is finished.
  if (combobox_) {
    combobox_->SetEnabled(true);
  }
#endif

  UpdateContents(text);
  AdjustWidgetSize();

  if (on_get_text_callback_for_test_) {
    std::move(on_get_text_callback_for_test_).Run(supported_text);
  }
}

void TextRecognitionDialogView::UpdateContents(
    const std::vector<std::string>& text) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  CHECK(!show_result_timer_.IsRunning())
      << "Update when timer is fired or stopped.";

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

  CHECK(!scroll_view_);
  scroll_view_ = AddChildView(std::make_unique<views::ScrollView>());
  scroll_view_->SetProperty(views::kMarginsKey, gfx::Insets::VH(0, 10));
  scroll_view_->ClipHeightTo(0, 350);

  auto* label =
      scroll_view_->SetContents(std::make_unique<views::Label>(unified_string));
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetSelectable(true);
  label->SetMultiLine(true);
}

void TextRecognitionDialogView::AdjustWidgetSize() {
  DCHECK(GetWidget());
  GetWidget()->SetSize(GetDialogClientView()->GetPreferredSize());
}

void TextRecognitionDialogView::OnShowResultTimerFired() {
  // Fired before getting text from image.
  // Will be updated when text is fetched.
  if (!result_) {
    return;
  }

  // Fired after getting text from image.
  // Show the result now.
  OnGetTextFromImage({true, *result_});
}

#if BUILDFLAG(IS_WIN)
void TextRecognitionDialogView::OnGetAvailableRecognizerLanguages(
    const std::vector<std::string>& languages) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Add combobox for selecting languages with fetched available languages.
  auto* spacer =
      header_container_->AddChildView(std::make_unique<views::View>());
  spacer->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kUnbounded));
  combobox_ = header_container_->AddChildView(std::make_unique<views::Combobox>(
      std::make_unique<TargetLanguageComboboxModel>(languages)));
  combobox_->SetMenuSelectionAtCallback(
      base::BindRepeating(&TextRecognitionDialogView::OnLanguageOptionchanged,
                          base::Unretained(this)));
  combobox_->SetProperty(views::kMarginsKey, gfx::Insets::TLBR(0, 0, 10, 0));

  // Enabled state is updated during the extracting.
  // While extracting, it's disabled and then enabled when extracting is done.
  combobox_->SetEnabled(false);

  StartExtractingText();
}

bool TextRecognitionDialogView::OnLanguageOptionchanged(size_t index) {
  CHECK(combobox_);
  StartExtractingText(
      base::UTF16ToUTF8(combobox_->GetModel()->GetItemAt(index)));
  return false;
}

#endif

BEGIN_METADATA(TextRecognitionDialogView)
END_METADATA
