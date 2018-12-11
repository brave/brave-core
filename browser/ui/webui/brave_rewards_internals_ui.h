/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_INTERNALS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_INTERNALS_UI_H_

#include "brave/browser/ui/webui/basic_ui.h"

namespace brave_rewards {
struct RewardsInternalsInfo;
class RewardsService;
}  // namespace brave_rewards

class BraveRewardsInternalsUI : public BasicUI {
 public:
  BraveRewardsInternalsUI(content::WebUI* web_ui, const std::string& host);
  ~BraveRewardsInternalsUI() override;

 private:
  // BasicUI overrides:
  void UpdateWebUIProperties() override;

  void OnGetRewardsInternalsInfo(
      std::unique_ptr<brave_rewards::RewardsInternalsInfo> info);

  brave_rewards::RewardsService* rewards_service_;  // NOT OWNED

  DISALLOW_COPY_AND_ASSIGN(BraveRewardsInternalsUI);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_INTERNALS_UI_H_
