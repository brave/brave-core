/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_settings_clear_rewards_data_handler.h"  // NOLINT

#include <stddef.h>
#include <vector>

#include "base/stl_util.h"
#include "base/values.h"
#include "brave/components/brave_rewards/browser/brave_rewards_data_remover_delegate.h"  // NOLINT
#include "brave/components/brave_rewards/browser/counters/rewards_data_counter_utils.h"  // NOLINT
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "chrome/browser/browsing_data/browsing_data_important_sites_util.h"
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_delegate_factory.h"  // NOLINT
#include "chrome/browser/browsing_data/counters/browsing_data_counter_factory.h"
#include "components/prefs/pref_member.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browsing_data_filter_builder.h"
#include "content/public/browser/web_ui.h"

namespace {

settings::RewardsDataType GetDataTypeFromDeletionPreference(
    const std::string& pref_name) {
  using DataTypeMap = base::flat_map<std::string, settings::RewardsDataType>;
  static base::NoDestructor<DataTypeMap> preference_to_datatype(
      std::initializer_list<DataTypeMap::value_type>{
          {brave_rewards::prefs::kRewardsAutoContributeSites,
              settings::RewardsDataType::REWARDS_AUTO_CONTRIBUTE},
          {brave_rewards::prefs::kRewardsAllData,
              settings::RewardsDataType::REWARDS_ALL_DATA},
      },
      base::KEEP_FIRST_OF_DUPES);

  auto iter = preference_to_datatype->find(pref_name);
  DCHECK(iter != preference_to_datatype->end());
  return iter->second;
}

const char* kCounterPrefs[] = {
  brave_rewards::prefs::kRewardsAutoContributeSites
};

} // namespace

namespace settings {

// ClearRewardsDataHandler ----------------------------------------------------

ClearRewardsDataHandler::ClearRewardsDataHandler(content::WebUI* webui)
    : profile_(Profile::FromWebUI(webui)),
      weak_ptr_factory_(this) {
  Profile* profile = Profile::FromWebUI(webui);
  rewards_service_ =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service_) {
    rewards_service_->AddObserver(this);
  }
}

ClearRewardsDataHandler::~ClearRewardsDataHandler() {
  if (rewards_service_) {
    rewards_service_->RemoveObserver(this);
  }
}

void ClearRewardsDataHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "clearRewardsData",
      base::BindRepeating(&ClearRewardsDataHandler::HandleClearRewardsData,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "initializeClearRewardsData",
      base::BindRepeating(&ClearRewardsDataHandler::HandleInitialize,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "isContributionInProgress",
      base::BindRepeating(
                &ClearRewardsDataHandler::HandleIsContributionInProgress,
                          base::Unretained(this)));
}

void ClearRewardsDataHandler::OnJavascriptAllowed() {
  DCHECK(counters_.empty());
  for (const std::string& pref : kCounterPrefs) {
    if (pref == brave_rewards::prefs::kRewardsAutoContributeSites) {
      AddCounter(std::make_unique<RewardsCounter>(profile_),
        browsing_data::ClearBrowsingDataTab::ADVANCED);
    }
  }
}

void ClearRewardsDataHandler::OnJavascriptDisallowed() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  counters_.clear();
}

void ClearRewardsDataHandler::HandleClearRewardsData(
    const base::ListValue* args) {
  CHECK_EQ(2U, args->GetSize());
  std::string webui_callback_id;
  CHECK(args->GetString(0, &webui_callback_id));

  int remove_mask = 0;
  std::vector<RewardsDataType> data_type_vector;
  const base::ListValue* data_type_list = nullptr;
  CHECK(args->GetList(1, &data_type_list));
  for (const base::Value& type : *data_type_list) {
    std::string pref_name;
    CHECK(type.GetAsString(&pref_name));
    RewardsDataType data_type =
        GetDataTypeFromDeletionPreference(pref_name);
    data_type_vector.push_back(data_type);

    switch (data_type) {
      case RewardsDataType::REWARDS_AUTO_CONTRIBUTE:
          remove_mask |=
            BraveRewardsDataRemoverDelegate::DATA_TYPE_REWARDS_AUTO_CONTRIBUTE;
        break;
      case RewardsDataType::REWARDS_ALL_DATA:
          remove_mask |=
            BraveRewardsDataRemoverDelegate::DATA_TYPE_REWARDS_ALL_DATA;
        break;
      case RewardsDataType::NUM_TYPES:
        NOTREACHED();
        break;
    }
  }

  base::flat_set<RewardsDataType> data_types(std::move(data_type_vector));

  content::BrowsingDataRemover* remover =
      content::BrowserContext::GetBrowsingDataRemover(profile_);

  base::OnceClosure callback =
      base::BindOnce(&ClearRewardsDataHandler::OnClearingTaskFinished,
                     weak_ptr_factory_.GetWeakPtr(), webui_callback_id,
                     std::move(data_types));

  browsing_data::TimePeriod time_period =
      static_cast<browsing_data::TimePeriod>(0);  // all data

  remover->SetEmbedderDelegate(
        static_cast<BraveRewardsDataRemoverDelegate*>(
        ChromeBrowsingDataRemoverDelegateFactory::GetForProfile(profile_)));

  browsing_data_important_sites_util::Remove(
      remove_mask, 0, time_period,
      content::BrowsingDataFilterBuilder::Create(
          content::BrowsingDataFilterBuilder::BLACKLIST),
      remover, std::move(callback));
}

void ClearRewardsDataHandler::OnClearingTaskFinished(
    const std::string& webui_callback_id,
    const base::flat_set<RewardsDataType>& data_types) {
  ResolveJavascriptCallback(base::Value(webui_callback_id), base::Value(true));
}

void ClearRewardsDataHandler::HandleInitialize(const base::ListValue* args) {
  AllowJavascript();
  const base::Value* callback_id;
  CHECK(args->Get(0, &callback_id));

  // Needed because WebUI doesn't handle renderer crashes. See crbug.com/610450.
  weak_ptr_factory_.InvalidateWeakPtrs();

  // Restart the counters each time the dialog is reopened.
  for (const auto& counter : counters_)
    counter->Restart();

  ResolveJavascriptCallback(*callback_id, base::Value() /* Promise<void> */);
}

void ClearRewardsDataHandler::AddCounter(
    std::unique_ptr<browsing_data::BrowsingDataCounter> counter,
    browsing_data::ClearBrowsingDataTab tab) {
  DCHECK(counter);
  counter->Init(profile_->GetPrefs(), tab,
                base::Bind(&ClearRewardsDataHandler::UpdateCounterText,
                           base::Unretained(this)));
  counters_.push_back(std::move(counter));
}

void ClearRewardsDataHandler::UpdateCounterText(
    std::unique_ptr<browsing_data::BrowsingDataCounter::Result> result) {
  FireWebUIListener(
      "update-counter-text", base::Value(result->source()->GetPrefName()),
      base::Value(rewards_data_counter_utils::GetBraveCounterTextFromResult(
          result.get(), profile_)));
}

void ClearRewardsDataHandler::UpdateContributionInProgress(
    bool contribution_in_progress) {
  AllowJavascript();
  FireWebUIListener(
    "update-contribution-in-progress", base::Value(contribution_in_progress));
}

void ClearRewardsDataHandler::UpdateContributionInProgressPromise(
    const std::string& webui_callback_id,
    bool contribution_in_progress) {
  ResolveJavascriptCallback(
      base::Value(webui_callback_id),
      base::Value(contribution_in_progress));
}

void ClearRewardsDataHandler::OnContributionInProgressChanged(
    brave_rewards::RewardsService* rewards_service,
    bool contribution_in_progress) {
  UpdateContributionInProgress(contribution_in_progress);
}

void ClearRewardsDataHandler::HandleIsContributionInProgress(
    const base::ListValue* args) {
  std::string webui_callback_id;
  CHECK(args->GetString(0, &webui_callback_id));
  brave_rewards::RewardsService* rewards_service =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile_);
  if (rewards_service) {
    rewards_service->IsContributionInProgress(
      base::Bind(&ClearRewardsDataHandler::UpdateContributionInProgressPromise,
                       weak_ptr_factory_.GetWeakPtr(), webui_callback_id));
  }
}

}  // namespace settings
