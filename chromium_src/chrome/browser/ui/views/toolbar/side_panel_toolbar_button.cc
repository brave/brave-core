// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/toolbar/side_panel_toolbar_button.h"

#include <memory>

#include "base/memory/raw_ptr.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/components/constants/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/views/context_menu_controller.h"

#define SidePanelToolbarButton SidePanelToolbarButton_ChromiumImpl

#include "src/chrome/browser/ui/views/toolbar/side_panel_toolbar_button.cc"

#undef SidePanelToolbarButton

namespace {
class SidePanelMenuModel : public ui::SimpleMenuModel,
                           public ui::SimpleMenuModel::Delegate {
 public:
  explicit SidePanelMenuModel(PrefService* prefs)
      : ui::SimpleMenuModel(this), prefs_(prefs) {
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
    if (command_id == ContextMenuCommand::kHideSideBarButton)
      prefs_->SetBoolean(kShowSidePanelButton, false);
  }

  raw_ptr<PrefService> prefs_ = nullptr;
};
}  // namespace

SidePanelToolbarButton::SidePanelToolbarButton(Browser* browser)
    : SidePanelToolbarButton_ChromiumImpl(browser) {
  auto* prefs = browser->profile()->GetOriginalProfile()->GetPrefs();

  SetMenuModel(std::make_unique<SidePanelMenuModel>(prefs));

  // Visibility is managed by |SideBarContainerView|.
  SetVisible(false);
  sidebar_alignment_.Init(
      prefs::kSidePanelHorizontalAlignment, prefs,
      base::BindRepeating(&SidePanelToolbarButton::UpdateButtonImage,
                          base::Unretained(this)));
  UpdateButtonImage();
}

SidePanelToolbarButton::~SidePanelToolbarButton() = default;

void SidePanelToolbarButton::UpdateButtonImage() {
  SetVectorIcon(sidebar_alignment_.GetValue() ? kSidebarToolbarButtonRightIcon
                                              : kSidebarToolbarButtonIcon);
}

BEGIN_METADATA(SidePanelToolbarButton)
END_METADATA
