// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_REWARDS_ADD_FUNDS_POPUP_H_
#define BRAVE_BROWSER_REWARDS_ADD_FUNDS_POPUP_H_

#include <map>
#include <string>

#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/widget/widget_observer.h"

namespace content {
class WebContents;
}

namespace views {
class Widget;
}

FORWARD_DECLARE_TEST(BraveAddFundsPopupTest, TestAddFundsPopupClosed);
FORWARD_DECLARE_TEST(BraveAddFundsPopupTest, TestAddFundsPopupDeleted);

namespace brave_rewards {

class AddFundsPopupContentSettings;
class RewardsService;

class AddFundsPopup : public views::WidgetObserver {
 public:
  AddFundsPopup();
  ~AddFundsPopup() override;

  // content::WidgetObserver implementation.
  void OnWidgetClosing(views::Widget* widget) override;

  // Show existing or open a new popup.
  void ShowPopup(content::WebContents* initiator,
    RewardsService* rewards_service);

  // RewardsService callback.
  void OnGetAddresses(const std::map<std::string, std::string>& addresses);

private:
  FRIEND_TEST_ALL_PREFIXES(::BraveAddFundsPopupTest, TestAddFundsPopupClosed);
  FRIEND_TEST_ALL_PREFIXES(::BraveAddFundsPopupTest, TestAddFundsPopupDeleted);
  // Popup management.
  void OpenPopup(const std::map<std::string, std::string>& addresses);
  void ClosePopup();
  gfx::Rect CalculatePopupWindowBounds(content::WebContents* initiator);
  void Focus();

  // Addreses handling.
  std::string GetAddressesAsJSON(
    const std::map<std::string, std::string>& addresses);
  std::string ToQueryString(const std::string& data);

  // Override Brave Shields to set needed content permissions.
  std::unique_ptr<AddFundsPopupContentSettings> EnsureContentPermissions(
      content::WebContents* initiator);

  // Hide Brave actions in the popup location bar.
  void HideBraveActions();

  // Popup content handle.
  content::WebContents* add_funds_popup_;

  // Content settings helper.
  std::unique_ptr<AddFundsPopupContentSettings> popup_content_settings_;

  // Rewards service to reload wallet info.
  brave_rewards::RewardsService* rewards_service_;  // NOT OWNED
  // Content that initiated popup creation.
  content::WebContents* initiator_;  // NOT OWNED

  base::WeakPtrFactory<AddFundsPopup> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AddFundsPopup);
};

} // namespace brav_rewards

#endif  // BRAVE_BROWSER_REWARDS_ADD_FUNDS_POPUP_H_
