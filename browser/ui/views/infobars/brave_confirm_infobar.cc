/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/brave_confirm_infobar.h"

#include <algorithm>
#include <utility>

#include "base/functional/bind.h"
#include "build/build_config.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/ui_base_features.h"
#include "ui/base/window_open_disposition.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/view_class_properties.h"

namespace {

constexpr int kCheckboxSpacing = 20;

}  // namespace

std::unique_ptr<infobars::InfoBar> CreateBraveConfirmInfoBar(
    std::unique_ptr<BraveConfirmInfoBarDelegate> delegate) {
  return std::make_unique<BraveConfirmInfoBar>(std::move(delegate));
}

BraveConfirmInfoBar::BraveConfirmInfoBar(
    std::unique_ptr<BraveConfirmInfoBarDelegate> delegate)
    : InfoBarView(std::move(delegate)) {
  auto* delegate_ptr = GetDelegate();
  label_ = AddChildView(CreateLabel(delegate_ptr->GetMessageText()));
  label_->SetElideBehavior(delegate_ptr->GetMessageElideBehavior());

  const auto create_button =
      [this](ConfirmInfoBarDelegate::InfoBarButton type,
             void (BraveConfirmInfoBar::*click_function)()) {
        auto* button = AddChildView(std::make_unique<views::MdTextButton>(
            base::BindRepeating(click_function, weak_ptr_factory_.GetWeakPtr()),
            GetDelegate()->GetButtonLabel(type)));
        button->SetProperty(
            views::kMarginsKey,
            gfx::Insets::VH(ChromeLayoutProvider::Get()->GetDistanceMetric(
                                DISTANCE_TOAST_CONTROL_VERTICAL),
                            0));
        return button;
      };

  const auto buttons = delegate_ptr->GetButtons();
  if (buttons & ConfirmInfoBarDelegate::BUTTON_OK) {
    ok_button_ = create_button(ConfirmInfoBarDelegate::BUTTON_OK,
                               &BraveConfirmInfoBar::OkButtonPressed);
    ok_button_->SetStyle(ui::ButtonStyle::kProminent);
    ok_button_->SetImageModel(
        views::Button::STATE_NORMAL,
        delegate_ptr->GetButtonImage(ConfirmInfoBarDelegate::BUTTON_OK));
    ok_button_->SetEnabled(
        delegate_ptr->GetButtonEnabled(ConfirmInfoBarDelegate::BUTTON_OK));
    ok_button_->SetTooltipText(
        delegate_ptr->GetButtonTooltip(ConfirmInfoBarDelegate::BUTTON_OK));
  }

  if (buttons & ConfirmInfoBarDelegate::BUTTON_CANCEL) {
    cancel_button_ = create_button(ConfirmInfoBarDelegate::BUTTON_CANCEL,
                                   &BraveConfirmInfoBar::CancelButtonPressed);
    if (buttons == ConfirmInfoBarDelegate::BUTTON_CANCEL ||
        delegate_ptr->IsProminent(ConfirmInfoBarDelegate::BUTTON_CANCEL)) {
      cancel_button_->SetStyle(ui::ButtonStyle::kProminent);
    }
    cancel_button_->SetImageModel(
        views::Button::STATE_NORMAL,
        delegate_ptr->GetButtonImage(ConfirmInfoBarDelegate::BUTTON_CANCEL));
    cancel_button_->SetEnabled(
        delegate_ptr->GetButtonEnabled(ConfirmInfoBarDelegate::BUTTON_CANCEL));
    cancel_button_->SetTooltipText(
        delegate_ptr->GetButtonTooltip(ConfirmInfoBarDelegate::BUTTON_CANCEL));
  }

  if (buttons & ConfirmInfoBarDelegate::BUTTON_EXTRA) {
    extra_button_ = create_button(ConfirmInfoBarDelegate::BUTTON_EXTRA,
                                  &BraveConfirmInfoBar::ExtraButtonPressed);
    if (buttons == ConfirmInfoBarDelegate::BUTTON_EXTRA ||
        delegate_ptr->IsProminent(ConfirmInfoBarDelegate::BUTTON_EXTRA)) {
      extra_button_->SetStyle(ui::ButtonStyle::kProminent);
    }
    extra_button_->SetImageModel(
        views::Button::STATE_NORMAL,
        delegate_ptr->GetButtonImage(ConfirmInfoBarDelegate::BUTTON_EXTRA));
    extra_button_->SetEnabled(
        delegate_ptr->GetButtonEnabled(ConfirmInfoBarDelegate::BUTTON_EXTRA));
    extra_button_->SetTooltipText(
        delegate_ptr->GetButtonTooltip(ConfirmInfoBarDelegate::BUTTON_EXTRA));
  }

  link_ = AddChildView(CreateLink(delegate_ptr->GetLinkText()));

  if (delegate_ptr->HasCheckbox()) {
    checkbox_ = AddChildView(std::make_unique<views::Checkbox>(
        delegate_ptr->GetCheckboxText(),
        base::BindRepeating(&BraveConfirmInfoBar::CheckboxPressed,
                            weak_ptr_factory_.GetWeakPtr())));
  }
}

BraveConfirmInfoBar::~BraveConfirmInfoBar() = default;

views::MdTextButton* BraveConfirmInfoBar::GetButtonById(int id) {
  switch (id) {
    case ConfirmInfoBarDelegate::BUTTON_OK:
      return ok_button_;
    case ConfirmInfoBarDelegate::BUTTON_CANCEL:
      return cancel_button_;
    case ConfirmInfoBarDelegate::BUTTON_EXTRA:
      return extra_button_;
  }
  NOTREACHED();
}

void BraveConfirmInfoBar::Layout(PassKey) {
  LayoutSuperclass<InfoBarView>(this);

  if (ok_button_) {
    ok_button_->SizeToPreferredSize();
  }

  if (cancel_button_) {
    cancel_button_->SizeToPreferredSize();
  }

  if (extra_button_) {
    extra_button_->SizeToPreferredSize();
  }

  int x = GetStartX();
  Views views;
  views.push_back(label_.get());
  views.push_back(link_.get());
  AssignWidths(&views, std::max(0, GetEndX() - x - NonLabelWidth()));

  ChromeLayoutProvider* layout_provider = ChromeLayoutProvider::Get();

  label_->SetPosition(gfx::Point(x, OffsetY(label_)));
  if (!label_->GetText().empty()) {
    x = label_->bounds().right() +
        layout_provider->GetDistanceMetric(
            DISTANCE_INFOBAR_HORIZONTAL_ICON_LABEL_PADDING);
  }

  auto order = GetDelegate()->GetButtonsOrder();
  for (const auto& id : order) {
    auto* current_button = GetButtonById(id);
    if (!current_button) {
      continue;
    }
    current_button->SetPosition(gfx::Point(x, OffsetY(current_button)));
    x = current_button->bounds().right() +
        layout_provider->GetDistanceMetric(
            views::DISTANCE_RELATED_BUTTON_HORIZONTAL);
  }

  // Place checkbox after latest button
  if (checkbox_) {
    checkbox_->SizeToPreferredSize();
    x += kCheckboxSpacing;
    checkbox_->SetPosition(gfx::Point(x, OffsetY(checkbox_)));
  }

  link_->SetPosition(gfx::Point(GetEndX() - link_->width(), OffsetY(link_)));
}

void BraveConfirmInfoBar::CheckboxPressed() {
  GetDelegate()->SetCheckboxChecked(checkbox_->GetChecked());
}

void BraveConfirmInfoBar::CloseButtonPressed() {
  if (GetDelegate()->InterceptClosing()) {
    return;
  }

  InfoBarView::CloseButtonPressed();
}

void BraveConfirmInfoBar::OkButtonPressed() {
  if (!owner()) {
    return;  // We're closing; don't call anything, it might access the owner.
  }
  if (GetDelegate()->Accept()) {
    RemoveSelf();
  }
}

void BraveConfirmInfoBar::CancelButtonPressed() {
  if (!owner()) {
    return;  // We're closing; don't call anything, it might access the owner.
  }
  if (GetDelegate()->Cancel()) {
    RemoveSelf();
  }
}

void BraveConfirmInfoBar::ExtraButtonPressed() {
  if (!owner()) {
    return;  // We're closing; don't call anything, it might access the owner.
  }
  if (GetDelegate()->ExtraButtonPressed()) {
    RemoveSelf();
  }
}

BraveConfirmInfoBarDelegate* BraveConfirmInfoBar::GetDelegate() const {
  return reinterpret_cast<BraveConfirmInfoBarDelegate*>(delegate());
}

int BraveConfirmInfoBar::GetContentMinimumWidth() const {
  return label_->GetMinimumSize().width() + link_->GetMinimumSize().width() +
         NonLabelWidth();
}

int BraveConfirmInfoBar::NonLabelWidth() const {
  ChromeLayoutProvider* layout_provider = ChromeLayoutProvider::Get();

  const int label_spacing = layout_provider->GetDistanceMetric(
      views::DISTANCE_RELATED_LABEL_HORIZONTAL);
  const int button_spacing = layout_provider->GetDistanceMetric(
      views::DISTANCE_RELATED_BUTTON_HORIZONTAL);

  const int button_count = GetDelegate()->GetButtonsOrder().size();
  ;

  int width =
      (label_->GetText().empty() || button_count == 0) ? 0 : label_spacing;

  width += std::max(0, button_spacing * (button_count - 1));

  width += ok_button_ ? ok_button_->width() : 0;
  width += cancel_button_ ? cancel_button_->width() : 0;
  width += extra_button_ ? extra_button_->width() : 0;

  if (checkbox_) {
    width += checkbox_->width() + kCheckboxSpacing;
  }

  return width + ((link_->GetText().empty() || !width) ? 0 : label_spacing);
}

BEGIN_METADATA(BraveConfirmInfoBar)
END_METADATA
