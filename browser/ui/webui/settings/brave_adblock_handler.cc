/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_adblock_handler.h"

#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/json/values_util.h"
#include "base/values.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_adblock/resources/grit/brave_adblock_generated_map.h"
#include "brave/components/brave_shields/content/browser/ad_block_custom_filters_provider.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"
#include "brave/components/brave_shields/core/browser/ad_block_custom_resource_provider.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "content/public/browser/web_ui.h"
#include "ui/base/l10n/time_format.h"

BraveAdBlockHandler::BraveAdBlockHandler() = default;

BraveAdBlockHandler::~BraveAdBlockHandler() = default;

void BraveAdBlockHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "brave_adblock.getRegionalLists",
      base::BindRepeating(&BraveAdBlockHandler::GetRegionalLists,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "brave_adblock.enableFilterList",
      base::BindRepeating(&BraveAdBlockHandler::EnableFilterList,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "brave_adblock.updateFilterLists",
      base::BindRepeating(&BraveAdBlockHandler::UpdateFilterLists,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "brave_adblock.getListSubscriptions",
      base::BindRepeating(&BraveAdBlockHandler::GetListSubscriptions,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "brave_adblock.getCustomFilters",
      base::BindRepeating(&BraveAdBlockHandler::GetCustomFilters,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "brave_adblock.addSubscription",
      base::BindRepeating(&BraveAdBlockHandler::AddSubscription,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "brave_adblock.setSubscriptionEnabled",
      base::BindRepeating(&BraveAdBlockHandler::SetSubscriptionEnabled,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "brave_adblock.updateSubscription",
      base::BindRepeating(&BraveAdBlockHandler::UpdateSubscription,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "brave_adblock.deleteSubscription",
      base::BindRepeating(&BraveAdBlockHandler::DeleteSubscription,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "brave_adblock.viewSubscription",
      base::BindRepeating(&BraveAdBlockHandler::ViewSubscriptionSource,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "brave_adblock.updateCustomFilters",
      base::BindRepeating(&BraveAdBlockHandler::UpdateCustomFilters,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "brave_adblock.getCustomScriptlets",
      base::BindRepeating(&BraveAdBlockHandler::GetCustomScriptlets,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "brave_adblock.addCustomScriptlet",
      base::BindRepeating(&BraveAdBlockHandler::AddCustomScriptlet,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "brave_adblock.updateCustomScriptlet",
      base::BindRepeating(&BraveAdBlockHandler::UpdateCustomScriptlet,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "brave_adblock.removeCustomScriptlet",
      base::BindRepeating(&BraveAdBlockHandler::RemoveCustomScriptlet,
                          base::Unretained(this)));
  pref_change_registrar_.Init(g_browser_process->local_state());
}

void BraveAdBlockHandler::OnJavascriptAllowed() {
  service_observer_.Observe(g_brave_browser_process->ad_block_service()
                                ->subscription_service_manager());
  pref_change_registrar_.Add(
      brave_shields::prefs::kAdBlockCustomFilters,
      base::BindRepeating(&BraveAdBlockHandler::RefreshCustomFilters,
                          weak_factory_.GetWeakPtr()));
}

void BraveAdBlockHandler::OnJavascriptDisallowed() {
  service_observer_.Reset();
  pref_change_registrar_.RemoveAll();
}

void BraveAdBlockHandler::OnServiceUpdateEvent() {
  if (!IsJavascriptAllowed()) {
    return;
  }

  RefreshSubscriptionsList();
}

void BraveAdBlockHandler::GetRegionalLists(const base::Value::List& args) {
  AllowJavascript();
  auto regional_lists = g_brave_browser_process->ad_block_service()
                            ->component_service_manager()
                            ->GetRegionalLists();

  ResolveJavascriptCallback(args[0], regional_lists);
}

void BraveAdBlockHandler::EnableFilterList(const base::Value::List& args) {
  DCHECK_EQ(args.size(), 2U);

  if (!args[0].is_string() || !args[1].is_bool()) {
    return;
  }

  std::string uuid = args[0].GetString();
  bool enabled = args[1].GetBool();

  g_brave_browser_process->ad_block_service()
      ->component_service_manager()
      ->EnableFilterList(uuid, enabled);
}

void BraveAdBlockHandler::UpdateFilterLists(const base::Value::List& args) {
  AllowJavascript();

  DCHECK_EQ(args.size(), 1U);
  if (!args[0].is_string()) {
    return;
  }

  std::string callback_id = args[0].GetString();

  g_brave_browser_process->ad_block_service()
      ->component_service_manager()
      ->UpdateFilterLists(
          base::BindOnce(&BraveAdBlockHandler::OnFilterListsUpdated,
                         weak_factory_.GetWeakPtr(), std::move(callback_id)));
}

void BraveAdBlockHandler::GetListSubscriptions(const base::Value::List& args) {
  AllowJavascript();
  ResolveJavascriptCallback(args[0], GetSubscriptions());
}

void BraveAdBlockHandler::GetCustomFilters(const base::Value::List& args) {
  AllowJavascript();
  const std::string custom_filters = g_brave_browser_process->ad_block_service()
                                         ->custom_filters_provider()
                                         ->GetCustomFilters();

  ResolveJavascriptCallback(args[0], base::Value(custom_filters));
}

void BraveAdBlockHandler::AddSubscription(const base::Value::List& args) {
  DCHECK_EQ(args.size(), 1U);
  AllowJavascript();
  if (!args[0].is_string()) {
    return;
  }

  std::string subscription_url_string = args[0].GetString();
  const GURL subscription_url = GURL(subscription_url_string);

  if (!subscription_url.is_valid()) {
    return;
  }

  g_brave_browser_process->ad_block_service()
      ->subscription_service_manager()
      ->CreateSubscription(subscription_url);

  RefreshSubscriptionsList();
}

void BraveAdBlockHandler::SetSubscriptionEnabled(
    const base::Value::List& args) {
  DCHECK_EQ(args.size(), 2U);
  AllowJavascript();
  if (!args[0].is_string() || !args[1].is_bool()) {
    return;
  }

  std::string subscription_url_string = args[0].GetString();
  bool enabled = args[1].GetBool();
  const GURL subscription_url = GURL(subscription_url_string);
  if (!subscription_url.is_valid()) {
    return;
  }
  g_brave_browser_process->ad_block_service()
      ->subscription_service_manager()
      ->EnableSubscription(subscription_url, enabled);

  RefreshSubscriptionsList();
}

void BraveAdBlockHandler::UpdateSubscription(const base::Value::List& args) {
  DCHECK_EQ(args.size(), 1U);
  AllowJavascript();
  if (!args[0].is_string()) {
    return;
  }

  std::string subscription_url_string = args[0].GetString();
  const GURL subscription_url = GURL(subscription_url_string);

  if (!subscription_url.is_valid()) {
    return;
  }
  g_brave_browser_process->ad_block_service()
      ->subscription_service_manager()
      ->RefreshSubscription(subscription_url, true);
}

void BraveAdBlockHandler::DeleteSubscription(const base::Value::List& args) {
  DCHECK_EQ(args.size(), 1U);
  AllowJavascript();
  if (!args[0].is_string()) {
    return;
  }

  std::string subscription_url_string = args[0].GetString();
  const GURL subscription_url = GURL(subscription_url_string);
  if (!subscription_url.is_valid()) {
    return;
  }
  g_brave_browser_process->ad_block_service()
      ->subscription_service_manager()
      ->DeleteSubscription(subscription_url);

  RefreshSubscriptionsList();
}

void BraveAdBlockHandler::ViewSubscriptionSource(
    const base::Value::List& args) {
  DCHECK_EQ(args.size(), 1U);
  if (!args[0].is_string()) {
    return;
  }

  std::string subscription_url_string = args[0].GetString();
  const GURL subscription_url = GURL(subscription_url_string);
  if (!subscription_url.is_valid()) {
    return;
  }

  const GURL file_url = g_brave_browser_process->ad_block_service()
                            ->subscription_service_manager()
                            ->GetListTextFileUrl(subscription_url);

  auto* browser = chrome::FindBrowserWithTab(web_ui()->GetWebContents());
  ShowSingletonTabOverwritingNTP(browser, file_url);
}

void BraveAdBlockHandler::UpdateCustomFilters(const base::Value::List& args) {
  if (!args[0].is_string()) {
    return;
  }

  std::string custom_filters = args[0].GetString();
  g_brave_browser_process->ad_block_service()
      ->custom_filters_provider()
      ->UpdateCustomFilters(custom_filters);
}

void BraveAdBlockHandler::GetCustomScriptlets(const base::Value::List& args) {
  CHECK(base::FeatureList::IsEnabled(
      brave_shields::features::kCosmeticFilteringCustomScriptlets));
  CHECK(args.size() == 1u && args[0].is_string());

  g_brave_browser_process->ad_block_service()
      ->custom_resource_provider()
      ->GetCustomResources(
          base::BindOnce(&BraveAdBlockHandler::OnGetCustomScriptlets,
                         weak_factory_.GetWeakPtr(), args[0].GetString()));
}

void BraveAdBlockHandler::OnGetCustomScriptlets(const std::string& callback_id,
                                                base::Value custom_resources) {
  AllowJavascript();
  ResolveJavascriptCallback(callback_id, custom_resources);
}

void BraveAdBlockHandler::AddCustomScriptlet(const base::Value::List& args) {
  CHECK(base::FeatureList::IsEnabled(
      brave_shields::features::kCosmeticFilteringCustomScriptlets));
  CHECK(args.size() == 2u && args[0].is_string() && args[1].is_dict());

  g_brave_browser_process->ad_block_service()
      ->custom_resource_provider()
      ->AddResource(
          args[1],
          base::BindOnce(&BraveAdBlockHandler::OnScriptletUpdateStatus,
                         weak_factory_.GetWeakPtr(), args[0].GetString()));
}

void BraveAdBlockHandler::UpdateCustomScriptlet(const base::Value::List& args) {
  CHECK(base::FeatureList::IsEnabled(
      brave_shields::features::kCosmeticFilteringCustomScriptlets));
  CHECK(args.size() == 3u && args[0].is_string() && args[1].is_string() &&
        args[2].is_dict());

  g_brave_browser_process->ad_block_service()
      ->custom_resource_provider()
      ->UpdateResource(
          args[1].GetString(), args[2],
          base::BindOnce(&BraveAdBlockHandler::OnScriptletUpdateStatus,
                         weak_factory_.GetWeakPtr(), args[0].GetString()));
}

void BraveAdBlockHandler::RemoveCustomScriptlet(const base::Value::List& args) {
  CHECK(base::FeatureList::IsEnabled(
      brave_shields::features::kCosmeticFilteringCustomScriptlets));
  CHECK(args.size() == 2u && args[0].is_string() && args[1].is_string());

  g_brave_browser_process->ad_block_service()
      ->custom_resource_provider()
      ->RemoveResource(
          args[1].GetString(),
          base::BindOnce(&BraveAdBlockHandler::OnScriptletUpdateStatus,
                         weak_factory_.GetWeakPtr(), args[0].GetString()));
}

void BraveAdBlockHandler::OnScriptletUpdateStatus(
    const std::string& callback_id,
    brave_shields::AdBlockCustomResourceProvider::ErrorCode error_code) {
  AllowJavascript();
  ResolveJavascriptCallback(callback_id,
                            base::Value(static_cast<int>(error_code)));
}

void BraveAdBlockHandler::RefreshSubscriptionsList() {
  FireWebUIListener("brave_adblock.onGetListSubscriptions", GetSubscriptions());
}

void BraveAdBlockHandler::RefreshCustomFilters() {
  const std::string& custom_filters =
      g_brave_browser_process->ad_block_service()
          ->custom_filters_provider()
          ->GetCustomFilters();
  FireWebUIListener("brave_adblock.onCustomFiltersChanged",
                    base::Value(custom_filters));
}

base::Value::List BraveAdBlockHandler::GetSubscriptions() {
  auto list_subscriptions = g_brave_browser_process->ad_block_service()
                                ->subscription_service_manager()
                                ->GetSubscriptions();

  base::Value::List list_value;
  base::Time now = base::Time::Now();

  for (const auto& subscription : list_subscriptions) {
    base::Value::Dict dict;

    base::TimeDelta relative_time_delta =
        now - subscription.last_successful_update_attempt;

    auto time_str = ui::TimeFormat::Simple(
        ui::TimeFormat::Format::FORMAT_ELAPSED,
        ui::TimeFormat::Length::LENGTH_LONG, relative_time_delta);

    dict.Set("subscription_url", subscription.subscription_url.spec());
    dict.Set("enabled", subscription.enabled);
    dict.Set("last_update_attempt",
             subscription.last_update_attempt.InMillisecondsFSinceUnixEpoch());
    dict.Set("last_successful_update_attempt",
             subscription.last_successful_update_attempt
                 .InMillisecondsFSinceUnixEpoch());
    dict.Set("last_updated_pretty_text", time_str);
    if (subscription.homepage) {
      dict.Set("homepage", *subscription.homepage);
    }
    if (subscription.title && !subscription.title->empty()) {
      dict.Set("title", *subscription.title);
    } else {
      dict.Set("title", subscription.subscription_url.spec());
    }

    list_value.Append(std::move(dict));
  }

  return list_value;
}

void BraveAdBlockHandler::OnFilterListsUpdated(std::string callback_id,
                                               bool success) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  if (success) {
    ResolveJavascriptCallback(base::Value(callback_id), base::Value());
  } else {
    RejectJavascriptCallback(base::Value(callback_id), base::Value());
  }
}
