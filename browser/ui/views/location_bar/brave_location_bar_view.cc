/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"

#include <memory>
#include <utility>

#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/themes/brave_theme_service.h"
#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/omnibox/omnibox_theme.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/grit/chromium_strings.h"
#include "components/grit/brave_components_strings.h"
#include "components/omnibox/browser/omnibox_edit_model.h"
#include "components/version_info/channel.h"
#include "content/public/browser/navigation_entry.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/highlight_path_generator.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/ui/views/location_bar/onion_location_view.h"
#endif
#if BUILDFLAG(ENABLE_IPFS)
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/browser/ui/views/location_bar/ipfs_location_view.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_utils.h"
#endif

namespace {

class BraveLocationBarViewFocusRingHighlightPathGenerator
    : public views::HighlightPathGenerator {
 public:
  BraveLocationBarViewFocusRingHighlightPathGenerator() = default;
  BraveLocationBarViewFocusRingHighlightPathGenerator(
      const BraveLocationBarViewFocusRingHighlightPathGenerator&) = delete;
  BraveLocationBarViewFocusRingHighlightPathGenerator& operator=(
      const BraveLocationBarViewFocusRingHighlightPathGenerator&) = delete;

  // HighlightPathGenerator
  SkPath GetHighlightPath(const views::View* view) override {
    return static_cast<const BraveLocationBarView*>(view)
        ->GetFocusRingHighlightPath();
  }
};

absl::optional<SkColor> GetFocusRingColor(Profile* profile) {
  constexpr SkColor kPrivateFocusRingColor = SkColorSetRGB(0xC6, 0xB3, 0xFF);
  constexpr SkColor kTorPrivateFocusRingColor = SkColorSetRGB(0xCF, 0xAB, 0xE2);
  if (brave::IsRegularProfile(profile) || profile->IsGuestSession()) {
    // Don't update color.
    return absl::nullopt;
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
  views::FocusRing* focus_ring = views::FocusRing::Get(this);
  if (focus_ring) {
    focus_ring->SetPathGenerator(
        std::make_unique<
            BraveLocationBarViewFocusRingHighlightPathGenerator>());
    if (const auto color = GetFocusRingColor(profile()))
      focus_ring->SetColor(color.value());
  }
#if BUILDFLAG(ENABLE_TOR)
  onion_location_view_ = new OnionLocationView(browser_->profile());
  AddChildView(onion_location_view_);
#endif
#if BUILDFLAG(ENABLE_IPFS)
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

bool BraveLocationBarView::ShouldShowIPFSLocationView() const {
#if BUILDFLAG(ENABLE_IPFS)
  const GURL& url = GetLocationBarModel()->GetURL();
  if (!ipfs::IpfsServiceFactory::IsIpfsEnabled(profile_) ||
      !ipfs::IsIPFSScheme(url) ||
      !ipfs::IsLocalGatewayConfigured(profile_->GetPrefs()))
    return false;

  return true;
#else
  return false;
#endif
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
#if BUILDFLAG(ENABLE_IPFS)
  if (ipfs_location_view_)
    ipfs_location_view_->Update(contents);
#endif

  LocationBarView::Update(contents);

  if (!ShouldShowIPFSLocationView())
    return;
  // Secure display text for a page was set by chromium.
  // We do not want to override this.
  if (!GetLocationBarModel()->GetSecureDisplayText().empty())
    return;
  auto badge_text = l10n_util::GetStringUTF16(IDS_IPFS_BADGE_TITLE);
  location_icon_view()->SetLabel(badge_text);
}

ui::ImageModel BraveLocationBarView::GetLocationIcon(
    LocationIconView::Delegate::IconFetchedCallback on_icon_fetched) const {
  if (!ShouldShowIPFSLocationView() ||
      !omnibox_view_->model()->ShouldShowCurrentPageIcon())
    return LocationBarView::GetLocationIcon(std::move(on_icon_fetched));

  auto& bundle = ui::ResourceBundle::GetSharedInstance();
  const auto& ipfs_logo = *bundle.GetImageSkiaNamed(IDR_BRAVE_IPFS_LOGO);
  return ui::ImageModel::FromImageSkia(ipfs_logo);
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
#if BUILDFLAG(ENABLE_IPFS)
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
#if BUILDFLAG(ENABLE_IPFS)
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
#if BUILDFLAG(ENABLE_IPFS)
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
      views::Emphasis::kHigh, size());
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
