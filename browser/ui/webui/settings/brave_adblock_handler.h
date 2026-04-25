/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ADBLOCK_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ADBLOCK_HANDLER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_shields/content/browser/ad_block_subscription_service_manager.h"
#include "brave/components/brave_shields/content/browser/ad_block_subscription_service_manager_observer.h"
#include "brave/components/brave_shields/core/browser/ad_block_custom_resource_provider.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "components/prefs/pref_change_registrar.h"

class Profile;

class BraveAdBlockHandler
    : public settings::SettingsPageUIHandler,
      public brave_shields::AdBlockSubscriptionServiceManagerObserver,
      public brave_shields::AdBlockCustomResourceProvider::Observer {
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

  // brave_shields::AdBlockCustomResourceProvider::Observer:
  void OnCustomResourcesChanged() override;

  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

  void GetRegionalLists(const base::ListValue& args);
  void EnableFilterList(const base::ListValue& args);
  void UpdateFilterLists(const base::ListValue& args);
  void GetListSubscriptions(const base::ListValue& args);
  void GetCustomFilters(const base::ListValue& args);
  void AddSubscription(const base::ListValue& args);
  void SetSubscriptionEnabled(const base::ListValue& args);
  void UpdateSubscription(const base::ListValue& args);
  void DeleteSubscription(const base::ListValue& args);
  void ViewSubscriptionSource(const base::ListValue& args);
  void UpdateCustomFilters(const base::ListValue& args);
  void GetCustomScriptlets(const base::ListValue& args);
  void OnGetCustomScriptlets(const std::string& callback_id,
                             base::Value custom_resources);
  void AddCustomScriptlet(const base::ListValue& args);
  void UpdateCustomScriptlet(const base::ListValue& args);
  void RemoveCustomScriptlet(const base::ListValue& args);
  void OnScriptletUpdateStatus(
      const std::string& callback_id,
      brave_shields::AdBlockCustomResourceProvider::ErrorCode error_code);

  void RefreshSubscriptionsList();
  void RefreshCustomFilters();

  base::ListValue GetSubscriptions();

  void OnFilterListsUpdated(std::string callback_id, bool success);

  base::ScopedObservation<
      brave_shields::AdBlockSubscriptionServiceManager,
      brave_shields::AdBlockSubscriptionServiceManagerObserver>
      service_observer_{this};

  base::ScopedObservation<
      brave_shields::AdBlockCustomResourceProvider,
      brave_shields::AdBlockCustomResourceProvider::Observer>
      custom_resources_observer_{this};

  PrefChangeRegistrar pref_change_registrar_;

  base::WeakPtrFactory<BraveAdBlockHandler> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ADBLOCK_HANDLER_H_
