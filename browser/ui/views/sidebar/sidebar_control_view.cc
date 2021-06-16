/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_control_view.h"

#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/themes/theme_properties.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/views/sidebar/sidebar_item_add_button.h"
#include "brave/browser/ui/views/sidebar/sidebar_items_scroll_view.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/common/webui_url_constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/menu/menu_runner.h"

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
  const gfx::FontList* GetLabelFontListAt(int index) const override {
    if (index == 0) {
      return &ui::ResourceBundle::GetSharedInstance().GetFontList(
          ui::ResourceBundle::BoldFont);
    }
    return SimpleMenuModel::GetLabelFontListAt(index);
  }
};

}  // namespace

SidebarControlView::SidebarControlView(BraveBrowser* browser)
    : browser_(browser) {
  set_context_menu_controller(this);

  AddChildViews();
  UpdateItemAddButtonState();
  UpdateSettingsButtonState();

  sidebar_model_observed_.Observe(browser_->sidebar_controller()->model());
}

void SidebarControlView::Layout() {
  // Add item/settings buttons have more high priority than items container.
  // If this view doesn't have enough space to show all child views, items
  // container will get insufficient bounds.
  gfx::Rect bounds = GetContentsBounds();
  gfx::Size preferred_size = GetPreferredSize();
  const bool has_sufficient_size = bounds.height() >= preferred_size.height();

  const gfx::Size settings_size = sidebar_settings_view_->GetPreferredSize();
  const gfx::Size items_view_size = sidebar_items_view_->GetPreferredSize();
  const gfx::Size add_view_size = sidebar_item_add_view_->GetPreferredSize();

  const int x = bounds.x();
  if (has_sufficient_size) {
    int y = bounds.y();
    sidebar_items_view_->SetBounds(x, y, items_view_size.width(),
                                   items_view_size.height());
    y += items_view_size.height();
    sidebar_item_add_view_->SetBounds(x, y, add_view_size.width(),
                                      add_view_size.height());
  } else {
    // Give remained area to items view.
    sidebar_items_view_->SetBounds(
        x, bounds.y(), items_view_size.width(),
        bounds.height() - add_view_size.width() - settings_size.width());
    sidebar_item_add_view_->SetBounds(
        x, bounds.height() - settings_size.height() - add_view_size.height(),
        add_view_size.width(), add_view_size.height());
  }
  // Locate settings button at bottom line.
  sidebar_settings_view_->SetBounds(x, bounds.height() - settings_size.height(),
                                    settings_size.width(),
                                    settings_size.height());
}

gfx::Size SidebarControlView::CalculatePreferredSize() const {
  gfx::Size preferred(sidebar_item_add_view_->GetPreferredSize().width(), 0);
  preferred.Enlarge(0, sidebar_items_view_->GetPreferredSize().height());
  preferred.Enlarge(0, sidebar_item_add_view_->GetPreferredSize().height());
  preferred.Enlarge(0, sidebar_settings_view_->GetPreferredSize().height());
  preferred += GetInsets().size();
  return preferred;
}

void SidebarControlView::OnThemeChanged() {
  View::OnThemeChanged();

  UpdateBackgroundAndBorder();
  UpdateItemAddButtonState();
  UpdateSettingsButtonState();
}

void SidebarControlView::UpdateBackgroundAndBorder() {
  if (const ui::ThemeProvider* theme_provider = GetThemeProvider()) {
    constexpr int kBorderThickness = 1;
    SetBackground(views::CreateSolidBackground(theme_provider->GetColor(
        BraveThemeProperties::COLOR_SIDEBAR_BACKGROUND)));
    SetBorder(views::CreateSolidSidedBorder(
        0, 0, 0, kBorderThickness,
        theme_provider->GetColor(BraveThemeProperties::COLOR_SIDEBAR_BORDER)));
  }
}

SidebarControlView::~SidebarControlView() = default;

void SidebarControlView::ShowContextMenuForViewImpl(
    views::View* source,
    const gfx::Point& point,
    ui::MenuSourceType source_type) {
  if (context_menu_runner_ && context_menu_runner_->IsRunning())
    return;

  context_menu_model_ = std::make_unique<ControlViewMenuModel>(this);
  context_menu_model_->AddTitle(
      l10n_util::GetStringUTF16(IDS_SIDEBAR_SHOW_OPTION_TITLE));
  context_menu_model_->AddCheckItem(
      static_cast<int>(ShowSidebarOption::kShowAlways),
      l10n_util::GetStringUTF16(IDS_SIDEBAR_SHOW_OPTION_ALWAYS));
  context_menu_model_->AddCheckItem(
      static_cast<int>(ShowSidebarOption::kShowOnMouseOver),
      l10n_util::GetStringUTF16(IDS_SIDEBAR_SHOW_OPTION_MOUSEOVER));
  context_menu_model_->AddCheckItem(
      static_cast<int>(ShowSidebarOption::kShowOnClick),
      l10n_util::GetStringUTF16(IDS_SIDEBAR_SHOW_OPTION_ONCLICK));
  context_menu_model_->AddCheckItem(
      static_cast<int>(ShowSidebarOption::kShowNever),
      l10n_util::GetStringUTF16(IDS_SIDEBAR_SHOW_OPTION_NEVER));
  context_menu_runner_ = std::make_unique<views::MenuRunner>(
      context_menu_model_.get(), views::MenuRunner::CONTEXT_MENU);
  context_menu_runner_->RunMenuAt(
      source->GetWidget(), nullptr, gfx::Rect(point, gfx::Size()),
      views::MenuAnchorPosition::kTopLeft, source_type);
}

void SidebarControlView::ExecuteCommand(int command_id, int event_flags) {
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

std::u16string SidebarControlView::GetTooltipTextFor(
    const views::View* view) const {
  if (view == sidebar_settings_view_)
    return l10n_util::GetStringUTF16(IDS_SIDEBAR_SETTINGS_BUTTON_TOOLTIP);

  return std::u16string();
}

void SidebarControlView::OnItemAdded(const sidebar::SidebarItem& item,
                                     int index,
                                     bool user_gesture) {
  UpdateItemAddButtonState();
}

void SidebarControlView::OnItemRemoved(int index) {
  UpdateItemAddButtonState();
}

void SidebarControlView::AddChildViews() {
  sidebar_items_view_ =
      AddChildView(std::make_unique<SidebarItemsScrollView>(browser_));

  sidebar_item_add_view_ =
      AddChildView(std::make_unique<SidebarItemAddButton>(browser_));
  sidebar_item_add_view_->set_context_menu_controller(this);

  sidebar_settings_view_ =
      AddChildView(std::make_unique<SidebarButtonView>(this));
  sidebar_settings_view_->SetCallback(
      base::BindRepeating(&SidebarControlView::OnButtonPressed,
                          base::Unretained(this), sidebar_settings_view_));
}

void SidebarControlView::OnButtonPressed(views::View* view) {
  if (view == sidebar_settings_view_) {
    browser_->sidebar_controller()->LoadAtTab(
        GURL(chrome::kChromeUISettingsURL));
  }
}

void SidebarControlView::Update() {
  UpdateItemAddButtonState();
}

void SidebarControlView::UpdateItemAddButtonState() {
  DCHECK(sidebar_item_add_view_);
  // Determine add button enabled state.
  bool should_enable = true;
  if (browser_->sidebar_controller()->model()->IsSidebarHasAllBuiltiInItems() &&
      !sidebar::CanAddCurrentActiveTabToSidebar(browser_)) {
    should_enable = false;
  }
  sidebar_item_add_view_->SetEnabled(should_enable);
}

void SidebarControlView::UpdateSettingsButtonState() {
  DCHECK(sidebar_settings_view_);
  if (const ui::ThemeProvider* theme_provider = GetThemeProvider()) {
    sidebar_settings_view_->SetImage(
        views::Button::STATE_NORMAL,
        gfx::CreateVectorIcon(
            kSidebarSettingsIcon,
            theme_provider->GetColor(
                BraveThemeProperties::COLOR_SIDEBAR_BUTTON_BASE)));
    auto& bundle = ui::ResourceBundle::GetSharedInstance();
    sidebar_settings_view_->SetImage(
        views::Button::STATE_HOVERED,
        bundle.GetImageSkiaNamed(IDR_SIDEBAR_SETTINGS_FOCUSED));
    sidebar_settings_view_->SetImage(
        views::Button::STATE_PRESSED,
        bundle.GetImageSkiaNamed(IDR_SIDEBAR_SETTINGS_FOCUSED));
  }
}

bool SidebarControlView::IsItemReorderingInProgress() const {
  return sidebar_items_view_->IsItemReorderingInProgress();
}

bool SidebarControlView::IsBubbleWidgetVisible() const {
  if (context_menu_runner_ && context_menu_runner_->IsRunning())
    return true;

  if (sidebar_item_add_view_->IsBubbleVisible())
    return true;

  if (sidebar_items_view_->IsBubbleVisible())
    return true;

  return false;
}
