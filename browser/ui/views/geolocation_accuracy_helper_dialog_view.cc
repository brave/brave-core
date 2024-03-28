/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/geolocation_accuracy_helper_dialog_view.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/task/thread_pool.h"
#include "brave/browser/ui/color/leo/colors.h"
#include "brave/browser/ui/geolocation/pref_names.h"
#include "brave/browser/ui/views/infobars/custom_styled_label.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view_class_properties.h"

#if BUILDFLAG(IS_WIN)
#include "brave/browser/ui/geolocation/geolocation_accuracy_utils_win.h"
#endif

namespace brave {

void ShowGeolocationAccuracyHelperDialog(content::WebContents* web_contents,
                                         base::OnceClosure closing_callback) {
  constrained_window::ShowWebModalDialogViews(
      new GeolocationAccuracyHelperDialogView(
          user_prefs::UserPrefs::Get(web_contents->GetBrowserContext()),
          std::move(closing_callback)),
      web_contents);
}

}  // namespace brave

namespace {

constexpr char kLearnMoreURL[] =
    "https://support.microsoft.com/en-us/windows/"
    "windows-location-service-and-privacy-3a8eee0a-5b0b-dc07-eede-2a5ca1c49088";

// Subclass for custom font.
class DontShowAgainCheckbox : public views::Checkbox {
 public:
  METADATA_HEADER(DontShowAgainCheckbox);

  using views::Checkbox::Checkbox;
  ~DontShowAgainCheckbox() override = default;
  DontShowAgainCheckbox(const DontShowAgainCheckbox&) = delete;
  DontShowAgainCheckbox& operator=(const DontShowAgainCheckbox&) = delete;

  void SetFontList(const gfx::FontList& font_list) {
    label()->SetFontList(font_list);
  }
};

BEGIN_METADATA(DontShowAgainCheckbox, views::Checkbox)
END_METADATA

}  // namespace

GeolocationAccuracyHelperDialogView::GeolocationAccuracyHelperDialogView(
    PrefService* prefs,
    base::OnceClosure closing_callback)
    : prefs_(*prefs) {
  RegisterWindowClosingCallback(std::move(closing_callback));
  set_should_ignore_snapping(true);
  SetModalType(ui::MODAL_TYPE_CHILD);
  SetShowCloseButton(false);
  SetButtons(ui::DIALOG_BUTTON_OK);

  // Safe to use Unretained() here because this callback is only called before
  // closing this widget.
  SetAcceptCallback(base::BindOnce(
      &GeolocationAccuracyHelperDialogView::OnAccept, base::Unretained(this)));

  constexpr int kPadding = 24;
  SetLayoutManager(std::make_unique<views::BoxLayout>(
                       views::BoxLayout::Orientation::kVertical,
                       gfx::Insets(kPadding), kPadding))
      ->set_cross_axis_alignment(views::BoxLayout::CrossAxisAlignment::kStart);
}

GeolocationAccuracyHelperDialogView::~GeolocationAccuracyHelperDialogView() =
    default;

void GeolocationAccuracyHelperDialogView::AddedToWidget() {
  SetupChildViews();
}

void GeolocationAccuracyHelperDialogView::OnWidgetInitialized() {
  // dialog button should be accessed after widget initialized.
  // See the comment of DialogDelegate::GetOkButton().
  GetOkButton()->SetKind(views::MdTextButton::kPrimary);
  GetOkButton()->SetProminent(false);
}

void GeolocationAccuracyHelperDialogView::SetupChildViews() {
  auto* header_label =
      AddChildView(std::make_unique<views::Label>(l10n_util::GetStringUTF16(
          IDS_GEOLOCATION_ACCURACY_HELPER_DLG_HEADER_LABEL)));
  header_label->SetFontList(gfx::FontList("SF Pro Text, Semi-Bold 16px"));
  header_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  std::vector<size_t> offsets;
  const std::u16string contents_text_part_one = l10n_util::GetStringUTF16(
      IDS_GEOLOCATION_ACCURACY_HELPER_DLG_CONTENTS_PART_ONE_LABEL);
  const std::u16string contents_text_part_two = l10n_util::GetStringUTF16(
      IDS_GEOLOCATION_ACCURACY_HELPER_DLG_CONTENTS_PART_TWO_LABEL);
  const std::u16string contents_text = l10n_util::GetStringFUTF16(
      IDS_GEOLOCATION_ACCURACY_HELPER_DLG_CONTENTS_LABEL,
      contents_text_part_one, contents_text_part_two, &offsets);

  auto* contents_label = AddChildView(std::make_unique<views::StyledLabel>());
  contents_label->SetText(contents_text);
  views::StyledLabel::RangeStyleInfo part_style;
  part_style.custom_font = gfx::FontList("SF Pro Text, Semi-Bold 14px");
  contents_label->AddStyleRange(
      gfx::Range(offsets[0], offsets[0] + contents_text_part_one.length()),
      part_style);
  contents_label->AddStyleRange(
      gfx::Range(offsets[1], offsets[1] + contents_text_part_two.length()),
      part_style);
  contents_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  constexpr int kMaxWidth = 445;
  contents_label->SizeToFit(kMaxWidth);

  views::StyledLabel::RangeStyleInfo default_style;
  default_style.custom_font = gfx::FontList("SF Pro Text, Normal 14px");
  contents_label->AddStyleRange(gfx::Range(0, offsets[0]), default_style);
  contents_label->AddStyleRange(
      gfx::Range(offsets[0] + contents_text_part_one.length(), offsets[1]),
      default_style);
  contents_label->AddStyleRange(
      gfx::Range(offsets[1] + contents_text_part_two.length(),
                 contents_text.length()),
      default_style);

  size_t offset;
  const std::u16string learn_more_part_text = l10n_util::GetStringUTF16(
      IDS_GEOLOCATION_ACCURACY_HELPER_DLG_LEARN_MORE_LABEL_PART);
  const std::u16string contents_second_text = l10n_util::GetStringFUTF16(
      IDS_GEOLOCATION_ACCURACY_HELPER_DLG_CONTENTS_SECOND_LABEL,
      learn_more_part_text, &offset);
  auto* contents_second_label =
      AddChildView(std::make_unique<CustomStyledLabel>());
  contents_second_label->SetText(contents_second_text);
  contents_second_label->AddStyleRange(gfx::Range(0, offset), default_style);

  // Safe to use Unretained() here because this link is owned by this class.
  views::StyledLabel::RangeStyleInfo learn_more_style =
      views::StyledLabel::RangeStyleInfo::CreateForLink(base::BindRepeating(
          &GeolocationAccuracyHelperDialogView::OnLearnMoreClicked,
          base::Unretained(this)));
  learn_more_style.custom_font = gfx::FontList("SF Pro Text, Normal 14px");
  learn_more_style.override_color = leo::GetColor(
      leo::Color::kColorTextInteractive, GetNativeTheme()->ShouldUseDarkColors()
                                             ? leo::Theme::kDark
                                             : leo::Theme::kLight);

  contents_second_label->AddStyleRange(
      gfx::Range(offset, offset + learn_more_part_text.length()),
      learn_more_style);
  contents_second_label->SizeToFit(kMaxWidth);

  // Using Unretained() is safe as |checkbox| is owned by this class.
  auto* checkbox = AddChildView(std::make_unique<DontShowAgainCheckbox>(
      l10n_util::GetStringUTF16(
          IDS_GEOLOCATION_ACCURACY_HELPER_DLG_DONT_SHOW_AGAIN_LABEL),
      base::BindRepeating(
          &GeolocationAccuracyHelperDialogView::OnCheckboxUpdated,
          base::Unretained(this))));
  checkbox->SetFontList(gfx::FontList("SF Pro Text, Normal 14px"));
  dont_show_again_checkbox_ = checkbox;
}

void GeolocationAccuracyHelperDialogView::OnCheckboxUpdated() {
  prefs_->SetBoolean(kShowGeolocationAccuracyHelperDialog,
                     !dont_show_again_checkbox_->GetChecked());
}

void GeolocationAccuracyHelperDialogView::OnAccept() {
#if BUILDFLAG(IS_WIN)
  base::ThreadPool::PostTask(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_BLOCKING,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(&LaunchLocationServiceSettings));
#endif
}

void GeolocationAccuracyHelperDialogView::OnLearnMoreClicked() {
  // Using active window is fine here as this dialog is tied with active tab.
  if (auto* browser = chrome::FindBrowserWithActiveWindow()) {
    chrome::AddSelectedTabWithURL(browser, GURL(kLearnMoreURL),
                                  ui::PAGE_TRANSITION_AUTO_TOPLEVEL);
  }
}

BEGIN_METADATA(GeolocationAccuracyHelperDialogView, views::DialogDelegateView)
END_METADATA
