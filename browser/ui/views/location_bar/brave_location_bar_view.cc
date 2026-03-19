/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/feature_list.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/themes/brave_theme_service.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/page_info/features.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"
#include "brave/browser/ui/views/location_bar/brave_search_conversion/promotion_button_controller.h"
#include "brave/browser/ui/views/location_bar/brave_search_conversion/promotion_button_view.h"
#include "brave/browser/ui/views/location_bar/brave_shields_page_info_controller.h"
#include "brave/browser/ui/views/playlist/playlist_action_icon_view.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/components/brave_news/common/buildflags/buildflags.h"
#include "brave/components/commander/common/buildflags/buildflags.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/omnibox/omnibox_controller.h"
#include "chrome/browser/ui/omnibox/omnibox_edit_model.h"
#include "chrome/browser/ui/omnibox/omnibox_theme.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_view_views.h"
#include "chrome/browser/ui/views/page_action/page_action_container_view.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_container.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_controller.h"
#include "chrome/grit/branded_strings.h"
#include "components/grit/brave_components_strings.h"
#include "components/version_info/channel.h"
#include "content/public/browser/navigation_entry.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/compositor/layer.h"
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

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
#include "brave/browser/ui/views/brave_news/brave_news_action_icon_view.h"
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

std::optional<BraveColorIds> GetFocusRingColor(Profile* profile) {
  if (profile->IsGuestSession()) {
    // Don't update color.
    return std::nullopt;
  }

  return kColorLocationBarFocusRing;
}

}  // namespace

BraveLocationBarView::BraveLocationBarView(Browser* browser,
                                           Profile* profile,
                                           CommandUpdater* command_updater,
                                           Delegate* delegate,
                                           bool is_popup_mode)
    : LocationBarView(browser,
                      profile,
                      command_updater,
                      delegate,
                      is_popup_mode) {}

BraveLocationBarView::~BraveLocationBarView() = default;

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

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
  if (!browser_->profile()->IsOffTheRecord()) {
    brave_news_action_icon_view_ =
        AddChildView(std::make_unique<BraveNewsActionIconView>(
            browser_->profile(), this, this));
    brave_news_action_icon_view_->SetVisible(false);
    views::InkDrop::Get(brave_news_action_icon_view_)
        ->SetVisibleOpacity(GetPageActionInkDropVisibleOpacity());
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_NEWS)
#if BUILDFLAG(ENABLE_TOR)
  onion_location_view_ = AddChildView(
      std::make_unique<OnionLocationView>(browser_->profile(), this, this));
#endif

  if (PromotionButtonController::PromotionEnabled(profile()->GetPrefs())) {
    promotion_button_ = AddChildView(std::make_unique<PromotionButtonView>());
    promotion_controller_ = std::make_unique<PromotionButtonController>(
        promotion_button_, omnibox_view_, browser());
  }

  if (page_info::features::IsShowBraveShieldsInPageInfoEnabled()) {
    shields_page_info_controller_ =
        std::make_unique<BraveShieldsPageInfoController>(location_icon_view());
  }

  // brave action buttons
  brave_actions_ = AddChildView(
      std::make_unique<BraveActionsContainer>(browser_, profile()));
  brave_actions_->Init();
  // Call Update again to cause a Layout
  Update(nullptr);

  // Stop slide animation for all content settings views icon.
  for (ContentSettingImageView* content_setting_view : content_setting_views_) {
    content_setting_view->disable_animation();
  }
}

void BraveLocationBarView::ShowPlaylistBubble(
    playlist::PlaylistBubblesController::BubbleType type) {
  if (auto* playlist_action_icon_view = GetPlaylistActionIconView()) {
    playlist_action_icon_view->ShowPlaylistBubble(type);
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

#if BUILDFLAG(ENABLE_TOR)
  if (onion_location_view_) {
    onion_location_view_->Update();
  }
#endif

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
  if (brave_news_action_icon_view_) {
    brave_news_action_icon_view_->Update();
  }
#endif

  if (shields_page_info_controller_) {
    shields_page_info_controller_->UpdateWebContents(contents);
  }

  LocationBarView::Update(contents);
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

void BraveLocationBarView::Layout(PassKey) {
  if (ignore_layout_) {
    return;
  }

  // Hide Brave trailing views when there isn't enough space for both them and
  // the omnibox minimum width. The omnibox should be the last view to shrink.
  // Guard against recursion: SetVisible() on a child triggers
  // ChildVisibilityChanged(), which calls DeprecatedLayoutImmediately() and
  // would re-enter Layout(). We suppress that re-entry here.
  if (!in_brave_space_update_) {
    in_brave_space_update_ = true;
    UpdateBraveViewsSpaceVisibility();
    in_brave_space_update_ = false;
  }

  LayoutSuperclass<LocationBarView>(this);
}

void BraveLocationBarView::OnVisibleBoundsChanged() {
  if (ignore_layout_) {
    return;
  }

  LocationBarView::OnVisibleBoundsChanged();
}

void BraveLocationBarView::OnChanged() {
  auto hide_page_actions = ShouldHidePageActionIcons();
  if (brave_actions_) {
    brave_actions_->SetShouldHide(hide_page_actions);
  }
#if BUILDFLAG(ENABLE_TOR)
  if (onion_location_view_) {
    onion_location_view_->Update();
  }
#endif

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
  if (brave_news_action_icon_view_) {
    brave_news_action_icon_view_->Update();
  }
#endif

  if (promotion_controller_) {
    const bool show_button =
        promotion_controller_->ShouldShowSearchPromotionButton() &&
        !ShouldChipOverrideLocationIcon() && !ShouldShowKeywordBubble();
    promotion_controller_->Show(show_button);
  }

  // OnChanged calls Layout
  LocationBarView::OnChanged();
}

std::vector<views::View*> BraveLocationBarView::GetRightMostTrailingViews() {
  std::vector<views::View*> views;
#if BUILDFLAG(ENABLE_BRAVE_NEWS)
  if (brave_news_action_icon_view_) {
    views.push_back(brave_news_action_icon_view_);
  }
#endif

  if (brave_actions_) {
    views.push_back(brave_actions_);
  }

  return views;
}

std::vector<views::View*> BraveLocationBarView::GetLeftMostTrailingViews() {
  std::vector<views::View*> views;
#if BUILDFLAG(ENABLE_TOR)
  if (onion_location_view_) {
    views.push_back(onion_location_view_);
  }
#endif
  return views;
}

views::View* BraveLocationBarView::GetSearchPromotionButton() const {
  return promotion_button_;
}

void BraveLocationBarView::RefreshBackground() {
  LocationBarView::RefreshBackground();

  if (shadow_) {
    const bool show_shadow =
        IsMouseHovered() &&
        !GetOmniboxController()->edit_model()->is_caret_visible();
    shadow_->SetVisible(show_shadow);
    return;
  }
}

gfx::Size BraveLocationBarView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size min_size = LocationBarView::CalculatePreferredSize(available_size);
  if (brave_actions_ && brave_actions_->GetVisible()) {
    const int brave_actions_min = brave_actions_->GetMinimumSize().width();
    const int extra_width =
        brave_actions_min +
        GetLayoutConstant(LayoutConstant::kLocationBarElementPadding);
    min_size.Enlarge(extra_width, 0);
  }
#if BUILDFLAG(ENABLE_BRAVE_NEWS)
  if (brave_news_action_icon_view_ &&
      brave_news_action_icon_view_->GetVisible()) {
    const int extra_width =
        GetLayoutConstant(LayoutConstant::kLocationBarElementPadding) +
        brave_news_action_icon_view_->GetMinimumSize().width();
    min_size.Enlarge(extra_width, 0);
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_NEWS)
#if BUILDFLAG(ENABLE_TOR)
  if (onion_location_view_ && onion_location_view_->GetVisible()) {
    const int extra_width =
        GetLayoutConstant(LayoutConstant::kLocationBarElementPadding) +
        onion_location_view_->GetMinimumSize().width();
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
  SetupShadow();
}

void BraveLocationBarView::AddedToWidget() {
  SetupShadow();
}

void BraveLocationBarView::UpdateBraveViewsSpaceVisibility() {
  CHECK(in_brave_space_update_);

  // If news/tor views were previously hidden for space, restore their natural
  // visibility so we can re-evaluate whether they fit at the current bar width.
  // brave_actions_ is self-managed via UpdateLayoutForAvailableWidth and does
  // not need an explicit restore step here.
  if (brave_views_collapsed_for_space_) {
    brave_views_collapsed_for_space_ = false;
#if BUILDFLAG(ENABLE_BRAVE_NEWS)
    if (brave_news_action_icon_view_) {
      brave_news_action_icon_view_->Update();
    }
#endif
#if BUILDFLAG(ENABLE_TOR)
    if (onion_location_view_) {
      onion_location_view_->Update();
    }
#endif
  }

  // If upstream trailing views were previously hidden because the URL would
  // have been truncated, restore them so we can re-evaluate at the current
  // bar width and URL length.
  if (upstream_trailing_collapsed_for_space_) {
    upstream_trailing_collapsed_for_space_ = false;
    if (page_action_icon_container_was_visible_) {
      page_action_icon_container_was_visible_ = false;
      page_action_icon_container_->SetVisible(true);
    }
    if (page_action_container_was_visible_) {
      page_action_container_was_visible_ = false;
      page_action_container()->SetVisible(true);
    }
  }

  if (!IsInitialized()) {
    return;
  }

  const int bar_width = GetLocalBounds().width();
  if (bar_width <= 0) {
    return;
  }

  // Minimum bar width needed to show the omnibox at its minimum size, not
  // counting any trailing decorations. brave_actions_ has higher priority than
  // upstream trailing views (translate, share, etc.), so we intentionally do
  // not reserve their space here — they are evicted later if necessary.
  const int elem_pad =
      GetLayoutConstant(LayoutConstant::kLocationBarElementPadding);
  const int inset_width = GetInsets().width();
  const int leading_min = GetMinimumLeadingWidth();
  const int omnibox_min = omnibox_view_->GetMinimumSize().width();
  const int trailing_upstream_min = GetMinimumTrailingWidth();
  const int pure_upstream_min =
      inset_width + std::max(omnibox_min, leading_min + elem_pad);

  // Space available for all optional trailing views (Brave + upstream
  // trailing).
  const int avail_brave = bar_width - pure_upstream_min;

  // brave_actions_ has the highest priority among Brave trailing views because
  // it is registered as the rightmost trailing decoration (closest to the bar's
  // right edge). Give it all the available space first; it will show Shields
  // first, then Rewards, and hide itself entirely when nothing fits.
  if (brave_actions_) {
    brave_actions_->UpdateLayoutForAvailableWidth(std::max(0, avail_brave));
  }

  // Width actually consumed by brave_actions_ after the update above.
  const int brave_actions_used =
      (brave_actions_ && brave_actions_->GetVisible())
          ? brave_actions_->GetPreferredSize().width() + elem_pad
          : 0;

  // Hide upstream trailing views (translate, share, etc.) when brave_actions_
  // and the omnibox minimum already claim all available space. brave_actions_
  // has higher priority, so upstream trailing is evicted before brave_actions_
  // shrinks further.
  if (!upstream_trailing_collapsed_for_space_ && trailing_upstream_min > 0 &&
      bar_width <
          pure_upstream_min + trailing_upstream_min + brave_actions_used) {
    upstream_trailing_collapsed_for_space_ = true;
    auto* pic = page_action_icon_container_.get();
    page_action_icon_container_was_visible_ = pic && pic->GetVisible();
    if (page_action_icon_container_was_visible_) {
      pic->SetVisible(false);
    }
    auto* pac = page_action_container();
    page_action_container_was_visible_ = pac && pac->GetVisible();
    if (page_action_container_was_visible_) {
      pac->SetVisible(false);
    }
  }

  // Remaining space available for news/tor after brave_actions_ and upstream
  // trailing take their respective shares.
  const int upstream_trailing_used =
      upstream_trailing_collapsed_for_space_ ? 0 : trailing_upstream_min;
  const int avail_for_news_tor =
      avail_brave - brave_actions_used - upstream_trailing_used;

  // Hide each news/tor view independently based on available space. Views are
  // evaluated in priority order (highest first): Onion/Tor is a security
  // indicator and has priority over Brave News. Each view consumes space from
  // |avail_remaining| only when it fits; the next view gets whatever is left.
  int avail_remaining = avail_for_news_tor;

#if BUILDFLAG(ENABLE_TOR)
  if (onion_location_view_ && onion_location_view_->GetVisible()) {
    const int tor_width =
        onion_location_view_->GetMinimumSize().width() + elem_pad;
    if (avail_remaining < tor_width) {
      brave_views_collapsed_for_space_ = true;
      onion_location_view_->SetVisible(false);
    } else {
      avail_remaining -= tor_width;
    }
  }
#endif

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
  if (brave_news_action_icon_view_ &&
      brave_news_action_icon_view_->GetVisible()) {
    const int news_width =
        brave_news_action_icon_view_->GetMinimumSize().width() + elem_pad;
    if (avail_remaining < news_width) {
      brave_views_collapsed_for_space_ = true;
      brave_news_action_icon_view_->SetVisible(false);
    }
  }
#endif
}

void BraveLocationBarView::ChildVisibilityChanged(views::View* child) {
  LocationBarView::ChildVisibilityChanged(child);
  // Normally, PageActionIcons are in a container which is always visible, only
  // the size changes when an icon is shown or hidden. The LocationBarView
  // does not listen to ChildVisibilityChanged events so we must make we Layout
  // and re-caculate trailing decorator positions when a child changes.
  if (std::ranges::contains(GetLeftMostTrailingViews(), child) ||
      std::ranges::contains(GetRightMostTrailingViews(), child)) {
    // Suppress re-entrant layout during UpdateBraveViewsSpaceVisibility() to
    // avoid infinite recursion (SetVisible → ChildVisibilityChanged →
    // DeprecatedLayoutImmediately → Layout → SetVisible → ...).
    if (!in_brave_space_update_) {
      DeprecatedLayoutImmediately();
    }
    SchedulePaint();
  }
}

void BraveLocationBarView::SetupShadow() {
  const auto* const color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  const int radius = GetBorderRadius();
  ViewShadow::ShadowParameters shadow{
      .offset_x = 0,
      .offset_y = 1,
      .blur_radius = radius,
      .shadow_color = color_provider->GetColor(kColorLocationBarHoveredShadow)};

  shadow_ = std::make_unique<ViewShadow>(this, radius, shadow);
}

int BraveLocationBarView::GetBorderRadius() const {
  return ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
      views::Emphasis::kMaximum, size());
}

void BraveLocationBarView::FocusLocation(bool is_user_initiated) {
  if (base::FeatureList::IsEnabled(tabs::kBraveSharedPinnedTabs) &&
      browser_->profile()->GetPrefs()->GetBoolean(
          brave_tabs::kSharedPinnedTab)) {
    // When updating dummy contents, this could be called even when the widget
    // is inactive. We shouldn't focus the omnibox in that case.
    if (auto* widget = GetWidget(); !widget || !widget->IsActive()) {
      return;
    }
  }

  omnibox_view_->SetFocus(is_user_initiated);
}

SkPath BraveLocationBarView::GetFocusRingHighlightPath() const {
  const SkScalar radius = GetBorderRadius();
  return SkPath::RRect(gfx::RectToSkRect(GetLocalBounds()), radius, radius);
}

ContentSettingImageView*
BraveLocationBarView::GetContentSettingsImageViewForTesting(size_t idx) {
  DCHECK(idx < content_setting_views_.size());
  return content_setting_views_[idx];
}

BEGIN_METADATA(BraveLocationBarView)
END_METADATA
