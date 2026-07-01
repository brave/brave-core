/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/side_panel_button.h"

#include <memory>

#include "base/check_deref.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/components/constants/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/menus/simple_menu_model.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/button/button_controller.h"

namespace {

class SidePanelMenuModel : public ui::SimpleMenuModel,
                           public ui::SimpleMenuModel::Delegate {
 public:
  explicit SidePanelMenuModel(PrefService* prefs)
      : ui::SimpleMenuModel(this), prefs_(*prefs) {
    Build();
  }

  SidePanelMenuModel(const SidePanelMenuModel&) = delete;
  SidePanelMenuModel& operator=(const SidePanelMenuModel&) = delete;
  ~SidePanelMenuModel() override = default;

 private:
  enum ContextMenuCommand { kHideSideBarButton };

  void Build() {
    AddItemWithStringId(ContextMenuCommand::kHideSideBarButton,
                        IDS_HIDE_SIDE_PANEL_TOOLBAR_BUTTON);
  }

  // ui::SimpleMenuModel::Delegate:
  void ExecuteCommand(int command_id, int event_flags) override {
    if (command_id == ContextMenuCommand::kHideSideBarButton) {
      prefs_->SetBoolean(kShowSidePanelButton, false);
    }
  }

  raw_ref<PrefService> prefs_;
};

}  // namespace

SidePanelButton::SidePanelButton(sidebar::SidebarController* sidebar_controller,
                                 PrefService* prefs)
    : ToolbarButton(base::BindRepeating(&SidePanelButton::ButtonPressed,
                                        base::Unretained(this))),
      sidebar_controller_(CHECK_DEREF(sidebar_controller)) {
  sidebar_service_observation_.Observe(
      sidebar_controller_->GetSidebarService());
  SetMenuModel(std::make_unique<SidePanelMenuModel>(prefs));

  sidebar_alignment_.Init(
      prefs::kSidePanelHorizontalAlignment, prefs,
      base::BindRepeating(&SidePanelButton::UpdateToolbarButtonIcon,
                          base::Unretained(this)));
  show_side_panel_button_.Init(
      kShowSidePanelButton, prefs,
      base::BindRepeating(&SidePanelButton::UpdateButtonVisibility,
                          base::Unretained(this)));

  UpdateToolbarButtonIcon();
  UpdateButtonVisibility();
  SetTooltipText(l10n_util::GetStringUTF16(IDS_TOOLTIP_SIDEBAR_SHOW));
  set_context_menu_controller(this);
  button_controller()->set_notify_action(
      views::ButtonController::NotifyAction::kOnPress);
  GetViewAccessibility().SetHasPopup(ax::mojom::HasPopup::kMenu);
}

SidePanelButton::~SidePanelButton() = default;

void SidePanelButton::OnShowSidebarOptionChanged(
    sidebar::SidebarService::ShowSidebarOption option) {
  UpdateButtonVisibility();
  UpdateButtonHighlight();
}

void SidePanelButton::ButtonPressed() {
  sidebar_controller_->ToggleSidebarPinning();
  UpdateButtonHighlight();
}

void SidePanelButton::UpdateToolbarButtonIcon() {
  SetVectorIcon(sidebar_alignment_.GetValue() ? kSidebarToolbarButtonRightIcon
                                              : kSidebarToolbarButtonIcon);
}

void SidePanelButton::UpdateButtonVisibility() {
  // We hide button when show always as pinning doesn't make sense
  // when always visible.
  const sidebar::SidebarService::ShowSidebarOption show_sidebar_option =
      sidebar_controller_->GetSidebarService()->GetSidebarShowOption();
  SetVisible(show_side_panel_button_.GetValue() &&
             show_sidebar_option !=
                 sidebar::SidebarService::ShowSidebarOption::kShowAlways);
}

void SidePanelButton::UpdateButtonHighlight() {
  SetHighlighted(sidebar_controller_->sidebar_pinned());
}

BEGIN_METADATA(SidePanelButton)
END_METADATA
