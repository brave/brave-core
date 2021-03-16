// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/ui/brave_ads/notification_header_view.h"

#include <memory>

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/ui/brave_ads/public/cpp/constants.h"
#include "build/build_config.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/flex_layout_types.h"
#include "ui/views/painter.h"
#include "ui/views/view_class_properties.h"

namespace brave_ads {

namespace {

constexpr int kHeaderHeight = 24;

// The padding between controls in the header.
constexpr gfx::Insets kHeaderSpacing(0, 0, 0, 0);

// The padding outer the header and the control buttons.
constexpr gfx::Insets kHeaderOuterPadding(0, 0, 0, 0);

constexpr int kInnerHeaderHeight = kHeaderHeight - kHeaderOuterPadding.height();

// Default paddings of the views of texts. Adjusted on Windows.
// Top: 9px = 11px (from the mock) - 2px (outer padding).
// Buttom: 6px from the mock.
constexpr gfx::Insets kTextViewPaddingDefault(9, 12, 6, 0);

// Bullet character. The divider symbol between different parts of the header.
constexpr wchar_t kNotificationHeaderDivider[] = L" \u2022 ";

constexpr int kHeaderTextFontSize = 14;

// Minimum spacing before the control buttons.
constexpr int kControlButtonSpacing = 10;

// ExpandButtton forwards all mouse and key events to NotificationHeaderView,
// but takes tab focus for accessibility purpose.
class ExpandButton : public views::ImageView {
 public:
  ExpandButton();
  ~ExpandButton() override;

  // Overridden from views::ImageView:
  void OnPaint(gfx::Canvas* canvas) override;
  void OnFocus() override;
  void OnBlur() override;

 private:
  std::unique_ptr<views::Painter> focus_painter_;
};

ExpandButton::ExpandButton() {
  focus_painter_ = views::Painter::CreateSolidFocusPainter(
      kFocusBorderColor, gfx::Insets(0, 0, 1, 1));
  SetFocusBehavior(FocusBehavior::ALWAYS);
}

ExpandButton::~ExpandButton() = default;

void ExpandButton::OnPaint(gfx::Canvas* canvas) {
  views::ImageView::OnPaint(canvas);
  if (HasFocus())
    views::Painter::PaintPainterAt(canvas, focus_painter_.get(),
                                   GetContentsBounds());
}

void ExpandButton::OnFocus() {
  views::ImageView::OnFocus();
  SchedulePaint();
}

void ExpandButton::OnBlur() {
  views::ImageView::OnBlur();
  SchedulePaint();
}

gfx::FontList GetHeaderTextFontList() {
  gfx::Font default_font;
  int font_size_delta = kHeaderTextFontSize - default_font.GetFontSize();
  gfx::Font font = default_font.Derive(font_size_delta, gfx::Font::NORMAL,
                                       gfx::Font::Weight::SEMIBOLD);
  return gfx::FontList(font);
}

gfx::Insets CalculateTopPadding(int font_list_height) {
#if defined(OS_WIN)
  // On Windows, the fonts can have slightly different metrics reported,
  // depending on where the code runs. In Chrome, DirectWrite is on, which means
  // font metrics are reported from Skia, which rounds from float using ceil.
  // In unit tests, however, GDI is used to report metrics, and the height
  // reported there is consistent with other platforms. This means there is a
  // difference of 1px in height between Chrome on Windows and everything else
  // (where everything else includes unit tests on Windows). This 1px causes the
  // text and everything else to stop aligning correctly, so we account for it
  // by shrinking the top padding by 1.
  if (font_list_height != 15) {
    return kTextViewPaddingDefault - gfx::Insets(1 /* top */, 0, 0, 0);
  }
#endif

  return kTextViewPaddingDefault;
}

}  // namespace

NotificationHeaderView::NotificationHeaderView(PressedCallback callback)
    : views::Button(callback) {
  const views::FlexSpecification kAppNameFlex =
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kPreferred)
          .WithOrder(1);

  const views::FlexSpecification kSpacerFlex =
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToMinimum,
                               views::MaximumFlexSizeRule::kUnbounded)
          .WithOrder(2);

  auto* layout = SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetDefault(views::kMarginsKey, kHeaderSpacing);
  layout->SetInteriorMargin(kHeaderOuterPadding);
  layout->SetCollapseMargins(true);

  // Font list for text views.
  gfx::FontList font_list = GetHeaderTextFontList();
  const int font_list_height = font_list.GetHeight();
  gfx::Insets text_view_padding(CalculateTopPadding(font_list_height));

  auto create_label = [&font_list, font_list_height, text_view_padding]() {
    auto* label = new views::Label();
    label->SetFontList(font_list);
    label->SetLineHeight(font_list_height);
    label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    label->SetBorder(views::CreateEmptyBorder(text_view_padding));
    return label;
  };

  // App name view
  ad_name_view_ = create_label();
  // Explicitly disable multiline to support proper text elision for URLs.
  ad_name_view_->SetMultiLine(false);
  ad_name_view_->SetEnabledColor(SK_ColorBLACK);
  ad_name_view_->SetProperty(views::kFlexBehaviorKey, kAppNameFlex);
  AddChildView(ad_name_view_);

  // Detail views which will be hidden in settings mode.
  detail_views_ = new views::View();
  auto* detail_layout =
      detail_views_->SetLayoutManager(std::make_unique<views::FlexLayout>());
  detail_layout->SetCollapseMargins(true);
  detail_layout->SetDefault(views::kMarginsKey, kHeaderSpacing);
  AddChildView(detail_views_);

  // Summary text divider
  summary_text_divider_ = create_label();
  summary_text_divider_->SetText(base::WideToUTF16(kNotificationHeaderDivider));
  summary_text_divider_->SetVisible(false);
  detail_views_->AddChildView(summary_text_divider_);

  // Summary text view
  summary_text_view_ = create_label();
  summary_text_view_->SetVisible(false);
  detail_views_->AddChildView(summary_text_view_);

  // Spacer between left-aligned views and right-aligned views
  views::View* spacer = new views::View;
  spacer->SetPreferredSize(
      gfx::Size(kControlButtonSpacing, kInnerHeaderHeight));
  spacer->SetProperty(views::kFlexBehaviorKey, kSpacerFlex);
  AddChildView(spacer);

  SetAccentColor(accent_color_);
  SetPreferredSize(gfx::Size(kNotificationWidth, kHeaderHeight));
}

NotificationHeaderView::~NotificationHeaderView() = default;

void NotificationHeaderView::SetAdIcon(const gfx::ImageSkia& img) {
  ad_icon_view_->SetImage(img);
  using_default_ad_icon_ = false;
}

void NotificationHeaderView::ClearAdIcon() {
  using_default_ad_icon_ = true;
}

void NotificationHeaderView::SetAdName(const std::u16string& name) {
  ad_name_view_->SetText(name);
}

void NotificationHeaderView::SetAdNameElideBehavior(
    gfx::ElideBehavior elide_behavior) {
  ad_name_view_->SetElideBehavior(elide_behavior);
}

void NotificationHeaderView::SetOverflowIndicator(int count) {
  summary_text_view_->SetText(l10n_util::GetStringFUTF16Int(
      IDS_MESSAGE_CENTER_LIST_NOTIFICATION_HEADER_OVERFLOW_INDICATOR, count));
  UpdateSummaryTextVisibility();
}

void NotificationHeaderView::SetAccentColor(SkColor color) {
  accent_color_ = color;
  summary_text_view_->SetEnabledColor(accent_color_);
  summary_text_divider_->SetEnabledColor(accent_color_);

  // If we are using the default app icon we should clear it so we refresh it
  // with the new accent color.
  if (using_default_ad_icon_)
    ClearAdIcon();
}

void NotificationHeaderView::SetBackgroundColor(SkColor color) {
  ad_name_view_->SetBackgroundColor(color);
  summary_text_divider_->SetBackgroundColor(color);
  summary_text_view_->SetBackgroundColor(color);
}

void NotificationHeaderView::SetSubpixelRenderingEnabled(bool enabled) {
  ad_name_view_->SetSubpixelRenderingEnabled(enabled);
  summary_text_divider_->SetSubpixelRenderingEnabled(enabled);
  summary_text_view_->SetSubpixelRenderingEnabled(enabled);
}

void NotificationHeaderView::SetAdIconVisible(bool visible) {
  ad_icon_view_->SetVisible(visible);
}

const std::u16string& NotificationHeaderView::ad_name_for_testing() const {
  return ad_name_view_->GetText();
}

const gfx::ImageSkia& NotificationHeaderView::ad_icon_for_testing() const {
  return ad_icon_view_->GetImage();
}

void NotificationHeaderView::UpdateSummaryTextVisibility() {
  const bool summary_visible = !summary_text_view_->GetText().empty();
  summary_text_divider_->SetVisible(summary_visible);
  summary_text_view_->SetVisible(summary_visible);

  // TODO(crbug.com/991492): this should not be necessary.
  detail_views_->InvalidateLayout();
}

}  // namespace brave_ads
