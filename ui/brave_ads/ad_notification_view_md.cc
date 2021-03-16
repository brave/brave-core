/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ui/brave_ads/ad_notification_view_md.h"

#include <stddef.h>
#include <memory>
#include <utility>

#include "base/files/file_path.h"
#include "base/i18n/case_conversion.h"
#include "base/metrics/histogram_macros.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/grit/brave_theme_resources.h"
#include "brave/grit/brave_unscaled_resources.h"
#include "brave/ui/brave_ads/message_popup_view.h"
#include "brave/ui/brave_ads/notification_background_painter.h"
#include "brave/ui/brave_ads/notification_control_buttons_view.h"
#include "brave/ui/brave_ads/notification_header_view.h"
#include "brave/ui/brave_ads/padded_button.h"
#include "brave/ui/brave_ads/public/cpp/constants.h"
#include "brave/ui/brave_ads/public/cpp/notification.h"
#include "build/build_config.h"
#include "components/url_formatter/elide_url.h"
#include "ui/base/class_property.h"
#include "ui/base/cursor/cursor.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/gesture_detection/gesture_provider_config_helper.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/skia_util.h"
#include "ui/gfx/text_constants.h"
#include "ui/gfx/text_elider.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/animation/flood_fill_ink_drop_ripple.h"
#include "ui/views/animation/ink_drop_highlight.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/animation/ink_drop_mask.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/radio_button.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/progress_bar.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/native_cursor.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace brave_ads {

namespace {

// Dimensions.
constexpr gfx::Insets kContentRowPadding(0, 12, 10, 6);
constexpr gfx::Size kIconViewSize(36, 36);
constexpr gfx::Insets kLeftContentPadding(0, 0, 0, 4);
constexpr gfx::Insets kLeftContentPaddingWithIcon(2, 4, 0, 12);

// Background color of the inline settings.
constexpr SkColor kInlineSettingsBackgroundColor =
    SkColorSetRGB(0xEE, 0xEE, 0xEE);

// Max number of lines for message_view_.
constexpr int kMaxLinesForNotificationView = 1;
constexpr int kMaxLinesForExpandedNotificationView = 4;

constexpr int kNotificationViewWidthWithIcon =
    kNotificationWidth - kIconViewSize.width() -
    kLeftContentPaddingWithIcon.left() - kLeftContentPaddingWithIcon.right() -
    kContentRowPadding.left() - kContentRowPadding.right();

// Character limit = pixels per line * line limit / min. pixels per character.
constexpr size_t kMessageCharacterLimitMD =
    kNotificationWidth * kMessageExpandedLineLimit / 3;

constexpr int kBodyTextFontSize = 13;

// Line height of title and message views.
constexpr int kLineHeightMD = 17;

// FontList for the texts except for the header.
gfx::FontList GetTextFontList() {
  gfx::Font default_font;
  int font_size_delta = kBodyTextFontSize - default_font.GetFontSize();
  gfx::Font font = default_font.Derive(font_size_delta, gfx::Font::NORMAL,
                                       gfx::Font::Weight::NORMAL);
  return gfx::FontList(font);
}

class ClickActivator : public ui::EventHandler {
 public:
  explicit ClickActivator(AdNotificationViewMD* owner) : owner_(owner) {}
  ~ClickActivator() override = default;

 private:
  // ui::EventHandler
  void OnEvent(ui::Event* event) override {
    if (event->type() == ui::ET_MOUSE_PRESSED ||
        event->type() == ui::ET_GESTURE_TAP) {
      owner_->Activate();
    }
  }

  AdNotificationViewMD* const owner_;

  DISALLOW_COPY_AND_ASSIGN(ClickActivator);
};

}  // anonymous namespace

// NotificationInkDropImpl /////////////////////////////////////////////////////

class NotificationInkDropImpl : public views::InkDropImpl {
 public:
  NotificationInkDropImpl(views::InkDropHostView* ink_drop_host,
                          const gfx::Size& host_size)
      : views::InkDropImpl(ink_drop_host, host_size) {
    SetAutoHighlightMode(views::InkDropImpl::AutoHighlightMode::SHOW_ON_RIPPLE);
  }

  void HostSizeChanged(const gfx::Size& new_size) override {
    // Prevent a call to InkDropImpl::HostSizeChanged which recreates the ripple
    // and stops the currently active animation: http://crbug.com/915222.
  }
};

// ////////////////////////////////////////////////////////////
// AdNotificationViewMD
// ////////////////////////////////////////////////////////////

class AdNotificationViewMD::AdNotificationViewMDPathGenerator
    : public views::HighlightPathGenerator {
 public:
  AdNotificationViewMDPathGenerator() = default;
  AdNotificationViewMDPathGenerator(const AdNotificationViewMDPathGenerator&) =
      delete;
  AdNotificationViewMDPathGenerator& operator=(
      const AdNotificationViewMDPathGenerator&) = delete;

  base::Optional<gfx::RRectF> GetRoundRect(const gfx::RectF& rect) override {
    gfx::RectF bounds = rect;
    if (!preferred_size_.IsEmpty())
      bounds.set_size(gfx::SizeF(preferred_size_));
    gfx::RoundedCornersF corner_radius(top_radius_, top_radius_, bottom_radius_,
                                       bottom_radius_);
    return gfx::RRectF(bounds, corner_radius);
  }

  void set_top_radius(int val) { top_radius_ = val; }
  void set_bottom_radius(int val) { bottom_radius_ = val; }
  void set_preferred_size(const gfx::Size& val) { preferred_size_ = val; }

 private:
  int top_radius_ = 0;
  int bottom_radius_ = 0;

  // This custom PathGenerator is used for the ink drop clipping bounds. By
  // setting |preferred_size_| we set the correct clip bounds in
  // GetRoundRect(). This is needed as the correct bounds for the ink drop are
  // required before a Layout() on the view is run. See
  // http://crbug.com/915222.
  gfx::Size preferred_size_;
};

void AdNotificationViewMD::CreateOrUpdateViews(
    const Notification& notification) {
  left_content_count_ = 0;

  CreateOrUpdateContextTitleView(notification);
  CreateOrUpdateNotificationView(notification);
  if (message_view_) {
    message_view_->SetMaxLines(kMaxLinesForExpandedNotificationView);
  }

  right_content_->SetVisible(true);
  left_content_->SetBorder(views::CreateEmptyBorder(kLeftContentPadding));

  // TODO(tetsui): Workaround https://crbug.com/682266 by explicitly setting
  // the width.
  // Ideally, we should fix the original bug, but it seems there's no obvious
  // solution for the bug according to https://crbug.com/678337#c7, we should
  // ensure that the change won't break any of the users of BoxLayout class.
  const int message_view_width =
      kNotificationViewWidthWithIcon - GetInsets().width();
  if (message_view_)
    message_view_->SizeToFit(message_view_width);

  content_row_->InvalidateLayout();
}

AdNotificationViewMD::AdNotificationViewMD(const Notification& notification)
    : NotificationView(notification),
      ink_drop_container_(new views::InkDropContainerView()) {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(), 0));

  SetInkDropVisibleOpacity(1);

  AddChildView(ink_drop_container_);

  control_buttons_view_ =
      std::make_unique<NotificationControlButtonsView>(this);
  control_buttons_view_->set_owned_by_client();

  // |header_row_| contains app_icon, app_name, control buttons, etc...
  header_row_ = new NotificationHeaderView();
  header_row_->AddChildView(control_buttons_view_.get());
  header_row_->SetBounds(0, 0, 30, 20);
  AddChildView(header_row_);

  // |content_row_| contains title, message, image, progressbar, etc...
  content_row_ = new views::View();
  auto* content_row_layout =
      content_row_->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal, kContentRowPadding, 0));
  content_row_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStart);
  AddChildView(content_row_);

  // |left_content_| contains most contents like title, message, etc...
  left_content_ = new views::View();
  left_content_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(), 0));
  left_content_->SetBorder(views::CreateEmptyBorder(kLeftContentPadding));
  content_row_->AddChildView(left_content_);
  content_row_layout->SetFlexForView(left_content_, 1);

  // |right_content_| contains notification icon and small image.
  right_content_ = new views::View();
  right_content_->SetLayoutManager(std::make_unique<views::FillLayout>());
  content_row_->AddChildView(right_content_);

  // |action_row_| contains inline action buttons and inline textfield.
  actions_row_ = new views::View();
  actions_row_->SetVisible(false);
  actions_row_->SetLayoutManager(std::make_unique<views::FillLayout>());
  AddChildView(actions_row_);

  CreateOrUpdateViews(notification);
  UpdateControlButtonsVisibilityWithNotification(notification);

  SetNotifyEnterExitOnChild(true);

  click_activator_ = std::make_unique<ClickActivator>(this);
  // Reasons to use pretarget handler instead of OnMousePressed:
  // - AdNotificationViewMD::OnMousePresssed would not fire on the inline reply
  //   textfield click in native notification.
  // - To make it look similar to ArcNotificationContentView::EventForwarder.
  AddPreTargetHandler(click_activator_.get());
  auto highlight_path_generator =
      std::make_unique<AdNotificationViewMDPathGenerator>();
  highlight_path_generator_ = highlight_path_generator.get();
  views::HighlightPathGenerator::Install(this,
                                         std::move(highlight_path_generator));
  UpdateCornerRadius(kNotificationCornerRadius, kNotificationCornerRadius);
}

AdNotificationViewMD::~AdNotificationViewMD() {
  RemovePreTargetHandler(click_activator_.get());
}

void AdNotificationViewMD::AddLayerBeneathView(ui::Layer* layer) {
  GetInkDrop()->AddObserver(this);
  for (auto* child : GetChildrenForLayerAdjustment()) {
    child->SetPaintToLayer();
    child->layer()->SetFillsBoundsOpaquely(false);
  }
  ink_drop_container_->AddLayerBeneathView(layer);
}

void AdNotificationViewMD::RemoveLayerBeneathView(ui::Layer* layer) {
  ink_drop_container_->RemoveLayerBeneathView(layer);
  for (auto* child : GetChildrenForLayerAdjustment())
    child->DestroyLayer();
  GetInkDrop()->RemoveObserver(this);
}

void AdNotificationViewMD::Layout() {
  NotificationView::Layout();

  // We need to call IsExpandable() at the end of Layout() call, since whether
  // we should show expand button or not depends on the current view layout.
  // (e.g. Show expand button when |message_view_| exceeds one line.)
  header_row_->Layout();

  // The notification background is rounded in NotificationView::Layout(),
  // but we also have to round the actions row background here.
  if (actions_row_->GetVisible()) {
    constexpr SkScalar kCornerRadius = SkIntToScalar(kNotificationCornerRadius);

    // Use vertically larger clip path, so that actions row's top coners will
    // not be rounded.
    SkPath path;
    gfx::Rect bounds = actions_row_->GetLocalBounds();
    bounds.set_y(bounds.y() - bounds.height());
    bounds.set_height(bounds.height() * 2);
    path.addRoundRect(gfx::RectToSkRect(bounds), kCornerRadius, kCornerRadius);
  }

  // The animation is needed to run inside of the border.
  ink_drop_container_->SetBoundsRect(GetLocalBounds());
}

void AdNotificationViewMD::OnFocus() {
  NotificationView::OnFocus();
  ScrollRectToVisible(GetLocalBounds());
}

bool AdNotificationViewMD::OnMousePressed(const ui::MouseEvent& event) {
  last_mouse_pressed_timestamp_ = base::TimeTicks(event.time_stamp());
  return true;
}

bool AdNotificationViewMD::OnMouseDragged(const ui::MouseEvent& event) {
  return true;
}

void AdNotificationViewMD::OnMouseReleased(const ui::MouseEvent& event) {
  if (!event.IsOnlyLeftMouseButton())
    return;

  // The mouse has been clicked for a long time.
  if (ui::EventTimeStampToSeconds(event.time_stamp()) -
          ui::EventTimeStampToSeconds(last_mouse_pressed_timestamp_) >
      ui::GetGestureProviderConfig(
          ui::GestureProviderConfigType::CURRENT_PLATFORM)
          .gesture_detector_config.longpress_timeout.InSecondsF()) {
    ToggleInlineSettings(event);
    return;
  }

  // Ignore click of actions row outside action buttons.
  gfx::Point point_in_child = event.location();
  ConvertPointToTarget(this, actions_row_, &point_in_child);
  if (actions_row_->HitTestPoint(point_in_child))
    return;

  // Ignore clicks of outside region when inline settings is shown.
  if (settings_row_ && settings_row_->GetVisible())
    return;

  MessagePopupView::Clicked(notification_id());
  NotificationView::OnMouseReleased(event);
}

void AdNotificationViewMD::OnMouseEvent(ui::MouseEvent* event) {
  switch (event->type()) {
    case ui::ET_MOUSE_ENTERED:
      UpdateControlButtonsVisibility();
      break;
    case ui::ET_MOUSE_EXITED:
      UpdateControlButtonsVisibility();
      break;
    default:
      break;
  }
  View::OnMouseEvent(event);
}

void AdNotificationViewMD::OnGestureEvent(ui::GestureEvent* event) {
  if (event->type() == ui::ET_GESTURE_LONG_TAP) {
    ToggleInlineSettings(*event);
    return;
  }
  NotificationView::OnGestureEvent(event);
}

void AdNotificationViewMD::PreferredSizeChanged() {
  NotificationView::PreferredSizeChanged();
}

void AdNotificationViewMD::UpdateWithNotification(
    const Notification& notification) {
  NotificationView::UpdateWithNotification(notification);
  UpdateControlButtonsVisibilityWithNotification(notification);

  CreateOrUpdateViews(notification);
  Layout();
  SchedulePaint();
}

void AdNotificationViewMD::UpdateControlButtonsVisibilityWithNotification(
    const Notification& notification) {
  control_buttons_view_->ShowInfoButton(true);
  control_buttons_view_->ShowCloseButton(true);
  UpdateControlButtonsVisibility();
}

void AdNotificationViewMD::CreateOrUpdateContextTitleView(
    const Notification& notification) {
  header_row_->SetAccentColor(SK_ColorTRANSPARENT);
  header_row_->SetBackgroundColor(kNotificationBackgroundColor);
  header_row_->SetAdNameElideBehavior(gfx::ELIDE_TAIL);
  header_row_->SetAdName(notification.title());
}

void AdNotificationViewMD::CreateOrUpdateNotificationView(
    const Notification& notification) {
  std::u16string text = gfx::TruncateString(
      notification.message(), kMessageCharacterLimitMD, gfx::WORD_BREAK);

  if (!message_view_) {
    const gfx::FontList& font_list = GetTextFontList();

    message_view_ = new views::Label(text);
    message_view_->SetFontList(font_list);
    message_view_->SetHorizontalAlignment(gfx::ALIGN_TO_HEAD);
    message_view_->SetEnabledColor(kDimTextColorMD);
    message_view_->SetBackgroundColor(kNotificationBackgroundColor);
    message_view_->SetLineHeight(kLineHeightMD);
    message_view_->SetMultiLine(true);
    message_view_->SetMaxLines(kMaxLinesForNotificationView);
    message_view_->SetAllowCharacterBreak(true);
    left_content_->AddChildViewAt(message_view_, left_content_count_);
  } else {
    message_view_->SetText(text);
  }

  message_view_->SetVisible(true);
  left_content_count_++;
}

void AdNotificationViewMD::ToggleInlineSettings(const ui::Event& event) {
  if (!settings_row_)
    return;

  bool inline_settings_visible = !settings_row_->GetVisible();

  settings_row_->SetVisible(inline_settings_visible);
  content_row_->SetVisible(!inline_settings_visible);
  header_row_->SetBackgroundColor(inline_settings_visible
                                      ? kInlineSettingsBackgroundColor
                                      : kNotificationBackgroundColor);

  PreferredSizeChanged();

  if (inline_settings_visible)
    AddBackgroundAnimation(event);
  else
    RemoveBackgroundAnimation();

  Layout();
  SchedulePaint();
}

void AdNotificationViewMD::UpdateCornerRadius(int top_radius,
                                              int bottom_radius) {
  NotificationView::UpdateCornerRadius(top_radius, bottom_radius);
  highlight_path_generator_->set_top_radius(top_radius);
  highlight_path_generator_->set_bottom_radius(bottom_radius);
}

NotificationControlButtonsView* AdNotificationViewMD::GetControlButtonsView()
    const {
  return control_buttons_view_.get();
}

void AdNotificationViewMD::Activate() {
  GetWidget()->widget_delegate()->SetCanActivate(true);
  GetWidget()->Activate();
}

void AdNotificationViewMD::AddBackgroundAnimation(const ui::Event& event) {
  SetInkDropMode(InkDropMode::ON_NO_GESTURE_HANDLER);
  // In case the animation is triggered from keyboard operation.
  if (!event.IsLocatedEvent()) {
    AnimateInkDrop(views::InkDropState::ACTION_PENDING, nullptr);
    return;
  }

  // Convert the point of |event| from the coordinate system of
  // |control_buttons_view_| to that of AdNotificationViewMD, create a new
  // LocatedEvent which has the new point.
  views::View* target = static_cast<views::View*>(event.target());
  const gfx::Point& location = event.AsLocatedEvent()->location();
  gfx::Point converted_location(location);
  View::ConvertPointToTarget(target, this, &converted_location);

  // Use default animation if location is out of bounds.
  if (!View::HitTestPoint(converted_location)) {
    AnimateInkDrop(views::InkDropState::ACTION_PENDING, nullptr);
    return;
  }

  std::unique_ptr<ui::Event> cloned_event = ui::Event::Clone(event);
  ui::LocatedEvent* cloned_located_event = cloned_event->AsLocatedEvent();
  cloned_located_event->set_location(converted_location);
  AnimateInkDrop(views::InkDropState::ACTION_PENDING, cloned_located_event);
}

void AdNotificationViewMD::RemoveBackgroundAnimation() {
  SetInkDropMode(InkDropMode::OFF);
  AnimateInkDrop(views::InkDropState::HIDDEN, nullptr);
}

std::unique_ptr<views::InkDrop> AdNotificationViewMD::CreateInkDrop() {
  return std::make_unique<NotificationInkDropImpl>(this, size());
}

std::unique_ptr<views::InkDropRipple>
AdNotificationViewMD::CreateInkDropRipple() const {
  return std::make_unique<views::FloodFillInkDropRipple>(
      GetPreferredSize(), GetInkDropCenterBasedOnLastEvent(),
      GetInkDropBaseColor(), GetInkDropVisibleOpacity());
}

std::vector<views::View*> AdNotificationViewMD::GetChildrenForLayerAdjustment()
    const {
  return {header_row_};
}

std::unique_ptr<views::InkDropMask> AdNotificationViewMD::CreateInkDropMask()
    const {
  return nullptr;
}

SkColor AdNotificationViewMD::GetInkDropBaseColor() const {
  return kInlineSettingsBackgroundColor;
}

void AdNotificationViewMD::InkDropAnimationStarted() {
  header_row_->SetSubpixelRenderingEnabled(false);
}

void AdNotificationViewMD::InkDropRippleAnimationEnded(
    views::InkDropState ink_drop_state) {
  if (ink_drop_state == views::InkDropState::HIDDEN)
    header_row_->SetSubpixelRenderingEnabled(true);
}

}  // namespace brave_ads
