/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_control_view.h"

#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/sidebar/sidebar_item_add_button.h"
#include "brave/browser/ui/views/sidebar/sidebar_items_scroll_view.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/layout/flex_layout.h"

namespace {

using ShowSidebarOption = sidebar::SidebarService::ShowSidebarOption;

// To use bold font for title at index 0.
class ControlViewMenuModel : public ui::SimpleMenuModel {
 public:
  using SimpleMenuModel::SimpleMenuModel;
  ~ControlViewMenuModel() override = default;
  ControlViewMenuModel(const ControlViewMenuModel&) = delete;
  ControlViewMenuModel& operator=(const ControlViewMenuModel&) = delete;

  // ui::SimpleMenuModel overrides:
  const gfx::FontList* GetLabelFontListAt(size_t index) const override {
    if (GetTypeAt(index) == ui::MenuModel::TYPE_TITLE) {
      return &ui::ResourceBundle::GetSharedInstance().GetFontList(
          ui::ResourceBundle::BoldFont);
    }
    return SimpleMenuModel::GetLabelFontListAt(index);
  }
};

bool IsSidebarOnLeft(Browser* browser) {
  return !browser->profile()->GetPrefs()->GetBoolean(
      prefs::kSidePanelHorizontalAlignment);
}

}  // namespace

SidebarControlView::SidebarControlView(Delegate* delegate,
                                       BraveBrowser* browser)
    : delegate_(delegate), browser_(browser) {
  set_context_menu_controller(this);

  AddChildViews();
  UpdateItemAddButtonState();
  UpdateSettingsButtonState();

  sidebar_model_observed_.Observe(browser_->sidebar_controller()->model());
  SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kVertical);
}

void SidebarControlView::OnThemeChanged() {
  View::OnThemeChanged();

  UpdateBackgroundAndBorder();
  UpdateItemAddButtonState();
}

void SidebarControlView::UpdateBackgroundAndBorder() {
  if (const ui::ColorProvider* color_provider = GetColorProvider()) {
    SetBackground(
        views::CreateSolidBackground(color_provider->GetColor(kColorToolbar)));
    int border_thickness =
        1 - BraveContentsViewUtil::GetRoundedCornersWebViewMargin(browser_);
    SetBorder(views::CreateEmptyBorder(
        gfx::Insets::TLBR(0, sidebar_on_left_ ? 0 : border_thickness, 0,
                          sidebar_on_left_ ? border_thickness : 0)));
  }
}

SidebarControlView::~SidebarControlView() = default;

void SidebarControlView::ShowContextMenuForViewImpl(
    views::View* source,
    const gfx::Point& point,
    ui::MenuSourceType source_type) {
  if (context_menu_runner_ && context_menu_runner_->IsRunning()) {
    return;
  }

  context_menu_model_ = std::make_unique<ControlViewMenuModel>(this);
  context_menu_model_->AddTitle(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_SIDEBAR_SHOW_OPTION_TITLE));
  context_menu_model_->AddCheckItem(
      static_cast<int>(ShowSidebarOption::kShowAlways),
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SIDEBAR_SHOW_OPTION_ALWAYS));
  context_menu_model_->AddCheckItem(
      static_cast<int>(ShowSidebarOption::kShowOnMouseOver),
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SIDEBAR_SHOW_OPTION_MOUSEOVER));
  context_menu_model_->AddCheckItem(
      static_cast<int>(ShowSidebarOption::kShowNever),
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SIDEBAR_SHOW_OPTION_NEVER));
  context_menu_model_->AddSeparator(
      ui::MenuSeparatorType::BOTH_SIDE_PADDED_SEPARATOR);
  context_menu_model_->AddTitle(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_SIDEBAR_MENU_MODEL_POSITION_OPTION_TITLE));
  context_menu_model_->AddItemWithStringId(
      IDC_SIDEBAR_TOGGLE_POSITION,
      IsSidebarOnLeft(browser_)
          ? IDS_SIDEBAR_MENU_MODEL_POSITION_MOVE_TO_RIGHT_OPTION
          : IDS_SIDEBAR_MENU_MODEL_POSITION_MOVE_TO_LEFT_OPTION);
  context_menu_runner_ = std::make_unique<views::MenuRunner>(
      context_menu_model_.get(), views::MenuRunner::CONTEXT_MENU);
  context_menu_runner_->RunMenuAt(
      source->GetWidget(), nullptr, gfx::Rect(point, gfx::Size()),
      views::MenuAnchorPosition::kTopLeft, source_type);
}

void SidebarControlView::ExecuteCommand(int command_id, int event_flags) {
  if (command_id == IDC_SIDEBAR_TOGGLE_POSITION) {
    browser_->command_controller()->ExecuteCommand(command_id);
    return;
  }
  auto* service =
      sidebar::SidebarServiceFactory::GetForProfile(browser_->profile());
  service->SetSidebarShowOption(static_cast<ShowSidebarOption>(command_id));
}

bool SidebarControlView::IsCommandIdChecked(int command_id) const {
  const auto* service =
      sidebar::SidebarServiceFactory::GetForProfile(browser_->profile());
  return static_cast<ShowSidebarOption>(command_id) ==
         service->GetSidebarShowOption();
}

void SidebarControlView::MenuClosed(ui::SimpleMenuModel* source) {
  delegate_->MenuClosed();
}

void SidebarControlView::OnItemAdded(const sidebar::SidebarItem& item,
                                     size_t index,
                                     bool user_gesture) {
  UpdateItemAddButtonState();
}

void SidebarControlView::OnItemRemoved(size_t index) {
  UpdateItemAddButtonState();
}

void SidebarControlView::AddChildViews() {
  sidebar_items_view_ =
      AddChildView(std::make_unique<SidebarItemsScrollView>(browser_));
  sidebar_items_view_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero)
          .WithOrder(2));
  sidebar_item_add_view_ = AddChildView(std::make_unique<SidebarItemAddButton>(
      browser_, brave_l10n::GetLocalizedResourceUTF16String(
                    IDS_SIDEBAR_ADD_ITEM_BUTTON_TOOLTIP)));
  sidebar_item_add_view_->set_context_menu_controller(this);
  // Remove top margin as the last item view has bottom margin.
  sidebar_item_add_view_->GetProperty(views::kMarginsKey)->set_top(0);

  // This helps the settings button to be on the bottom
  auto* spacer = AddChildView(std::make_unique<views::View>());
  spacer->SetEnabled(false);
  spacer->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kUnbounded)
          .WithOrder(1));

  sidebar_settings_view_ = AddChildView(std::make_unique<SidebarButtonView>(
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SIDEBAR_SETTINGS_BUTTON_TOOLTIP)));

  sidebar_settings_view_->SetCallback(
      base::BindRepeating(&SidebarControlView::OnButtonPressed,
                          base::Unretained(this), sidebar_settings_view_));
}

void SidebarControlView::OnButtonPressed(views::View* view) {
  if (view == sidebar_settings_view_) {
    ShowSingletonTabOverwritingNTP(
        browser_,
        GURL("brave://settings?search=" +
             l10n_util::GetStringUTF8(
                 IDS_SETTINGS_APPEARNCE_SETTINGS_SIDEBAR_PART_TITLE)));
  }
}

void SidebarControlView::Update() {
  UpdateItemAddButtonState();
  sidebar_items_view_->Update();
}

void SidebarControlView::UpdateItemAddButtonState() {
  DCHECK(sidebar_item_add_view_);
  // Determine add button enabled state.
  bool should_enable = true;
  if (browser_->sidebar_controller()->model()->IsSidebarHasAllBuiltInItems() &&
      !sidebar::CanAddCurrentActiveTabToSidebar(browser_)) {
    should_enable = false;
  }
  sidebar_item_add_view_->SetEnabled(should_enable);
}

void SidebarControlView::UpdateSettingsButtonState() {
  DCHECK(sidebar_settings_view_);
  sidebar_settings_view_->SetImageModel(
      views::Button::STATE_NORMAL,
      ui::ImageModel::FromVectorIcon(kLeoSettingsIcon, kColorSidebarButtonBase,
                                     SidebarButtonView::kDefaultIconSize));
  sidebar_settings_view_->SetImageModel(
      views::Button::STATE_PRESSED,
      ui::ImageModel::FromVectorIcon(kLeoSettingsIcon,
                                     kColorSidebarButtonPressed,
                                     SidebarButtonView::kDefaultIconSize));
  sidebar_settings_view_->SetImageModel(
      views::Button::STATE_DISABLED,
      ui::ImageModel::FromVectorIcon(kLeoSettingsIcon,
                                     kColorSidebarAddButtonDisabled,
                                     SidebarButtonView::kDefaultIconSize));
}

bool SidebarControlView::IsItemReorderingInProgress() const {
  return sidebar_items_view_->IsItemReorderingInProgress();
}

bool SidebarControlView::IsBubbleWidgetVisible() const {
  if (context_menu_runner_ && context_menu_runner_->IsRunning()) {
    return true;
  }

  if (sidebar_item_add_view_->IsBubbleVisible()) {
    return true;
  }

  if (sidebar_items_view_->IsBubbleVisible()) {
    return true;
  }

  return false;
}

void SidebarControlView::SetSidebarOnLeft(bool sidebar_on_left) {
  sidebar_on_left_ = sidebar_on_left;
  UpdateBackgroundAndBorder();
}

BEGIN_METADATA(SidebarControlView)
END_METADATA
