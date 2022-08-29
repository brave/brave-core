// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_actions/brave_rewards_action_view.h"

#include <memory>
#include <string>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/ui/brave_actions/brave_action_icon_with_badge_image_source.h"
#include "brave/browser/ui/webui/brave_rewards/rewards_panel_ui.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/skia_util.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/controls/button/label_button_border.h"
#include "ui/views/controls/button/menu_button_controller.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/view_class_properties.h"

namespace {

using brave_rewards::RewardsNotificationService;
using brave_rewards::RewardsPanelCoordinator;
using brave_rewards::RewardsServiceFactory;
using brave_rewards::RewardsTabHelper;

constexpr SkColor kIconColor = SK_ColorBLACK;
constexpr SkColor kBadgeTextColor = SK_ColorWHITE;
constexpr SkColor kBadgeNotificationBG = SkColorSetRGB(0xfb, 0x54, 0x2b);
constexpr SkColor kBadgeVerifiedBG = SkColorSetRGB(0x4c, 0x54, 0xd2);
const char kVerifiedCheck[] = "\u2713";

class ButtonHighlightPathGenerator : public views::HighlightPathGenerator {
 public:
  // views::HighlightPathGenerator:
  SkPath GetHighlightPath(const views::View* view) override {
    DCHECK(view);
    gfx::Rect rect(view->GetPreferredSize());
    rect.Inset(gfx::Insets::TLBR(0, 0, 0, kBraveActionRightMargin));

    auto* layout_provider = ChromeLayoutProvider::Get();
    DCHECK(layout_provider);

    int radius = layout_provider->GetCornerRadiusMetric(
        views::Emphasis::kMaximum, rect.size());

    SkPath path;
    path.addRoundRect(gfx::RectToSkRect(rect), radius, radius);
    return path;
  }
};

const ui::ColorProvider* GetColorProviderForWebContents(
    base::WeakPtr<content::WebContents> web_contents) {
  if (web_contents) {
    return &web_contents->GetColorProvider();
  }

  return ui::ColorProviderManager::Get().GetColorProviderFor(
      ui::NativeTheme::GetInstanceForNativeUi()->GetColorProviderKey(nullptr));
}

// Provides the context menu for the Rewards button.
class RewardsActionMenuModel : public ui::SimpleMenuModel,
                               public ui::SimpleMenuModel::Delegate {
 public:
  explicit RewardsActionMenuModel(PrefService* prefs)
      : SimpleMenuModel(this), prefs_(prefs) {
    Build();
  }

  ~RewardsActionMenuModel() override = default;
  RewardsActionMenuModel(const RewardsActionMenuModel&) = delete;
  RewardsActionMenuModel& operator=(const RewardsActionMenuModel&) = delete;

 private:
  enum ContextMenuCommand { kHideBraveRewardsIcon };

  // ui::SimpleMenuModel::Delegate override:
  void ExecuteCommand(int command_id, int event_flags) override {
    if (command_id == kHideBraveRewardsIcon) {
      prefs_->SetBoolean(brave_rewards::prefs::kShowButton, false);
    }
  }

  void Build() {
    AddItemWithStringId(kHideBraveRewardsIcon,
                        IDS_HIDE_BRAVE_REWARDS_ACTION_ICON);
  }

  raw_ptr<PrefService> prefs_ = nullptr;
};

}  // namespace

BraveRewardsActionView::BraveRewardsActionView(Browser* browser)
    : ToolbarButton(
          base::BindRepeating(&BraveRewardsActionView::OnButtonPressed,
                              base::Unretained(this)),
          std::make_unique<RewardsActionMenuModel>(
              browser->profile()->GetPrefs()),
          nullptr,
          false),
      browser_(browser),
      bubble_manager_(std::make_unique<WebUIBubbleManagerT<RewardsPanelUI>>(
          this,
          browser_->profile(),
          GURL(kBraveRewardsPanelURL),
          IDS_BRAVE_UI_BRAVE_REWARDS)) {
  DCHECK(browser_);

  SetButtonController(std::make_unique<views::MenuButtonController>(
      this,
      base::BindRepeating(&BraveRewardsActionView::OnButtonPressed,
                          base::Unretained(this)),
      std::make_unique<views::Button::DefaultButtonControllerDelegate>(this)));

  views::HighlightPathGenerator::Install(
      this, std::make_unique<ButtonHighlightPathGenerator>());

  // The highlight opacity set by |ToolbarButton| is different that the default
  // highlight opacity used by the other buttons in the actions container. Unset
  // the highlight opacity to match.
  views::InkDrop::Get(this)->SetHighlightOpacity({});

  SetHorizontalAlignment(gfx::ALIGN_CENTER);
  SetLayoutInsets(gfx::Insets(0));
  SetAccessibleName(
      brave_l10n::GetLocalizedResourceUTF16String(IDS_BRAVE_UI_BRAVE_REWARDS));

  auto* profile = browser_->profile();

  pref_change_registrar_.Init(profile->GetPrefs());
  pref_change_registrar_.Add(
      brave_rewards::prefs::kBadgeText,
      base::BindRepeating(&BraveRewardsActionView::OnPreferencesChanged,
                          base::Unretained(this)));

  browser_->tab_strip_model()->AddObserver(this);

  if (auto* rewards_service = GetRewardsService()) {
    rewards_service_observation_.Observe(rewards_service);
  }

  if (auto* notification_service = GetNotificationService()) {
    notification_service_observation_.Observe(notification_service);
  }

  panel_coordinator_ = RewardsPanelCoordinator::FromBrowser(browser_);
  if (panel_coordinator_) {
    panel_observation_.Observe(panel_coordinator_);
  }

  UpdateTabHelper(GetActiveWebContents());
}

BraveRewardsActionView::~BraveRewardsActionView() = default;

void BraveRewardsActionView::Update() {
  gfx::Size preferred_size = GetPreferredSize();
  auto* web_contents = GetActiveWebContents();
  auto weak_contents = web_contents ? web_contents->GetWeakPtr()
                                    : base::WeakPtr<content::WebContents>();

  auto image_source = std::make_unique<BraveActionIconWithBadgeImageSource>(
      preferred_size,
      base::BindRepeating(GetColorProviderForWebContents, weak_contents));

  image_source->SetIcon(gfx::Image(GetRewardsIcon()));

  auto [text, background_color] = GetBadgeTextAndBackground();
  image_source->SetBadge(std::make_unique<IconWithBadgeImageSource::Badge>(
      text, kBadgeTextColor, background_color));

  SetImage(views::Button::STATE_NORMAL,
           gfx::ImageSkia(std::move(image_source), preferred_size));
}

void BraveRewardsActionView::ClosePanelForTesting() {
  if (IsPanelOpen()) {
    ToggleRewardsPanel();
  }
}

gfx::Rect BraveRewardsActionView::GetAnchorBoundsInScreen() const {
  if (!GetVisible()) {
    // If the button is currently hidden, then anchor the bubble to the
    // location bar instead.
    auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser_);
    DCHECK(browser_view);
    return browser_view->GetLocationBarView()->GetAnchorBoundsInScreen();
  }
  return ToolbarButton::GetAnchorBoundsInScreen();
}

std::unique_ptr<views::LabelButtonBorder>
BraveRewardsActionView::CreateDefaultBorder() const {
  auto border = ToolbarButton::CreateDefaultBorder();
  border->set_insets(gfx::Insets::TLBR(0, 0, 0, 0));
  return border;
}

void BraveRewardsActionView::OnWidgetDestroying(views::Widget* widget) {
  DCHECK(bubble_observation_.IsObservingSource(widget));
  bubble_observation_.Reset();
}

void BraveRewardsActionView::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (selection.active_tab_changed()) {
    UpdateTabHelper(selection.new_contents);
  }
}

void BraveRewardsActionView::OnPublisherForTabUpdated(
    const std::string& publisher_id) {
  publisher_registered_ = {publisher_id, false};
  bool status_updating = UpdatePublisherStatus();
  if (!status_updating) {
    Update();
  }
}

void BraveRewardsActionView::OnRewardsPanelRequested(
    const brave_rewards::mojom::RewardsPanelArgs& args) {
  if (!IsPanelOpen()) {
    ToggleRewardsPanel();
  }
}

void BraveRewardsActionView::OnPublisherRegistryUpdated() {
  UpdatePublisherStatus();
}

void BraveRewardsActionView::OnPublisherUpdated(
    const std::string& publisher_id) {
  if (publisher_id == std::get<std::string>(publisher_registered_)) {
    UpdatePublisherStatus();
  }
}

void BraveRewardsActionView::OnNotificationAdded(
    RewardsNotificationService* service,
    const RewardsNotificationService::RewardsNotification& notification) {
  Update();
}

void BraveRewardsActionView::OnNotificationDeleted(
    RewardsNotificationService* service,
    const RewardsNotificationService::RewardsNotification& notification) {
  Update();
}

void BraveRewardsActionView::OnButtonPressed() {
  // If we are opening the Rewards panel, use `RewardsPanelCoordinator` to open
  // it so that the panel arguments will be correctly set.
  if (!IsPanelOpen() && panel_coordinator_) {
    panel_coordinator_->OpenRewardsPanel();
    return;
  }

  ToggleRewardsPanel();
}

void BraveRewardsActionView::OnPreferencesChanged(const std::string& key) {
  Update();
}

content::WebContents* BraveRewardsActionView::GetActiveWebContents() {
  return browser_->tab_strip_model()->GetActiveWebContents();
}

brave_rewards::RewardsService* BraveRewardsActionView::GetRewardsService() {
  return RewardsServiceFactory::GetForProfile(browser_->profile());
}

brave_rewards::RewardsNotificationService*
BraveRewardsActionView::GetNotificationService() {
  if (auto* rewards_service = GetRewardsService()) {
    return rewards_service->GetNotificationService();
  }
  return nullptr;
}

bool BraveRewardsActionView::IsPanelOpen() {
  return bubble_observation_.IsObserving();
}

void BraveRewardsActionView::ToggleRewardsPanel() {
  if (IsPanelOpen()) {
    DCHECK(bubble_manager_);
    bubble_manager_->CloseBubble();
    return;
  }

  // Clear the default-on-start badge text when the user opens the panel.
  auto* prefs = browser_->profile()->GetPrefs();
  prefs->SetString(brave_rewards::prefs::kBadgeText, "");

  bubble_manager_->ShowBubble();

  DCHECK(!bubble_observation_.IsObserving());
  bubble_observation_.Observe(bubble_manager_->GetBubbleWidget());
}

gfx::ImageSkia BraveRewardsActionView::GetRewardsIcon() {
  // Since the BAT icon has color the actual color value here is not relevant,
  // but |CreateVectorIcon| requires one.
  return gfx::CreateVectorIcon(kBatIcon, kBraveActionGraphicSize, kIconColor);
}

std::pair<std::string, SkColor>
BraveRewardsActionView::GetBadgeTextAndBackground() {
  // 1. Display the default-on-start Rewards badge text, if specified.
  std::string text_pref = browser_->profile()->GetPrefs()->GetString(
      brave_rewards::prefs::kBadgeText);
  if (!text_pref.empty()) {
    return {text_pref, kBadgeNotificationBG};
  }

  // 2. Display the number of current notifications, if non-zero.
  size_t notifications = GetRewardsNotificationCount();
  if (notifications > 0) {
    std::string text =
        notifications > 99 ? "99+" : base::NumberToString(notifications);

    return {text, kBadgeNotificationBG};
  }

  // 3. Display a verified checkmark for verified publishers.
  if (std::get<bool>(publisher_registered_)) {
    return {kVerifiedCheck, kBadgeVerifiedBG};
  }

  return {"", kBadgeNotificationBG};
}

size_t BraveRewardsActionView::GetRewardsNotificationCount() {
  size_t count = 0;

  if (auto* service = GetNotificationService()) {
    count += service->GetAllNotifications().size();
  }

  auto* prefs = browser_->profile()->GetPrefs();
  if (prefs->GetBoolean(brave_rewards::prefs::kEnabled) &&
      prefs->GetString(brave_rewards::prefs::kDeclaredCountry).empty()) {
    count += 1;
  }

  return count;
}

bool BraveRewardsActionView::UpdatePublisherStatus() {
  std::string& publisher_id = std::get<std::string>(publisher_registered_);
  if (publisher_id.empty()) {
    return false;
  }

  auto* rewards_service = GetRewardsService();
  if (!rewards_service || !rewards_service->IsRewardsEnabled()) {
    return false;
  }

  rewards_service->IsPublisherRegistered(
      publisher_id,
      base::BindOnce(&BraveRewardsActionView::IsPublisherRegisteredCallback,
                     weak_factory_.GetWeakPtr(), publisher_id));

  return true;
}

void BraveRewardsActionView::IsPublisherRegisteredCallback(
    const std::string& publisher_id,
    bool is_registered) {
  if (publisher_id == std::get<std::string>(publisher_registered_)) {
    publisher_registered_.second = is_registered;
    Update();
  }
}

void BraveRewardsActionView::UpdateTabHelper(
    content::WebContents* web_contents) {
  tab_helper_ = nullptr;
  if (tab_helper_observation_.IsObserving()) {
    tab_helper_observation_.Reset();
  }

  if (web_contents) {
    if (auto* helper = RewardsTabHelper::FromWebContents(web_contents)) {
      tab_helper_ = helper;
      tab_helper_observation_.Observe(helper);
    }
  }

  OnPublisherForTabUpdated(tab_helper_ ? tab_helper_->GetPublisherIdForTab()
                                       : "");
}
