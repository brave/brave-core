// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/tabs/brave_tab.h"

#include <algorithm>

#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/tabs/alert_indicator_button.h"
#include "chrome/browser/ui/views/tabs/tab_close_button.h"
#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/gfx/favicon_size.h"
#include "ui/gfx/skia_paint_util.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/controls/label.h"
#include "ui/views/view_class_properties.h"

namespace {

#define FILL_SHADOW_LAYER_FOR_DEBUG 0

class ShadowLayer : public ui::Layer, public ui::LayerDelegate {
 public:
  static constexpr int kBlurRadius = 4;

  static const gfx::ShadowValues& GetShadowValues() {
    static const base::NoDestructor<gfx::ShadowValues> shadow(([]() {
#if FILL_SHADOW_LAYER_FOR_DEBUG
      constexpr SkColor kShadowColor = SkColorSetA(SK_ColorCYAN, 0.07 * 255);
#else
      constexpr SkColor kShadowColor = SkColorSetA(SK_ColorBLACK, 0.07 * 255);
#endif

      // Shadow matches `box-shadow: 0 1px 4px rgba(0, 0, 0, .0.07)`
      constexpr gfx::ShadowValue kShadow{
          /* offset= */ {0, 1},
          kBlurRadius * 2 /* correction value used in shadow_value.cc */,
          kShadowColor};
      return gfx::ShadowValues{kShadow};
    })());

    return *shadow;
  }

  static gfx::Insets GetBlurRegionInsets() {
    return gfx::ShadowValue::GetBlurRegion(ShadowLayer::GetShadowValues());
  }

  static gfx::Rect GetShadowLayerBounds(const gfx::Rect& anchor_bounds) {
    // Enlarge shadow layer bigger than the |anchor_bounds| so that we can
    // draw the full range of blur.
    gfx::Rect shadow_layer_bounds = anchor_bounds;
    shadow_layer_bounds.Inset(-GetBlurRegionInsets());
    return shadow_layer_bounds;
  }

  explicit ShadowLayer(Tab* tab) : tab_(tab) {
    CHECK(tab);
    set_delegate(this);
  }
  ~ShadowLayer() override = default;

  // LayerDelegate:
  void OnPaintLayer(const ui::PaintContext& context) override {
    ui::PaintRecorder recorder(context, size());
    // Clear out the canvas so that transparency can be applied properly.
#if FILL_SHADOW_LAYER_FOR_DEBUG
    recorder.canvas()->DrawColor(gfx::kPlaceholderColor);
#else
    recorder.canvas()->DrawColor(SK_ColorTRANSPARENT);
#endif

    cc::PaintFlags flags;
    flags.setStyle(cc::PaintFlags::kFill_Style);
    flags.setAntiAlias(true);
    flags.setColor(SK_ColorTRANSPARENT);
    flags.setLooper(gfx::CreateShadowDrawLooper(GetShadowValues()));

    // The looper will draw around the area. So we should inset the layer
    // bounds.
    gfx::Rect shadow_bounds(size());
    shadow_bounds.Inset(GetBlurRegionInsets());
    const int kCornerRadius =
        (tab_->data().pinned ? tabs::kPinnedTabBorderRadius
                             : tabs::kUnpinnedTabBorderRadius);
    recorder.canvas()->DrawRoundRect(shadow_bounds, kCornerRadius, flags);
  }

  void OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                  float new_device_scale_factor) override {}

 private:
  raw_ptr<Tab> tab_;
};

}  // namespace

BraveTab::BraveTab(TabSlotController* controller)
    : Tab(controller),
      normal_font_(views::Label::GetDefaultFontList()),
      active_tab_font_(
          normal_font_.DeriveWithWeight(gfx::Font::Weight::MEDIUM)) {
  title_->SetFontList(IsActive() ? active_tab_font_ : normal_font_);
}

BraveTab::~BraveTab() = default;

std::u16string BraveTab::GetTooltipText(const gfx::Point& p) const {
  auto* browser = controller_->GetBrowser();
  if (browser &&
      brave_tabs::AreTooltipsEnabled(browser->profile()->GetPrefs())) {
    return Tab::GetTooltipText(data_.title,
                               GetAlertStateToShow(data_.alert_state));
  }
  return Tab::GetTooltipText(p);
}

int BraveTab::GetWidthOfLargestSelectableRegion() const {
  // Assume the entire region except the area that alert indicator/close buttons
  // occupied is available for click-to-select.
  // If neither are visible, the entire tab region is available.
  int selectable_width = width();
  if (alert_indicator_button_->GetVisible()) {
    selectable_width -= alert_indicator_button_->width();
  }

  if (close_button_->GetVisible()) {
    selectable_width -= close_button_->width();
  }

  return std::max(0, selectable_width);
}

void BraveTab::ActiveStateChanged() {
  Tab::ActiveStateChanged();

  title_->SetFontList(IsActive() ? active_tab_font_ : normal_font_);

  // This should be called whenever the active state changes
  // see comment on UpdateEnabledForMuteToggle();
  // https://github.com/brave/brave-browser/issues/23476/
  alert_indicator_button_->UpdateEnabledForMuteToggle();

  if (base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs)) {
    UpdateShadowForActiveTab();
  }
}

absl::optional<SkColor> BraveTab::GetGroupColor() const {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs)) {
    return Tab::GetGroupColor();
  }

  // Hide tab border with group color as it doesn't go well with vertical tabs.
  if (tabs::utils::ShouldShowVerticalTabs(controller()->GetBrowser())) {
    return {};
  }

  return Tab::GetGroupColor();
}

void BraveTab::UpdateIconVisibility() {
  Tab::UpdateIconVisibility();
  if (IsAtMinWidthForVerticalTabStrip()) {
    if (data().pinned) {
      center_icon_ = true;
      showing_icon_ = !showing_alert_indicator_;
      showing_close_button_ = false;
    } else {
      center_icon_ = true;

      const bool is_active = IsActive();
      const bool can_enter_floating_mode =
          tabs::utils::IsFloatingVerticalTabsEnabled(
              controller()->GetBrowser());
      // When floating mode enabled, we don't show close button as the tab strip
      // will be expanded as soon as mouse hovers onto the tab.
      showing_close_button_ =
          !showing_alert_indicator_ && !can_enter_floating_mode && is_active;
      showing_icon_ = !showing_alert_indicator_ && !showing_close_button_;
    }
  }
}

void BraveTab::ViewHierarchyChanged(
    const views::ViewHierarchyChangedDetails& details) {
  if (details.child != this) {
    return;
  }

  if (details.is_add && shadow_layer_) {
    AddLayerToBelowThis();
  }
}

void BraveTab::OnLayerBoundsChanged(const gfx::Rect& old_bounds,
                                    ui::PropertyChangeReason reason) {
  Tab::OnLayerBoundsChanged(old_bounds, reason);

  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs)) {
    return;
  }

  if (shadow_layer_ && shadow_layer_->parent() &&
      shadow_layer_->parent() == layer()->parent()) {
    LayoutShadowLayer();
  }
}

void BraveTab::Layout() {
  Tab::Layout();
  if (IsAtMinWidthForVerticalTabStrip()) {
    if (showing_close_button_) {
      close_button_->SetX(GetLocalBounds().CenterPoint().x() -
                          (close_button_->width() / 2));

      // In order to reset ink drop bounds based on new padding.
      auto* ink_drop = views::InkDrop::Get(close_button_)->GetInkDrop();
      DCHECK(ink_drop);
      ink_drop->HostSizeChanged(close_button_->size());
    }
  }
}

gfx::Insets BraveTab::GetInsets() const {
  // Supplement extra left side padding.
  // Upstream gives extra padding to balance with right side padding space but
  // it's gone when tab doesn't have sufficient available width. In our case,
  // As we have more narrow left & right padding than upstream, icon seems stick
  // to left side when extra padding is not used.
  // We only need to do that when |extra_padding_before_content_| is false.
  int extra_left_padding = 0;

  // Add extra padding if upstream tab doesn't have it.
  if (!extra_padding_before_content_) {
    extra_left_padding = kExtraLeftPadding;
  }

  auto insets = Tab::GetInsets();
  insets.set_left(insets.left() + extra_left_padding);
  return insets;
}

void BraveTab::ReorderChildLayers(ui::Layer* parent_layer) {
  Tab::ReorderChildLayers(parent_layer);

  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs)) {
    return;
  }

  if (!layer() || layer()->parent() != parent_layer || !shadow_layer_) {
    return;
  }

  if (shadow_layer_->parent() != layer()->parent()) {
    if (shadow_layer_->parent()) {
      shadow_layer_->parent()->Remove(shadow_layer_.get());
    }
    layer()->parent()->Add(shadow_layer_.get());
  }

  DCHECK_EQ(shadow_layer_->parent(), layer()->parent());
  layer()->parent()->StackBelow(shadow_layer_.get(), layer());

  LayoutShadowLayer();
}

void BraveTab::MaybeAdjustLeftForPinnedTab(gfx::Rect* bounds,
                                           int visual_width) const {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs) ||
      !tabs::utils::ShouldShowVerticalTabs(controller()->GetBrowser())) {
    Tab::MaybeAdjustLeftForPinnedTab(bounds, visual_width);
    return;
  }

  // We keep favicon on fixed position so that it won't move left and right
  // during animation.
  bounds->set_x((tabs::kVerticalTabMinWidth - gfx::kFaviconSize) / 2);
}

bool BraveTab::ShouldRenderAsNormalTab() const {
  if (IsAtMinWidthForVerticalTabStrip()) {
    // Returns false to hide title
    return false;
  }

  return Tab::ShouldRenderAsNormalTab();
}

bool BraveTab::IsAtMinWidthForVerticalTabStrip() const {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs)) {
    return false;
  }

  return tabs::utils::ShouldShowVerticalTabs(controller()->GetBrowser()) &&
         width() <= tabs::kVerticalTabMinWidth;
}

void BraveTab::UpdateShadowForActiveTab() {
  CHECK(base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs));
  if (IsActive() &&
      tabs::utils::ShouldShowVerticalTabs(controller()->GetBrowser())) {
    shadow_layer_ = CreateShadowLayer();
    AddLayerToBelowThis();
    LayoutShadowLayer();
  } else if (shadow_layer_) {
    if (layer()) {
      layer()->parent()->Remove(shadow_layer_.get());
    }
    shadow_layer_.reset();
    if (layer()) {
      DestroyLayer();
    }
  }
}

std::unique_ptr<ui::Layer> BraveTab::CreateShadowLayer() {
  auto layer = std::make_unique<ShadowLayer>(this);
  layer->SetFillsBoundsOpaquely(false);
  return layer;
}

void BraveTab::LayoutShadowLayer() {
  CHECK(shadow_layer_);
  CHECK(shadow_layer_->parent());
  CHECK(layer());
  DCHECK_EQ(layer()->parent(), shadow_layer_->parent());
  shadow_layer_->SetBounds(
      ShadowLayer::GetShadowLayerBounds(layer()->bounds()));
}

void BraveTab::AddLayerToBelowThis() {
  if (!layer()) {
    SetPaintToLayer();
    layer()->SetFillsBoundsOpaquely(false);
  }

  ReorderChildLayers(layer()->parent());
}
