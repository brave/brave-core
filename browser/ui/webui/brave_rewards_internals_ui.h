/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_INTERNALS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_INTERNALS_UI_H_

#include "brave/browser/ui/webui/basic_ui.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"

namespace brave_rewards {
struct RewardsInternalsInfo;
class RewardsService;
}  // namespace brave_rewards

class BraveRewardsInternalsUI : public BasicUI,
                                public brave_rewards::RewardsServiceObserver {
 public:
  BraveRewardsInternalsUI(content::WebUI* web_ui, const std::string& host);
  ~BraveRewardsInternalsUI() override;

 private:
  // BasicUI overrides:
  void UpdateWebUIProperties() override;

  // RewardsServiceObserver overrides:
  void OnRewardsInitialized(brave_rewards::RewardsService* rewards_service,
                            int error_code) override;
  void OnRewardsMainEnabled(brave_rewards::RewardsService* rewards_service,
                            bool rewards_main_enabled) override;

  void OnGetRewardsInternalsInfo(
      std::unique_ptr<brave_rewards::RewardsInternalsInfo> info);

  void CustomizeWebUIProperties(content::RenderViewHost* render_view_host);
  bool IsRewardsEnabled() const;

  Profile* profile_;
  brave_rewards::RewardsService* rewards_service_;  // NOT OWNED
  std::unique_ptr<brave_rewards::RewardsInternalsInfo> internals_info_;

  DISALLOW_COPY_AND_ASSIGN(BraveRewardsInternalsUI);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_INTERNALS_UI_H_
