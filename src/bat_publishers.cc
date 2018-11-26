/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat_publishers.h"

#include <ctime>
#include <cmath>
#include <algorithm>

#include "bat_helper.h"
#include "bignum.h"
#include "ledger_impl.h"
#include "rapidjson_bat_helper.h"
#include "static_values.h"

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

using namespace std::placeholders;

namespace braveledger_bat_publishers {

BatPublishers::BatPublishers(bat_ledger::LedgerImpl* ledger):
  ledger_(ledger),
  state_(new braveledger_bat_helper::PUBLISHER_STATE_ST),
  server_list_(std::map<std::string, braveledger_bat_helper::SERVER_LIST>()) {
  calcScoreConsts();
}

BatPublishers::~BatPublishers() {
}

void BatPublishers::calcScoreConsts() {
  uint64_t min_duration_ms = state_->min_publisher_duration_ * 1000;
  //TODO: check Warning	C4244	'=': conversion from 'double' to 'unsigned int', possible loss of data
  a_ = 1.0 / (braveledger_ledger::_d * 2.0) - min_duration_ms;
  a2_ = a_ * 2;
  a4_ = a2_ * 2;
  b_ = min_duration_ms - a_;
  b2_ = b_ * b_;
}

// courtesy of @dimitry-xyz: https://github.com/brave/ledger/issues/2#issuecomment-221752002
double BatPublishers::concaveScore(const uint64_t& duration) {
  return (std::sqrt(b2_ + a4_ * duration) - b_) / (double)a2_;
}

std::string getProviderName(const std::string& publisher_id) {
  // TODO - this is for the media stuff
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

void BatPublishers::AddRecurringPayment(const std::string& publisher_id, const double& value) {
  state_->recurring_donation_[publisher_id] = value;
  saveState();
}

void BatPublishers::MakePayment(const ledger::PaymentData& payment_data) {
  auto filter = CreatePublisherFilter(payment_data.publisher_id,
                                      payment_data.category,
                                      payment_data.local_month,
                                      payment_data.local_year);
  ledger_->GetPublisherInfo(filter,
      std::bind(&BatPublishers::makePaymentInternal, this,
          payment_data, _1, _2));
}

bool BatPublishers::saveVisitAllowed() const {
  return (ledger_->GetRewardsMainEnabled() && ledger_->GetAutoContribute());
}

void onVisitSavedDummy(ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> publisher_info) {
  // onPublisherInfoUpdated will always be called by LedgerImpl so do nothing
}

void BatPublishers::saveVisit(const std::string& publisher_id,
                              const ledger::VisitData& visit_data,
                              const uint64_t& duration) {
  if (!saveVisitAllowed() || publisher_id.empty()) {
    return;
  }

  auto filter = CreatePublisherFilter(publisher_id,
      ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE,
      visit_data.local_month,
      visit_data.local_year,
      ledger::PUBLISHER_EXCLUDE_FILTER::FILTER_ALL,
      false,
      ledger_->GetReconcileStamp());

  ledger::PublisherInfoCallback callbackGetPublishers = std::bind(&BatPublishers::saveVisitInternal, this,
                publisher_id,
                visit_data,
                duration,
                0,
                _1,
                _2);
  ledger_->GetPublisherInfo(filter, callbackGetPublishers);
}

ledger::PublisherInfoFilter BatPublishers::CreatePublisherFilter(
    const std::string &publisher_id,
    ledger::PUBLISHER_CATEGORY category,
    ledger::PUBLISHER_MONTH month,
    int year) {
  return CreatePublisherFilter(publisher_id,
                               category,
                               month,
                               year,
                               ledger::PUBLISHER_EXCLUDE_FILTER::FILTER_ALL,
                               true,
                               0);
}

ledger::PublisherInfoFilter BatPublishers::CreatePublisherFilter(
    const std::string& publisher_id,
    ledger::PUBLISHER_CATEGORY category,
    ledger::PUBLISHER_MONTH month,
    int year,
    ledger::PUBLISHER_EXCLUDE_FILTER excluded) {
  return CreatePublisherFilter(publisher_id,
                               category,
                               month,
                               year,
                               excluded,
                               true,
                               0);
}

ledger::PublisherInfoFilter BatPublishers::CreatePublisherFilter(
    const std::string &publisher_id,
    ledger::PUBLISHER_CATEGORY category,
    ledger::PUBLISHER_MONTH month,
    int year,
    bool min_duration) {
  return CreatePublisherFilter(publisher_id,
                               category,
                               month,
                               year,
                               ledger::PUBLISHER_EXCLUDE_FILTER::FILTER_ALL,
                               min_duration,
                               0);
}

ledger::PublisherInfoFilter BatPublishers::CreatePublisherFilter(
    const std::string& publisher_id,
    ledger::PUBLISHER_CATEGORY category,
    ledger::PUBLISHER_MONTH month,
    int year,
    ledger::PUBLISHER_EXCLUDE_FILTER excluded,
    bool min_duration,
    const uint64_t& currentReconcileStamp) {
  ledger::PublisherInfoFilter filter;
  filter.id = publisher_id;
  filter.category = category;
  filter.month = month;
  filter.year = year;
  filter.excluded = excluded;
  filter.min_duration = min_duration ? getPublisherMinVisitTime() : 0;
  filter.reconcile_stamp = currentReconcileStamp;

  return filter;
}

std::string BatPublishers::GetBalanceReportName(
    const ledger::PUBLISHER_MONTH month,
    int year) {
  return std::to_string(year) + "_" + std::to_string(month);
}

void BatPublishers::setNumExcludedSitesInternal(ledger::PUBLISHER_EXCLUDE exclude) {
  unsigned int previousNum = getNumExcludedSites();
  setNumExcludedSites((exclude == ledger::PUBLISHER_EXCLUDE::EXCLUDED)
                      ? ++previousNum
                      : --previousNum);
}

void BatPublishers::makePaymentInternal(
      ledger::PaymentData payment_data,
      ledger::Result result,
      std::unique_ptr<ledger::PublisherInfo> publisher_info) {
  if (result != ledger::Result::LEDGER_OK) {
    // TODO error handling
    return;
  }

  if (!publisher_info.get())
    publisher_info.reset(new ledger::PublisherInfo(payment_data.publisher_id,
                                                   payment_data.local_month,
                                                   payment_data.local_year));
  publisher_info->category = payment_data.category;

  publisher_info->contributions.push_back(ledger::ContributionInfo(payment_data.value, payment_data.timestamp));

  ledger_->SetPublisherInfo(std::move(publisher_info),
      std::bind(&onVisitSavedDummy, _1, _2));
}

void BatPublishers::saveVisitInternal(
    std::string publisher_id,
    ledger::VisitData visit_data,
    uint64_t duration,
    uint64_t window_id,
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> publisher_info) {
  DCHECK(result != ledger::Result::TOO_MANY_RESULTS);
  if (result != ledger::Result::LEDGER_OK && result != ledger::Result::NOT_FOUND) {
    // TODO error handling
    return;
  }

  bool new_visit = false;
  if (!publisher_info.get()) {
    new_visit = true;
    publisher_info.reset(new ledger::PublisherInfo(publisher_id,
                                                   visit_data.local_month,
                                                   visit_data.local_year));
  }

  if (!ignoreMinTime(publisher_id) && duration < getPublisherMinVisitTime()) {
    duration = 0;
  }

  publisher_info->favicon_url = visit_data.favicon_url;
  publisher_info->name = visit_data.name;
  publisher_info->provider = visit_data.provider;
  publisher_info->url = visit_data.url;
  publisher_info->visits += 1;
  publisher_info->category = ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE;
  if (!isExcluded(publisher_info->id, publisher_info->excluded)) {
    publisher_info->duration += duration;
  } else {
    publisher_info->excluded = ledger::PUBLISHER_EXCLUDE::EXCLUDED;
    if (new_visit) {
      publisher_info->duration = 0; // don't log auto-excluded
    }
  }
  publisher_info->score += concaveScore(duration);
  publisher_info->verified = isVerified(publisher_info->id);
  publisher_info->reconcile_stamp = ledger_->GetReconcileStamp();

  auto media_info = std::make_unique<ledger::PublisherInfo>(*publisher_info);

  ledger_->SetPublisherInfo(std::move(publisher_info), std::bind(&onVisitSavedDummy, _1, _2));

  if (window_id > 0) {
    onPublisherActivity(ledger::Result::LEDGER_OK, std::move(media_info), window_id, visit_data);
  }
}

std::unique_ptr<ledger::PublisherInfo> BatPublishers::onPublisherInfoUpdated(
    ledger::Result result, std::unique_ptr<ledger::PublisherInfo> info) {
  if (result != ledger::Result::LEDGER_OK || !info.get()) {
    // TODO error handling
    return info;
  }

  if (!isEligibleForContribution(*info)) {
    return info;
  }

  synopsisNormalizer(*info);

  return info;
}

void BatPublishers::setExclude(const std::string& publisher_id, const ledger::PUBLISHER_EXCLUDE& exclude) {
  uint64_t currentReconcileStamp = ledger_->GetReconcileStamp();
  auto filter = CreatePublisherFilter(publisher_id,
      ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE,
      ledger::PUBLISHER_MONTH::ANY,
      -1,
      ledger::PUBLISHER_EXCLUDE_FILTER::FILTER_ALL,
      false,
      currentReconcileStamp);
    ledger_->GetPublisherInfo(filter, std::bind(&BatPublishers::onSetExcludeInternal,
                            this, exclude, _1, _2));
}

void BatPublishers::setPanelExclude(const std::string& publisher_id,
  const ledger::PUBLISHER_EXCLUDE& exclude, uint64_t windowId) {
  uint64_t currentReconcileStamp = ledger_->GetReconcileStamp();
  auto filter = CreatePublisherFilter(publisher_id,
      ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE,
      ledger::PUBLISHER_MONTH::ANY,
      -1,
      ledger::PUBLISHER_EXCLUDE_FILTER::FILTER_ALL,
      false,
      currentReconcileStamp);
    ledger_->GetPublisherInfo(filter, std::bind(
      &BatPublishers::onSetPanelExcludeInternal,
      this, exclude, windowId, _1, _2));
}

void BatPublishers::onSetExcludeInternal(ledger::PUBLISHER_EXCLUDE exclude,
                                         ledger::Result result,
                                         std::unique_ptr<ledger::PublisherInfo> publisher_info) {
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    return;
  }

  if (!publisher_info) {
    // handle error
    return;
  }

  publisher_info->year = -1;
  if (publisher_info->excluded == ledger::PUBLISHER_EXCLUDE::DEFAULT ||
      publisher_info->excluded == ledger::PUBLISHER_EXCLUDE::INCLUDED) {
    publisher_info->excluded = ledger::PUBLISHER_EXCLUDE::EXCLUDED;
  } else {
    publisher_info->excluded = ledger::PUBLISHER_EXCLUDE::INCLUDED;
  }
  publisher_info->month = ledger::PUBLISHER_MONTH::ANY;
  setNumExcludedSitesInternal(exclude);

  ledger_->SetPublisherInfo(std::move(publisher_info),
    std::bind(&BatPublishers::onSetPublisherInfo, this, _1, _2));

  OnExcludedSitesChanged();
}

void BatPublishers::onSetPublisherInfo(ledger::Result result,
  std::unique_ptr<ledger::PublisherInfo> publisher_info) {
  if (result != ledger::Result::LEDGER_OK) {
    return;
  }
  synopsisNormalizer(*publisher_info);
}

void BatPublishers::onSetPanelExcludeInternal(ledger::PUBLISHER_EXCLUDE exclude,
  uint64_t windowId,
  ledger::Result result,
  std::unique_ptr<ledger::PublisherInfo> publisher_info) {
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    return;
  }

  if (!publisher_info) {
    // handle error
    return;
  }

  publisher_info->year = -1;
  if (publisher_info->excluded == ledger::PUBLISHER_EXCLUDE::DEFAULT ||
      publisher_info->excluded == ledger::PUBLISHER_EXCLUDE::INCLUDED) {
    publisher_info->excluded = ledger::PUBLISHER_EXCLUDE::EXCLUDED;
  } else {
    publisher_info->excluded = ledger::PUBLISHER_EXCLUDE::INCLUDED;
  }
  publisher_info->month = ledger::PUBLISHER_MONTH::ANY;
  setNumExcludedSitesInternal(exclude);

  ledger::VisitData visit_data;
  ledger_->SetPublisherInfo(std::move(publisher_info),
      std::bind(&BatPublishers::onPublisherActivity, this, _1, _2,
      windowId, visit_data));
  OnExcludedSitesChanged();
}

void BatPublishers::restorePublishers() {
  uint64_t currentReconcileStamp = ledger_->GetReconcileStamp();
  auto filter = CreatePublisherFilter("",
      ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE,
      ledger::PUBLISHER_MONTH::ANY,
      -1,
      ledger::PUBLISHER_EXCLUDE_FILTER::FILTER_EXCLUDED,
      false,
      currentReconcileStamp);
  ledger_->GetPublisherInfoList(0, 0, filter, std::bind(&BatPublishers::onRestorePublishersInternal,
                                this, _1, _2));
}

void BatPublishers::onRestorePublishersInternal(const ledger::PublisherInfoList& publisherInfoList, uint32_t /* next_record */) {
  if (publisherInfoList.size() == 0) {
    return;
  }

  for (size_t i = 0; i < publisherInfoList.size(); i++) {
    // Set to PUBLISHER_EXCLUDE::DEFAULT (0)
    setExclude(publisherInfoList[i].id,
               ledger::PUBLISHER_EXCLUDE::DEFAULT);
  }
}

void BatPublishers::setPublisherMinVisitTime(const uint64_t& duration) { // In seconds
  state_->min_publisher_duration_ = duration;
  saveState();
}

void BatPublishers::setPublisherMinVisits(const unsigned int& visits) {
  state_->min_visits_ = visits;
  saveState();
}

void BatPublishers::setPublishersLastRefreshTimestamp(uint64_t ts) {
  state_->pubs_load_timestamp_ = ts;
  saveState();
}

void BatPublishers::setNumExcludedSites(const unsigned int& amount) {
  state_->num_excluded_sites_ = amount;
  saveState();
}

void BatPublishers::setPublisherAllowNonVerified(const bool& allow) {
  state_->allow_non_verified_ = allow;
  saveState();
}

void BatPublishers::setPublisherAllowVideos(const bool& allow) {
  state_->allow_videos_ = allow;
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

unsigned int BatPublishers::getNumExcludedSites() const {
  return state_->num_excluded_sites_;
}

bool BatPublishers::getPublisherAllowVideos() const {
  return state_->allow_videos_;
}

void BatPublishers::NormalizeContributeWinners(ledger::PublisherInfoList* newList, bool saveData,
    const braveledger_bat_helper::PublisherList& oldList, uint32_t record) {

  ledger::PublisherInfoList list;

  for(const auto& publisher: oldList) {
    ledger::PublisherInfo new_publisher;
    new_publisher.id = publisher.id_;
    new_publisher.percent = publisher.percent_;
    new_publisher.weight = publisher.weight_;
    new_publisher.duration = publisher.duration_;
    new_publisher.score = publisher.score_;
    new_publisher.visits = publisher.visits_;

    list.push_back(new_publisher);
  }

  synopsisNormalizerInternal(newList, saveData, list, record);
}

void BatPublishers::synopsisNormalizerInternal(ledger::PublisherInfoList* newList, bool saveData,
    const ledger::PublisherInfoList& oldList, uint32_t /* next_record */) {
  // TODO SZ: We can pass non const value here to avoid copying
  ledger::PublisherInfoList list = oldList;
  if (list.size() == 0) {
    return;
  }
  double totalScores = 0.0;
  for (size_t i = 0; i < list.size(); i++) {
    totalScores += list[i].score;
  }
  std::vector<unsigned int> percents;
  std::vector<double> weights;
  std::vector<double> realPercents;
  std::vector<double> roundoffs;
  unsigned int totalPercents = 0;
  for (size_t i = 0; i < list.size(); i++) {
    realPercents.push_back((double)list[i].score / (double)totalScores * 100.0);
    percents.push_back((unsigned int)std::lround(realPercents[realPercents.size() - 1]));
    double roundoff = percents[percents.size() - 1] - realPercents[realPercents.size() - 1];
    if (roundoff < 0.0) {
      roundoff *= -1.0;
    }
    roundoffs.push_back(roundoff);
    totalPercents += percents[percents.size() - 1];
    weights.push_back((double)list[i].score / (double)list.size() * 100.0);
  }
  while (totalPercents != 100) {
    size_t valueToChange = 0;
    double currentRoundOff = 0.0;
    for (size_t i = 0; i < percents.size(); i++) {
      if (0 == i) {
        currentRoundOff = roundoffs[i];
        continue;
      }
      if (roundoffs[i] > currentRoundOff) {
        currentRoundOff = roundoffs[i];
        valueToChange = i;
      }
    }
    if (0 != percents.size()) {
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
    if (saveData) {
      std::unique_ptr<ledger::PublisherInfo> publisher_info;
      publisher_info.reset(new ledger::PublisherInfo(list[i]));
      ledger_->SetPublisherInfo(std::move(publisher_info),
        std::bind(&onVisitSavedDummy, _1, _2));
    }
    if (newList) {
      newList->push_back(list[i]);
    }
  }
}

void BatPublishers::synopsisNormalizer(const ledger::PublisherInfo& info) {
  auto filter = CreatePublisherFilter("",
      ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE,
      info.month,
      info.year,
      ledger::PUBLISHER_EXCLUDE_FILTER::FILTER_ALL_EXCEPT_EXCLUDED,
      true,
      ledger_->GetReconcileStamp());
  // TODO SZ: We pull the whole list currently, I don't think it consumes lots of RAM, but could.
  // We need to limit it and iterate.
  ledger_->GetPublisherInfoList(0, 0, filter, std::bind(&BatPublishers::synopsisNormalizerInternal, this,
          nullptr, true, _1, _2));
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

bool BatPublishers::isExcluded(const std::string& publisher_id, const ledger::PUBLISHER_EXCLUDE& excluded) {
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

bool BatPublishers::isEligibleForContribution(const ledger::PublisherInfo& info) {

  if (isExcluded(info.id, info.excluded) || (!state_->allow_non_verified_ && !isVerified(info.id)))
    return false;

  return info.score > 0 &&
    info.duration >= state_->min_publisher_duration_ &&
    info.visits >= state_->min_visits_;

}

void BatPublishers::clearAllBalanceReports() {
  if (state_->monthly_balances_.empty()) {
    return;
  }
  state_->monthly_balances_.clear();
  saveState();
}

void BatPublishers::setBalanceReport(ledger::PUBLISHER_MONTH month,
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
  total = braveledger_bat_bignum::sub(total, report_balance.recurring_donation_);
  total = braveledger_bat_bignum::sub(total, report_balance.one_time_donation_);

  report_balance.total_ = total;
  state_->monthly_balances_[GetBalanceReportName(month, year)] = report_balance;
  saveState();
}

bool BatPublishers::getBalanceReport(ledger::PUBLISHER_MONTH month,
                                     int year,
                                     ledger::BalanceReportInfo* report_info) {
  std::string name = GetBalanceReportName(month, year);
  std::map<std::string, braveledger_bat_helper::REPORT_BALANCE_ST>::const_iterator iter =
    state_->monthly_balances_.find(name);
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

std::map<std::string, ledger::BalanceReportInfo> BatPublishers::getAllBalanceReports() {
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
  braveledger_bat_helper::saveToJsonString(*state_, data);
  ledger_->SavePublisherState(data, this);
}

bool BatPublishers::loadState(const std::string& data) {
  braveledger_bat_helper::PUBLISHER_STATE_ST state;
  if (!braveledger_bat_helper::loadFromJson(state, data.c_str()))
    return false;

  state_.reset(new braveledger_bat_helper::PUBLISHER_STATE_ST(state));
  calcScoreConsts();
  return true;
}

void BatPublishers::OnPublisherStateSaved(ledger::Result result) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Could not save publisher state";
    // TODO - error handling
    return;
  }
  // SZ: We don't need to normalize on state save, all normalizing is done on AUTO_CONTRIBUTE publishers
  // save visit
  //synopsisNormalizer();
}

std::vector<ledger::ContributionInfo> BatPublishers::GetRecurringDonationList() {
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
  uint64_t ts = (ledger::Result::LEDGER_OK == result) ? std::time(nullptr) : 0ull;
  setPublishersLastRefreshTimestamp(ts);
}

bool BatPublishers::loadPublisherList(const std::string& data) {
  std::map<std::string, braveledger_bat_helper::SERVER_LIST> list;
  bool success = braveledger_bat_helper::getJSONServerList(data, list);

  if (success) {
    server_list_ = std::map<std::string, braveledger_bat_helper::SERVER_LIST>(list);
  }

  return success;
}

void BatPublishers::getPublisherActivityFromUrl(uint64_t windowId, const ledger::VisitData& visit_data) {
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

    ledger_->GetMediaActivityFromUrl(windowId, new_visit_data, type);
    return;
  }

  auto filter = CreatePublisherFilter(visit_data.domain,
        ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE,
        visit_data.local_month,
        visit_data.local_year,
        ledger::PUBLISHER_EXCLUDE_FILTER::FILTER_ALL,
        false,
        ledger_->GetReconcileStamp());

  ledger::VisitData new_data;
  new_data.domain = visit_data.domain;
  new_data.path = visit_data.path;
  new_data.local_month = visit_data.local_month;
  new_data.local_year = visit_data.local_year;
  new_data.name = visit_data.name;
  new_data.url = visit_data.url;
  new_data.favicon_url = "";

  ledger_->GetPublisherInfo(filter,
        std::bind(&BatPublishers::onPublisherActivity, this, _1, _2, windowId, new_data));
}

void BatPublishers::onPublisherActivity(ledger::Result result,
                                        std::unique_ptr<ledger::PublisherInfo> info,
                                        uint64_t windowId,
                                        const ledger::VisitData& visit_data) {
  if (result == ledger::Result::LEDGER_OK) {
    ledger_->OnPublisherActivity(result, std::move(info), windowId);
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

void BatPublishers::OnExcludedSitesChanged() {
  ledger_->OnExcludedSitesChanged();
}

void BatPublishers::setBalanceReportItem(ledger::PUBLISHER_MONTH month,
                                         int year,
                                         ledger::ReportType type,
                                         const std::string& probi) {
  ledger::BalanceReportInfo report_info;
  getBalanceReport(month, year, &report_info);

  switch (type) {
    case ledger::ReportType::GRANT:
      report_info.grants_ = braveledger_bat_bignum::sum(report_info.grants_, probi);
      break;
    case ledger::ReportType::AUTO_CONTRIBUTION:
      report_info.auto_contribute_ = braveledger_bat_bignum::sum(report_info.auto_contribute_, probi);
      break;
    case ledger::ReportType::DONATION:
      report_info.one_time_donation_ = braveledger_bat_bignum::sum(report_info.one_time_donation_, probi);
      break;
    case ledger::ReportType::DONATION_RECURRING:
      report_info.recurring_donation_ = braveledger_bat_bignum::sum(report_info.recurring_donation_, probi);
      break;
    default:
      break;
  }

  setBalanceReport(month, year, report_info);
}

void BatPublishers::getPublisherBanner(const std::string& publisher_id,
                                       ledger::PublisherBannerCallback callback) {
  ledger::PublisherBanner banner;
  banner.publisher_key = publisher_id;

  if (!server_list_.empty()) {
    auto result = server_list_.find(publisher_id);

    if (result != server_list_.end()) {
      const braveledger_bat_helper::SERVER_LIST values = result->second;

      banner.title = values.banner.title_;
      banner.description = values.banner.description_;
      banner.background = values.banner.background_;
      banner.logo = values.banner.logo_;
      banner.amounts = values.banner.amounts_;
      banner.social = values.banner.social_;
    }
  }

  uint64_t currentReconcileStamp = ledger_->GetReconcileStamp();
  auto filter = CreatePublisherFilter(publisher_id,
      ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE,
      ledger::PUBLISHER_MONTH::ANY,
      -1,
      ledger::PUBLISHER_EXCLUDE_FILTER::FILTER_ALL,
      false,
      currentReconcileStamp);

  ledger::PublisherInfoCallback callbackGetPublisher = std::bind(&BatPublishers::onPublisherBanner,
                                      this,
                                      callback,
                                      banner,
                                      _1,
                                      _2);

  ledger_->GetPublisherInfo(filter, callbackGetPublisher);

}

void BatPublishers::onPublisherBanner(ledger::PublisherBannerCallback callback,
                                      ledger::PublisherBanner banner,
                                      ledger::Result result,
                                      std::unique_ptr<ledger::PublisherInfo> publisher_info) {

  auto new_banner = std::make_unique<ledger::PublisherBanner>(banner);

  if (result != ledger::Result::LEDGER_OK) {
    callback(std::move(new_banner));
    return;
  }

  new_banner->name = publisher_info->name;

  if (new_banner->logo.empty()) {
    new_banner->logo = publisher_info->favicon_url;
  }

  callback(std::move(new_banner));
}

}  // namespace braveledger_bat_publisher
