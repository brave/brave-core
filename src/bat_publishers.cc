/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat_publishers.h"

#include <cmath>
#include <algorithm>

#include "bat_helper.h"
#include "ledger_impl.h"
#include "leveldb/db.h"
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
  state_(new braveledger_bat_helper::PUBLISHER_STATE_ST) {
  calcScoreConsts();
}

BatPublishers::~BatPublishers() {
}

void BatPublishers::calcScoreConsts() {
  //TODO: check Warning	C4244	'=': conversion from 'double' to 'unsigned int', possible loss of data
  a_ = 1.0 / (braveledger_ledger::_d * 2.0) - state_->min_pubslisher_duration_;
  a2_ = a_ * 2;
  a4_ = a2_ * 2;
  b_ = state_->min_pubslisher_duration_ - a_;
  b2_ = b_ * b_;
}

// courtesy of @dimitry-xyz: https://github.com/brave/ledger/issues/2#issuecomment-221752002
double BatPublishers::concaveScore(const uint64_t& duration) {
  return (std::sqrt(b2_ + a4_ * duration) - b_) / (double)a2_;
}

void BatPublishers::initSynopsis() {
  ledger_->LoadPublisherState(this);
}

const ledger::PublisherInfo::id_type getPublisherID(
    const ledger::VisitData& visit_data) {
  return visit_data.tld;
}

std::string getProviderName(const ledger::PublisherInfo::id_type publisher_id) {
  // TODO - this is for the media stuff
  return "";
}

bool ignoreMinTime(const ledger::PublisherInfo::id_type publisher_id) {
  return !getProviderName(publisher_id).empty();
}

void BatPublishers::saveVisit(const ledger::VisitData& visit_data) {
  const ledger::PublisherInfo::id_type publisher_id =
      getPublisherID(visit_data);

  if (!ignoreMinTime(publisher_id) &&
      visit_data.duration < state_->min_pubslisher_duration_)
    return;

  ledger_->GetPublisherInfo(publisher_id,
      std::bind(&BatPublishers::saveVisitInternal, this,
                    publisher_id, visit_data, _1, _2));
}

void onVisitSavedDummy(ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> publisher_info) {
  // onPublisherInfoUpdated will always be called by LedgerImpl so do nothing
}

void BatPublishers::saveVisitInternal(
    ledger::PublisherInfo::id_type publisher_id,
    ledger::VisitData visit_data,
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> publisher_info) {
  if (result != ledger::Result::OK) {
    // TODO error handling
    return;
  }

  if (!publisher_info.get())
    publisher_info.reset(new ledger::PublisherInfo(publisher_id));

  publisher_info->duration += visit_data.duration;
  publisher_info->visits += 1;
  publisher_info->score += concaveScore(visit_data.duration);

  ledger_->SetPublisherInfo(std::move(publisher_info),
      std::bind(&onVisitSavedDummy, _1, _2));
}

std::unique_ptr<ledger::PublisherInfo> BatPublishers::onPublisherInfoUpdated(
    ledger::Result result, std::unique_ptr<ledger::PublisherInfo> info) {
  if (result != ledger::Result::OK) {
    // TODO error handling
    return info;
  }

  if (!isEligableForContribution(*info)) {
    publishers_.erase(info->id);
    return info;
  }

  const ledger::PublisherInfo::id_type& publisher_id = info->id;

  auto publisher = publishers_.find(info->id);
  if (publisher != publishers_.end()) {
    publishers_[publisher_id] = braveledger_bat_helper::PUBLISHER_ST();
    publishers_[publisher_id].id_ = publisher_id;
  }

  publishers_[publisher_id].duration_ = info->duration;
  publishers_[publisher_id].score_ = info->score;
  publishers_[publisher_id].visits_ = info->visits;
  publishers_[publisher_id].percent_ = info->percent;
  publishers_[publisher_id].weight_ = info->weight;

  synopsisNormalizer();

  return info;
}

void BatPublishers::setPublisherMinVisitTime(const uint64_t& duration) { // In milliseconds
  state_->min_pubslisher_duration_ = duration; //TODO: conversion from 'const uint64_t' to 'unsigned int', possible loss of data
  saveState();
}

void BatPublishers::setPublisherMinVisits(const unsigned int& visits) {
  state_->min_visits_ = visits;
  saveState();
}

void BatPublishers::setPublisherAllowNonVerified(const bool& allow) {
  state_->allow_non_verified_ = allow;
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

void BatPublishers::synopsisNormalizer() {
  LOG(ERROR)<<"BatPublishers::synopsisNormalizer";
  double totalScores = 0.0;
  for (std::map<std::string, braveledger_bat_helper::PUBLISHER_ST>::const_iterator iter = publishers_.begin(); iter != publishers_.end(); iter++) {
    totalScores += iter->second.score_;
  }
  std::vector<unsigned int> percents;
  std::vector<double> weights;
  std::vector<double> realPercents;
  std::vector<double> roundoffs;
  unsigned int totalPercents = 0;
  for (std::map<std::string, braveledger_bat_helper::PUBLISHER_ST>::iterator iter = publishers_.begin(); iter != publishers_.end(); iter++) {
    realPercents.push_back((double)iter->second.score_ / (double)totalScores * 100.0);
    percents.push_back((unsigned int)std::lround(realPercents[realPercents.size() - 1]));
    double roundoff = percents[percents.size() - 1] - realPercents[realPercents.size() - 1];
    if (roundoff < 0.0) {
      roundoff *= -1.0;
    }
    roundoffs.push_back(roundoff);
    totalPercents += percents[percents.size() - 1];
    // TODO make pinned, unpinned publishers
    weights.push_back((double)iter->second.score_ / (double)publishers_.size() * 100.0);
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
  for (std::map<std::string, braveledger_bat_helper::PUBLISHER_ST>::iterator iter = publishers_.begin(); iter != publishers_.end(); iter++) {
    iter->second.percent_ = percents[currentValue];
    iter->second.weight_ = weights[currentValue];
    currentValue++;
  }
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

  for (std::map<std::string, braveledger_bat_helper::PUBLISHER_ST>::const_iterator iter = publishers_.begin(); iter != publishers_.end(); iter++)
    res.push_back(iter->second);

  std::sort(res.begin(), res.end());

  return res;
}

bool BatPublishers::isVerified(const ledger::PublisherInfo& publisher_id) {
  // TODO - implement bloom filter
  return true;
}

bool BatPublishers::isEligableForContribution(const ledger::PublisherInfo& info) {

  if (info.excluded || (!state_->allow_non_verified_ && !isVerified(info.id)))
    return false;

  return info.score > 0 &&
    info.duration >= state_->min_pubslisher_duration_ &&
    info.visits >= state_->min_visits_;

}

void BatPublishers::saveState() {
  std::string data;
  braveledger_bat_helper::saveToJsonString(*state_, data);
  ledger_->SavePublisherState(data, this);
}

void BatPublishers::loadState(const std::string& data) {
  braveledger_bat_helper::PUBLISHER_STATE_ST state;
  braveledger_bat_helper::loadFromJson(state, data.c_str());
  state_.reset(new braveledger_bat_helper::PUBLISHER_STATE_ST(state));
  calcScoreConsts();
}

void BatPublishers::OnPublisherStateLoaded(ledger::Result result,
                                           const std::string& data) {
  if (result != ledger::Result::OK) {
    LOG(ERROR) << "Could not load publisher state";
    return;
    // TODO - error handling
  }

  loadState(data);
}

void BatPublishers::OnPublisherStateSaved(ledger::Result result) {
  if (result != ledger::Result::OK) {
    LOG(ERROR) << "Could not save publisher state";
    // TODO - error handling
    return;
  }
  synopsisNormalizer();
}

}  // namespace braveledger_bat_publisher
