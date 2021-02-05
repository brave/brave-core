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
#include "brave/browser/ui/views/sidebar/sidebar_add_item_bubble_delegate_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_button_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_items_scroll_view.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/grit/brave_theme_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/menu/menu_runner.h"

namespace {

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

  sidebar_model_observed_.Add(browser_->sidebar_controller()->model());
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
      sidebar::SidebarService::kShowAlways,
      l10n_util::GetStringUTF16(IDS_SIDEBAR_SHOW_OPTION_ALWAYS));
  context_menu_model_->AddCheckItem(
      sidebar::SidebarService::kShowOnMouseOver,
      l10n_util::GetStringUTF16(IDS_SIDEBAR_SHOW_OPTION_MOUSEOVER));
  context_menu_model_->AddCheckItem(
      sidebar::SidebarService::kShowOnClick,
      l10n_util::GetStringUTF16(IDS_SIDEBAR_SHOW_OPTION_ONCLICK));
  context_menu_model_->AddCheckItem(
      sidebar::SidebarService::kShowNever,
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
  service->SetSidebarShowOption(command_id);
}

bool SidebarControlView::IsCommandIdChecked(int command_id) const {
  const auto* service =
      sidebar::SidebarServiceFactory::GetForProfile(browser_->profile());
  return command_id == service->GetSidebarShowOption();
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
      AddChildView(std::make_unique<SidebarButtonView>(nullptr));
  sidebar_item_add_view_->set_context_menu_controller(this);
  sidebar_item_add_view_->SetCallback(
      base::BindRepeating(&SidebarControlView::OnButtonPressed,
                          base::Unretained(this), sidebar_item_add_view_));

  sidebar_settings_view_ =
      AddChildView(std::make_unique<SidebarButtonView>(nullptr));
  sidebar_settings_view_->SetCallback(
      base::BindRepeating(&SidebarControlView::OnButtonPressed,
                          base::Unretained(this), sidebar_settings_view_));
}

void SidebarControlView::OnButtonPressed(views::View* view) {
  if (view == sidebar_item_add_view_) {
    auto* bubble = views::BubbleDialogDelegateView::CreateBubble(
        new SidebarAddItemBubbleDelegateView(browser_, view));
    bubble->Show();
    return;
  }

  if (view == sidebar_settings_view_) {
    // TODO(simonhong): Handle settings button here.
    NOTIMPLEMENTED();
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

  SkColor button_base_color = SK_ColorWHITE;
  SkColor button_disabled_color = SK_ColorWHITE;
  if (const ui::ThemeProvider* theme_provider = GetThemeProvider()) {
    button_base_color = theme_provider->GetColor(
        BraveThemeProperties::COLOR_SIDEBAR_BUTTON_BASE);
    button_disabled_color = theme_provider->GetColor(
        BraveThemeProperties::COLOR_SIDEBAR_ADD_BUTTON_DISABLED);
  }

  // Update add button image based on enabled state.
  sidebar_item_add_view_->SetImage(views::Button::STATE_NORMAL, nullptr);
  sidebar_item_add_view_->SetImage(views::Button::STATE_DISABLED, nullptr);
  sidebar_item_add_view_->SetImage(views::Button::STATE_HOVERED, nullptr);
  sidebar_item_add_view_->SetImage(views::Button::STATE_PRESSED, nullptr);
  auto& bundle = ui::ResourceBundle::GetSharedInstance();
  if (should_enable) {
    sidebar_item_add_view_->SetImage(
        views::Button::STATE_NORMAL,
        gfx::CreateVectorIcon(kSidebarAddItemIcon, button_base_color));
    sidebar_item_add_view_->SetImage(
        views::Button::STATE_HOVERED,
        bundle.GetImageSkiaNamed(IDR_SIDEBAR_ITEM_ADD_FOCUSED));
    sidebar_item_add_view_->SetImage(
        views::Button::STATE_PRESSED,
        bundle.GetImageSkiaNamed(IDR_SIDEBAR_ITEM_ADD_FOCUSED));
  } else {
    sidebar_item_add_view_->SetImage(
        views::Button::STATE_NORMAL,
        gfx::CreateVectorIcon(kSidebarAddItemIcon, button_disabled_color));
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
