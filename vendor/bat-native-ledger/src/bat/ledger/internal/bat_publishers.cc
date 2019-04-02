/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <cmath>
#include <ctime>
#include <utility>

#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/bat_publishers.h"
#include "bat/ledger/internal/bignum.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/rapidjson_bat_helper.h"
#include "bat/ledger/internal/static_values.h"

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
  server_list_(std::map<std::string, braveledger_bat_helper::SERVER_LIST>()) {
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
  }
  return "";
}

bool ignoreMinTime(const std::string& publisher_id) {
  return !getProviderName(publisher_id).empty();
}

void BatPublishers::AddRecurringPayment(const std::string& publisher_id,
                                        const double& value) {
  state_->recurring_donation_[publisher_id] = value;
  saveState();
}

void BatPublishers::saveVisit(const std::string& publisher_id,
                              const ledger::VisitData& visit_data,
                              const uint64_t& duration,
                              uint64_t window_id) {
  if (!ledger_->GetRewardsMainEnabled() || publisher_id.empty()) {
    return;
  }

  auto filter = CreateActivityFilter(publisher_id,
      ledger::EXCLUDE_FILTER::FILTER_ALL,
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
                _1,
                _2);
  ledger_->GetActivityInfo(filter, callbackGetPublishers);
}

ledger::ActivityInfoFilter BatPublishers::CreateActivityFilter(
    const std::string& publisher_id,
    ledger::EXCLUDE_FILTER excluded,
    bool min_duration,
    const uint64_t& currentReconcileStamp,
    bool non_verified,
    bool min_visits) {
  ledger::ActivityInfoFilter filter;
  filter.id = publisher_id;
  filter.excluded = excluded;
  filter.min_duration = min_duration ? getPublisherMinVisitTime() : 0;
  filter.reconcile_stamp = currentReconcileStamp;
  filter.non_verified = non_verified;
  filter.min_visits = min_visits ? getPublisherMinVisits() : 0;

  return filter;
}

std::string BatPublishers::GetBalanceReportName(
    const ledger::ACTIVITY_MONTH month,
    int year) {
  return std::to_string(year) + "_" + std::to_string(month);
}

void BatPublishers::saveVisitInternal(
    std::string publisher_id,
    ledger::VisitData visit_data,
    uint64_t duration,
    uint64_t window_id,
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> publisher_info) {
  DCHECK(result != ledger::Result::TOO_MANY_RESULTS);
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    // TODO(anyone) error handling
    return;
  }
  bool verified = isVerified(publisher_id);

  bool new_visit = false;
  if (!publisher_info.get()) {
    new_visit = true;
    publisher_info.reset(new ledger::PublisherInfo(publisher_id));
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
    publisher_info->favicon_url = ledger::_clear_favicon;
  }

  publisher_info->name = visit_data.name;
  publisher_info->provider = visit_data.provider;
  publisher_info->url = visit_data.url;
  publisher_info->verified = verified;

  bool excluded = isExcluded(publisher_info->id, publisher_info->excluded);
  bool ignore_time = ignoreMinTime(publisher_id);
  if (duration == 0) {
    ignore_time = false;
  }

  std::unique_ptr<ledger::PublisherInfo> panel_info = nullptr;

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
    panel_info = std::make_unique<ledger::PublisherInfo>(*publisher_info);

    ledger_->SetPublisherInfo(std::move(publisher_info));
  } else if (!excluded &&
             ledger_->GetAutoContribute() &&
             min_duration_ok &&
             verified_old) {
    publisher_info->visits += 1;
    publisher_info->duration += duration;
    publisher_info->score += concaveScore(duration);
    publisher_info->reconcile_stamp = ledger_->GetReconcileStamp();

    panel_info = std::make_unique<ledger::PublisherInfo>(*publisher_info);

    ledger_->SetActivityInfo(std::move(publisher_info));
  }

  if (panel_info && window_id > 0) {
    if (panel_info->favicon_url == ledger::_clear_favicon) {
      panel_info->favicon_url = std::string();
    }

    OnPanelPublisherInfo(ledger::Result::LEDGER_OK,
                        std::move(panel_info),
                        window_id,
                        visit_data);
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
    std::unique_ptr<ledger::PublisherInfo> info,
    const std::string& favicon_url,
    uint64_t window_id) {
  if (result == ledger::Result::LEDGER_OK && !favicon_url.empty()) {
    info->favicon_url = favicon_url;

    std::unique_ptr<ledger::PublisherInfo> panel_info =
        std::make_unique<ledger::PublisherInfo>(*info);

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
    std::unique_ptr<ledger::PublisherInfo> info) {
  if (result != ledger::Result::LEDGER_OK || !info.get()) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Publisher info was not saved!";
  }

  SynopsisNormalizer();
}

void BatPublishers::setExclude(const std::string& publisher_id,
                               const ledger::PUBLISHER_EXCLUDE& exclude) {
    ledger_->GetPublisherInfo(publisher_id,
        std::bind(&BatPublishers::onSetExcludeInternal,
                  this,
                  exclude,
                  _1,
                  _2));
}

void BatPublishers::onSetExcludeInternal(
    ledger::PUBLISHER_EXCLUDE exclude,
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> publisher_info) {
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    return;
  }

  if (!publisher_info || publisher_info->excluded == exclude) {
    // handle error
    return;
  }

  publisher_info->excluded = exclude;

  std::string publisherKey = publisher_info->id;

  ledger_->SetPublisherInfo(std::move(publisher_info));

  OnExcludedSitesChanged(publisherKey, exclude);
}

void BatPublishers::RestorePublishers() {
  ledger_->OnRestorePublishers(
      std::bind(&BatPublishers::OnRestorePublishersInternal,
                this,
                _1));
}

void BatPublishers::OnRestorePublishersInternal(bool success) {
  if (success) {
    OnExcludedSitesChanged("-1", ledger::PUBLISHER_EXCLUDE::ALL);
    SynopsisNormalizer();
  } else {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Could not restore publishers.";
  }
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

unsigned int BatPublishers::getPublisherMinVisits() const {
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
    const ledger::PublisherInfoList& list,
    uint32_t record) {

  synopsisNormalizerInternal(newList, list, record);
}

void BatPublishers::synopsisNormalizerInternal(
    ledger::PublisherInfoList* newList,
    const ledger::PublisherInfoList& oldList,
    uint32_t /* next_record */) {
  // TODO(SZ): We can pass non const value here to avoid copying
  ledger::PublisherInfoList list = oldList;
  if (list.size() == 0) {
    return;
  }

  double totalScores = 0.0;
  for (size_t i = 0; i < list.size(); i++) {
    // Check which would test uint problem from this issue
    // https://github.com/brave/brave-browser/issues/3134
    if (GetMigrateScore()) {
      list[i].score = concaveScore(list[i].duration);
    }
    totalScores += list[i].score;
  }

  if (GetMigrateScore()) {
    SetMigrateScore(false);
  }

  std::vector<unsigned int> percents;
  std::vector<double> weights;
  std::vector<double> realPercents;
  std::vector<double> roundoffs;
  unsigned int totalPercents = 0;
  for (size_t i = 0; i < list.size(); i++) {
    double floatNumber = (list[i].score / totalScores) * 100.0;
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
        percents[valueToChange] -= 1;
        totalPercents -= 1;
      } else {
        percents[valueToChange] += 1;
        totalPercents += 1;
      }
      roundoffs[valueToChange] = 0;
    }
  }
  size_t currentValue = 0;
  for (size_t i = 0; i < list.size(); i++) {
    list[i].percent = percents[currentValue];
    list[i].weight = weights[currentValue];
    currentValue++;
    if (newList) {
      newList->push_back(list[i]);
    }
  }
}

void BatPublishers::SynopsisNormalizer() {
  auto filter = CreateActivityFilter("",
      ledger::EXCLUDE_FILTER::FILTER_ALL_EXCEPT_EXCLUDED,
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
      filter,
      std::bind(&BatPublishers::SynopsisNormalizerCallback, this, _1, _2));
}

void BatPublishers::SynopsisNormalizerCallback(
    const ledger::PublisherInfoList& list,
    uint32_t record) {
  ledger::PublisherInfoList normalized_list;
  synopsisNormalizerInternal(&normalized_list, list, 0);
  ledger_->SaveNormalizedPublisherList(normalized_list);
}

bool BatPublishers::isVerified(const std::string& publisher_id) {
  if (server_list_.empty()) {
    return false;
  }

  auto result = server_list_.find(publisher_id);

  if (result == server_list_.end()) {
    return false;
  }

  const braveledger_bat_helper::SERVER_LIST values = result->second;

  return values.verified;
}

bool BatPublishers::isExcluded(const std::string& publisher_id,
                               const ledger::PUBLISHER_EXCLUDE& excluded) {
  // If exclude is set to 1, we should avoid further computation and return true
  if (excluded == ledger::PUBLISHER_EXCLUDE::EXCLUDED) {
    return true;
  }

  if (excluded == ledger::PUBLISHER_EXCLUDE::INCLUDED || server_list_.empty()) {
    return false;
  }

  auto result = server_list_.find(publisher_id);

  if (result == server_list_.end()) {
    return false;
  }

  const braveledger_bat_helper::SERVER_LIST values = result->second;

  return values.excluded;
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
  report_balance.opening_balance_ = report_info.opening_balance_;
  report_balance.closing_balance_ = report_info.closing_balance_;
  report_balance.grants_ = report_info.grants_;
  report_balance.deposits_ = report_info.deposits_;
  report_balance.earning_from_ads_ = report_info.earning_from_ads_;
  report_balance.recurring_donation_ = report_info.recurring_donation_;
  report_balance.one_time_donation_ = report_info.one_time_donation_;
  report_balance.auto_contribute_ = report_info.auto_contribute_;

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
    setBalanceReport(month, year, new_report_info);
    bool successGet = getBalanceReport(month, year, report_info);
    if (successGet) {
      iter = state_->monthly_balances_.find(name);
    } else {
      return false;
    }
  }

  report_info->opening_balance_ = iter->second.opening_balance_;
  report_info->closing_balance_ = iter->second.closing_balance_;
  report_info->grants_ = iter->second.grants_;
  report_info->earning_from_ads_ = iter->second.earning_from_ads_;
  report_info->auto_contribute_ = iter->second.auto_contribute_;
  report_info->recurring_donation_ = iter->second.recurring_donation_;
  report_info->one_time_donation_ = iter->second.one_time_donation_;

  return true;
}

std::map<std::string, ledger::BalanceReportInfo>
BatPublishers::getAllBalanceReports() {
  std::map<std::string, ledger::BalanceReportInfo> newReports;
  for (auto const& report : state_->monthly_balances_) {
    ledger::BalanceReportInfo newReport;
    const braveledger_bat_helper::REPORT_BALANCE_ST oldReport = report.second;
    newReport.opening_balance_ = oldReport.opening_balance_;
    newReport.closing_balance_ = oldReport.closing_balance_;
    newReport.grants_ = oldReport.grants_;
    newReport.earning_from_ads_ = oldReport.earning_from_ads_;
    newReport.auto_contribute_ = oldReport.auto_contribute_;
    newReport.recurring_donation_ = oldReport.recurring_donation_;
    newReport.one_time_donation_ = oldReport.one_time_donation_;

    newReports[report.first] = newReport;
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

std::vector<ledger::ContributionInfo>
BatPublishers::GetRecurringDonationList() {
  std::vector<ledger::ContributionInfo> res;

  for (const auto & e : state_->recurring_donation_) {
    ledger::ContributionInfo info;
    info.publisher = e.first;
    info.value = e.second;
    res.push_back(info);
  }

  return res;
}

void BatPublishers::RefreshPublishersList(const std::string& json) {
  ledger_->SavePublishersList(json);
  loadPublisherList(json);
}

void BatPublishers::OnPublishersListSaved(ledger::Result result) {
  uint64_t ts = (ledger::Result::LEDGER_OK == result)
                ? std::time(nullptr)
                : 0ull;
  setPublishersLastRefreshTimestamp(ts);
}

bool BatPublishers::loadPublisherList(const std::string& data) {
  std::map<std::string, braveledger_bat_helper::SERVER_LIST> list;
  bool success = braveledger_bat_helper::getJSONServerList(data, &list);

  if (success) {
    server_list_ = list;
  }

  return success;
}

void BatPublishers::getPublisherActivityFromUrl(
    uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& publisher_blob) {
  if (!ledger_->GetRewardsMainEnabled()) {
    return;
  }

  if ((visit_data.domain == YOUTUBE_TLD || visit_data.domain == TWITCH_TLD) &&
      visit_data.path != "" && visit_data.path != "/") {
    std::string type = YOUTUBE_MEDIA_TYPE;
    if (visit_data.domain == TWITCH_TLD) {
      type = TWITCH_MEDIA_TYPE;
    }

    ledger::VisitData new_visit_data(visit_data);

    if (!new_visit_data.url.empty()) {
      new_visit_data.url.pop_back();
    }

    new_visit_data.url = new_visit_data.url + new_visit_data.path;

    ledger_->GetMediaActivityFromUrl(
        windowId, new_visit_data, type, publisher_blob);
    return;
  }

  auto filter = CreateActivityFilter(visit_data.domain,
        ledger::EXCLUDE_FILTER::FILTER_ALL,
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

  ledger_->GetPanelPublisherInfo(filter,
        std::bind(&BatPublishers::OnPanelPublisherInfo,
                  this,
                  _1,
                  _2,
                  windowId,
                  new_data));
}

void BatPublishers::OnPanelPublisherInfo(
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> info,
    uint64_t windowId,
    const ledger::VisitData& visit_data) {
  if (result == ledger::Result::LEDGER_OK) {
    ledger_->OnPanelPublisherInfo(result, std::move(info), windowId);
  }

  if (result == ledger::Result::NOT_FOUND && !visit_data.domain.empty()) {
    saveVisitInternal(visit_data.domain,
                      visit_data,
                      0,
                      windowId,
                      result,
                      std::move(info));
  }
}

void BatPublishers::OnExcludedSitesChanged(const std::string& publisher_id,
                                           ledger::PUBLISHER_EXCLUDE exclude) {
  ledger_->OnExcludedSitesChanged(publisher_id, exclude);
}

void BatPublishers::setBalanceReportItem(ledger::ACTIVITY_MONTH month,
                                         int year,
                                         ledger::ReportType type,
                                         const std::string& probi) {
  ledger::BalanceReportInfo report_info;
  getBalanceReport(month, year, &report_info);

  switch (type) {
    case ledger::ReportType::GRANT:
      report_info.grants_ =
          braveledger_bat_bignum::sum(report_info.grants_, probi);
      break;
    case ledger::ReportType::ADS:
      report_info.earning_from_ads_ =
          braveledger_bat_bignum::sum(report_info.earning_from_ads_, probi);
      break;
    case ledger::ReportType::AUTO_CONTRIBUTION:
      report_info.auto_contribute_ =
          braveledger_bat_bignum::sum(report_info.auto_contribute_, probi);
      break;
    case ledger::ReportType::DONATION:
      report_info.one_time_donation_ =
          braveledger_bat_bignum::sum(report_info.one_time_donation_, probi);
      break;
    case ledger::ReportType::DONATION_RECURRING:
      report_info.recurring_donation_ =
          braveledger_bat_bignum::sum(report_info.recurring_donation_, probi);
      break;
    default:
      break;
  }

  setBalanceReport(month, year, report_info);
}

void BatPublishers::getPublisherBanner(
    const std::string& publisher_id,
    ledger::PublisherBannerCallback callback) {
  ledger::PublisherBanner banner;
  banner.publisher_key = publisher_id;

  if (!server_list_.empty()) {
    auto result = server_list_.find(publisher_id);

    if (result != server_list_.end()) {
      const braveledger_bat_helper::SERVER_LIST values = result->second;

      banner.title = values.banner.title_;
      banner.description = values.banner.description_;
      banner.amounts = values.banner.amounts_;
      banner.social = values.banner.social_;

      // WebUI must not make external network requests, so map
      // external resopurces to chrome://rewards-image and handle them
      // via our custom data source
      if (!values.banner.background_.empty()) {
        banner.background = "chrome://rewards-image/"
            + values.banner.background_;
      }

      if (!values.banner.logo_.empty()) {
        banner.logo = "chrome://rewards-image/" + values.banner.logo_;
      }
    }
  }

  ledger::PublisherInfoCallback callbackGetPublisher =
      std::bind(&BatPublishers::onPublisherBanner,
                this,
                callback,
                banner,
                _1,
                _2);

  ledger_->GetPublisherInfo(publisher_id, callbackGetPublisher);
}

void BatPublishers::onPublisherBanner(
    ledger::PublisherBannerCallback callback,
    ledger::PublisherBanner banner,
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> publisher_info) {

  auto new_banner = std::make_unique<ledger::PublisherBanner>(banner);

  if (!publisher_info || result != ledger::Result::LEDGER_OK) {
    callback(std::move(new_banner));
    return;
  }

  new_banner->name = publisher_info->name;
  new_banner->provider = publisher_info->provider;
  new_banner->verified = publisher_info->verified;

  if (new_banner->logo.empty()) {
    new_banner->logo = publisher_info->favicon_url;
  }

  callback(std::move(new_banner));
}

void BatPublishers::ResetState() {
  state_.reset(new braveledger_bat_helper::PUBLISHER_STATE_ST);
  server_list_.clear();
}

}  // namespace braveledger_bat_publishers
