/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/web_discovery_infobar_content_view.h"

#include <limits>
#include <utility>
#include <vector>

#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/infobars/custom_styled_label.h"
#include "brave/browser/web_discovery/web_discovery_infobar_delegate.h"
#include "brave/components/constants/url_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"

namespace {

using ButtonState = views::Button::ButtonState;

constexpr int kWideLayoutHorizontalPadding = 67;
constexpr int kNarrowLayoutHorizontalPadding = 10;

void OpenMoreInfoPage() {
  if (auto* browser = BrowserList::GetInstance()->GetLastActive())
    ShowSingletonTab(browser, GURL(kWebDiscoveryLearnMoreUrl));
}

// Customize min size as half size of preferred.
// Re-calculated preferred size as it doesn't give proper
// size when enlarged.
class InfoBarStyledLabel : public CustomStyledLabel {
  METADATA_HEADER(InfoBarStyledLabel, CustomStyledLabel)

 public:
  using CustomStyledLabel::CustomStyledLabel;
  InfoBarStyledLabel(const InfoBarStyledLabel&) = delete;
  InfoBarStyledLabel& operator=(const InfoBarStyledLabel&) = delete;
  ~InfoBarStyledLabel() override = default;

 private:
  // CustomStyledLabel overrides:
  gfx::Size GetMinimumSize() const override {
    const auto pref_size = GetPreferredSize();
    return gfx::Size(pref_size.width() * 0.55, pref_size.height());
  }

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override {
    // Reset message label's width so that it can calculate preferred size
    // ignoring the current size. This will allow the label to grow bigger than
    // it is.
    // https://github.com/chromium/chromium/blob/366e028e485ffd5de42a2f5a898a5eda1edbbc08/ui/views/controls/styled_label.cc#L228
    return GetLayoutSizeInfoForWidth(std::numeric_limits<int>::max())
        .total_size;
  }

  void OnBoundsChanged(const gfx::Rect& previous_bounds) override {
    CustomStyledLabel::OnBoundsChanged(previous_bounds);

    auto height = GetHeightForWidth(width());
    SetSize({width(), height});
    SetPosition({x(), (parent()->height() - height) / 2});
  }
};

BEGIN_METADATA(InfoBarStyledLabel)
END_METADATA

// TODO(simonhong): Use leo MdTextButton when it's stabilized.
class OkButton : public views::LabelButton {
  METADATA_HEADER(OkButton, views::LabelButton)
 public:
  explicit OkButton(PressedCallback callback, const std::u16string& text)
      : LabelButton(std::move(callback), text) {
    SetHorizontalAlignment(gfx::ALIGN_CENTER);
    SetEnabledTextColors(SK_ColorWHITE);
    SetTextColor(ButtonState::STATE_DISABLED, SK_ColorWHITE);
  }

  OkButton(const OkButton&) = delete;
  OkButton& operator=(const OkButton&) = delete;

  void UpdateBackgroundColor() override {
    static constexpr auto kBgColor =
        std::to_array<std::array<const SkColor, ButtonState::STATE_COUNT>>(
            {{
                 // Light theme.
                 SkColorSetRGB(0x4E, 0x32, 0xEE),  // normal
                 SkColorSetRGB(0x32, 0x2F, 0xB4),  // hover
                 SkColorSetRGB(0x4E, 0x32, 0xEE),  // focused
                 SkColorSetRGB(0xAC, 0xAF, 0xBB)   // disabled
             },
             {
                 // Dark theme.
                 SkColorSetRGB(0x4E, 0x32, 0xEE),  // normal
                 SkColorSetRGB(0x87, 0x84, 0xF4),  // hover
                 SkColorSetRGB(0x4E, 0x32, 0xEE),  // focused
                 SkColorSetRGB(0x58, 0x5C, 0x6D),  // disabled
             }});

    const int theme =
        ui::NativeTheme::GetInstanceForNativeUi()->ShouldUseDarkColors();
    SetBackground(CreateBackgroundFromPainter(
        views::Painter::CreateRoundRectWith1PxBorderPainter(
            UNSAFE_TODO(kBgColor[theme][GetVisualState()]), SK_ColorTRANSPARENT,
            100)));
  }

  void OnThemeChanged() override {
    LabelButton::OnThemeChanged();
    UpdateBackgroundColor();
  }
};

BEGIN_METADATA(OkButton)
END_METADATA

// Subclassed for font setting.
class NoThanksButton : public views::LabelButton {
  METADATA_HEADER(NoThanksButton, views::LabelButton)
 public:
  using views::LabelButton::LabelButton;
  NoThanksButton(const NoThanksButton&) = delete;
  NoThanksButton& operator=(const NoThanksButton&) = delete;
  ~NoThanksButton() override = default;

  void SetFontList(const gfx::FontList& font_list) {
    label()->SetFontList(font_list);
  }
};

BEGIN_METADATA(NoThanksButton)
END_METADATA

// Use image as background.
class WebDiscoveryInfoBarContentViewBackground : public views::Background {
 public:
  using Background::Background;
  ~WebDiscoveryInfoBarContentViewBackground() override = default;
  WebDiscoveryInfoBarContentViewBackground(
      const WebDiscoveryInfoBarContentViewBackground&) = delete;
  WebDiscoveryInfoBarContentViewBackground& operator=(
      const WebDiscoveryInfoBarContentViewBackground&) = delete;

  void Paint(gfx::Canvas* canvas, views::View* view) const override {
    ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
    canvas->DrawImageInt(
        gfx::ImageSkiaOperations::CreateResizedImage(
            *rb.GetImageSkiaNamed(
                ui::NativeTheme::GetInstanceForNativeUi()->ShouldUseDarkColors()
                    ? IDR_BRAVE_WEB_DISCOVERY_INFOBAR_BG_GRAPHIC_DARK
                    : IDR_BRAVE_WEB_DISCOVERY_INFOBAR_BG_GRAPHIC),
            skia::ImageOperations::RESIZE_BEST, view->size()),
        0, 0);
  }
};

}  // namespace

WebDiscoveryInfoBarContentView::WebDiscoveryInfoBarContentView(
    WebDiscoveryInfoBarDelegate* delegate)
    : delegate_(delegate) {
  SetBackground(std::make_unique<WebDiscoveryInfoBarContentViewBackground>());
}

WebDiscoveryInfoBarContentView::~WebDiscoveryInfoBarContentView() = default;

void WebDiscoveryInfoBarContentView::OnPaint(gfx::Canvas* canvas) {
  SkPath mask;
  mask.addRoundRect(gfx::RectToSkRect(GetContentsBounds()), 16, 16);
  canvas->ClipPath(mask, true);
  View::OnPaint(canvas);
}

void WebDiscoveryInfoBarContentView::OnThemeChanged() {
  View::OnThemeChanged();

  // Re-initialze whenever theme changed because it's complicate to change the
  // StyledLabel's text color.
  InitChildren();
  SwitchChildLayout();
}

void WebDiscoveryInfoBarContentView::AddedToWidget() {
  // When active tab is changed to others, infobar is also hidden.
  // And it's added to widget again when brave search tab is activated.
  // When activated, infobar should be switched to proper layout again.
  SwitchChildLayout();
}

void WebDiscoveryInfoBarContentView::OnBoundsChanged(
    const gfx::Rect& previous_bounds) {
  SwitchChildLayout();
}

void WebDiscoveryInfoBarContentView::SwitchChildLayout() {
  // Not initialized yet.
  if (wide_layout_min_width_ == 0 || narrow_layout_preferred_width_ == 0)
    return;

  // TODO(simonhong): This is workaround to prevent re-layout from narrow layout
  // to wide layout at startup as we have a regression that StyledLabel doesn't
  // do proper layout when its width is growing. With this workaround, we can
  // show wdp infobar w/o wrong layout.
  if (width() == 0) {
    return;
  }

  // There are three layout.
  // - Wide layout with wide border
  // - Wide layout with narrow border
  // - Narrow layout with narrow border
  const bool needs_wide_layout = (width() > narrow_layout_preferred_width_);
  const bool needs_small_padding_with_wide_layout =
      (narrow_layout_preferred_width_ < width() &&
       width() <= wide_layout_min_width_);
  if (needs_wide_layout) {
    SetBorder(views::CreateEmptyBorder(
        gfx::Insets::VH(10, needs_small_padding_with_wide_layout
                                ? 10
                                : kWideLayoutHorizontalPadding)));
  } else {
    SetBorder(views::CreateEmptyBorder(
        gfx::Insets::VH(8, kNarrowLayoutHorizontalPadding)));
  }

  wide_layout_container_->SetVisible(needs_wide_layout);
  wide_layout_container_->SetBoundsRect(GetContentsBounds());
  narrow_layout_container_->SetVisible(!needs_wide_layout);
  narrow_layout_container_->SetBoundsRect(GetContentsBounds());

  constexpr int kWideLayoutHeight = 84;
  constexpr int kNarrowLayoutHeight = 151;
  SetPreferredSize(gfx::Size(
      width(), needs_wide_layout ? kWideLayoutHeight : kNarrowLayoutHeight));
}

void WebDiscoveryInfoBarContentView::InitChildren() {
  RemoveAllChildViews();

  InitChildrenForWideLayout();
  InitChildrenForNarrowLayout();
  wide_layout_container_->SetVisible(false);
  narrow_layout_container_->SetVisible(false);
}

// Locate all children in one horizontal layout.
void WebDiscoveryInfoBarContentView::InitChildrenForWideLayout() {
  const int infobar_content_height = GetContentsBounds().height();

  wide_layout_container_ = AddChildView(std::make_unique<views::View>());
  wide_layout_container_
      ->SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetCrossAxisAlignment(views::LayoutAlignment::kCenter);

  wide_layout_container_->AddChildView(
      GetSpacer({40, infobar_content_height}, 2));
  wide_layout_container_->AddChildView(
      GetSpacer({197, infobar_content_height}, 4,
                views::MinimumFlexSizeRule::kScaleToZero));
  wide_layout_container_->AddChildView(GetIcon(1));
  wide_layout_container_->AddChildView(
      GetSpacer({24, infobar_content_height}, 2));
  wide_layout_container_->AddChildView(GetMessage(3, 24));
  wide_layout_container_->AddChildView(
      GetSpacer({40, infobar_content_height}, 4,
                views::MinimumFlexSizeRule::kScaleToZero));
  wide_layout_container_->AddChildView(GetOkButton({101, 38}, 3));
  wide_layout_container_->AddChildView(
      GetSpacer({}, 4, views::MinimumFlexSizeRule::kScaleToZero,
                views::MaximumFlexSizeRule::kUnbounded));
  wide_layout_container_->AddChildView(GetNoThanksButton(3));
  wide_layout_container_->AddChildView(GetCloseButton());
  wide_layout_container_->AddChildView(
      GetSpacer({40, infobar_content_height}, 2));

  wide_layout_min_width_ = wide_layout_container_->GetMinimumSize().width() +
                           kWideLayoutHorizontalPadding * 2;
}

void WebDiscoveryInfoBarContentView::InitChildrenForNarrowLayout() {
  // |narrow_layout_container_| has two parts - contents and
  // close button horizontally.
  narrow_layout_container_ = AddChildView(std::make_unique<views::View>());
  narrow_layout_container_
      ->SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetCrossAxisAlignment(views::LayoutAlignment::kStart);

  // |center_contents| has two parts vertically - message and button rows.
  auto* contents =
      narrow_layout_container_->AddChildView(std::make_unique<views::View>());
  contents->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToMinimum,
                               views::MaximumFlexSizeRule::kUnbounded)
          .WithOrder(2));
  contents->SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kVertical)
      .SetCrossAxisAlignment(views::LayoutAlignment::kStart);

  contents->AddChildView(GetSpacer({10, 18}, 1));

  auto* message_row = contents->AddChildView(std::make_unique<views::View>());
  message_row->SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal);
  message_row->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToMinimum,
                               views::MaximumFlexSizeRule::kUnbounded)
          .WithOrder(2));

  message_row->AddChildView(GetSpacer({22, 20}, 1));
  message_row->AddChildView(GetIcon(1));
  message_row->AddChildView(GetSpacer({18, 20}, 1));
  message_row->AddChildView(GetMessage(3, 22));

  // Space between message and buttons rows.
  contents->AddChildView(GetSpacer({10, 14}, 1));

  auto* buttons_row = contents->AddChildView(std::make_unique<views::View>());
  buttons_row->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kUnbounded)
          .WithOrder(3));

  buttons_row->SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetCrossAxisAlignment(views::LayoutAlignment::kCenter);

  buttons_row->AddChildView(GetSpacer({60, 20}, 1));
  buttons_row->AddChildView(GetNoThanksButton(1));
  buttons_row->AddChildView(GetSpacer({40, 38}, 2,
                                      views::MinimumFlexSizeRule::kScaleToZero,
                                      views::MaximumFlexSizeRule::kUnbounded));
  buttons_row->AddChildView(GetOkButton({196, 38}, 1));
  contents->AddChildView(GetSpacer({10, 18}, 1));

  auto* close = narrow_layout_container_->AddChildView(GetCloseButton());
  close->SetProperty(views::kMarginsKey, gfx::Insets::TLBR(8, 20, 105, 10));

  narrow_layout_preferred_width_ =
      narrow_layout_container_->GetPreferredSize().width() +
      kNarrowLayoutHorizontalPadding * 2;
}

std::unique_ptr<views::View> WebDiscoveryInfoBarContentView::GetMessage(
    int order,
    int line_height) {
  const std::u16string brave_search_text =
      l10n_util::GetStringUTF16(IDS_WEB_DISCOVERY_INFOBAR_MESSAGE_BRAVE_SEARCH);
  const std::u16string more_info_text =
      l10n_util::GetStringUTF16(IDS_WEB_DISCOVERY_INFOBAR_MESSAGE_MORE_INFO);
  std::vector<size_t> offsets;
  const std::u16string message_text =
      l10n_util::GetStringFUTF16(IDS_WEB_DISCOVERY_INFOBAR_MESSAGE,
                                 brave_search_text, more_info_text, &offsets);
  auto message_label = std::make_unique<InfoBarStyledLabel>();
  message_label->SetLineHeight(line_height);
  message_label->SetText(message_text);

  views::StyledLabel::RangeStyleInfo brave_search_style;
  brave_search_style.custom_font = gfx::FontList("Poppins, Semi-Bold 14px");
  brave_search_style.override_color =
      GetColorProvider()->GetColor(kColorWebDiscoveryInfoBarMessage);
  message_label->AddStyleRange(
      gfx::Range(offsets[0], offsets[0] + brave_search_text.length()),
      brave_search_style);

  views::StyledLabel::RangeStyleInfo more_info_style =
      views::StyledLabel::RangeStyleInfo::CreateForLink(
          base::BindRepeating(OpenMoreInfoPage));
  more_info_style.custom_font = gfx::FontList("Poppins, Normal 14px");
  more_info_style.override_color =
      GetColorProvider()->GetColor(kColorWebDiscoveryInfoBarLink);
  message_label->AddStyleRange(
      gfx::Range(offsets[1], offsets[1] + more_info_text.length()),
      more_info_style);

  views::StyledLabel::RangeStyleInfo default_style;
  default_style.custom_font = gfx::FontList("Poppins, Normal 14px");
  default_style.override_color =
      GetColorProvider()->GetColor(kColorWebDiscoveryInfoBarMessage);
  message_label->AddStyleRange(gfx::Range(0, offsets[0]), default_style);
  message_label->AddStyleRange(
      gfx::Range(offsets[0] + brave_search_text.length(),
                 message_text.length() - more_info_text.length()),
      default_style);
  message_label->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToMinimum,
                               views::MaximumFlexSizeRule::kPreferred)
          .WithOrder(order));
  return message_label;
}

std::unique_ptr<views::View> WebDiscoveryInfoBarContentView::GetIcon(
    int order) {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  auto icon = std::make_unique<views::ImageView>(ui::ImageModel::FromImageSkia(
      *rb.GetImageSkiaNamed(IDR_BRAVE_WEB_DISCOVERY_INFOBAR_ICON)));
  icon->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kPreferred)
          .WithOrder(order));
  return icon;
}

std::unique_ptr<views::View> WebDiscoveryInfoBarContentView::GetNoThanksButton(
    int order) {
  auto no_thanks = std::make_unique<NoThanksButton>(
      base::BindRepeating(&WebDiscoveryInfoBarContentView::Dismiss,
                          base::Unretained(this)),
      l10n_util::GetStringUTF16(IDS_WEB_DISCOVERY_INFOBAR_NO_THANKS_LABEL));
  no_thanks->SetFontList(gfx::FontList("Poppins, Semi-Bold 12px"));
  no_thanks->SetEnabledTextColors(
      GetColorProvider()->GetColor(kColorWebDiscoveryInfoBarNoThanks));
  no_thanks->SetProperty(views::kMarginsKey, gfx::Insets::TLBR(0, 0, 0, 16));
  no_thanks->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kPreferred)
          .WithOrder(order));
  return no_thanks;
}

std::unique_ptr<views::View> WebDiscoveryInfoBarContentView::GetOkButton(
    const gfx::Size& size,
    int order) {
  auto ok_button = std::make_unique<OkButton>(
      base::BindRepeating(&WebDiscoveryInfoBarContentView::EnableWebDiscovery,
                          base::Unretained(this)),
      l10n_util::GetStringUTF16(IDS_WEB_DISCOVERY_INFOBAR_OK_BUTTON_LABEL));
  ok_button->SetPreferredSize(size);
  ok_button->SetProperty(views::kMarginsKey, gfx::Insets::TLBR(0, 16, 0, 16));
  ok_button->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kPreferred)
          .WithOrder(order));
  return ok_button;
}

std::unique_ptr<views::View> WebDiscoveryInfoBarContentView::GetCloseButton() {
  auto close_button = std::make_unique<views::ImageButton>(base::BindRepeating(
      &WebDiscoveryInfoBarContentView::CloseInfoBar, base::Unretained(this)));
  close_button->SetImageModel(
      views::Button::STATE_NORMAL,
      ui::ImageModel::FromVectorIcon(
          kWebDiscoveryInfobarCloseButtonIcon,
          GetColorProvider()->GetColor(kColorWebDiscoveryInfoBarClose)));
  close_button->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kPreferred));
  return close_button;
}

std::unique_ptr<views::View> WebDiscoveryInfoBarContentView::GetSpacer(
    const gfx::Size& size,
    int order,
    views::MinimumFlexSizeRule min_rule,
    views::MaximumFlexSizeRule max_rule) {
  auto spacer = std::make_unique<views::View>();
  spacer->SetPreferredSize(size);
  spacer->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(min_rule, max_rule).WithOrder(order));
  return spacer;
}

void WebDiscoveryInfoBarContentView::EnableWebDiscovery() {
  delegate_->EnableWebDiscovery();
}

void WebDiscoveryInfoBarContentView::Dismiss() {
  delegate_->Close(true);
}

void WebDiscoveryInfoBarContentView::CloseInfoBar() {
  delegate_->Close(false);
}

BEGIN_METADATA(WebDiscoveryInfoBarContentView)
END_METADATA
