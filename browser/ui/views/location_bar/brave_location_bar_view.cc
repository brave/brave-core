/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"

#include <memory>

#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/themes/brave_theme_service.h"
#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/omnibox/omnibox_theme.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "components/version_info/channel.h"
#include "ui/views/controls/highlight_path_generator.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/ui/views/location_bar/onion_location_view.h"
#endif
#if BUILDFLAG(IPFS_ENABLED)
#include "brave/browser/ui/views/location_bar/ipfs_location_view.h"
#endif

namespace {

class BraveLocationBarViewFocusRingHighlightPathGenerator
    : public views::HighlightPathGenerator {
 public:
  BraveLocationBarViewFocusRingHighlightPathGenerator() = default;

  // HighlightPathGenerator
  SkPath GetHighlightPath(const views::View* view) override {
    return static_cast<const BraveLocationBarView*>(view)
        ->GetFocusRingHighlightPath();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveLocationBarViewFocusRingHighlightPathGenerator);
};

base::Optional<SkColor> GetFocusRingColor(Profile* profile) {
  constexpr SkColor kPrivateFocusRingColor = SkColorSetRGB(0xC6, 0xB3, 0xFF);
  constexpr SkColor kTorPrivateFocusRingColor = SkColorSetRGB(0xCF, 0xAB, 0xE2);
  if (brave::IsRegularProfile(profile) || profile->IsGuestSession()) {
    // Don't update color.
    return base::nullopt;
  }
  if (profile->IsTor())
    return kTorPrivateFocusRingColor;

  // Private window.
  return kPrivateFocusRingColor;
}

}  // namespace

void BraveLocationBarView::Init() {
  // base method calls Update and Layout
  LocationBarView::Init();
  // Change focus ring highlight path
  if (focus_ring_) {
    focus_ring_->SetPathGenerator(
        std::make_unique<
            BraveLocationBarViewFocusRingHighlightPathGenerator>());
    if (const auto color = GetFocusRingColor(profile()))
      focus_ring_->SetColor(color.value());
  }
#if BUILDFLAG(ENABLE_TOR)
  onion_location_view_ = new OnionLocationView(browser_->profile());
  AddChildView(onion_location_view_);
#endif
#if BUILDFLAG(IPFS_ENABLED)
  ipfs_location_view_ = new IPFSLocationView(browser_->profile());
  AddChildView(ipfs_location_view_);
#endif

  // brave action buttons
  brave_actions_ = new BraveActionsContainer(browser_, profile());
  brave_actions_->Init();
  AddChildView(brave_actions_);
  // Call Update again to cause a Layout
  Update(nullptr);

  // Stop slide animation for all content settings views icon.
  for (auto* content_setting_view : content_setting_views_)
    content_setting_view->disable_animation();
}

void BraveLocationBarView::Update(content::WebContents* contents) {
  // base Init calls update before our Init is run, so our children
  // may not be initialized yet
  if (brave_actions_) {
    brave_actions_->Update();
  }
#if BUILDFLAG(ENABLE_TOR)
  if (onion_location_view_)
    onion_location_view_->Update(contents);
#endif
#if BUILDFLAG(IPFS_ENABLED)
  if (ipfs_location_view_)
    ipfs_location_view_->Update(contents);
#endif

  LocationBarView::Update(contents);
}

void BraveLocationBarView::OnChanged() {
  if (brave_actions_) {
    // Do not show actions whilst omnibar is open or url is being edited
    const bool should_hide =
        ShouldHidePageActionIcons() && !omnibox_view_->GetText().empty();
    brave_actions_->SetShouldHide(should_hide);
  }
#if BUILDFLAG(ENABLE_TOR)
  if (onion_location_view_)
    onion_location_view_->Update(
        browser_->tab_strip_model()->GetActiveWebContents());
#endif
#if BUILDFLAG(IPFS_ENABLED)
  if (ipfs_location_view_)
    ipfs_location_view_->Update(
        browser_->tab_strip_model()->GetActiveWebContents());
#endif

  // OnChanged calls Layout
  LocationBarView::OnChanged();
}

std::vector<views::View*> BraveLocationBarView::GetTrailingViews() {
  std::vector<views::View*> views;
#if BUILDFLAG(ENABLE_TOR)
  if (onion_location_view_)
    views.push_back(onion_location_view_);
#endif
#if BUILDFLAG(IPFS_ENABLED)
  if (ipfs_location_view_)
    views.push_back(ipfs_location_view_);
#endif

  if (brave_actions_)
    views.push_back(brave_actions_);

  return views;
}

gfx::Size BraveLocationBarView::CalculatePreferredSize() const {
  gfx::Size min_size = LocationBarView::CalculatePreferredSize();
  if (brave_actions_ && brave_actions_->GetVisible()) {
    const int brave_actions_min = brave_actions_->GetMinimumSize().width();
    const int extra_width = brave_actions_min +
                              GetLayoutConstant(LOCATION_BAR_ELEMENT_PADDING);
    min_size.Enlarge(extra_width, 0);
  }
#if BUILDFLAG(ENABLE_TOR)
  if (onion_location_view_ && onion_location_view_->GetVisible()) {
    const int extra_width = GetLayoutConstant(LOCATION_BAR_ELEMENT_PADDING) +
        onion_location_view_->GetMinimumSize().width();
    min_size.Enlarge(extra_width, 0);
  }
#endif
#if BUILDFLAG(IPFS_ENABLED)
  if (ipfs_location_view_ && ipfs_location_view_->GetVisible()) {
    const int extra_width = GetLayoutConstant(LOCATION_BAR_ELEMENT_PADDING) +
                            ipfs_location_view_->GetMinimumSize().width();
    min_size.Enlarge(extra_width, 0);
  }
#endif

  return min_size;
}

void BraveLocationBarView::OnThemeChanged() {
  LocationBarView::OnThemeChanged();

  if (!IsInitialized())
    return;

  Update(nullptr);
  RefreshBackground();
}

void BraveLocationBarView::ChildPreferredSizeChanged(views::View* child) {
  LocationBarView::ChildPreferredSizeChanged(child);

  if (child != brave_actions_)
    return;

  Layout();
}

int BraveLocationBarView::GetBorderRadius() const {
  return ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
      views::EMPHASIS_HIGH, size());
}

SkPath BraveLocationBarView::GetFocusRingHighlightPath() const {
  const SkScalar radius = GetBorderRadius();
  return SkPath().addRoundRect(gfx::RectToSkRect(GetLocalBounds()),
                               radius, radius);
}

ContentSettingImageView*
BraveLocationBarView::GetContentSettingsImageViewForTesting(size_t idx) {
  DCHECK(idx < content_setting_views_.size());
  return content_setting_views_[idx];
}
