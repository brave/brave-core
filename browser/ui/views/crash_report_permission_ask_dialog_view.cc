/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/crash_report_permission_ask_dialog_view.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/metrics/metrics_reporting_state.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/session_crashed_bubble.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/common/webui_url_constants.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"

namespace brave {

void ShowCrashReportPermissionAskDialog(Browser* browser) {
  CrashReportPermissionAskDialogView::Show(browser);
}

}  // namespace brave

namespace {

void ScheduleSessionCrashedBubble() {
  // It's ok to use lastly used browser because there will be only one when
  // this launched after un-cleaned exit.
  if (auto* browser = BrowserList::GetInstance()->GetLastActive())
    SessionCrashedBubble::ShowIfNotOffTheRecordProfile(
        browser, /*skip_tab_checking=*/false);
}

gfx::FontList GetFont(int font_size, gfx::Font::Weight weight) {
  gfx::FontList font_list;
  return font_list.DeriveWithSizeDelta(font_size - font_list.GetFontSize())
      .DeriveWithWeight(weight);
}

void OpenSettingPage() {
  if (auto* browser = BrowserList::GetInstance()->GetLastActive())
    chrome::ShowSettingsSubPageForProfile(browser->profile(),
                                          chrome::kPrivacySubPage);
}

}  // namespace

// static
void CrashReportPermissionAskDialogView::Show(Browser* browser) {
  constrained_window::CreateBrowserModalDialogViews(
      new CrashReportPermissionAskDialogView(browser),
      browser->window()->GetNativeWindow())
      ->Show();
}

CrashReportPermissionAskDialogView::CrashReportPermissionAskDialogView(
    Browser* browser) {
  set_should_ignore_snapping(true);

  SetButtonLabel(ui::DIALOG_BUTTON_OK,
                 brave_l10n::GetLocalizedResourceUTF16String(
                     IDS_CRASH_REPORT_PERMISSION_ASK_DIALOG_OK_BUTTON_LABEL));
  SetButtonLabel(
      ui::DIALOG_BUTTON_CANCEL,
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_CRASH_REPORT_PERMISSION_ASK_DIALOG_CANCEL_BUTTON_LABEL));
  SetAcceptCallback(
      base::BindOnce(&CrashReportPermissionAskDialogView::OnAcceptButtonClicked,
                     base::Unretained(this)));
  RegisterWindowClosingCallback(
      base::BindOnce(&CrashReportPermissionAskDialogView::OnWindowClosing,
                     base::Unretained(this)));

  CreateChildViews(BrowserView::GetBrowserViewForBrowser(browser)->GetWidget());
}

CrashReportPermissionAskDialogView::~CrashReportPermissionAskDialogView() =
    default;

void CrashReportPermissionAskDialogView::CreateChildViews(
    views::Widget* parent) {
  constexpr int kPadding = 24;
  constexpr int kChildSpacing = 16;
  constexpr int kIconSize = 24;

  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(kPadding),
      kChildSpacing));

  // Construct header text area
  auto* header = AddChildView(std::make_unique<views::View>());
  header->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
      kChildSpacing));

  constexpr SkColor kDefaultSadImageColor = SkColorSetRGB(0x49, 0x50, 0x57);
  auto* header_image =
      header->AddChildView(std::make_unique<views::ImageView>());
  header_image->SetImageSize(gfx::Size(kIconSize, kIconSize));
  SkColor header_image_color = kDefaultSadImageColor;
  if (parent && parent->GetColorProvider()) {
    header_image_color = parent->GetColorProvider()->GetColor(kColorIconBase);
  }
  header_image->SetImage(ui::ImageModel::FromVectorIcon(
      kBraveSadIcon, header_image_color, kIconSize));

  const std::u16string header_browser_name =
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_CRASH_REPORT_PERMISSION_ASK_DIALOG_HEADER_TEXT_BROWSER_NAME_PART);
  size_t offset;
  const std::u16string header_text = l10n_util::GetStringFUTF16(
      IDS_CRASH_REPORT_PERMISSION_ASK_DIALOG_HEADER_TEXT, header_browser_name,
      &offset);
  auto* header_label =
      header->AddChildView(std::make_unique<views::StyledLabel>());
  header_label->SetText(header_text);

  views::StyledLabel::RangeStyleInfo name_style;
  name_style.custom_font = GetFont(14, gfx::Font::Weight::SEMIBOLD);
  header_label->AddStyleRange(
      gfx::Range(offset, offset + header_browser_name.length()), name_style);

  views::StyledLabel::RangeStyleInfo default_style;
  default_style.custom_font = GetFont(14, gfx::Font::Weight::NORMAL);

  header_label->AddStyleRange(
      gfx::Range(offset + header_browser_name.length(), header_text.length()),
      default_style);
  // If browser name is located in the middle of full text, apply default style
  // all other area.
  if (offset != 0)
    header_label->AddStyleRange(gfx::Range(0, offset), default_style);

  // Construct contents area that includes main text and checkbox.
  auto* contents = AddChildView(std::make_unique<views::View>());
  contents->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::TLBR(0, kPadding + kChildSpacing, 0, 0), 5));
  constexpr int kContentsTextFontSize = 13;
  auto* contents_label = contents->AddChildView(std::make_unique<views::Label>(
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_CRASH_REPORT_PERMISSION_ASK_DIALOG_CONTENT_TEXT),
      views::Label::CustomFont{
          GetFont(kContentsTextFontSize, gfx::Font::Weight::NORMAL)}));
  contents_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  contents_label->SetMultiLine(true);
  constexpr int kContentsLabelMaxWidth = 350;
  contents_label->SetMaximumWidth(kContentsLabelMaxWidth);
  dont_ask_again_checkbox_ =
      contents->AddChildView(std::make_unique<views::Checkbox>(
          brave_l10n::GetLocalizedResourceUTF16String(
              IDS_CRASH_REPORT_PERMISSION_ASK_DIALOG_DONT_ASK_TEXT)));

  // Construct footnote text area
  constexpr int kFootnoteVerticalPadding = 16;
  auto* footnote = SetFootnoteView(std::make_unique<views::View>());
  auto* footnote_layout =
      footnote->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(kFootnoteVerticalPadding, 0)));
  footnote_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kCenter);
  footnote->SetBackground(
      views::CreateThemedSolidBackground(ui::kColorDialogBackground));

  const std::u16string setting_text =
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_CRASH_REPORT_PERMISSION_ASK_DIALOG_FOOTNOTE_TEXT_SETTING_PART);
  const std::u16string footnote_text = l10n_util::GetStringFUTF16(
      IDS_CRASH_REPORT_PERMISSION_ASK_DIALOG_FOOTNOTE_TEXT, setting_text,
      &offset);
  auto* footnote_label =
      footnote->AddChildView(std::make_unique<views::StyledLabel>());
  footnote_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  footnote_label->SetText(footnote_text);

  views::StyledLabel::RangeStyleInfo setting_style =
      views::StyledLabel::RangeStyleInfo::CreateForLink(
          base::BindRepeating(OpenSettingPage));
  footnote_label->AddStyleRange(
      gfx::Range(offset, offset + setting_text.length()), setting_style);

  views::StyledLabel::RangeStyleInfo footnote_default_style;
  constexpr int kFootnoteFontSize = 12;
  footnote_default_style.custom_font =
      GetFont(kFootnoteFontSize, gfx::Font::Weight::NORMAL);

  if (offset != 0)
    footnote_label->AddStyleRange(gfx::Range(0, offset),
                                  footnote_default_style);

  if (offset + setting_text.length() != footnote_text.length())
    footnote_label->AddStyleRange(
        gfx::Range(offset + setting_text.length(), footnote_text.length()),
        footnote_default_style);
}

ui::mojom::ModalType CrashReportPermissionAskDialogView::GetModalType() const {
  return ui::mojom::ModalType::kWindow;
}

bool CrashReportPermissionAskDialogView::ShouldShowCloseButton() const {
  return false;
}

bool CrashReportPermissionAskDialogView::ShouldShowWindowTitle() const {
  return false;
}

void CrashReportPermissionAskDialogView::OnWidgetInitialized() {
  SetButtonRowInsets(gfx::Insets::TLBR(0, 0, 18, 24));
}

void CrashReportPermissionAskDialogView::OnAcceptButtonClicked() {
  // Enable crash reporting.
  ChangeMetricsReportingState(true);
}

void CrashReportPermissionAskDialogView::OnWindowClosing() {
  g_browser_process->local_state()->SetBoolean(
      kDontAskForCrashReporting, dont_ask_again_checkbox_->GetChecked());

  // On macOS, this dialog is not destroyed properly when session crashed bubble
  // is launched directly.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ScheduleSessionCrashedBubble));
}
