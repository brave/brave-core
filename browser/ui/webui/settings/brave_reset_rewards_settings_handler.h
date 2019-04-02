/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_RESET_REWARDS_SETTINGS_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_RESET_REWARDS_SETTINGS_HANDLER_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"

namespace base {
class ListValue;
}  // namespace base

class Profile;

namespace settings {

class BraveResetRewardsSettingsHandler : public SettingsPageUIHandler {
 public:
  explicit BraveResetRewardsSettingsHandler(Profile* profile);
  ~BraveResetRewardsSettingsHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override;

 protected:
  // Javascript callback to start clearing data.
  void HandleResetRewards(const base::ListValue* args);

 private:

  // Resets Rewards settings to default values.
  void ResetRewards(const std::string& callback_id);

  // Closes the dialog once all requested settings has been reset.
  void OnResetRewardsDone(std::string callback_id, bool success);

  Profile* const profile_;

  // Used to cancel callbacks when JavaScript becomes disallowed.
  base::WeakPtrFactory<BraveResetRewardsSettingsHandler>
      callback_weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BraveResetRewardsSettingsHandler);
};

}  // namespace settings

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_RESET_REWARDS_SETTINGS_HANDLER_H_  // NOLINT
