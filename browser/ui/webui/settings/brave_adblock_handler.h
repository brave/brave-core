/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ADBLOCK_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ADBLOCK_HANDLER_H_

#include "base/scoped_observation.h"

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_shields/browser/ad_block_subscription_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_subscription_service_manager_observer.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"

class Profile;
using brave_shields::AdBlockSubscriptionServiceManager;
using brave_shields::AdBlockSubscriptionServiceManagerObserver;

class BraveAdBlockHandler : public settings::SettingsPageUIHandler,
                            public AdBlockSubscriptionServiceManagerObserver {
 public:
  BraveAdBlockHandler();
  BraveAdBlockHandler(const BraveAdBlockHandler&) = delete;
  BraveAdBlockHandler& operator=(const BraveAdBlockHandler&) = delete;
  ~BraveAdBlockHandler() override;

 private:
  // SettingsPageUIHandler overrides
  void RegisterMessages() override;

  // brave_shields::AdblockSubscriptionServiceManagerObserver overrides:
  void OnServiceUpdateEvent() override;

  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

  void GetRegionalLists(const base::Value::List& args);
  void EnableFilterList(const base::Value::List& args);
  void GetListSubscriptions(const base::Value::List& args);
  void GetCustomFilters(const base::Value::List& args);
  void AddSubscription(const base::Value::List& args);
  void SetSubscriptionEnabled(const base::Value::List& args);
  void UpdateSubscription(const base::Value::List& args);
  void DeleteSubscription(const base::Value::List& args);
  void ViewSubscriptionSource(const base::Value::List& args);
  void UpdateCustomFilters(const base::Value::List& args);

  void RefreshSubscriptionsList();

  base::Value::List GetSubscriptions();

  raw_ptr<Profile> profile_ = nullptr;

  base::ScopedObservation<AdBlockSubscriptionServiceManager,
                          AdBlockSubscriptionServiceManagerObserver>
      service_observer_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ADBLOCK_HANDLER_H_
