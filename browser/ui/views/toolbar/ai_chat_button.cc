/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/ai_chat_button.h"

#include <memory>

#include "brave/app/brave_command_ids.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/views/accessibility/view_accessibility.h"

namespace {

class AIChatButtonMenuModel : public ui::SimpleMenuModel,
                              public ui::SimpleMenuModel::Delegate {
 public:
  explicit AIChatButtonMenuModel(PrefService* prefs)
      : ui::SimpleMenuModel(this), prefs_(*prefs) {
    Build();
  }

  AIChatButtonMenuModel(const AIChatButtonMenuModel&) = delete;
  AIChatButtonMenuModel& operator=(const AIChatButtonMenuModel&) = delete;
  ~AIChatButtonMenuModel() override = default;

 private:
  enum ContextMenuCommand { kHideAIChatButton };

  void Build() {
    AddItemWithStringId(ContextMenuCommand::kHideAIChatButton,
                        IDS_HIDE_BRAVE_AI_CHAT_ICON_ON_TOOLBAR);
  }

  // ui::SimpleMenuModel::Delegate:
  void ExecuteCommand(int command_id, int event_flags) override {
    if (command_id == ContextMenuCommand::kHideAIChatButton) {
      prefs_->SetBoolean(ai_chat::prefs::kBraveAIChatShowToolbarButton, false);
    }
  }

  raw_ref<PrefService> prefs_;
};

}  // namespace

AIChatButton::AIChatButton(Browser* browser)
    : ToolbarButton(base::BindRepeating(&AIChatButton::ButtonPressed,
                                        base::Unretained(this))),
      browser_(*browser) {
  auto* prefs = browser->profile()->GetOriginalProfile()->GetPrefs();
  SetMenuModel(std::make_unique<AIChatButtonMenuModel>(prefs));
  SetVectorIcon(kLeoProductBraveLeoIcon);

  // Visibility is managed by |ToolbarView|.
  SetVisible(false);

  SetTooltipText(l10n_util::GetStringUTF16(IDS_TOOLTIP_AI_CHAT_TOOLBAR_BUTTON));
  set_context_menu_controller(this);
  GetViewAccessibility().SetHasPopup(ax::mojom::HasPopup::kMenu);
}

AIChatButton::~AIChatButton() = default;

void AIChatButton::ButtonPressed() {
  chrome::ExecuteCommand(&browser_.get(), IDC_TOGGLE_AI_CHAT);

  ai_chat::AIChatMetrics* metrics =
      g_brave_browser_process->process_misc_metrics()->ai_chat_metrics();
  CHECK(metrics);
  metrics->HandleOpenViaEntryPoint(ai_chat::EntryPoint::kToolbarButton);
}

BEGIN_METADATA(AIChatButton)
END_METADATA
