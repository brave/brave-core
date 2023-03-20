// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/tabs/brave_tab.h"

#include <algorithm>

#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/tabs/alert_indicator_button.h"
#include "chrome/browser/ui/views/tabs/tab_close_button.h"
#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/gfx/skia_paint_util.h"

namespace {

#define FILL_SHADOW_LAYER_FOR_DEBUG 0

class ShadowLayer : public ui::Layer, public ui::LayerDelegate {
 public:
  static const gfx::ShadowValues& GetShadowValues() {
    static const base::NoDestructor<gfx::ShadowValues> shadow(([]() {
      // Shadow matches `box-shadow: 0 1px 4px rgba(0, 0, 0, .0.07)`
      constexpr int kBlurRadius = 4;
      constexpr gfx::ShadowValue kShadow{
          /* offset= */ {0, 1},
          kBlurRadius * 2 /* correction value used in shadow_value.cc */,
          SkColorSetA(SK_ColorBLACK, 0.07 * 255)};
      return gfx::ShadowValues{kShadow};
    })());

    return *shadow;
  }

  ShadowLayer() { set_delegate(this); }
  ~ShadowLayer() override = default;

  // LayerDelegate:
  void OnPaintLayer(const ui::PaintContext& context) override {
    ui::PaintRecorder recorder(context, size());
    // Clear out the canvas so that transparency can be applied properly.
    recorder.canvas()->DrawColor(SK_ColorTRANSPARENT);
#if FILL_SHADOW_LAYER_FOR_DEBUG
    recorder.canvas()->DrawColor(gfx::kPlaceholderColor);
#endif

    cc::PaintFlags flags;
    flags.setStyle(cc::PaintFlags::kFill_Style);
    flags.setAntiAlias(true);
    flags.setColor(SK_ColorTRANSPARENT);
    flags.setLooper(gfx::CreateShadowDrawLooper(GetShadowValues()));

    gfx::Rect shadow_bounds(size());
    shadow_bounds.Inset(gfx::ShadowValue::GetBlurRegion(GetShadowValues()));
    constexpr int kCornerRadius = 4;
    recorder.canvas()->DrawRoundRect(shadow_bounds, kCornerRadius, flags);
  }

  void OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                  float new_device_scale_factor) override {}
};

}  // namespace

BraveTab::BraveTab(TabSlotController* controller) : Tab(controller) {}

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

  if (close_button_->GetVisible())
    selectable_width -= close_button_->width();

  return std::max(0, selectable_width);
}

void BraveTab::ActiveStateChanged() {
  Tab::ActiveStateChanged();
  // This should be called whenever acitve state changes
  // see comment on UpdateEnabledForMuteToggle();
  // https://github.com/brave/brave-browser/issues/23476/
  alert_indicator_button_->UpdateEnabledForMuteToggle();

  if (base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs)) {
    UpdateShadowForActiveTab();
  }
}

absl::optional<SkColor> BraveTab::GetGroupColor() const {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
    return Tab::GetGroupColor();

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
      showing_close_button_ = !can_enter_floating_mode && is_active;
      showing_icon_ = !showing_close_button_;
    }
  }
}

void BraveTab::Layout() {
  Tab::Layout();
  if (IsAtMinWidthForVerticalTabStrip()) {
    if (showing_close_button_) {
      close_button_->SetX(GetLocalBounds().CenterPoint().x() -
                          (close_button_->width() / 2));
      close_button_->SetButtonPadding({});
    }
  }
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
}

void BraveTab::OnBoundsChanged(const gfx::Rect& previous_bounds) {
  Tab::OnBoundsChanged(previous_bounds);

  if (shadow_layer_ && shadow_layer_->parent()) {
    LayoutShadowLayer();
  }
}

bool BraveTab::ShouldRenderAsNormalTab() const {
  if (IsAtMinWidthForVerticalTabStrip()) {
    // Returns false to hide title
    return false;
  }

  return Tab::ShouldRenderAsNormalTab();
}

bool BraveTab::IsAtMinWidthForVerticalTabStrip() const {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
    return false;

  return tabs::utils::ShouldShowVerticalTabs(controller()->GetBrowser()) &&
         width() <= tabs::kVerticalTabMinWidth;
}

void BraveTab::UpdateShadowForActiveTab() {
  DCHECK(base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs));
  if (IsActive() &&
      tabs::utils::ShouldShowVerticalTabs(controller()->GetBrowser())) {
    shadow_layer_ = CreateShadowLayer();
    AddLayerToBelowThis();
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
  auto layer = std::make_unique<ShadowLayer>();
  layer->SetFillsBoundsOpaquely(false);
  return layer;
}

void BraveTab::LayoutShadowLayer() {
  DCHECK(shadow_layer_);
  DCHECK(shadow_layer_->parent());
  DCHECK(layer());
  DCHECK_EQ(layer()->parent(), shadow_layer_->parent());

  auto shadow_bounds = layer()->bounds();

  // Expand shadow layer by the size of blur region
  shadow_bounds.Inset(
      -gfx::ShadowValue::GetBlurRegion(ShadowLayer::GetShadowValues()));

  // Exclude stroke thickness so that shadow could be more natural.
  shadow_bounds.Inset(controller_->GetStrokeThickness());

  shadow_layer_->SetBounds(shadow_bounds);
}

void BraveTab::AddLayerToBelowThis() {
  if (!layer()) {
    SetPaintToLayer();
    layer()->SetFillsBoundsOpaquely(false);
  }

  ReorderChildLayers(layer()->parent());
}
