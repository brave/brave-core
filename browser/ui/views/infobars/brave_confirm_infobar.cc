/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/brave_confirm_infobar.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ref.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/window_open_disposition.h"
#include "ui/base/window_open_disposition_utils.h"
#include "ui/color/color_provider.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/animation/animation_delegate_notifier.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/image_button_factory.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/focus/external_focus_tracker.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/widget/widget.h"

namespace {

constexpr int kCheckboxSpacing = 20;

}  // namespace

std::unique_ptr<infobars::InfoBar> CreateBraveConfirmInfoBar(
    std::unique_ptr<BraveConfirmInfoBarDelegate> delegate) {
  return std::make_unique<BraveConfirmInfoBar>(std::move(delegate));
}

class BraveConfirmInfoBar::FocusTracker : public views::ExternalFocusTracker {
 public:
  explicit FocusTracker(BraveConfirmInfoBar& owner)
      : views::ExternalFocusTracker(&owner, nullptr), owner_(owner) {}
  FocusTracker(const FocusTracker&) = delete;
  FocusTracker& operator=(const FocusTracker&) = delete;
  ~FocusTracker() override = default;

  // views::ExternalFocusTracker:
  void OnWillChangeFocus(views::View* focused_before,
                         views::View* focused_now) override {
    views::ExternalFocusTracker::OnWillChangeFocus(focused_before, focused_now);
    // Fire an accessibility alert when focus enters the infobar from outside
    // so screen readers announce its contents.
    if (focused_before && focused_now && !owner_->Contains(focused_before) &&
        owner_->Contains(focused_now)) {
      owner_->NotifyAccessibilityEventDeprecated(ax::mojom::Event::kAlert,
                                                 true);
    }
  }

 private:
  const raw_ref<BraveConfirmInfoBar> owner_;
};

BraveConfirmInfoBar::BraveConfirmInfoBar(
    std::unique_ptr<BraveConfirmInfoBarDelegate> delegate)
    : infobars::InfoBar(std::move(delegate)),
      focus_tracker_(std::make_unique<FocusTracker>(*this)) {
  // Drive the InfoBar animation off the compositor.
  SetNotifier(std::make_unique<
              gfx::AnimationDelegateNotifier<views::AnimationDelegateViews>>(
      this, this));

  // infobars::InfoBar manages deletion of this view.
  set_owned_by_client(OwnedByClientPassKey());

  // Clip child layers so buttons render correctly during the slide-in/out
  // animation.
  SetPaintToLayer();
  layer()->SetMasksToBounds(true);

  auto* delegate_ptr = GetDelegate();
  CHECK(delegate_ptr);

  // Icon.
  const ui::ImageModel& image = delegate_ptr->GetIcon();
  if (!image.IsEmpty()) {
    auto icon = std::make_unique<views::ImageView>();
    icon->SetImage(image);
    icon->SizeToPreferredSize();
    icon->SetProperty(
        views::kMarginsKey,
        gfx::Insets::VH(ChromeLayoutProvider::Get()->GetDistanceMetric(
                            DISTANCE_TOAST_LABEL_VERTICAL),
                        0));
    icon_ = AddChildView(std::move(icon));
  }

  // Message label.
  label_ = AddChildView(CreateLabel(delegate_ptr->GetMessageText()));
  label_->SetMultiLine(delegate_ptr->ShouldSupportMultiLine());
  label_->SetMaxLines(delegate_ptr->GetMaxLines());
  label_->SetElideBehavior(delegate_ptr->GetMessageElideBehavior());

  // Buttons.
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
    if (buttons == ConfirmInfoBarDelegate::BUTTON_CANCEL) {
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

  // Link.
  link_ = AddChildView(CreateLink(delegate_ptr->GetLinkText()));
  link_->SetMultiLine(delegate_ptr->ShouldSupportMultiLine());
  link_->SetMaxLines(delegate_ptr->GetMaxLines());
  link_->SetHorizontalAlignment(gfx::ALIGN_CENTER);

  // Optional checkbox.
  if (delegate_ptr->HasCheckbox()) {
    checkbox_ = AddChildView(std::make_unique<views::Checkbox>(
        delegate_ptr->GetCheckboxText(),
        base::BindRepeating(&BraveConfirmInfoBar::CheckboxPressed,
                            weak_ptr_factory_.GetWeakPtr())));
  }

  // Close button (always last for accessibility focus order).
  if (delegate_ptr->IsCloseable()) {
    auto close_button = views::CreateVectorImageButton(base::BindRepeating(
        &BraveConfirmInfoBar::CloseButtonPressed, base::Unretained(this)));
    views::SetImageFromVectorIconWithColor(
        close_button.get(), vector_icons::kCloseChromeRefreshOldIcon,
        {kColorInfoBarButtonIcon, kColorInfoBarForeground,
         kColorInfoBarButtonIconHovered});
    close_button->SetTooltipText(l10n_util::GetStringUTF16(IDS_ACCNAME_CLOSE));
    close_button_ = AddChildView(std::move(close_button));

    const gfx::Insets close_button_spacing = GetCloseButtonSpacing();
    close_button_->SetProperty(
        views::kMarginsKey,
        gfx::Insets::TLBR(close_button_spacing.top(), 0,
                          close_button_spacing.bottom(), 0));
    views::InstallCircleHighlightPathGenerator(close_button_);
  }

  SetTargetHeight(
      ChromeLayoutProvider::Get()->GetDistanceMetric(DISTANCE_INFOBAR_HEIGHT));

  GetViewAccessibility().SetRole(ax::mojom::Role::kAlertDialog);
  GetViewAccessibility().SetName(l10n_util::GetStringUTF8(IDS_ACCNAME_INFOBAR));
  GetViewAccessibility().SetKeyShortcuts("Alt+Shift+A");
}

BraveConfirmInfoBar::~BraveConfirmInfoBar() = default;

void BraveConfirmInfoBar::Layout(PassKey) {
  const int spacing = GetElementSpacing();

  int start_x = 0;
  if (icon_) {
    icon_->SetPosition(gfx::Point(spacing, OffsetY(icon_)));
    start_x = icon_->bounds().right();
  }

  const int content_minimum_width = label_->GetMinimumSize().width() +
                                    link_->GetMinimumSize().width() +
                                    NonLabelWidth();
  int reserved_x = start_x;
  if (content_minimum_width > 0) {
    reserved_x += spacing + content_minimum_width;
  }

  if (close_button_) {
    const gfx::Insets close_button_spacing = GetCloseButtonSpacing();
    close_button_->SizeToPreferredSize();
    close_button_->SetPosition(gfx::Point(
        std::max(
            reserved_x + close_button_spacing.left(),
            width() - close_button_spacing.right() - close_button_->width()),
        OffsetY(close_button_)));
  }

  // Size the button content first so NonLabelWidth() reflects real widths.
  if (ok_button_) {
    ok_button_->SizeToPreferredSize();
  }
  if (cancel_button_) {
    cancel_button_->SizeToPreferredSize();
  }

  // Distribute remaining width between label and link.
  int x = GetStartX();
  Views label_and_link;
  label_and_link.push_back(label_.get());
  label_and_link.push_back(link_.get());
  AssignWidths(&label_and_link, std::max(0, GetEndX() - x - NonLabelWidth()));

  MaybeLayoutMultiLineLabelAndLink();

  auto* layout_provider = ChromeLayoutProvider::Get();

  label_->SetPosition(gfx::Point(x, OffsetY(label_)));
  if (!label_->GetText().empty()) {
    x = label_->bounds().right() +
        layout_provider->GetDistanceMetric(
            DISTANCE_INFOBAR_HORIZONTAL_ICON_LABEL_PADDING);
  }

  const auto place_button = [&](views::MdTextButton* button) {
    if (!button) {
      return;
    }
    button->SetPosition(gfx::Point(x, OffsetY(button)));
    x = button->bounds().right() +
        layout_provider->GetDistanceMetric(
            views::DISTANCE_RELATED_BUTTON_HORIZONTAL);
  };
  place_button(ok_button_);
  place_button(cancel_button_);

  // Place checkbox after latest button
  if (checkbox_) {
    checkbox_->SizeToPreferredSize();
    x += kCheckboxSpacing;
    checkbox_->SetPosition(gfx::Point(x, OffsetY(checkbox_)));
  }

  link_->SetPosition(gfx::Point(GetEndX() - link_->width(), OffsetY(link_)));
}

gfx::Size BraveConfirmInfoBar::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  int width = 0;
  const int spacing = GetElementSpacing();
  if (icon_) {
    width += spacing + icon_->width();
  }

  const int content_width = label_->GetMinimumSize().width() +
                            link_->GetMinimumSize().width() + NonLabelWidth();
  if (content_width > 0) {
    width += spacing + content_width;
  }

  const int trailing_space =
      close_button_ ? GetCloseButtonSpacing().width() + close_button_->width()
                    : spacing;
  return gfx::Size(width + trailing_space, computed_height());
}

void BraveConfirmInfoBar::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  const SkColor background_color = cp->GetColor(kColorInfoBarBackground);
  SetBackground(views::CreateSolidBackground(background_color));

  const SkColor text_color = cp->GetColor(kColorInfoBarForeground);
  if (label_) {
    label_->SetBackgroundColor(background_color);
    label_->SetEnabledColor(text_color);
    label_->SetAutoColorReadabilityEnabled(false);
  }
  if (link_) {
    link_->SetBackgroundColor(background_color);
  }

  // Set dark mode status so the delegate can pick a dark-mode-appropriate
  // icon.
  delegate()->set_dark_mode(color_utils::IsDark(background_color));
  if (icon_) {
    icon_->SetImage(delegate()->GetIcon());
  }
}

BraveConfirmInfoBarDelegate* BraveConfirmInfoBar::GetDelegate() {
  return static_cast<BraveConfirmInfoBarDelegate*>(delegate());
}

const BraveConfirmInfoBarDelegate* BraveConfirmInfoBar::GetDelegate() const {
  return static_cast<const BraveConfirmInfoBarDelegate*>(delegate());
}

void BraveConfirmInfoBar::PlatformSpecificShow(bool animate) {
  // Capture focus tracking once attached to a Widget so we can restore focus
  // on dismissal.
  focus_tracker_->SetFocusManager(GetFocusManager());
  NotifyAccessibilityEventDeprecated(ax::mojom::Event::kAlert, true);
}

void BraveConfirmInfoBar::PlatformSpecificHide(bool animate) {
  // Called twice (with animate=true then animate=false); the second
  // SetFocusManager(nullptr) is a silent no-op.
  focus_tracker_->SetFocusManager(nullptr);

  if (!animate) {
    return;
  }

  // Restore focus only if we're still in the active window.
  views::Widget* widget = GetWidget();
  if (!widget || widget->IsActive()) {
    focus_tracker_->FocusLastFocusedExternalView();
  }
}

void BraveConfirmInfoBar::PlatformSpecificOnHeightRecalculated() {
  InvalidateLayout();
}

std::unique_ptr<views::Label> BraveConfirmInfoBar::CreateLabel(
    const std::u16string& text) const {
  auto label = std::make_unique<views::Label>(
      text, views::style::CONTEXT_DIALOG_BODY_TEXT);
  SetLabelDetails(label.get());
  return label;
}

std::unique_ptr<views::Link> BraveConfirmInfoBar::CreateLink(
    const std::u16string& text) {
  auto link = std::make_unique<views::Link>(
      text, views::style::CONTEXT_DIALOG_BODY_TEXT);
  SetLabelDetails(link.get());
  link->SetCallback(base::BindRepeating(&BraveConfirmInfoBar::LinkClicked,
                                        base::Unretained(this)));
  return link;
}

void BraveConfirmInfoBar::SetLabelDetails(views::Label* label) const {
  label->SizeToPreferredSize();
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetProperty(
      views::kMarginsKey,
      gfx::Insets::VH(ChromeLayoutProvider::Get()->GetDistanceMetric(
                          DISTANCE_TOAST_LABEL_VERTICAL),
                      0));
}

// static
void BraveConfirmInfoBar::AssignWidths(Views* views, int available_width) {
  std::sort(views->begin(), views->end(), [](views::View* a, views::View* b) {
    return a->GetPreferredSize().width() > b->GetPreferredSize().width();
  });
  AssignWidthsSorted(views, available_width);
}

// static
void BraveConfirmInfoBar::AssignWidthsSorted(Views* views,
                                             int available_width) {
  if (views->empty()) {
    return;
  }
  gfx::Size back_view_size(views->back()->GetPreferredSize());
  back_view_size.set_width(
      std::min(back_view_size.width(),
               available_width / static_cast<int>(views->size())));
  views->back()->SetSize(back_view_size);
  views->pop_back();
  AssignWidthsSorted(views, available_width - back_view_size.width());
}

int BraveConfirmInfoBar::GetStartX() const {
  // Don't return a value greater than GetEndX() — keeps callers safe when
  // computing label/link widths as (GetEndX() - GetStartX()).
  return std::min((icon_ ? icon_->bounds().right() : 0) + GetElementSpacing(),
                  GetEndX());
}

int BraveConfirmInfoBar::GetEndX() const {
  return close_button_ ? close_button_->x() - GetCloseButtonSpacing().left()
                       : width() - GetElementSpacing();
}

int BraveConfirmInfoBar::OffsetY(views::View* view) const {
  return std::max((target_height() - view->height()) / 2, 0) -
         (target_height() - height());
}

gfx::Insets BraveConfirmInfoBar::GetCloseButtonSpacing() const {
  auto* provider = ChromeLayoutProvider::Get();
  // Match upstream's legacy close button spacing.
  return gfx::Insets::TLBR(
      provider->GetDistanceMetric(DISTANCE_TOAST_CONTROL_VERTICAL),
      provider->GetDistanceMetric(views::DISTANCE_UNRELATED_CONTROL_HORIZONTAL),
      provider->GetDistanceMetric(DISTANCE_TOAST_CONTROL_VERTICAL),
      provider->GetDistanceMetric(
          views::DISTANCE_UNRELATED_CONTROL_HORIZONTAL));
}

int BraveConfirmInfoBar::GetElementSpacing() const {
  return ChromeLayoutProvider::Get()->GetDistanceMetric(
      views::DISTANCE_UNRELATED_CONTROL_HORIZONTAL);
}

void BraveConfirmInfoBar::LinkClicked(const ui::Event& event) {
  if (!owner()) {
    return;  // We're closing; don't call anything, it might access the owner.
  }
  if (delegate()->LinkClicked(ui::DispositionFromEventFlags(event.flags()))) {
    RemoveSelf();
  }
}

int BraveConfirmInfoBar::NonLabelWidth() const {
  auto* layout_provider = ChromeLayoutProvider::Get();
  const int label_spacing = layout_provider->GetDistanceMetric(
      views::DISTANCE_RELATED_LABEL_HORIZONTAL);
  const int button_spacing = layout_provider->GetDistanceMetric(
      views::DISTANCE_RELATED_BUTTON_HORIZONTAL);

  const int button_count = (ok_button_ ? 1 : 0) + (cancel_button_ ? 1 : 0);

  int width =
      (label_->GetText().empty() || button_count == 0) ? 0 : label_spacing;

  width += std::max(0, button_spacing * (button_count - 1));

  width += ok_button_ ? ok_button_->width() : 0;
  width += cancel_button_ ? cancel_button_->width() : 0;

  if (checkbox_) {
    width += checkbox_->width() + kCheckboxSpacing;
  }

  return width + ((link_->GetText().empty() || !width) ? 0 : label_spacing);
}

void BraveConfirmInfoBar::MaybeLayoutMultiLineLabelAndLink() {
  if (!GetDelegate()->ShouldSupportMultiLine()) {
    return;
  }

  CHECK(label_);
  CHECK(link_);

  const int available_width = GetEndX() - GetStartX() - NonLabelWidth();
  label_->SizeToFit(std::max(0, available_width - link_->width()));

  // When label and link have different line counts, share the row width
  // proportionally to their text lengths for a balanced result.
  if (label_->GetRequiredLines() != link_->GetRequiredLines()) {
    const size_t text_size = label_->GetText().size() + link_->GetText().size();
    const int label_width =
        available_width * label_->GetText().size() / text_size;
    label_->SizeToFit(label_width);
    link_->SizeToFit(available_width - label_width);
  }

  const int max_height = std::max(label_->height(), link_->height());
  const int target_height = std::max(
      max_height,
      ChromeLayoutProvider::Get()->GetDistanceMetric(DISTANCE_INFOBAR_HEIGHT));

  // BraveSetTargetHeight sets both target_height_ and the computed height_
  // without invalidating layout — avoids re-entering Layout() mid-pass.
  BraveSetTargetHeight(target_height);
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

void BraveConfirmInfoBar::CloseButtonPressed() {
  if (!owner()) {
    return;  // We're closing; don't call anything, it might access the owner.
  }
  if (GetDelegate()->InterceptClosing()) {
    return;
  }
  delegate()->InfoBarDismissed();
  RemoveSelf();
}

void BraveConfirmInfoBar::CheckboxPressed() {
  GetDelegate()->SetCheckboxChecked(checkbox_->GetChecked());
}

BEGIN_METADATA(BraveConfirmInfoBar)
END_METADATA
