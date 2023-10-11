/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_adblock_handler.h"

#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/json/values_util.h"
#include "base/values.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_adblock/resources/grit/brave_adblock_generated_map.h"
#include "brave/components/brave_shields/browser/ad_block_custom_filters_provider.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "content/public/browser/web_ui.h"
#include "ui/base/l10n/time_format.h"

BraveAdBlockHandler::BraveAdBlockHandler() = default;

BraveAdBlockHandler::~BraveAdBlockHandler() = default;

void BraveAdBlockHandler::RegisterMessages() {
  profile_ = Profile::FromWebUI(web_ui());
  web_ui()->RegisterMessageCallback(
      "brave_adblock.getRegionalLists",
      base::BindRepeating(&BraveAdBlockHandler::GetRegionalLists,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "brave_adblock.enableFilterList",
      base::BindRepeating(&BraveAdBlockHandler::EnableFilterList,
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
}

void BraveAdBlockHandler::OnJavascriptAllowed() {
  service_observer_.Observe(g_brave_browser_process->ad_block_service()
                                ->subscription_service_manager());
}

void BraveAdBlockHandler::OnJavascriptDisallowed() {
  service_observer_.Reset();
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
                            ->regional_service_manager()
                            ->GetRegionalLists();

  ResolveJavascriptCallback(args[0], regional_lists);
}

void BraveAdBlockHandler::EnableFilterList(const base::Value::List& args) {
  DCHECK_EQ(args.size(), 2U);

  if (!args[0].is_string() || !args[1].is_bool())
    return;

  std::string uuid = args[0].GetString();
  bool enabled = args[1].GetBool();

  g_brave_browser_process->ad_block_service()
      ->regional_service_manager()
      ->EnableFilterList(uuid, enabled);
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
  if (!args[0].is_string())
    return;

  std::string subscription_url_string = args[0].GetString();
  const GURL subscription_url = GURL(subscription_url_string);

  if (!subscription_url.is_valid())
    return;

  g_brave_browser_process->ad_block_service()
      ->subscription_service_manager()
      ->CreateSubscription(subscription_url);

  RefreshSubscriptionsList();
}

void BraveAdBlockHandler::SetSubscriptionEnabled(
    const base::Value::List& args) {
  DCHECK_EQ(args.size(), 2U);
  AllowJavascript();
  if (!args[0].is_string() || !args[1].is_bool())
    return;

  std::string subscription_url_string = args[0].GetString();
  bool enabled = args[1].GetBool();
  const GURL subscription_url = GURL(subscription_url_string);
  if (!subscription_url.is_valid())
    return;
  g_brave_browser_process->ad_block_service()
      ->subscription_service_manager()
      ->EnableSubscription(subscription_url, enabled);

  RefreshSubscriptionsList();
}

void BraveAdBlockHandler::UpdateSubscription(const base::Value::List& args) {
  DCHECK_EQ(args.size(), 1U);
  AllowJavascript();
  if (!args[0].is_string())
    return;

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
  if (!args[0].is_string())
    return;

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
  if (!args[0].is_string())
    return;

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
  if (!args[0].is_string())
    return;

  std::string custom_filters = args[0].GetString();
  g_brave_browser_process->ad_block_service()
      ->custom_filters_provider()
      ->UpdateCustomFilters(custom_filters);
}

void BraveAdBlockHandler::RefreshSubscriptionsList() {
  FireWebUIListener("brave_adblock.onGetListSubscriptions", GetSubscriptions());
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
             subscription.last_update_attempt.ToJsTime());
    dict.Set("last_successful_update_attempt",
             subscription.last_successful_update_attempt.ToJsTime());
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
