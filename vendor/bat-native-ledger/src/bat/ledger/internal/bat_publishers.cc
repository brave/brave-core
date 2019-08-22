/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <cmath>
#include <ctime>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/bat_publishers.h"
#include "bat/ledger/internal/bignum.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/rapidjson_bat_helper.h"
#include "bat/ledger/internal/static_values.h"
#include "mojo/public/cpp/bindings/map.h"

/* foo.bar.example.com
   QLD = 'bar'
   RLD = 'foo.bar'
   SLD = 'example.com'
   TLD = 'com'

  search.yahoo.co.jp
   QLD = 'search'
   RLD = 'search'
   SLD = 'yahoo.co.jp'
   TLD = 'co.jp'
*/

using std::placeholders::_1;
using std::placeholders::_2;

namespace braveledger_bat_publishers {

BatPublishers::BatPublishers(bat_ledger::LedgerImpl* ledger):
  ledger_(ledger),
  state_(new braveledger_bat_helper::PUBLISHER_STATE_ST),
  server_list_(ledger::ServerPublisherInfoList()) {
  calcScoreConsts(state_->min_publisher_duration_);
}

BatPublishers::~BatPublishers() {
}

void BatPublishers::calcScoreConsts(const uint64_t& min_duration_seconds) {
  // we increase duration for 100 to keep it as close to muon implementation
  // as possible (we used 1000 in muon)
  // keeping it with only seconds visits are not spaced out equally
  uint64_t min_duration_big = min_duration_seconds * 100;
  a_ = (1.0 / (braveledger_ledger::_d * 2.0)) - min_duration_big;
  a2_ = a_ * 2.0;
  a4_ = a2_ * 2.0;
  b_ = min_duration_big - a_;
  b2_ = b_ * b_;
}

// courtesy of @dimitry-xyz:
// https://github.com/brave/ledger/issues/2#issuecomment-221752002
double BatPublishers::concaveScore(const uint64_t& duration_seconds) {
  uint64_t duration_big = duration_seconds * 100;
  return (-b_ + std::sqrt(b2_ + (a4_ * duration_big))) / a2_;
}

std::string getProviderName(const std::string& publisher_id) {
  // TODO(anyone) this is for the media stuff
  if (publisher_id.find(YOUTUBE_MEDIA_TYPE) != std::string::npos) {
    return YOUTUBE_MEDIA_TYPE;
  } else if (publisher_id.find(TWITCH_MEDIA_TYPE) != std::string::npos) {
    return TWITCH_MEDIA_TYPE;
  } else if (publisher_id.find(TWITTER_MEDIA_TYPE) != std::string::npos) {
    return TWITTER_MEDIA_TYPE;
  } else if (publisher_id.find(VIMEO_MEDIA_TYPE) != std::string::npos) {
    return VIMEO_MEDIA_TYPE;
  }

  return "";
}

bool ignoreMinTime(const std::string& publisher_id) {
  return !getProviderName(publisher_id).empty();
}

void BatPublishers::saveVisit(const std::string& publisher_id,
                              const ledger::VisitData& visit_data,
                              const uint64_t& duration,
                              uint64_t window_id,
                              const ledger::PublisherInfoCallback callback) {
  if (!ledger_->GetRewardsMainEnabled() || publisher_id.empty()) {
    return;
  }

  auto filter = CreateActivityFilter(publisher_id,
      ledger::ExcludeFilter::FILTER_ALL,
      false,
      ledger_->GetReconcileStamp(),
      true,
      false);

  ledger::PublisherInfoCallback callbackGetPublishers =
      std::bind(&BatPublishers::saveVisitInternal, this,
                publisher_id,
                visit_data,
                duration,
                window_id,
                callback,
                _1,
                _2);

  ledger_->GetActivityInfo(std::move(filter), callbackGetPublishers);
}

ledger::ActivityInfoFilterPtr BatPublishers::CreateActivityFilter(
    const std::string& publisher_id,
    ledger::ExcludeFilter excluded,
    bool min_duration,
    const uint64_t& current_reconcile_stamp,
    bool non_verified,
    bool min_visits) {
  auto filter = ledger::ActivityInfoFilter::New();
  filter->id = publisher_id;
  filter->excluded = excluded;
  filter->min_duration = min_duration ? getPublisherMinVisitTime() : 0;
  filter->reconcile_stamp = current_reconcile_stamp;
  filter->non_verified = non_verified;
  filter->min_visits = min_visits ? GetPublisherMinVisits() : 0;

  return filter;
}

std::string BatPublishers::GetBalanceReportName(
    const ledger::ACTIVITY_MONTH month,
    int year) {
  return std::to_string(year) + "_" + std::to_string(month);
}

void BatPublishers::saveVisitInternal(
    std::string publisher_id,
    const ledger::VisitData& visit_data,
    uint64_t duration,
    uint64_t window_id,
    const ledger::PublisherInfoCallback callback,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
  DCHECK(result != ledger::Result::TOO_MANY_RESULTS);
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }
  bool verified = isVerified(publisher_id);

  bool new_visit = false;
  if (!publisher_info) {
    new_visit = true;
    publisher_info = ledger::PublisherInfo::New();
    publisher_info->id = publisher_id;
  }

  std::string fav_icon = visit_data.favicon_url;
  if (verified && !fav_icon.empty()) {
    if (fav_icon.find(".invalid") == std::string::npos) {
    ledger_->FetchFavIcon(fav_icon,
                          "https://" + ledger_->GenerateGUID() + ".invalid",
                          std::bind(&BatPublishers::onFetchFavIcon,
                                    this,
                                    publisher_info->id,
                                    window_id,
                                    _1,
                                    _2));
    } else {
        publisher_info->favicon_url = fav_icon;
    }
  } else {
    publisher_info->favicon_url = ledger::kClearFavicon;
  }

  publisher_info->name = visit_data.name;
  publisher_info->provider = visit_data.provider;
  publisher_info->url = visit_data.url;
  publisher_info->verified = verified;

  bool excluded = isExcluded(
      publisher_info->id,
      static_cast<ledger::PUBLISHER_EXCLUDE>(publisher_info->excluded));
  bool ignore_time = ignoreMinTime(publisher_id);
  if (duration == 0) {
    ignore_time = false;
  }

  ledger::PublisherInfoPtr panel_info = nullptr;

  if (excluded) {
    publisher_info->excluded = ledger::PUBLISHER_EXCLUDE::EXCLUDED;
  }

  // for new visits that are excluded or are not long enough or ac is off
  bool min_duration_new = duration < getPublisherMinVisitTime() &&
      !ignore_time;
  bool min_duration_ok = duration > getPublisherMinVisitTime() || ignore_time;
  bool verified_new = !ledger_->GetPublisherAllowNonVerified() && !verified;
  bool verified_old = ((!ledger_->GetPublisherAllowNonVerified() && verified) ||
      ledger_->GetPublisherAllowNonVerified());

  if (new_visit &&
      (excluded ||
       !ledger_->GetAutoContribute() ||
       min_duration_new ||
       verified_new)) {
    panel_info = publisher_info->Clone();

    ledger_->SetPublisherInfo(std::move(publisher_info));
  } else if (!excluded &&
             ledger_->GetAutoContribute() &&
             min_duration_ok &&
             verified_old) {
    publisher_info->visits += 1;
    publisher_info->duration += duration;
    publisher_info->score += concaveScore(duration);
    publisher_info->reconcile_stamp = ledger_->GetReconcileStamp();

    panel_info = publisher_info->Clone();

    ledger_->SetActivityInfo(std::move(publisher_info));
  }

  if (panel_info) {
    if (panel_info->favicon_url == ledger::kClearFavicon) {
      panel_info->favicon_url = std::string();
    }

    auto callback_info = panel_info->Clone();
    callback(ledger::Result::LEDGER_OK, std::move(callback_info));

    if (window_id > 0) {
      OnPanelPublisherInfo(ledger::Result::LEDGER_OK,
                           std::move(panel_info),
                           window_id,
                           visit_data);
    }
  }
}

void BatPublishers::onFetchFavIcon(const std::string& publisher_key,
                                   uint64_t window_id,
                                   bool success,
                                   const std::string& favicon_url) {
  if (!success || favicon_url.empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_WARNING) <<
      "Missing or corrupted favicon file for: " << publisher_key;
    return;
  }

  ledger_->GetPublisherInfo(publisher_key,
      std::bind(&BatPublishers::onFetchFavIconDBResponse,
                this,
                _1,
                _2,
                favicon_url,
                window_id));
}

void BatPublishers::onFetchFavIconDBResponse(
    ledger::Result result,
    ledger::PublisherInfoPtr info,
    const std::string& favicon_url,
    uint64_t window_id) {
  if (result == ledger::Result::LEDGER_OK && !favicon_url.empty()) {
    info->favicon_url = favicon_url;

    ledger::PublisherInfoPtr panel_info = info->Clone();

    ledger_->SetPublisherInfo(std::move(info));

    if (window_id > 0) {
      ledger::VisitData visit_data;
      OnPanelPublisherInfo(ledger::Result::LEDGER_OK,
                          std::move(panel_info),
                          window_id,
                          visit_data);
    }
  } else {
    BLOG(ledger_, ledger::LogLevel::LOG_WARNING) <<
      "Missing or corrupted favicon file";
  }
}

void BatPublishers::OnPublisherInfoSaved(
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
  if (result != ledger::Result::LEDGER_OK || !info) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Publisher info was not saved!";
  }

  SynopsisNormalizer();
}

void BatPublishers::SetPublisherExclude(
    const std::string& publisher_id,
    const ledger::PUBLISHER_EXCLUDE& exclude,
    ledger::SetPublisherExcludeCallback callback) {
  ledger_->GetPublisherInfo(
    publisher_id,
    std::bind(&BatPublishers::OnSetPublisherExclude,
              this,
              exclude,
              _1,
              _2,
              callback));
}

void BatPublishers::OnSetPublisherExclude(
    ledger::PUBLISHER_EXCLUDE exclude,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info,
    ledger::SetPublisherExcludeCallback callback) {
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    callback(result);
    return;
  }

  if (!publisher_info || publisher_info->excluded == exclude) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  publisher_info->excluded = exclude;
  ledger_->SetPublisherInfo(publisher_info->Clone());
  if (exclude == ledger::PUBLISHER_EXCLUDE::EXCLUDED) {
    ledger_->DeleteActivityInfo(
      publisher_info->id,
      [](ledger::Result _){});
  }
  callback(ledger::Result::LEDGER_OK);
}

void BatPublishers::OnRestorePublishers(
    const ledger::Result result,
    ledger::RestorePublishersCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
    << "Could not restore publishers.";
    callback(result);
    return;
  }

  SynopsisNormalizer();
  callback(ledger::Result::LEDGER_OK);
}

// In seconds
void BatPublishers::setPublisherMinVisitTime(const uint64_t& duration) {
  state_->min_publisher_duration_ = duration;
  calcScoreConsts(duration);
  SynopsisNormalizer();
  saveState();
}

void BatPublishers::setPublisherMinVisits(const unsigned int visits) {
  state_->min_visits_ = visits;
  SynopsisNormalizer();
  saveState();
}

void BatPublishers::setPublishersLastRefreshTimestamp(uint64_t ts) {
  state_->pubs_load_timestamp_ = ts;
  saveState();
}

void BatPublishers::setPublisherAllowNonVerified(const bool& allow) {
  state_->allow_non_verified_ = allow;
  SynopsisNormalizer();
  saveState();
}

void BatPublishers::setPublisherAllowVideos(const bool& allow) {
  state_->allow_videos_ = allow;
  SynopsisNormalizer();
  saveState();
}

uint64_t BatPublishers::getPublisherMinVisitTime() const {
  return state_->min_publisher_duration_;
}

unsigned int BatPublishers::GetPublisherMinVisits() const {
  return state_->min_visits_;
}

bool BatPublishers::getPublisherAllowNonVerified() const {
  return state_->allow_non_verified_;
}

uint64_t BatPublishers::getLastPublishersListLoadTimestamp() const {
  return state_->pubs_load_timestamp_;
}

bool BatPublishers::getPublisherAllowVideos() const {
  return state_->allow_videos_;
}

bool BatPublishers::GetMigrateScore() const {
  return state_->migrate_score_2;
}

void BatPublishers::SetMigrateScore(bool value) {
  state_->migrate_score_2 = value;
  saveState();
}

void BatPublishers::NormalizeContributeWinners(
    ledger::PublisherInfoList* newList,
    const ledger::PublisherInfoList* list,
    uint32_t record) {

  synopsisNormalizerInternal(newList, list, record);
}

void BatPublishers::synopsisNormalizerInternal(
    ledger::PublisherInfoList* newList,
    const ledger::PublisherInfoList* list,
    uint32_t /* next_record */) {
  if (list->size() == 0) {
    return;
  }

  double totalScores = 0.0;
  for (size_t i = 0; i < list->size(); i++) {
    // Check which would test uint problem from this issue
    // https://github.com/brave/brave-browser/issues/3134
    if (GetMigrateScore()) {
      (*list)[i]->score = concaveScore((*list)[i]->duration);
    }
    totalScores += (*list)[i]->score;
  }

  if (GetMigrateScore()) {
    SetMigrateScore(false);
  }

  std::vector<unsigned int> percents;
  std::vector<double> weights;
  std::vector<double> realPercents;
  std::vector<double> roundoffs;
  unsigned int totalPercents = 0;
  for (size_t i = 0; i < list->size(); i++) {
    double floatNumber = ((*list)[i]->score / totalScores) * 100.0;
    double roundNumber = (unsigned int)std::lround(floatNumber);
    realPercents.push_back(floatNumber);
    percents.push_back(roundNumber);
    double roundoff = roundNumber - floatNumber;
    if (roundoff < 0.0) {
      roundoff *= -1.0;
    }
    roundoffs.push_back(roundoff);
    totalPercents += roundNumber;
    weights.push_back(floatNumber);
  }
  while (totalPercents != 100) {
    size_t valueToChange = 0;
    double currentRoundOff = 0.0;
    for (size_t i = 0; i < percents.size(); i++) {
      if (i == 0) {
        currentRoundOff = roundoffs[i];
        continue;
      }
      if (roundoffs[i] > currentRoundOff) {
        currentRoundOff = roundoffs[i];
        valueToChange = i;
      }
    }
    if (percents.size() != 0) {
      if (totalPercents > 100) {
        if (percents[valueToChange] != 0) {
          percents[valueToChange] -= 1;
          totalPercents -= 1;
        }
      } else {
        if (percents[valueToChange] != 100) {
          percents[valueToChange] += 1;
          totalPercents += 1;
        }
      }
      roundoffs[valueToChange] = 0;
    }
  }
  size_t currentValue = 0;
  for (size_t i = 0; i < list->size(); i++) {
    (*list)[i]->percent = percents[currentValue];
    (*list)[i]->weight = weights[currentValue];
    currentValue++;
    if (newList) {
      newList->push_back((*list)[i]->Clone());
    }
  }
}

void BatPublishers::SynopsisNormalizer() {
  auto filter = CreateActivityFilter("",
      ledger::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED,
      true,
      ledger_->GetReconcileStamp(),
      ledger_->GetPublisherAllowNonVerified(),
      ledger_->GetPublisherMinVisits());
  // TODO(SZ): We pull the whole list currently,
  // I don't think it consumes lots of RAM, but could.
  // We need to limit it and iterate.
  ledger_->GetActivityInfoList(
      0,
      0,
      std::move(filter),
      std::bind(&BatPublishers::SynopsisNormalizerCallback, this, _1, _2));
}

void BatPublishers::SynopsisNormalizerCallback(
    ledger::PublisherInfoList list,
    uint32_t record) {
  ledger::PublisherInfoList normalized_list;
  synopsisNormalizerInternal(&normalized_list, &list, 0);
  ledger_->SaveNormalizedPublisherList(std::move(normalized_list));
}

bool BatPublishers::isVerified(const std::string& publisher_id) {
  if (server_list_.empty()) {
    return false;
  }

  // TODO ADD SQL QUERY
//  auto result = server_list_.find(publisher_id);
//
//  if (result == server_list_.end()) {
//    return false;
//  }
//
//  if (!result->second) {
//    return false;
//  }
//
//  return result->second->verified;
  return false;
}

bool BatPublishers::isExcluded(const std::string& publisher_id,
                               const ledger::PUBLISHER_EXCLUDE& excluded) {
  // If exclude is set to 1, we should avoid further computation and return true
  if (excluded == ledger::PUBLISHER_EXCLUDE::EXCLUDED) {
    return true;
  }
  // TODO ADD SQL QUERY
//  if (excluded == ledger::PUBLISHER_EXCLUDE::INCLUDED || server_list_.empty()) {
//    return false;
//  }
//
//  auto result = server_list_.find(publisher_id);
//
//  if (result == server_list_.end()) {
//    return false;
//  }
//  if (!result->second) {
//    return false;
//  }
//
//  return result->second->excluded;
  return false;
}

void BatPublishers::clearAllBalanceReports() {
  if (state_->monthly_balances_.empty()) {
    return;
  }
  state_->monthly_balances_.clear();
  saveState();
}

void BatPublishers::setBalanceReport(ledger::ACTIVITY_MONTH month,
                                int year,
                                const ledger::BalanceReportInfo& report_info) {
  braveledger_bat_helper::REPORT_BALANCE_ST report_balance;
  report_balance.opening_balance_ = report_info.opening_balance;
  report_balance.closing_balance_ = report_info.closing_balance;
  report_balance.grants_ = report_info.grants;
  report_balance.deposits_ = report_info.deposits;
  report_balance.earning_from_ads_ = report_info.earning_from_ads;
  report_balance.recurring_donation_ = report_info.recurring_donation;
  report_balance.one_time_donation_ = report_info.one_time_donation;
  report_balance.auto_contribute_ = report_info.auto_contribute;

  std::string total = "0";
  total = braveledger_bat_bignum::sum(total, report_balance.grants_);
  total = braveledger_bat_bignum::sum(total, report_balance.earning_from_ads_);
  total = braveledger_bat_bignum::sum(total, report_balance.deposits_);
  total = braveledger_bat_bignum::sub(total, report_balance.auto_contribute_);
  total = braveledger_bat_bignum::sub(total,
                                      report_balance.recurring_donation_);
  total = braveledger_bat_bignum::sub(total, report_balance.one_time_donation_);

  report_balance.total_ = total;
  state_->monthly_balances_[GetBalanceReportName(month, year)] = report_balance;
  saveState();
}

bool BatPublishers::getBalanceReport(ledger::ACTIVITY_MONTH month,
                                     int year,
                                     ledger::BalanceReportInfo* report_info) {
  std::string name = GetBalanceReportName(month, year);
  auto iter = state_->monthly_balances_.find(name);
  if (!report_info) {
    return false;
  }

  if (iter == state_->monthly_balances_.end()) {
    ledger::BalanceReportInfo new_report_info;
    new_report_info.opening_balance = "0";
    new_report_info.closing_balance = "0";
    new_report_info.grants = "0";
    new_report_info.earning_from_ads = "0";
    new_report_info.auto_contribute = "0";
    new_report_info.recurring_donation = "0";
    new_report_info.one_time_donation = "0";
    new_report_info.total = "0";

    setBalanceReport(month, year, new_report_info);
    bool successGet = getBalanceReport(month, year, report_info);
    if (successGet) {
      iter = state_->monthly_balances_.find(name);
    } else {
      return false;
    }
  }

  report_info->opening_balance = iter->second.opening_balance_;
  report_info->closing_balance = iter->second.closing_balance_;
  report_info->grants = iter->second.grants_;
  report_info->earning_from_ads = iter->second.earning_from_ads_;
  report_info->auto_contribute = iter->second.auto_contribute_;
  report_info->recurring_donation = iter->second.recurring_donation_;
  report_info->one_time_donation = iter->second.one_time_donation_;

  return true;
}

std::map<std::string, ledger::BalanceReportInfoPtr>
BatPublishers::GetAllBalanceReports() {
  std::map<std::string, ledger::BalanceReportInfoPtr> newReports;
  for (auto const& report : state_->monthly_balances_) {
    ledger::BalanceReportInfoPtr newReport = ledger::BalanceReportInfo::New();
    const braveledger_bat_helper::REPORT_BALANCE_ST oldReport = report.second;
    newReport->opening_balance = oldReport.opening_balance_;
    newReport->closing_balance = oldReport.closing_balance_;
    newReport->grants = oldReport.grants_;
    newReport->earning_from_ads = oldReport.earning_from_ads_;
    newReport->auto_contribute = oldReport.auto_contribute_;
    newReport->recurring_donation = oldReport.recurring_donation_;
    newReport->one_time_donation = oldReport.one_time_donation_;

    newReports[report.first] = std::move(newReport);
  }

  return newReports;
}

void BatPublishers::saveState() {
  std::string data;
  braveledger_bat_helper::saveToJsonString(*state_, &data);
  ledger_->SavePublisherState(data, this);
}

bool BatPublishers::loadState(const std::string& data) {
  braveledger_bat_helper::PUBLISHER_STATE_ST state;
  if (!braveledger_bat_helper::loadFromJson(&state, data.c_str()))
    return false;

  state_.reset(new braveledger_bat_helper::PUBLISHER_STATE_ST(state));
  calcScoreConsts(state_->min_publisher_duration_);
  return true;
}

void BatPublishers::OnPublisherStateSaved(ledger::Result result) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Could not save publisher state";
    // TODO(anyone) error handling
    return;
  }
}

void BatPublishers::RefreshPublishersList(const std::string& json) {
  // TODO remove as it's in db now
  ledger_->SavePublishersList(json);

  const auto callback =
      std::bind(&BatPublishers::OnParsePublisherList, this, _1);
  ParsePublisherList(json, callback);
}

void BatPublishers::OnParsePublisherList(const ledger::Result result) {
  if (result == ledger::Result::LEDGER_OK) {
    ledger_->ContributeUnverifiedPublishers();
  }
}

void BatPublishers::OnPublishersListSaved(ledger::Result result) {
  uint64_t ts = (ledger::Result::LEDGER_OK == result)
                ? std::time(nullptr)
                : 0ull;
  setPublishersLastRefreshTimestamp(ts);
}

void BatPublishers::ParsePublisherList(
    const std::string& data,
    ParsePublisherListCallback callback) {
  ledger::ServerPublisherInfoList list;

  base::Optional<base::Value> value = base::JSONReader::Read(data);
  if (!value || !value->is_list()) {
    callback(ledger::Result::LEDGER_ERROR);
  }

  base::ListValue* publishers = nullptr;
  if (!value->GetAsList(&publishers)) {
    callback(ledger::Result::LEDGER_ERROR);
  }

  for (auto& item : *publishers) {
    base::ListValue* values = nullptr;
    if (!item.GetAsList(&values)) {
      callback(ledger::Result::LEDGER_ERROR);
    }

    auto publisher = ledger::ServerPublisherInfo::New();

    // Publisher key
    std::string publisher_key = "";
    if (!values->GetList()[0].is_string()) {
      continue;
    }
    publisher_key = values->GetList()[0].GetString();

    if (publisher_key.empty()) {
      continue;
    }

    publisher->publisher_key = publisher_key;

    // Verified
    if (!values->GetList()[1].is_bool()) {
      continue;
    }
    publisher->verified = values->GetList()[1].GetBool();

    // Excluded
    if (!values->GetList()[2].is_bool()) {
      continue;
    }
    publisher->excluded = values->GetList()[2].GetBool();

    // Address
    if (!values->GetList()[3].is_string()) {
      continue;
    }
    publisher->address = values->GetList()[3].GetString();

    // Banner
    base::DictionaryValue* banner = nullptr;
    if (values->GetList()[4].GetAsDictionary(&banner)) {
      publisher->banner = ParsePublisherBanner(banner);
    }

    list.push_back(std::move(publisher));
  }

  if (list.size() == 0) {
    callback(ledger::Result::LEDGER_ERROR);
  }

  ledger_->ClearAndInsertServerPublisherList(std::move(list), callback);
}

ledger::PublisherBannerPtr BatPublishers::ParsePublisherBanner(
    base::DictionaryValue* dictionary) {
  if (!dictionary->is_dict()) {
    return nullptr;
  }

  auto banner = ledger::PublisherBanner::New();

  auto* title = dictionary->FindKey("title");
  if (title && title->is_string()) {
    banner->title = title->GetString();
  }

  auto* description = dictionary->FindKey("description");
  if (description && description->is_string()) {
    banner->description = description->GetString();
  }

  auto* background = dictionary->FindKey("backgroundUrl");
  if (background && background->is_string()) {
    banner->background = background->GetString();

    if (!banner->background.empty()) {
      banner->background = "chrome://rewards-image/" + banner->background;
    }
  }

  auto* logo = dictionary->FindKey("logoUrl");
  if (logo && logo->is_string()) {
    banner->logo = logo->GetString();

    if (!banner->logo.empty()) {
      banner->logo = "chrome://rewards-image/" + banner->logo;
    }
  }

  auto* amounts = dictionary->FindKey("donationAmounts");
  if (amounts && amounts->is_list()) {
    for (const auto& it : amounts->GetList()) {
      banner->amounts.push_back(it.GetInt());
    }
  }

  auto* social = dictionary->FindKey("socialLinks");
  if (social && social->is_dict()) {
    for (const auto& it : social->DictItems()) {
      banner->social.insert(std::make_pair(it.first, it.second.GetString()));
    }
  }

  return banner;
}

void BatPublishers::getPublisherActivityFromUrl(
    uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& publisher_blob) {
  if (!ledger_->GetRewardsMainEnabled()) {
    return;
  }

  const bool is_media = visit_data.domain == YOUTUBE_TLD ||
                        visit_data.domain == TWITCH_TLD ||
                        visit_data.domain == TWITTER_TLD ||
                        visit_data.domain == REDDIT_TLD ||
                        visit_data.domain == VIMEO_TLD ||
                        visit_data.domain == GITHUB_TLD;

  if (is_media &&
      visit_data.path != "" && visit_data.path != "/") {
    std::string type = YOUTUBE_MEDIA_TYPE;
    if (visit_data.domain == TWITCH_TLD) {
      type = TWITCH_MEDIA_TYPE;
    } else if (visit_data.domain == TWITTER_TLD) {
      type = TWITTER_MEDIA_TYPE;
    } else if (visit_data.domain == REDDIT_TLD) {
      type = REDDIT_MEDIA_TYPE;
    } else if (visit_data.domain == VIMEO_TLD) {
      type = VIMEO_MEDIA_TYPE;
    } else if (visit_data.domain == GITHUB_TLD) {
      type = GITHUB_MEDIA_TYPE;
    }

    ledger::VisitDataPtr new_visit_data = ledger::VisitData::New(visit_data);

    if (!new_visit_data->url.empty()) {
      new_visit_data->url.pop_back();
    }

    new_visit_data->url += new_visit_data->path;

    ledger_->GetMediaActivityFromUrl(windowId,
                                     std::move(new_visit_data),
                                     type,
                                     publisher_blob);
    return;
  }

  auto filter = CreateActivityFilter(visit_data.domain,
        ledger::ExcludeFilter::FILTER_ALL,
        false,
        ledger_->GetReconcileStamp(),
        true,
        false);

  ledger::VisitData new_data;
  new_data.domain = visit_data.domain;
  new_data.path = visit_data.path;
  new_data.name = visit_data.name;
  new_data.url = visit_data.url;
  new_data.favicon_url = "";

  ledger_->GetPanelPublisherInfo(std::move(filter),
        std::bind(&BatPublishers::OnPanelPublisherInfo,
                  this,
                  _1,
                  _2,
                  windowId,
                  new_data));
}

void BatPublishers::OnSaveVisitInternal(
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
  // TODO(nejczdovc): handle if needed
}

void BatPublishers::OnPanelPublisherInfo(
    ledger::Result result,
    ledger::PublisherInfoPtr info,
    uint64_t windowId,
    const ledger::VisitData& visit_data) {
  if (result == ledger::Result::LEDGER_OK) {
    ledger_->OnPanelPublisherInfo(result, std::move(info), windowId);
  }

  if (result == ledger::Result::NOT_FOUND && !visit_data.domain.empty()) {
    auto callback = std::bind(&BatPublishers::OnSaveVisitInternal,
                              this,
                              _1,
                              _2);

    saveVisitInternal(visit_data.domain,
                      visit_data,
                      0,
                      windowId,
                      callback,
                      result,
                      nullptr);
  }
}

void BatPublishers::setBalanceReportItem(ledger::ACTIVITY_MONTH month,
                                         int year,
                                         ledger::ReportType type,
                                         const std::string& probi) {
  ledger::BalanceReportInfo report_info;
  getBalanceReport(month, year, &report_info);

  switch (type) {
    case ledger::ReportType::GRANT:
      report_info.grants =
          braveledger_bat_bignum::sum(report_info.grants, probi);
      break;
    case ledger::ReportType::ADS:
      report_info.earning_from_ads =
          braveledger_bat_bignum::sum(report_info.earning_from_ads, probi);
      break;
    case ledger::ReportType::AUTO_CONTRIBUTION:
      report_info.auto_contribute =
          braveledger_bat_bignum::sum(report_info.auto_contribute, probi);
      break;
    case ledger::ReportType::TIP:
      report_info.one_time_donation =
          braveledger_bat_bignum::sum(report_info.one_time_donation, probi);
      break;
    case ledger::ReportType::TIP_RECURRING:
      report_info.recurring_donation =
          braveledger_bat_bignum::sum(report_info.recurring_donation, probi);
      break;
    default:
      break;
  }

  setBalanceReport(month, year, report_info);
}

void BatPublishers::GetPublisherBanner(
    const std::string& publisher_id,
    ledger::PublisherBannerCallback callback) {
  ledger::PublisherBannerPtr banner = ledger::PublisherBanner::New();

//  if (!server_list_.empty()) {
//    auto result = server_list_.find(publisher_id);
//
//    if (result != server_list_.end()) {
//      if (result->second && result->second->banner) {
//        banner = result->second->banner->Clone();
//      }
//    }
//  }

  banner->publisher_key = publisher_id;
  banner->verified = isVerified(publisher_id);

  ledger::PublisherInfoCallback callbackGetPublisher =
      std::bind(&BatPublishers::OnPublisherBanner,
                this,
                callback,
                *banner,
                _1,
                _2);

  ledger_->GetPublisherInfo(publisher_id, callbackGetPublisher);
}

void BatPublishers::OnPublisherBanner(
    ledger::PublisherBannerCallback callback,
    const ledger::PublisherBanner& banner,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {

  auto new_banner = ledger::PublisherBanner::New(banner);

  if (!publisher_info || result != ledger::Result::LEDGER_OK) {
    callback(std::move(new_banner));
    return;
  }

  new_banner->name = publisher_info->name;
  new_banner->provider = publisher_info->provider;

  if (new_banner->logo.empty()) {
    new_banner->logo = publisher_info->favicon_url;
  }

  callback(std::move(new_banner));
}

void BatPublishers::RefreshPublisherVerifiedStatus(
    const std::string& publisher_key,
    ledger::OnRefreshPublisherCallback callback) {
  callback(isVerified(publisher_key));
}

void BatPublishers::SavePublisherProcessed(const std::string& publisher_key) {
  const std::vector<std::string> list = state_->processed_pending_publishers;
  if (std::find(list.begin(), list.end(), publisher_key) == list.end()) {
    state_->processed_pending_publishers.push_back(publisher_key);
  }
  saveState();
}

bool BatPublishers::WasPublisherAlreadyProcessed(
    const std::string& publisher_key) const {
  const std::vector<std::string> list = state_->processed_pending_publishers;
  return std::find(list.begin(), list.end(), publisher_key) != list.end();
}

std::string BatPublishers::GetPublisherAddress(
    const std::string& publisher_key) const {
  if (server_list_.empty()) {
    return "";
  }

//  auto result = server_list_.find(publisher_key);
//
//  if (result == server_list_.end()) {
//    return "";
//  }
//
//  if (!result->second) {
//    return "";
//  }
//
//  return result->second->address;
  return "";
}

}  // namespace braveledger_bat_publishers
