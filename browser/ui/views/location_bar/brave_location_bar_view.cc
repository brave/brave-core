/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"

#include <memory>
#include <utility>

#include "base/containers/contains.h"
#include "base/feature_list.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/themes/brave_theme_service.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"
#include "brave/browser/ui/views/location_bar/brave_news_location_view.h"
#include "brave/browser/ui/views/playlist/playlist_action_icon_view.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/components/commander/common/buildflags/buildflags.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/omnibox/omnibox_theme.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_view_views.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_controller.h"
#include "chrome/grit/chromium_strings.h"
#include "components/grit/brave_components_strings.h"
#include "components/omnibox/browser/omnibox_edit_model.h"
#include "components/version_info/channel.h"
#include "content/public/browser/navigation_entry.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/view_utils.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/ui/views/location_bar/onion_location_view.h"
#endif
#if BUILDFLAG(ENABLE_IPFS)
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/browser/ui/views/location_bar/ipfs_location_view.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_utils.h"
#endif

#if BUILDFLAG(ENABLE_COMMANDER)
#include "brave/browser/ui/commander/commander_service_factory.h"
#include "brave/components/commander/browser/commander_frontend_delegate.h"
#include "brave/components/commander/common/features.h"
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

absl::optional<BraveColorIds> GetFocusRingColor(Profile* profile) {
  if (brave::IsRegularProfile(profile) || profile->IsGuestSession()) {
    // Don't update color.
    return absl::nullopt;
  }
  // Private or Tor window - use color mixer.
  return kColorLocationBarFocusRing;
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
    if (const auto color_id = GetFocusRingColor(profile())) {
      focus_ring->SetColorId(color_id.value());
    }
  }

  if (!browser_->profile()->IsOffTheRecord()) {
    brave_news_location_view_ =
        AddChildView(std::make_unique<BraveNewsLocationView>(
            browser_->profile(), this, this));
    brave_news_location_view_->SetVisible(false);
    views::InkDrop::Get(brave_news_location_view_)
        ->SetVisibleOpacity(GetPageActionInkDropVisibleOpacity());
  }
#if BUILDFLAG(ENABLE_TOR)
  onion_location_view_ =
      AddChildView(std::make_unique<OnionLocationView>(browser_->profile()));
#endif
#if BUILDFLAG(ENABLE_IPFS)
  ipfs_location_view_ =
      AddChildView(std::make_unique<IPFSLocationView>(browser_->profile()));
#endif

  // brave action buttons
  brave_actions_ = AddChildView(
      std::make_unique<BraveActionsContainer>(browser_, profile()));
  brave_actions_->Init();
  // Call Update again to cause a Layout
  Update(nullptr);

  // Stop slide animation for all content settings views icon.
  for (auto* content_setting_view : content_setting_views_) {
    content_setting_view->disable_animation();
  }
}

bool BraveLocationBarView::ShouldShowIPFSLocationView() const {
#if BUILDFLAG(ENABLE_IPFS)
  const GURL& url = GetLocationBarModel()->GetURL();
  if (!ipfs::IpfsServiceFactory::IsIpfsEnabled(profile_) ||
      !ipfs::IsIPFSScheme(url) ||
      !ipfs::IsLocalGatewayConfigured(profile_->GetPrefs())) {
    return false;
  }

  return true;
#else
  return false;
#endif
}

void BraveLocationBarView::ShowPlaylistBubble() {
  if (auto* playlist_action_icon_view = GetPlaylistActionIconView()) {
    playlist_action_icon_view->ShowPlaylistBubble();
  }
}

PlaylistActionIconView* BraveLocationBarView::GetPlaylistActionIconView() {
  auto* playlist_action_icon_view =
      page_action_icon_controller()->GetPlaylistActionIconView();
  if (!playlist_action_icon_view) {
    return nullptr;
  }

  return views::AsViewClass<PlaylistActionIconView>(playlist_action_icon_view);
}

void BraveLocationBarView::Update(content::WebContents* contents) {
  // base Init calls update before our Init is run, so our children
  // may not be initialized yet
  if (brave_actions_) {
    brave_actions_->Update();
  }

  auto show_page_actions = !ShouldHidePageActionIcons();
#if BUILDFLAG(ENABLE_TOR)
  if (onion_location_view_) {
    onion_location_view_->Update(contents, show_page_actions);
  }
#endif
#if BUILDFLAG(ENABLE_IPFS)
  if (ipfs_location_view_) {
    ipfs_location_view_->Update(contents, show_page_actions);
  }
#endif

  if (brave_news_location_view_) {
    brave_news_location_view_->Update();
  }

  LocationBarView::Update(contents);

  if (!ShouldShowIPFSLocationView()) {
    return;
  }
  // Secure display text for a page was set by chromium.
  // We do not want to override this.
  if (!GetLocationBarModel()->GetSecureDisplayText().empty()) {
    return;
  }
  auto badge_text =
      brave_l10n::GetLocalizedResourceUTF16String(IDS_IPFS_BADGE_TITLE);
  location_icon_view()->SetLabel(badge_text);
}

ui::ImageModel BraveLocationBarView::GetLocationIcon(
    LocationIconView::Delegate::IconFetchedCallback on_icon_fetched) const {
  if (!ShouldShowIPFSLocationView() ||
      !omnibox_view_->model()->ShouldShowCurrentPageIcon()) {
    return LocationBarView::GetLocationIcon(std::move(on_icon_fetched));
  }

  auto& bundle = ui::ResourceBundle::GetSharedInstance();
  const auto& ipfs_logo = *bundle.GetImageSkiaNamed(IDR_BRAVE_IPFS_LOGO);
  return ui::ImageModel::FromImageSkia(ipfs_logo);
}

void BraveLocationBarView::OnOmniboxBlurred() {
#if BUILDFLAG(ENABLE_COMMANDER)
  if (base::FeatureList::IsEnabled(features::kBraveCommander)) {
    if (auto* commander_service =
            commander::CommanderServiceFactory::GetForBrowserContext(
                profile_)) {
      commander_service->Hide();
    }
  }
#endif
  LocationBarView::OnOmniboxBlurred();
}

void BraveLocationBarView::OnChanged() {
  auto hide_page_actions = ShouldHidePageActionIcons();
  if (brave_actions_) {
    brave_actions_->SetShouldHide(hide_page_actions);
  }
#if BUILDFLAG(ENABLE_TOR)
  if (onion_location_view_) {
    onion_location_view_->Update(
        browser_->tab_strip_model()->GetActiveWebContents(),
        !hide_page_actions);
  }
#endif
#if BUILDFLAG(ENABLE_IPFS)
  if (ipfs_location_view_) {
    ipfs_location_view_->Update(
        browser_->tab_strip_model()->GetActiveWebContents(),
        !hide_page_actions);
  }
#endif

  if (brave_news_location_view_) {
    brave_news_location_view_->Update();
  }

  // OnChanged calls Layout
  LocationBarView::OnChanged();
}

std::vector<views::View*> BraveLocationBarView::GetTrailingViews() {
  std::vector<views::View*> views;
  if (brave_news_location_view_) {
    views.push_back(brave_news_location_view_);
  }
#if BUILDFLAG(ENABLE_TOR)
  if (onion_location_view_) {
    views.push_back(onion_location_view_);
  }
#endif
#if BUILDFLAG(ENABLE_IPFS)
  if (ipfs_location_view_) {
    views.push_back(ipfs_location_view_);
  }
#endif

  if (brave_actions_) {
    views.push_back(brave_actions_);
  }

  return views;
}

gfx::Size BraveLocationBarView::CalculatePreferredSize() const {
  gfx::Size min_size = LocationBarView::CalculatePreferredSize();
  if (brave_actions_ && brave_actions_->GetVisible()) {
    const int brave_actions_min = brave_actions_->GetMinimumSize().width();
    const int extra_width =
        brave_actions_min + GetLayoutConstant(LOCATION_BAR_ELEMENT_PADDING);
    min_size.Enlarge(extra_width, 0);
  }
  if (brave_news_location_view_ && brave_news_location_view_->GetVisible()) {
    const int extra_width = GetLayoutConstant(LOCATION_BAR_ELEMENT_PADDING) +
                            brave_news_location_view_->GetMinimumSize().width();
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

  if (!IsInitialized()) {
    return;
  }

  Update(nullptr);
  RefreshBackground();
}

void BraveLocationBarView::ChildVisibilityChanged(views::View* child) {
  LocationBarView::ChildVisibilityChanged(child);
  // Normally, PageActionIcons are in a container which is always visible, only
  // the size changes when an icon is shown or hidden. The LocationBarView
  // does not listen to ChildVisibilityChanged events so we must make we Layout
  // and re-caculate trailing decorator positions when a child changes.
  if (base::Contains(GetTrailingViews(), child)) {
    Layout();
    SchedulePaint();
  }
}

int BraveLocationBarView::GetBorderRadius() const {
  return ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
      views::Emphasis::kHigh, size());
}

SkPath BraveLocationBarView::GetFocusRingHighlightPath() const {
  const SkScalar radius = GetBorderRadius();
  return SkPath().addRoundRect(gfx::RectToSkRect(GetLocalBounds()), radius,
                               radius);
}

ContentSettingImageView*
BraveLocationBarView::GetContentSettingsImageViewForTesting(size_t idx) {
  DCHECK(idx < content_setting_views_.size());
  return content_setting_views_[idx];
}
