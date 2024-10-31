/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/brave_app_menu.h"

#include <memory>

#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
#include "base/scoped_observation.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/toolbar/brave_app_menu_model.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/misc_metrics/menu_metrics.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "brave/grit/brave_generated_resources.h"
#include "cc/paint/paint_flags.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/toolbar/app_menu_model.h"
#include "chrome/browser/ui/views/toolbar/app_menu.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/button_menu_item_model.h"
#include "ui/base/models/menu_model.h"
#include "ui/gfx/canvas.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/layout/box_layout_view.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/view.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/ui/views/toolbar/brave_vpn_status_label.h"
#include "brave/browser/ui/views/toolbar/brave_vpn_toggle_button.h"
#endif

using views::MenuItemView;

namespace {

// Copied from app_menu.cc to dump w/o crashing
bool IsBookmarkCommand(int command_id) {
  return command_id >= IDC_FIRST_UNBOUNDED_MENU &&
         ((command_id - IDC_FIRST_UNBOUNDED_MENU) %
              AppMenuModel::kNumUnboundedMenuTypes ==
          0);
}

// A button that resides inside menu item.
class SidebarShowOptionInMenuButton : public views::LabelButton,
                                      sidebar::SidebarService::Observer {
  METADATA_HEADER(SidebarShowOptionInMenuButton, views::LabelButton)
 public:
  SidebarShowOptionInMenuButton(BraveAppMenu* app_menu,
                                ui::ButtonMenuItemModel* model,
                                int index);
  ~SidebarShowOptionInMenuButton() override = default;

  // views::LabelButton:
  void PaintButtonContents(gfx::Canvas* canvas) override;

  // sidebar::SidebarService::Observer:
  void OnShowSidebarOptionChanged(
      sidebar::SidebarService::ShowSidebarOption option) override;

 private:
  sidebar::SidebarService::ShowSidebarOption show_option_;
  bool is_active_option_ = false;

  base::ScopedObservation<sidebar::SidebarService,
                          sidebar::SidebarService::Observer>
      sidebar_service_observation_{this};
};

SidebarShowOptionInMenuButton::SidebarShowOptionInMenuButton(
    BraveAppMenu* app_menu,
    ui::ButtonMenuItemModel* model,
    int index)
    : LabelButton(
          base::BindRepeating([](ui::ButtonMenuItemModel* model,
                                 int index) { model->ActivatedAt(index); },
                              model,
                              index),
          model->GetLabelAt(index)),
      show_option_(BraveAppMenuModel::ConvertIDCToSidebarShowOptions(
          model->GetCommandIdAt(index))) {
  SetFocusBehavior(FocusBehavior::ALWAYS);
  SetBackground(app_menu->CreateInMenuButtonBackgroundWithLeadingBorder());

  auto* service = sidebar::SidebarServiceFactory::GetForProfile(
      app_menu->browser()->profile());
  CHECK(service);

  sidebar_service_observation_.Observe(service);
  OnShowSidebarOptionChanged(service->GetSidebarShowOption());
}

void SidebarShowOptionInMenuButton::PaintButtonContents(gfx::Canvas* canvas) {
  if (is_active_option_) {
    // Draws highlight if this option is the chosen one.
    auto* cp = GetColorProvider();
    CHECK(cp);

    auto bounds = GetLocalBounds();
    bounds.Inset(2);
    cc::PaintFlags flags;
    flags.setColor(cp->GetColor(kColorBraveAppMenuAccentColor));
    flags.setStyle(cc::PaintFlags::kFill_Style);
    canvas->DrawRoundRect(bounds, /*radius*/ 2, flags);
  }

  views::LabelButton::PaintButtonContents(canvas);
}

void SidebarShowOptionInMenuButton::OnShowSidebarOptionChanged(
    sidebar::SidebarService::ShowSidebarOption option) {
  is_active_option_ = option == show_option_;
  SchedulePaint();
}

BEGIN_METADATA(SidebarShowOptionInMenuButton)
END_METADATA

// A view to contain the "sidebar show option" buttons. Each button
// represents an entry in the ui::ButtonMenuModel for the option
class SidebarShowOptionMenu : public views::BoxLayoutView {
  METADATA_HEADER(SidebarShowOptionMenu, views::BoxLayoutView)

 public:
  SidebarShowOptionMenu(BraveAppMenu* app_menu, ui::ButtonMenuItemModel* model);
  ~SidebarShowOptionMenu() override = default;

 private:
  raw_ptr<ui::ButtonMenuItemModel> model_ = nullptr;

  raw_ptr<views::LabelButton> on_button_ = nullptr;
  raw_ptr<views::LabelButton> hover_button_ = nullptr;
  raw_ptr<views::LabelButton> off_button_ = nullptr;
};

BEGIN_METADATA(SidebarShowOptionMenu)
END_METADATA

SidebarShowOptionMenu::SidebarShowOptionMenu(BraveAppMenu* app_menu,
                                             ui::ButtonMenuItemModel* model)
    : model_(model) {
  CHECK_EQ(3u, model->GetItemCount());

  CHECK_EQ(IDC_SIDEBAR_SHOW_OPTION_ALWAYS, model_->GetCommandIdAt(0));
  on_button_ = AddChildView(
      std::make_unique<SidebarShowOptionInMenuButton>(app_menu, model_, 0));

  CHECK_EQ(IDC_SIDEBAR_SHOW_OPTION_MOUSEOVER, model_->GetCommandIdAt(1));
  hover_button_ = AddChildView(
      std::make_unique<SidebarShowOptionInMenuButton>(app_menu, model_, 1));

  CHECK_EQ(IDC_SIDEBAR_SHOW_OPTION_NEVER, model_->GetCommandIdAt(2));
  off_button_ = AddChildView(
      std::make_unique<SidebarShowOptionInMenuButton>(app_menu, model_, 2));
}

}  // namespace

BraveAppMenu::BraveAppMenu(Browser* browser,
                           ui::MenuModel* model,
                           int run_types)
    : AppMenu(browser, model, run_types),
      menu_metrics_(
          g_brave_browser_process->process_misc_metrics()->menu_metrics()) {
  DCHECK(menu_metrics_);
  UpdateMenuItemView();
}

BraveAppMenu::~BraveAppMenu() = default;

void BraveAppMenu::RunMenu(views::MenuButtonController* host) {
  AppMenu::RunMenu(host);
  menu_metrics_->RecordMenuShown();
}

void BraveAppMenu::ExecuteCommand(int command_id, int mouse_event_flags) {
  // Suspect that the entry is null but can't imagine which command causes it.
  // See
  // https://github.com/brave/brave-browser/issues/37862#issuecomment-2078553575
  if (!IsBookmarkCommand(command_id) && command_id != IDC_EDIT_MENU &&
      command_id != IDC_ZOOM_MENU &&
      command_id_to_entry_.find(command_id) == command_id_to_entry_.end()) {
    LOG(ERROR) << __func__ << " entry should be exsit for " << command_id;
    SCOPED_CRASH_KEY_NUMBER("BraveAppMenu", "command_id", command_id);
    base::debug::DumpWithoutCrashing();
    return;
  }
  AppMenu::ExecuteCommand(command_id, mouse_event_flags);
  RecordMenuUsage(command_id);
}

void BraveAppMenu::OnMenuClosed(views::MenuItemView* menu) {
  AppMenu::OnMenuClosed(menu);
  if (menu == nullptr) {
    menu_metrics_->RecordMenuDismiss();
  }
}

void BraveAppMenu::RecordMenuUsage(int command_id) {
  misc_metrics::MenuGroup group;

  if (command_id == IDC_TOGGLE_AI_CHAT) {
    ai_chat::AIChatMetrics* metrics =
        g_brave_browser_process->process_misc_metrics()->ai_chat_metrics();
    CHECK(metrics);
    metrics->HandleOpenViaEntryPoint(ai_chat::EntryPoint::kMenuItem);
  }

  switch (command_id) {
    case IDC_NEW_WINDOW:
    case IDC_NEW_TAB:
    case IDC_NEW_INCOGNITO_WINDOW:
    case IDC_NEW_OFFTHERECORD_WINDOW_TOR:
    case IDC_OPEN_GUEST_PROFILE:
      group = misc_metrics::MenuGroup::kTabWindow;
      break;
    case IDC_SHOW_BRAVE_WALLET:
    case IDC_TOGGLE_AI_CHAT:
    case IDC_OPEN_FULL_PAGE_CHAT:
    case IDC_SHOW_BRAVE_SYNC:
    case IDC_SHOW_BRAVE_REWARDS:
      group = misc_metrics::MenuGroup::kBraveFeatures;
      break;
    case IDC_SHOW_HISTORY:
    case IDC_MANAGE_EXTENSIONS:
    case IDC_SHOW_BOOKMARK_MANAGER:
    case IDC_BOOKMARK_THIS_TAB:
    case IDC_BOOKMARK_ALL_TABS:
    case IDC_SHOW_BOOKMARK_BAR:
    case IDC_IMPORT_SETTINGS:
    case IDC_OPTIONS:
    case IDC_SHOW_DOWNLOADS:
      group = misc_metrics::MenuGroup::kBrowserViews;
      break;
    default:
      if (command_id >= IDC_FIRST_UNBOUNDED_MENU) {
        group = misc_metrics::MenuGroup::kBrowserViews;
      } else {
        return;
      }
  }
  menu_metrics_->RecordMenuGroupAction(group);
}

void BraveAppMenu::UpdateMenuItemView() {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  CHECK(root_menu_item());
  if (auto* menu_item =
          root_menu_item()->GetMenuItemByID(IDC_TOGGLE_BRAVE_VPN)) {
    menu_item->AddChildView(std::make_unique<BraveVPNStatusLabel>(browser_));
    menu_item->AddChildView(std::make_unique<BraveVPNToggleButton>(browser_));
  }
#endif

  if (auto* sidebar_item =
          root_menu_item()->GetMenuItemByID(IDC_SIDEBAR_SHOW_OPTION_MENU)) {
    // Find button model for Sidebar visibility
    size_t i = 0;
    for (; i < model_->GetItemCount(); i++) {
      if (model_->GetCommandIdAt(i) == IDC_SIDEBAR_SHOW_OPTION_MENU) {
        break;
      }
    }
    CHECK_LT(i, model_->GetItemCount());
    auto* sidebar_model = model_->GetButtonMenuItemAt(i);

    // Configure the menu item
    sidebar_item->SetTitle(sidebar_model->label());
    sidebar_item->set_children_use_full_width(true);
    sidebar_item->AddChildView(
        std::make_unique<SidebarShowOptionMenu>(this, sidebar_model));
  }
}
