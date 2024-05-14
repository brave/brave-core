/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/omnibox/brave_search_conversion_promotion_view.h"

#include <utility>

#include "base/logging.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/color/leo/colors.h"
#include "brave/browser/ui/views/omnibox/brave_omnibox_popup_view_views.h"
#include "brave/browser/ui/views/omnibox/brave_omnibox_result_view.h"
#include "brave/components/brave_search_conversion/p3a.h"
#include "brave/components/brave_search_conversion/utils.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/grit/brave_theme_resources.h"
#include "cc/paint/paint_flags.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/omnibox/omnibox_theme.h"
#include "chrome/browser/ui/views/chrome_typography.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "components/vector_icons/vector_icons.h"
#include "third_party/skia/include/core/SkPath.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/range/range.h"
#include "ui/gfx/skia_paint_util.h"
#include "ui/views/background.h"
#include "ui/views/cascading_property.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/image_button_factory.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/focus_ring.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"

using brave_search_conversion::ConversionType;
using brave_search_conversion::SetDismissed;
using brave_search_conversion::SetMaybeLater;

namespace {

constexpr int kBannerTypeMargin = 12;
// Use small margin because omnibox popup has its own bottom padding.
constexpr int kBannerTypeMarginBottom = 4;
constexpr int kBannerTypeRadius = 8;
constexpr int kMaxBannerDescLines = 5;
constexpr int kBannerTypeCloseButtonSize = 24;
constexpr int kBannerTypeCloseButtonMargin = 8;
constexpr int kBannerTypeContentsMargin = 13;

gfx::FontList GetFont(int font_size, gfx::Font::Weight weight) {
  gfx::FontList font_list;
  return font_list.DeriveWithSizeDelta(font_size - font_list.GetFontSize())
      .DeriveWithWeight(weight);
}

// Draw graphic over gradient background for banner type.
class HorizontalGradientBackground : public views::Background {
 public:
  using Background::Background;
  ~HorizontalGradientBackground() override = default;
  HorizontalGradientBackground(const HorizontalGradientBackground&) = delete;
  HorizontalGradientBackground& operator=(const HorizontalGradientBackground&) =
      delete;

  void Paint(gfx::Canvas* canvas, views::View* view) const override {
    // Fill with base color first.
    canvas->DrawColor(
        view->GetColorProvider()->GetColor(kColorOmniboxResultsBackground));

    SkColor from_color = gfx::kPlaceholderColor;
    SkColor to_color = gfx::kPlaceholderColor;
    if (const ui::ColorProvider* color_provider = view->GetColorProvider()) {
      from_color = color_provider->GetColor(
          kColorSearchConversionBannerTypeBackgroundGradientFrom);
      to_color = color_provider->GetColor(
          kColorSearchConversionBannerTypeBackgroundGradientTo);
    }

    // Gradient background from design.
    //  - linear-gradient(90deg, from_color, 19.6%, to_color, 100%).
    cc::PaintFlags flags;
    SkPoint points[2] = {SkPoint::Make(0, 0), SkPoint::Make(view->width(), 0)};
    SkColor4f colors[2] = {SkColor4f::FromColor(from_color),
                           SkColor4f::FromColor(to_color)};
    SkScalar positions[2] = {0.196f, 1.f};
    flags.setShader(cc::PaintShader::MakeLinearGradient(
        points, colors, positions, 2, SkTileMode::kClamp));
    flags.setStyle(cc::PaintFlags::kFill_Style);
    gfx::RectF bounds(view->GetContentsBounds());
    canvas->DrawRect(bounds, flags);

    auto& bundle = ui::ResourceBundle::GetSharedInstance();
    const auto* graphic = bundle.GetImageSkiaNamed(
        ui::NativeTheme::GetInstanceForNativeUi()->ShouldUseDarkColors()
            ? IDR_BRAVE_SEARCH_CONVERSION_BANNER_GRAPHIC_DARK
            : IDR_BRAVE_SEARCH_CONVERSION_BANNER_GRAPHIC);

    constexpr int kGraphicRightPadding = 27;
    const auto host_insets = view->GetInsets();
    const int graphic_x = view->size().width() - host_insets.right() -
                          kGraphicRightPadding - graphic->width();
    const int graphic_y =
        host_insets.top() + (bounds.height() - graphic->height()) / 2 + 1;
    canvas->DrawImageInt(*graphic, graphic_x, graphic_y);
  }
};

// For customizing label's font size.
class CustomMdTextButton : public views::MdTextButton {
  METADATA_HEADER(CustomMdTextButton, views::MdTextButton)
 public:
  using MdTextButton::MdTextButton;
  CustomMdTextButton(const CustomMdTextButton&) = delete;
  CustomMdTextButton& operator=(const CustomMdTextButton&) = delete;

  void SetFontSize(int size) {
    label()->SetFontList(GetFont(size, gfx::Font::Weight::SEMIBOLD));
  }
};

BEGIN_METADATA(CustomMdTextButton)
END_METADATA

class CloseButton : public views::ImageButton {
  METADATA_HEADER(CloseButton, views::ImageButton)
 public:
  explicit CloseButton(PressedCallback callback = PressedCallback())
      : ImageButton(std::move(callback)) {
    views::ConfigureVectorImageButton(this);
  }
  CloseButton(const CloseButton&) = delete;
  CloseButton& operator=(const CloseButton&) = delete;

  void GetAccessibleNodeData(ui::AXNodeData* node_data) override {
    // Although this appears visually as a button, expose as a list box option
    // so that it matches the other options within its list box container.
    node_data->role = ax::mojom::Role::kListBoxOption;
    node_data->SetName(brave_l10n::GetLocalizedResourceUTF16String(
        IDS_ACC_BRAVE_SEARCH_CONVERSION_DISMISS_BUTTON));
  }
};

BEGIN_METADATA(CloseButton)
END_METADATA

// For rounded.
class BannerTypeContainer : public views::View {
  METADATA_HEADER(BannerTypeContainer, views::View)
 public:
  using View::View;
  BannerTypeContainer(const BannerTypeContainer&) = delete;
  BannerTypeContainer& operator=(const BannerTypeContainer&) = delete;

  // views::View overrides:
  void OnPaint(gfx::Canvas* canvas) override {
    SkPath mask;
    mask.addRoundRect(gfx::RectToSkRect(GetLocalBounds()), kBannerTypeRadius,
                      kBannerTypeRadius);
    canvas->ClipPath(mask, true);

    View::OnPaint(canvas);
  }
};

BEGIN_METADATA(BannerTypeContainer)
END_METADATA

////////////////////////////////////////////////////////////////////////////////
// OmniboxResultSelectionIndicator

class BraveOmniboxResultSelectionIndicator : public views::View {
  METADATA_HEADER(BraveOmniboxResultSelectionIndicator, views::View)
 public:

  static constexpr int kStrokeThickness = 3;

  explicit BraveOmniboxResultSelectionIndicator(
      BraveSearchConversionPromotionView* parent_view)
      : parent_view_(parent_view) {
    SetPreferredSize(gfx::Size(kStrokeThickness, 0));
  }

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override {
    SkPath path = GetPath();
    cc::PaintFlags flags;
    flags.setAntiAlias(true);
    flags.setColor(color_);
    flags.setStyle(cc::PaintFlags::kFill_Style);
    canvas->DrawPath(path, flags);
  }

  // views::View:
  void OnThemeChanged() override {
    views::View::OnThemeChanged();

    color_ = views::GetCascadingAccentColor(parent_view_);
  }

 private:
  SkColor color_;

  // Pointer to the parent view.
  const raw_ptr<BraveSearchConversionPromotionView> parent_view_;

  // The focus bar is a straight vertical line with half-rounded endcaps. Since
  // this geometry is nontrivial to represent using primitives, it's instead
  // represented using a fill path. This matches the style and implementation
  // used in Tab Groups.
  SkPath GetPath() const {
    SkPath path;

    path.moveTo(0, 0);
    path.arcTo(kStrokeThickness, kStrokeThickness, 0, SkPath::kSmall_ArcSize,
               SkPathDirection::kCW, kStrokeThickness, kStrokeThickness);
    path.lineTo(kStrokeThickness, height() - kStrokeThickness);
    path.arcTo(kStrokeThickness, kStrokeThickness, 0, SkPath::kSmall_ArcSize,
               SkPathDirection::kCW, 0, height());
    path.close();

    return path;
  }
};

BEGIN_METADATA(BraveOmniboxResultSelectionIndicator)
END_METADATA

}  // namespace

BraveSearchConversionPromotionView::BraveSearchConversionPromotionView(
    BraveOmniboxResultView* result_view,
    PrefService* local_state,
    PrefService* profile_prefs)
    : result_view_(result_view),
      mouse_enter_exit_handler_(base::BindRepeating(
          &BraveSearchConversionPromotionView::UpdateHoverState,
          base::Unretained(this))),
      local_state_(local_state),
      profile_prefs_(profile_prefs) {
  SetLayoutManager(std::make_unique<views::FillLayout>());
  mouse_enter_exit_handler_.ObserveMouseEnterExitOn(this);
}

BraveSearchConversionPromotionView::~BraveSearchConversionPromotionView() =
    default;

void BraveSearchConversionPromotionView::SetTypeAndInput(
    brave_search_conversion::ConversionType type,
    const std::u16string& input) {
  // Don't need to update promotion ui if input is same.
  // Not sure why but upstream calls OmniboxResultView::SetMatch() multiple
  // times for same match. As this called again after selected,
  // |selected_| state is cleared if not early returned for same input
  // condition.
  if (input_ == input) {
    return;
  }

  DCHECK_NE(ConversionType::kNone, type);

  type_ = type;
  input_ = input;

  ConfigureForBannerType();
  UpdateState();

  brave_search_conversion::p3a::RecordPromoShown(local_state_, type);
}

void BraveSearchConversionPromotionView::OnSelectionStateChanged(
    bool selected) {
  selected_ = selected;
  UpdateState();
}

void BraveSearchConversionPromotionView::OpenMatch() {
  brave_search_conversion::p3a::RecordPromoTrigger(local_state_, type_);
  result_view_->OpenMatch();
}

void BraveSearchConversionPromotionView::Dismiss() {
  SetDismissed(profile_prefs_);
  result_view_->RefreshOmniboxResult();
}

void BraveSearchConversionPromotionView::MaybeLater() {
  SetMaybeLater(profile_prefs_);
  result_view_->RefreshOmniboxResult();
}

void BraveSearchConversionPromotionView::UpdateState() {
  if (!banner_type_container_) {
    return;
  }

  SkColor desc_color = gfx::kPlaceholderColor;
  SkColor border_color = gfx::kPlaceholderColor;
  const bool is_selected_or_hovered = selected_ || IsMouseHovered();
  if (const ui::ColorProvider* color_provider = GetColorProvider()) {
    desc_color =
        color_provider->GetColor(kColorSearchConversionBannerTypeDescText);
    const auto border_id =
        is_selected_or_hovered
            ? kColorSearchConversionBannerTypeBackgroundBorderHovered
            : kColorSearchConversionBannerTypeBackgroundBorder;
    border_color = color_provider->GetColor(border_id);
  }
  const int border_thickness = is_selected_or_hovered ? 2 : 1;
  banner_type_container_->SetBorder(views::CreateRoundedRectBorder(
      border_thickness, kBannerTypeRadius, border_color));
  banner_type_description_->SetEnabledColor(desc_color);

  gfx::Insets container_margin =
      gfx::Insets::TLBR(kBannerTypeMargin, kBannerTypeMargin,
                        kBannerTypeMarginBottom, kBannerTypeMargin);
  // By adjusting only container's margin when border thickness is changed,
  // container's overall bounds is not changed regardless of selected state.
  if (is_selected_or_hovered) {
    container_margin += gfx::Insets(-1);
  }
  SetBorder(views::CreateEmptyBorder(container_margin));

  SetBackground(views::CreateSolidBackground(
      GetColorProvider()->GetColor(kColorOmniboxResultsBackground)));
  banner_type_container_->SetBackground(
      std::make_unique<HorizontalGradientBackground>());
}

void BraveSearchConversionPromotionView::ConfigureForBannerType() {
  if (banner_type_container_) {
    return;
  }

  banner_type_container_ =
      AddChildView(std::make_unique<BannerTypeContainer>());

  banner_type_container_
      ->SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetCrossAxisAlignment(views::LayoutAlignment::kStart);

  // contents has title/label and buttons.
  auto* banner_contents =
      banner_type_container_->AddChildView(std::make_unique<views::View>());
  banner_contents->SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kVertical);
  banner_contents->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kUnbounded)
          .WithOrder(2));
  banner_contents->SetProperty(
      views::kMarginsKey, gfx::Insets::TLBR(kBannerTypeContentsMargin,
                                            kBannerTypeContentsMargin, 0, 0));

  // Setup banner contents.
  const std::u16string title_label =
      brave_l10n::GetLocalizedResourceUTF16String(
          GetBannerTypeTitleStringResourceId());
  views::Label::CustomFont title_font = {
      GetFont(16, gfx::Font::Weight::SEMIBOLD)};
  auto* banner_title = banner_contents->AddChildView(
      std::make_unique<views::Label>(title_label, title_font));
  banner_title->SetAutoColorReadabilityEnabled(false);
  banner_title->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  const std::u16string desc_label = brave_l10n::GetLocalizedResourceUTF16String(
      GetBannerTypeDescStringResourceId());
  views::Label::CustomFont desc_font = {GetFont(14, gfx::Font::Weight::NORMAL)};
  banner_type_description_ = banner_contents->AddChildView(
      std::make_unique<views::Label>(desc_label, desc_font));
  // Give right margin to not overlap with background image.
  banner_type_description_->SetProperty(views::kMarginsKey,
                                        gfx::Insets::TLBR(4, 0, 0, 70));
  banner_type_description_->SetMultiLine(true);
  banner_type_description_->SetMaxLines(kMaxBannerDescLines);
  banner_type_description_->SetAutoColorReadabilityEnabled(false);
  banner_type_description_->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  auto* button_row =
      banner_contents->AddChildView(std::make_unique<views::View>());
  button_row->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kPreferred));
  button_row->SetProperty(views::kMarginsKey, gfx::Insets::TLBR(10, 0, 13, 0));
  button_row->SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal);
  auto* try_button = button_row->AddChildView(
      std::make_unique<CustomMdTextButton>(views::Button::PressedCallback(
          base::BindRepeating(&BraveSearchConversionPromotionView::OpenMatch,
                              base::Unretained(this)))));
  try_button->SetKind(views::MdTextButton::Kind::kPrimary);
  try_button->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kPreferred));
  try_button->SetText(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_SEARCH_CONVERSION_TRY_BUTTON_LABEL));
  try_button->SetFontSize(13);

  auto* maybe_later_button = button_row->AddChildView(
      std::make_unique<CustomMdTextButton>(views::Button::PressedCallback(
          base::BindRepeating(&BraveSearchConversionPromotionView::MaybeLater,
                              base::Unretained(this)))));
  maybe_later_button->SetKind(views::MdTextButton::Kind::kQuaternary);
  maybe_later_button->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kPreferred));
  maybe_later_button->SetText(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_SEARCH_CONVERSION_MAYBE_LATER_BUTTON_LABEL));
  maybe_later_button->SetFontSize(13);
  maybe_later_button->SetProperty(views::kMarginsKey,
                                  gfx::Insets::TLBR(0, 13, 0, 0));
  maybe_later_button->SetTooltipText(
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_BRAVE_SEARCH_CONVERSION_MAYBE_LATER_BUTTON_TOOLTIP));

  auto* close_button = banner_type_container_->AddChildView(
      std::make_unique<CloseButton>(views::Button::PressedCallback(
          base::BindRepeating(&BraveSearchConversionPromotionView::Dismiss,
                              base::Unretained(this)))));
  views::SetImageFromVectorIconWithColor(
      close_button, kLeoCloseIcon, kBannerTypeCloseButtonSize,
      GetCloseButtonColor(), gfx::kPlaceholderColor);
  views::InstallCircleHighlightPathGenerator(close_button);
  views::FocusRing::Install(close_button);
  close_button->SetProperty(views::kMarginsKey,
                            gfx::Insets(kBannerTypeCloseButtonMargin));
  close_button->SetTooltipText(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_SEARCH_CONVERSION_CLOSE_BUTTON_TOOLTIP));
}

SkColor BraveSearchConversionPromotionView::GetCloseButtonColor() const {
  SkColor button_color = gfx::kPlaceholderColor;
  if (const ui::ColorProvider* color_provider = GetColorProvider()) {
    button_color = color_provider->GetColor(kColorSearchConversionCloseButton);
  }

  return button_color;
}

gfx::Size BraveSearchConversionPromotionView::CalculatePreferredSize() const {
  // Ask preferred size + margin for banner.
  auto size = banner_type_container_->GetPreferredSize();
  const auto margin = GetInsets();
  size.Enlarge(0, margin.height());

  // When it's called, omnibox popup's bound is not determined.
  // To give proper size, we need final width because this promotion view's
  // height could be changed for width because description is multi line label.
  // To get final width, we use location bar's width.
  const auto final_width_for_description =
      result_view_->GetPopupView()->GetLocationBarViewWidth() -
      GetOverallHorizontalMarginAroundDescription();

  // Only add increased size as |size| already includes description's default
  // height.
  size.Enlarge(0, banner_type_description_->GetHeightForWidth(
                      final_width_for_description) -
                      banner_type_description_->GetLineHeight());

  return size;
}

void BraveSearchConversionPromotionView::OnThemeChanged() {
  View::OnThemeChanged();

  UpdateState();
}

int BraveSearchConversionPromotionView::
    GetOverallHorizontalMarginAroundDescription() const {
  CHECK(banner_type_container_);

  int horizontal_margin =
      GetInsets().width() + banner_type_container_->GetInsets().width();
  horizontal_margin +=
      banner_type_description_->GetProperty(views::kMarginsKey)->width();
  horizontal_margin += kBannerTypeContentsMargin;
  horizontal_margin +=
      (kBannerTypeCloseButtonSize + kBannerTypeCloseButtonMargin * 2);
  return horizontal_margin;
}

void BraveSearchConversionPromotionView::UpdateHoverState() {
  UpdateState();
}

int BraveSearchConversionPromotionView::GetBannerTypeTitleStringResourceId() {
  switch (type_) {
    case ConversionType::kBannerTypeA:
      NOTREACHED_NORETURN();
    case ConversionType::kBannerTypeB:
      return IDS_BRAVE_SEARCH_CONVERSION_PROMOTION_BANNER_TYPE_B_TITLE;
    case ConversionType::kBannerTypeC:
      return IDS_BRAVE_SEARCH_CONVERSION_PROMOTION_BANNER_TYPE_C_TITLE;
    case ConversionType::kBannerTypeD:
      return IDS_BRAVE_SEARCH_CONVERSION_PROMOTION_BANNER_TYPE_D_TITLE;
    default:
      break;
  }

  NOTREACHED_NORETURN();
}

int BraveSearchConversionPromotionView::GetBannerTypeDescStringResourceId() {
  switch (type_) {
    case ConversionType::kBannerTypeA:
      NOTREACHED_NORETURN();
    case ConversionType::kBannerTypeB:
      return IDS_BRAVE_SEARCH_CONVERSION_PROMOTION_BANNER_TYPE_B_DESC;
    case ConversionType::kBannerTypeC:
      return IDS_BRAVE_SEARCH_CONVERSION_PROMOTION_BANNER_TYPE_C_DESC;
    case ConversionType::kBannerTypeD:
      return IDS_BRAVE_SEARCH_CONVERSION_PROMOTION_BANNER_TYPE_D_DESC;
    default:
      break;
  }

  NOTREACHED_NORETURN();
}

BEGIN_METADATA(BraveSearchConversionPromotionView)
END_METADATA
