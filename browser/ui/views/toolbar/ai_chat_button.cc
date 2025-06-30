/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/ai_chat_button.h"

#include <memory>

#include "brave/app/brave_command_ids.h"
#include "brave/browser/misc_metrics/profile_misc_metrics_service.h"
#include "brave/browser/misc_metrics/profile_misc_metrics_service_factory.h"
#include "brave/browser/ui/brave_pages.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/constants/url_constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/menus/simple_menu_model.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "url/gurl.h"

AIChatButton::AIChatButton(Browser* browser)
    : ToolbarButton(base::BindRepeating(&AIChatButton::ButtonPressed,
                                        base::Unretained(this))),
      browser_(*browser),
      prefs_(*browser_->profile()->GetOriginalProfile()->GetPrefs()) {
  SetMenuModel(CreateMenuModel());

  SetVectorIcon(kLeoProductBraveLeoIcon);

  // Visibility is managed by |ToolbarView|.
  SetVisible(false);

  SetTooltipText(l10n_util::GetStringUTF16(IDS_TOOLTIP_AI_CHAT_TOOLBAR_BUTTON));
  set_context_menu_controller(this);
  GetViewAccessibility().SetHasPopup(ax::mojom::HasPopup::kMenu);
}

AIChatButton::~AIChatButton() = default;

void AIChatButton::ButtonPressed() {
  auto* prefs = browser_->profile()->GetOriginalProfile()->GetPrefs();
  if (prefs->GetBoolean(
          ai_chat::prefs::kBraveAIChatToolbarButtonOpensFullPage)) {
    brave::ShowFullpageChat(base::to_address(browser_));
  } else {
    chrome::ExecuteCommand(&browser_.get(), IDC_TOGGLE_AI_CHAT);
  }

  auto* profile_metrics =
      misc_metrics::ProfileMiscMetricsServiceFactory::GetServiceForContext(
          browser_->profile());
  if (!profile_metrics) {
    return;
  }
  auto* ai_chat_metrics = profile_metrics->GetAIChatMetrics();
  if (!ai_chat_metrics) {
    return;
  }
  ai_chat_metrics->HandleOpenViaEntryPoint(ai_chat::EntryPoint::kToolbarButton);
}

std::unique_ptr<ui::SimpleMenuModel> AIChatButton::CreateMenuModel() {
  auto model = std::make_unique<ui::SimpleMenuModel>(this);
  model->AddCheckItemWithStringId(ContextMenuCommand::kOpenInSidebar,
                                  IDS_OPEN_BRAVE_AI_CHAT_SIDE_PANEL);
  model->AddCheckItemWithStringId(ContextMenuCommand::kOpenInFullPage,
                                  IDS_OPEN_BRAVE_AI_CHAT_FULL_PAGE);
  model->AddSeparator(ui::NORMAL_SEPARATOR);
  model->AddItemWithStringId(ContextMenuCommand::kAboutLeoAI,
                             IDS_ABOUT_BRAVE_AI_CHAT);
  model->AddItemWithStringId(ContextMenuCommand::kManageLeoAI,
                             IDS_MANAGE_BRAVE_AI_CHAT);
  model->AddItemWithStringId(ContextMenuCommand::kHideAIChatButton,
                             IDS_HIDE_BRAVE_AI_CHAT_ICON_ON_TOOLBAR);
  return model;
}

void AIChatButton::ExecuteCommand(int command_id, int event_flags) {
  switch (command_id) {
    case ContextMenuCommand::kOpenInFullPage:
      prefs_->SetBoolean(ai_chat::prefs::kBraveAIChatToolbarButtonOpensFullPage,
                         true);
      break;
    case ContextMenuCommand::kOpenInSidebar:
      prefs_->SetBoolean(ai_chat::prefs::kBraveAIChatToolbarButtonOpensFullPage,
                         false);
      break;
    case ContextMenuCommand::kAboutLeoAI:
      ShowSingletonTab(base::to_address(browser_), GURL(kAIChatAboutUrl));
      break;
    case ContextMenuCommand::kManageLeoAI:
      ShowSingletonTab(base::to_address(browser_), GURL(kAIChatSettingsURL));
      break;
    case ContextMenuCommand::kHideAIChatButton:
      prefs_->SetBoolean(ai_chat::prefs::kBraveAIChatShowToolbarButton, false);
      break;
    default:
      NOTREACHED();
  }
}

bool AIChatButton::IsCommandIdChecked(int command_id) const {
  const bool open_in_full_page = prefs_->GetBoolean(
      ai_chat::prefs::kBraveAIChatToolbarButtonOpensFullPage);
  if (command_id == ContextMenuCommand::kOpenInFullPage) {
    return open_in_full_page;
  }

  if (command_id == ContextMenuCommand::kOpenInSidebar) {
    return !open_in_full_page;
  }

  NOTREACHED();
}

BEGIN_METADATA(AIChatButton)
END_METADATA
