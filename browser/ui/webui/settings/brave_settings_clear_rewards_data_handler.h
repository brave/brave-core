/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SETTINGS_CLEAR_REWARDS_DATA_HANDLER_H_  // NOLINT
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SETTINGS_CLEAR_REWARDS_DATA_HANDLER_H_  // NOLINT

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "base/observer_list_types.h"
#include "base/scoped_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "components/browsing_data/core/counters/browsing_data_counter.h"

namespace base {
class ListValue;
}

namespace content {
class WebUI;
}

namespace settings {

enum class RewardsDataType {
  REWARDS_AUTO_CONTRIBUTE,
  REWARDS_ALL_DATA,
  NUM_TYPES
};

// Brave browser startup settings handler.
class ClearRewardsDataHandler : public SettingsPageUIHandler,
                                public brave_rewards::RewardsServiceObserver {
 public:
  explicit ClearRewardsDataHandler(content::WebUI* webui);
  ~ClearRewardsDataHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

 private:
  // Clears Rewards data, called by Javascript.
  void HandleClearRewardsData(const base::ListValue* value);

  // Called when a clearing task finished. |webui_callback_id| is provided
  // by the WebUI action that initiated it.
  void OnClearingTaskFinished(
      const std::string& webui_callback_id,
      const base::flat_set<RewardsDataType>& data_types);

  // Initializes the dialog UI. Called by JavaScript when the DOM is ready.
  void HandleInitialize(const base::ListValue* args);

  // Called when Clear Rewards dialog is opened to fetch value to
  // tell if contribution is in progress
  void HandleIsContributionInProgress(const base::ListValue* args);

  // Adds a Rewards data |counter|.
  void AddCounter(std::unique_ptr<browsing_data::BrowsingDataCounter> counter,
                  browsing_data::ClearBrowsingDataTab tab);

  // Updates a counter text according to the |result|.
  void UpdateCounterText(
      std::unique_ptr<browsing_data::BrowsingDataCounter::Result> result);

  // Updates enabled/disabled state according to Phase 1 of contribution
  // This value is pushed from native ledger and broadcast to an
  // already open dialog
  void UpdateContributionInProgress(bool contribution_in_progress);

  // Same as above except this is for when the dialog opens, the dialog
  // fetches the value
  void UpdateContributionInProgressPromise(
    const std::string& webui_callback_id,
    bool contribution_in_progress);

  void OnContributionInProgressChanged(
    brave_rewards::RewardsService* rewards_service,
    bool contribution_in_progress) override;

  // Cached profile corresponding to the WebUI of this handler.
  Profile* profile_;

  // Counters that calculate the data volume for individual data types.
  std::vector<std::unique_ptr<browsing_data::BrowsingDataCounter>> counters_;

  brave_rewards::RewardsService* rewards_service_;  // NOT OWNED

  // A weak pointer factory for asynchronous calls referencing this class.
  // The weak pointers are invalidated in |OnJavascriptDisallowed()| and
  // |HandleInitialize()| to cancel previously initiated tasks.
  base::WeakPtrFactory<ClearRewardsDataHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ClearRewardsDataHandler);
};

}  // namespace settings

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SETTINGS_CLEAR_REWARDS_DATA_HANDLER_H_