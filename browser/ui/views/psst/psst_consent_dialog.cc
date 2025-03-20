/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/psst/psst_consent_dialog.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/grit/brave_theme_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/progress_bar.h"
#include "ui/views/layout/box_layout_view.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/window/dialog_client_view.h"

namespace {
constexpr int kHeaderFontSize = 18;
constexpr int kListTitleFontSize = 15;
constexpr std::u16string kDoneMessage = u"Done";

void OnOkCallBackCompleteDlgWithClose(base::WeakPtr<PsstConsentDialog> dialog) {
  if (dialog) {
    dialog->GetWidget()->CloseWithReason(
        views::Widget::ClosedReason::kCancelButtonClicked);
  }
}

void CallBackWithClose(base::WeakPtr<PsstConsentDialog> dialog,
                       base::OnceClosure callback) {
  std::move(callback).Run();
  OnOkCallBackCompleteDlgWithClose(dialog);
}

void SetFont(views::Label* label, const int size) {
  const auto& header_font_list = label->font_list();
  label->SetFontList(
      header_font_list
          .DeriveWithSizeDelta(size - header_font_list.GetFontSize())
          .DeriveWithWeight(gfx::Font::Weight::SEMIBOLD));
}
}  // namespace

PsstConsentDialog::PsstConsentDialog(bool prompt_for_new_version,
                                     base::Value::List requests,
                                     ConsentDialogCallback consent_callback,
                                     base::OnceClosure cancel_callback)
    : consent_callback_(std::move(consent_callback)) {
  set_margins(gfx::Insets(20));
  SetModalType(ui::mojom::ModalType::kChild);
  SetShowCloseButton(false);
  SetButtons(static_cast<int>(ui::mojom::DialogButton::kNone));

  std::u16string body_text = base::StrCat(
      {l10n_util::GetStringUTF16(IDS_PSST_CONSENT_DIALOG_BODY), u"\n\n",
       l10n_util::GetStringUTF16(
           IDS_PSST_CONSENT_DIALOG_BODY_LIST_OF_CHANGES)});

  SetLayoutManager(std::make_unique<views::FillLayout>());

  views::Label* header = nullptr;
  views::Label* body = nullptr;
  views::Label* new_version_label = nullptr;

  auto box =
      views::Builder<views::BoxLayoutView>()
          .SetOrientation(views::BoxLayout::Orientation::kVertical)
          .SetCrossAxisAlignment(views::BoxLayout::CrossAxisAlignment::kStretch)
          .CopyAddressTo(&box_status_view_)
          .AddChild(
              views::Builder<views::Label>()
                  .CopyAddressTo(&header)
                  .SetText(
                      l10n_util::GetStringUTF16(IDS_PSST_CONSENT_DIALOG_HEADER))
                  .SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT))
          .AddChild(views::Builder<views::Label>()
                        .CopyAddressTo(&body)
                        .SetText(body_text)
                        .SetMultiLine(true)
                        .SetHorizontalAlignment(
                            gfx::HorizontalAlignment::ALIGN_LEFT));

  for (const auto& request : requests) {
    const auto* request_item_dict = request.GetIfDict();
    if (!request_item_dict) {
      continue;
    }

    const auto* description = request_item_dict->FindString("description");
    if (!description) {
      continue;
    }

    const auto* url = request_item_dict->FindString("url");
    if (!url) {
      continue;
    }

    auto current_status_line = std::make_unique<StatusCheckedLine>();
    auto change_item_box =
        views::Builder<views::BoxLayoutView>()
            .SetOrientation(views::BoxLayout::Orientation::kHorizontal)
            .SetCrossAxisAlignment(
                views::BoxLayout::CrossAxisAlignment::kStretch);

    change_item_box.AddChild(
        views::Builder<views::Checkbox>()
            .SetText(base::ASCIIToUTF16(*description))
            .SetChecked(true)
            .CopyAddressTo(&current_status_line->check_box));
    change_item_box.AddChild(
        views::Builder<views::Label>()
            .CopyAddressTo(&current_status_line->status_label)
            .SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT)
            //.SetEnabledColor(SK_ColorRED)
            .SetProperty(views::kMarginsKey, gfx::Insets().set_left(16)));

    box.AddChild(std::move(change_item_box));

    SetFont(current_status_line->status_label, kListTitleFontSize);

    task_checked_list_[*url] = std::move(current_status_line);
  }

  box.AddChild(views::Builder<views::ProgressBar>()
                   .SetPreferredSize(gfx::Size(50, 10))
                   .CopyAddressTo(&progress_bar_)
                   .SetValue(0)
                   .SetProperty(views::kMarginsKey,
                                gfx::Insets().set_bottom(16).set_top(24)));

  if (prompt_for_new_version) {
    box.AddChild(
        views::Builder<views::Label>()
            .SetText(l10n_util::GetStringUTF16(
                IDS_PSST_CONSENT_DIALOG_BODY_NEW_VERSION))
            .CopyAddressTo(&new_version_label)
            .SetMultiLine(true)
            .SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT));
  }

  auto button_box =
      views::Builder<views::BoxLayoutView>()
          .SetOrientation(views::BoxLayout::Orientation::kHorizontal)
          .SetMainAxisAlignment(views::LayoutAlignment::kEnd)
          .SetCrossAxisAlignment(views::BoxLayout::CrossAxisAlignment::kEnd)
          .SetProperty(views::kMarginsKey,
                       gfx::Insets().set_bottom(16).set_top(16))
          .AddChild(views::Builder<views::MdTextButton>()
                        .SetText(l10n_util::GetStringUTF16(
                            IDS_PSST_CONSENT_DIALOG_CANCEL))
                        .SetStyle(ui::ButtonStyle::kText)
                        .SetCallback(base::BindOnce(&CallBackWithClose,
                                                    weak_factory_.GetWeakPtr(),
                                                    std::move(cancel_callback)))
                        .CopyAddressTo(&no_button_)
                        .SetHorizontalAlignment(
                            gfx::HorizontalAlignment::ALIGN_CENTER))
          .AddChild(views::Builder<views::MdTextButton>()
                        .SetText(l10n_util::GetStringUTF16(
                            IDS_PSST_CONSENT_DIALOG_OK))
                        .SetStyle(ui::ButtonStyle::kDefault)
                        .SetCallback(base::BindRepeating(
                            &PsstConsentDialog::OnConsentClicked,
                            weak_factory_.GetWeakPtr()))
                        .CopyAddressTo(&ok_button_)
                        .SetHorizontalAlignment(
                            gfx::HorizontalAlignment::ALIGN_CENTER));
  box.AddChild(std::move(button_box));
  AddChildView(std::move(box).Build());

  CHECK(header);
  CHECK(body);

  SetFont(header, kHeaderFontSize);
  header->SetProperty(views::kMarginsKey, gfx::Insets().set_bottom(16));
  SetFont(body, kListTitleFontSize);

  const auto& body_font_list = body->font_list();
  body->SetFontList(
      body_font_list.DeriveWithSizeDelta(14 - body_font_list.GetFontSize()));

  if (prompt_for_new_version) {
    CHECK(new_version_label);
    new_version_label->SetFontList(
        body_font_list.DeriveWithSizeDelta(14 - body_font_list.GetFontSize()));
  }

  // Preparing Complete view
  views::Label* complete_view_header = nullptr;
  auto complete_view_box =
      views::Builder<views::BoxLayoutView>()
          .SetOrientation(views::BoxLayout::Orientation::kVertical)
          .SetMainAxisAlignment(views::LayoutAlignment::kStretch)
          .SetCrossAxisAlignment(views::BoxLayout::CrossAxisAlignment::kStretch)
          .SetVisible(false)
          .CopyAddressTo(&box_complete_view_);

  complete_view_box.AddChild(
      views::Builder<views::Label>()
          .CopyAddressTo(&complete_view_header)
          .SetText(l10n_util::GetStringUTF16(
              IDS_PSST_CONSENT_COMPLETE_DIALOG_HEADER))
          .SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT));

  complete_view_box.AddChild(
      views::Builder<views::BoxLayoutView>()
          .SetOrientation(views::BoxLayout::Orientation::kHorizontal)
          .SetMainAxisAlignment(views::LayoutAlignment::kStretch)
          .SetCrossAxisAlignment(views::BoxLayout::CrossAxisAlignment::kStretch)
          .SetBetweenChildSpacing(50)
          .SetProperty(views::kMarginsKey, gfx::Insets().set_top(16))
          .AddChild(
              views::Builder<views::BoxLayoutView>()
                  .SetOrientation(views::BoxLayout::Orientation::kVertical)
                  .SetMainAxisAlignment(views::LayoutAlignment::kStart)
                  .SetCrossAxisAlignment(
                      views::BoxLayout::CrossAxisAlignment::kStart)
                  .AddChild(
                      views::Builder<views::Label>()
                          .CopyAddressTo(&complete_view_body_applied_title_)
                          .SetMultiLine(true)
                          .SetHorizontalAlignment(
                              gfx::HorizontalAlignment::ALIGN_LEFT))
                  .AddChild(views::Builder<views::Label>()
                                .CopyAddressTo(&complete_view_body_applied_)
                                .SetMultiLine(true)
                                .SetHorizontalAlignment(
                                    gfx::HorizontalAlignment::ALIGN_LEFT)))
          .AddChild(
              views::Builder<views::BoxLayoutView>()
                  .SetOrientation(views::BoxLayout::Orientation::kVertical)
                  .SetMainAxisAlignment(views::LayoutAlignment::kStart)
                  .SetCrossAxisAlignment(
                      views::BoxLayout::CrossAxisAlignment::kStart)
                  .AddChild(
                      views::Builder<views::Label>()
                          .CopyAddressTo(&complete_view_body_failed_title_)
                          .SetMultiLine(true)
                          .SetHorizontalAlignment(
                              gfx::HorizontalAlignment::ALIGN_LEFT))
                  .AddChild(views::Builder<views::Label>()
                                .CopyAddressTo(&complete_view_body_failed_)
                                .SetMultiLine(true)
                                .SetHorizontalAlignment(
                                    gfx::HorizontalAlignment::ALIGN_LEFT))));

  complete_view_box.AddChild(
      views::Builder<views::BoxLayoutView>()
          .SetOrientation(views::BoxLayout::Orientation::kHorizontal)
          .SetMainAxisAlignment(views::LayoutAlignment::kEnd)
          .SetProperty(views::kMarginsKey, gfx::Insets().set_top(16))
          .CopyAddressTo(&box_complete_buttons_view_)
          .AddChild(
              views::Builder<views::MdTextButton>()
                  .SetText(l10n_util::GetStringUTF16(
                      IDS_PSST_COMPLETE_CONSENT_DIALOG_REPORT))
                  .SetStyle(ui::ButtonStyle::kText)
                  // .SetCallback(base::BindRepeating(&OnOkCallBackCompleteDlgWithClose,
                  //                             weak_factory_.GetWeakPtr()))
                  .SetProperty(views::kMarginsKey, gfx::Insets().set_left(16))
                  .SetHorizontalAlignment(
                      gfx::HorizontalAlignment::ALIGN_CENTER))
          .AddChild(
              views::Builder<views::MdTextButton>()
                  .SetText(l10n_util::GetStringUTF16(
                      IDS_PSST_COMPLETE_CONSENT_DIALOG_OK))
                  .SetStyle(ui::ButtonStyle::kDefault)
                  .SetCallback(
                      base::BindRepeating(&OnOkCallBackCompleteDlgWithClose,
                                          weak_factory_.GetWeakPtr()))
                  .SetProperty(views::kMarginsKey, gfx::Insets().set_left(16))
                  .SetHorizontalAlignment(
                      gfx::HorizontalAlignment::ALIGN_CENTER)));
  AddChildView(std::move(complete_view_box).Build());

  DCHECK(complete_view_header);
  SetFont(complete_view_header, kHeaderFontSize);
  complete_view_header->SetProperty(views::kMarginsKey,
                                    gfx::Insets().set_bottom(16));

  DCHECK(complete_view_body_applied_title_);
  SetFont(complete_view_body_applied_title_, kListTitleFontSize);

  DCHECK(complete_view_body_failed_title_);
  SetFont(complete_view_body_failed_title_, kListTitleFontSize);
}

PsstConsentDialog::~PsstConsentDialog() = default;

gfx::Size PsstConsentDialog::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size result;
  for (const View* child : children()) {
    if (!child->GetProperty(views::kViewIgnoredByLayoutKey) &&
        child->GetVisible()) {
      result.SetToMax(child->GetPreferredSize(
          bounds().IsEmpty() ? views::SizeBounds()
                             : views::SizeBounds(GetContentsBounds().size())));
    }
  }

  return !result.IsZero()
             ? result
             : DialogDelegateView::CalculatePreferredSize(available_size);
}

void PsstConsentDialog::WindowClosing() {
  DialogDelegateView::WindowClosing();
}

void PsstConsentDialog::SetProgressValue(const double value) {
  if (!progress_bar_) {
    return;
  }
  progress_bar_->SetValue(std::move(value));
}

void PsstConsentDialog::SetRequestDone(const std::string& url) {
  if (!task_checked_list_.contains(url)) {
    return;
  }

  const auto status_checked_line_to_mark = task_checked_list_[url].get();
  if (!status_checked_line_to_mark) {
    return;
  }

  if (auto* checkbox_to_mark = status_checked_line_to_mark->check_box.get()) {
    checkbox_to_mark->SetChecked(true);
  }
  LOG(INFO) << "[PSST] SetRequestDone url:" << url;
  if (auto* status_label = status_checked_line_to_mark->status_label.get()) {
    status_label->SetText(kDoneMessage);
    status_label->SetEnabledColor(
        status_label->GetColorProvider()->GetColor(ui::kColorLabelForeground));
  }
}

void PsstConsentDialog::SetRequestError(const std::string& url,
                                        const std::string& error) {
  if (!task_checked_list_.contains(url)) {
    return;
  }

  const auto status_checked_line_to_mark = task_checked_list_[url].get();
  if (!status_checked_line_to_mark) {
    return;
  }

  LOG(INFO) << "[PSST] SetRequestError url:" << url;
  if (auto* status_label = status_checked_line_to_mark->status_label.get()) {
    status_label->SetText(base::ASCIIToUTF16(error));
    status_label->SetEnabledColor(
        status_label->GetColorProvider()->GetColor(ui::kColorSysError));
  }
}

void PsstConsentDialog::OnConsentClicked() {
  if (!consent_callback_) {
    return;
  }

  if (ok_button_) {
    ok_button_->SetEnabled(false);
  }
  if (no_button_) {
    no_button_->SetEnabled(false);
  }

  std::vector<std::string> skip_checks;
  for (auto& [url, status_data] : task_checked_list_) {
    if (!status_data->check_box->GetChecked()) {
      skip_checks.push_back(url);
    }
    status_data->check_box->SetEnabled(false);
  }
  std::move(consent_callback_).Run(std::move(skip_checks));
}

// void PsstConsentDialog::SetStatusView() {
//   if(!box_complete_view_ || owned_box_complete_view_) {
//     return;
//   }

//   owned_box_complete_view_ = RemoveChildViewT(box_complete_view_);
//   box_complete_view_ = nullptr;

//   box_status_view_ = AddChildView(std::move(owned_box_status_view_));
// }

void PsstConsentDialog::SetCompletedView(
    const std::vector<std::string>& applied_checks,
    const std::vector<std::string>& errors) {
  if (!box_status_view_ || !box_complete_view_) {
    return;
  }

  if (!applied_checks.empty()) {
    complete_view_body_applied_title_->SetText(l10n_util::GetStringUTF16(
        IDS_PSST_COMPLETE_CONSENT_DIALOG_APPLIED_LIST_TITLE));
    complete_view_body_applied_->SetText(
        base::ASCIIToUTF16(base::JoinString(applied_checks, "\n")));
  }

  if (!errors.empty()) {
    complete_view_body_failed_title_->SetText(l10n_util::GetStringUTF16(
        IDS_PSST_COMPLETE_CONSENT_DIALOG_FAILED_LIST_TITLE));
    complete_view_body_failed_->SetText(
        base::ASCIIToUTF16(base::JoinString(errors, "\n")));
  }

  box_status_view_->SetVisible(false);
  box_complete_view_->SetVisible(true);

  GetWidget()->SetBounds(GetDesiredWidgetBounds());
}
