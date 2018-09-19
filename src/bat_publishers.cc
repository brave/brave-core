/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat_publishers.h"

#include <ctime>
#include <cmath>
#include <algorithm>

#include "bat_helper.h"
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

static bool winners_votes_compare(const braveledger_bat_helper::WINNERS_ST& first, const braveledger_bat_helper::WINNERS_ST& second){
    return (first.votes_ < second.votes_);
}

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
  uint64_t min_duration_ms = state_->min_pubslisher_duration_ * 1000;
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
  if (publisher_id.find(YOUTUBE_PROVIDER_NAME) != std::string::npos) {
    return YOUTUBE_PROVIDER_NAME;
  } else if (publisher_id.find(TWITCH_PROVIDER_NAME) != std::string::npos) {
    return TWITCH_PROVIDER_NAME;
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
                                      payment_data.local_year,
                                      ledger::PUBLISHER_EXCLUDE::DEFAULT);
  ledger_->GetPublisherInfo(filter,
      std::bind(&BatPublishers::makePaymentInternal, this,
          payment_data, _1, _2));
}

bool BatPublishers::saveVisitAllowed() const {
  return (ledger_->GetRewardsMainEnabled() && ledger_->GetAutoContribute());
}

void BatPublishers::saveVisit(const std::string& publisher_id,
                              const ledger::VisitData& visit_data,
                              const uint64_t& duration) {
  if (saveVisitAllowed()) {
    if (publisher_id.empty() || (!ignoreMinTime(publisher_id) &&
        duration < state_->min_pubslisher_duration_))
      return;

    auto filter = CreatePublisherFilter(publisher_id,
        ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE,
        visit_data.local_month,
        visit_data.local_year,
        ledger::PUBLISHER_EXCLUDE::DEFAULT);
    ledger_->GetPublisherInfo(filter,
        std::bind(&BatPublishers::saveVisitInternal, this,
            publisher_id, visit_data, duration, _1, _2));
  }
}

ledger::PublisherInfoFilter BatPublishers::CreatePublisherFilter(
    const std::string& publisher_id,
    ledger::PUBLISHER_CATEGORY category,
    ledger::PUBLISHER_MONTH month,
    int year,
    ledger::PUBLISHER_EXCLUDE excluded) {
  ledger::PublisherInfoFilter filter;
  filter.id = publisher_id;
  filter.category = category;
  filter.month = month;
  filter.year = year;
  filter.excluded = excluded;

  return filter;
}

std::string BatPublishers::GetBalanceReportName(
    const ledger::PUBLISHER_MONTH month,
    int year) {
  return std::to_string(year) + "_" + std::to_string(month);
}

void onVisitSavedDummy(ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> publisher_info) {
  // onPublisherInfoUpdated will always be called by LedgerImpl so do nothing
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
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> publisher_info) {
  DCHECK(result != ledger::Result::TOO_MANY_RESULTS);
  if (result != ledger::Result::LEDGER_OK && result != ledger::Result::NOT_FOUND) {
    // TODO error handling
    return;
  }

  if (!publisher_info.get())
    publisher_info.reset(new ledger::PublisherInfo(publisher_id,
                                                   visit_data.local_month,
                                                   visit_data.local_year));

  if (isExcluded(publisher_info->id, publisher_info->excluded)) {
    return;
  }


  publisher_info->favicon_url = visit_data.favicon_url;
  publisher_info->name = visit_data.name;
  publisher_info->provider = visit_data.provider;
  publisher_info->url = visit_data.url;
  publisher_info->duration += duration;
  publisher_info->visits += 1;
  publisher_info->category = ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE;
  publisher_info->score += concaveScore(duration);
  publisher_info->verified = isVerified(publisher_info->id);

  ledger_->SetPublisherInfo(std::move(publisher_info),
      std::bind(&onVisitSavedDummy, _1, _2));
}

std::unique_ptr<ledger::PublisherInfo> BatPublishers::onPublisherInfoUpdated(
    ledger::Result result, std::unique_ptr<ledger::PublisherInfo> info) {
  if (result != ledger::Result::LEDGER_OK || !info.get()) {
    // TODO error handling
    return info;
  }

  if (!isEligableForContribution(*info)) {
    return info;
  }

  synopsisNormalizer(*info);

  return info;
}

void BatPublishers::setExclude(const std::string& publisher_id, const ledger::PUBLISHER_EXCLUDE& exclude) {
  auto filter = CreatePublisherFilter(publisher_id,
      ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE,
      ledger::PUBLISHER_MONTH::ANY,
      -1,
      (exclude == ledger::PUBLISHER_EXCLUDE::DEFAULT)
      ? ledger::PUBLISHER_EXCLUDE::EXCLUDED
      : ledger::PUBLISHER_EXCLUDE::DEFAULT);
  ledger_->GetPublisherInfo(filter, std::bind(&BatPublishers::onSetExcludeInternal,
                            this, exclude, _1, _2));
}

void BatPublishers::onSetExcludeInternal(ledger::PUBLISHER_EXCLUDE exclude,
                                         ledger::Result result,
                                         std::unique_ptr<ledger::PublisherInfo> publisher_info) {
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    return;
  }

  publisher_info->year = -1;
  publisher_info->excluded = exclude;
  publisher_info->month = ledger::PUBLISHER_MONTH::ANY;

  ledger_->SetPublisherInfo(std::move(publisher_info),
      std::bind(&onVisitSavedDummy, _1, _2));
}

void BatPublishers::restorePublishers() {
  auto filter = CreatePublisherFilter("",
      ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE,
      ledger::PUBLISHER_MONTH::ANY,
      -1,
      ledger::PUBLISHER_EXCLUDE::EXCLUDED);
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
  state_->min_pubslisher_duration_ = duration;
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

void BatPublishers::setPublisherAllowNonVerified(const bool& allow) {
  state_->allow_non_verified_ = allow;
  saveState();
}

void BatPublishers::setPublisherAllowVideos(const bool& allow) {
  state_->allow_videos_ = allow;
  saveState();
}

uint64_t BatPublishers::getPublisherMinVisitTime() const {
  return state_->min_pubslisher_duration_;
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

void BatPublishers::synopsisNormalizerInternal(const ledger::PublisherInfoList& oldList, uint32_t /* next_record */) {
  // TODO SZ: We can pass non const value here to avoid copying
  ledger::PublisherInfoList list = oldList;
  //LOG(ERROR) << "!!!list.size() == " << list.size();
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
    //LOG(ERROR) << "!!!publisher_id == " << list[i].id;
    //LOG(ERROR) << "!!!new percent == " << list[i].percent;
    //LOG(ERROR) << "!!!new weight == " << list[i].weight;
    currentValue++;
    std::unique_ptr<ledger::PublisherInfo> publisher_info;
    publisher_info.reset(new ledger::PublisherInfo(list[i]));
    ledger_->SetPublisherInfo(std::move(publisher_info),
      std::bind(&onVisitSavedDummy, _1, _2));
  }
}

void BatPublishers::synopsisNormalizer(const ledger::PublisherInfo& info) {
  auto filter = CreatePublisherFilter("",
      ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE,
      info.month,
      info.year,
      ledger::PUBLISHER_EXCLUDE::DEFAULT);
  // TODO SZ: We pull the whole list currently, I don't think it consumes lots of RAM, but could.
  // We need to limit it and iterate.
  ledger_->GetPublisherInfoList(0, 0, filter, std::bind(&BatPublishers::synopsisNormalizerInternal, this,
          _1, _2));
}

std::vector<braveledger_bat_helper::WINNERS_ST> BatPublishers::winners(const unsigned int& ballots) {
  std::vector<braveledger_bat_helper::WINNERS_ST> res;
  std::vector<braveledger_bat_helper::PUBLISHER_ST> top = topN();
  unsigned int totalVotes = 0;
  std::vector<unsigned int> votes;
  // TODO there is underscore.shuffle
  for (size_t i = 0; i < top.size(); i++) {
    LOG(ERROR) << "!!!name == " << top[i].id_ << ", score == " << top[i].score_;
    if (top[i].percent_ <= 0) {
      continue;
    }
    braveledger_bat_helper::WINNERS_ST winner;
    winner.votes_ = (unsigned int)std::lround((double)top[i].percent_ * (double)ballots / 100.0);
    totalVotes += winner.votes_;
    winner.publisher_data_ = top[i];
    res.push_back(winner);
  }
  if (res.size()) {
    while (totalVotes > ballots) {
      std::vector<braveledger_bat_helper::WINNERS_ST>::iterator max = std::max_element(res.begin(), res.end(), winners_votes_compare);
      (max->votes_)--;
      totalVotes--;
    }
  }

  return res;
}

std::vector<braveledger_bat_helper::PUBLISHER_ST> BatPublishers::topN() {
  std::vector<braveledger_bat_helper::PUBLISHER_ST> res;

  // TODO: we need to implement it for reconcile based on the current reconcile month
  // for (std::map<std::string, braveledger_bat_helper::PUBLISHER_ST>::const_iterator iter = publishers_.begin(); iter != publishers_.end(); iter++)
  //   res.push_back(iter->second);

  // std::sort(res.begin(), res.end());

  return res;
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

bool BatPublishers::isEligableForContribution(const ledger::PublisherInfo& info) {

  if (isExcluded(info.id, info.excluded) || (!state_->allow_non_verified_ && !isVerified(info.id)))
    return false;

  return info.score > 0 &&
    info.duration >= state_->min_pubslisher_duration_ &&
    info.visits >= state_->min_visits_;

}

void BatPublishers::setBalanceReport(ledger::PUBLISHER_MONTH month,
                                int year,
                                const ledger::BalanceReportInfo& report_info) {
  braveledger_bat_helper::REPORT_BALANCE_ST report_balance;
  report_balance.opening_balance_ = report_info.opening_balance_;
  report_balance.closing_balance_ = report_info.closing_balance_;
  report_balance.grants_ = report_info.grants_;
  report_balance.earning_from_ads_ = report_info.earning_from_ads_;
  report_balance.auto_contribute_ = report_info.auto_contribute_;
  report_balance.recurring_donation_ = report_info.recurring_donation_;
  report_balance.one_time_donation_ = report_info.one_time_donation_;

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
    LOG(ERROR) << "Could not save publisher state";
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

}  // namespace braveledger_bat_publisher
