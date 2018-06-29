/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cmath>
#include <algorithm>

#if defined CHROMIUM_BUILD
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/browser_process.h"
#include "logging.h"

#endif

#include "bat_helper.h"
#include "bat_publishers.h"
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

static bool winners_votes_compare(const braveledger_bat_helper::WINNERS_ST& first, const braveledger_bat_helper::WINNERS_ST& second){
    return (first.votes_ < second.votes_);
}


namespace braveledger_bat_publishers {

BatPublishers::BatPublishers():
  level_db_(nullptr) {
  calcScoreConsts();
}

BatPublishers::~BatPublishers() {
}


void BatPublishers::calcScoreConsts() {
  //TODO: check Warning	C4244	'=': conversion from 'double' to 'unsigned int', possible loss of data
  a_ = 1.0 / (braveledger_ledger::_d * 2.0) - state_.min_pubslisher_duration_;
  a2_ = a_ * 2;
  a4_ = a2_ * 2;
  b_ = state_.min_pubslisher_duration_ - a_;
  b2_ = b_ * b_;
}

void BatPublishers::openPublishersDB() {
  std::string pubDbPath;
  braveledger_bat_helper::getDbFile(PUBLISHERS_DB_NAME, pubDbPath);
  
  level_db_.reset(nullptr); //release the existing db connection
  leveldb::Options options;
  options.create_if_missing = true;
  leveldb::DB * db_ptr = nullptr;
  leveldb::Status status = leveldb::DB::Open(options, pubDbPath, &db_ptr);
  if (!status.ok() || db_ptr == nullptr) {
      if (db_ptr != nullptr) {
        delete db_ptr;
      }
      LOG(ERROR) << "openPublishersDB level db open error " << pubDbPath.c_str();
  }
  else  {
    level_db_.reset(db_ptr);
  }
}

void BatPublishers::loadPublishers() {
  openPublishersDB();
  if (!level_db_) {
    assert(false);
    LOG(ERROR) << "loadPublishers level db is not initialized";

    return;
  }

  std::lock_guard<std::mutex> guard(publishers_map_mutex_);
  leveldb::Iterator* it = level_db_->NewIterator(leveldb::ReadOptions());
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    std::string publisher = it->key().ToString();
    braveledger_bat_helper::PUBLISHER_ST publisher_st;
    std::string publisher_value = it->value().ToString();
    braveledger_bat_helper::loadFromJson(publisher_st, publisher_value);
    publishers_[publisher] = publisher_st;
  }
  assert(it->status().ok());  // Check for any errors found during the scan
  delete it;
}

void BatPublishers::loadStateCallback(bool result, const braveledger_bat_helper::PUBLISHER_STATE_ST& state) {
  if (!result) {
    return;
  }
  state_ = state;
  calcScoreConsts();
}

void BatPublishers::initSynopsis() {

  braveledger_bat_helper::ReadPublisherStateCallback runnable1 = braveledger_bat_helper::bat_mem_fun_binder2(*this, &BatPublishers::loadStateCallback);
  braveledger_bat_helper::loadPublisherState(runnable1);  

  auto runnable2 = braveledger_bat_helper::bat_mem_fun_binder(*this, &BatPublishers::loadPublishers);
  braveledger_bat_helper::PostTask(runnable2);
}

void BatPublishers::saveVisitInternal(const std::string& publisher, const uint64_t& duration,
    BatPublishers::SaveVisitCallback callback) {
  double currentScore = concaveScore(duration);

  std::string stringifiedPublisher;
  uint64_t verifiedTimestamp = 0;
  {
    std::lock_guard<std::mutex> guard(publishers_map_mutex_);
    std::map<std::string, braveledger_bat_helper::PUBLISHER_ST>::iterator iter = publishers_.find(publisher);
    if (publishers_.end() == iter) {
      braveledger_bat_helper::PUBLISHER_ST publisher_st;
      publisher_st.duration_ = duration;
      publisher_st.score_ = currentScore;
      publisher_st.visits_ = 1;
      publishers_[publisher] = publisher_st;      
      braveledger_bat_helper::saveToJson(publisher_st, stringifiedPublisher);
    } else {
      iter->second.duration_ += duration;
      iter->second.score_ += currentScore;
      iter->second.visits_ += 1;
      verifiedTimestamp = iter->second.verifiedTimeStamp_;      
      braveledger_bat_helper::saveToJson(iter->second, stringifiedPublisher);
    }
    if (!level_db_) {
      assert(false);

      return;
    }

    // Save the publisher to the database
    leveldb::Status status = level_db_->Put(leveldb::WriteOptions(), publisher, stringifiedPublisher);
    assert(status.ok());
  }
  
  braveledger_bat_helper::run_runnable <BatPublishers::SaveVisitCallback, const std::string&, const uint64_t& >(callback, publisher, std::cref(verifiedTimestamp) );  
  synopsisNormalizerInternal();
}

void BatPublishers::saveVisit(const std::string& publisher, const uint64_t& duration,
    BatPublishers::SaveVisitCallback callback, bool ignoreMinTime) {
  if (!ignoreMinTime && duration < state_.min_pubslisher_duration_) {
    return;
  }

  // TODO checks if the publisher verified, disabled and etc
  auto runnable = braveledger_bat_helper::bat_mem_fun_binder(*this, &BatPublishers::saveVisitInternal, publisher, duration, callback);
  braveledger_bat_helper::PostTask(runnable);
}

void BatPublishers::setPublisherTimestampVerifiedInternal(const std::string& publisher,
    const uint64_t& verifiedTimestamp, const bool& verified) {
  {
    std::string stringifiedPublisher;
    std::lock_guard<std::mutex> guard(publishers_map_mutex_);
    std::map<std::string, braveledger_bat_helper::PUBLISHER_ST>::iterator iter = publishers_.find(publisher);
    if (publishers_.end() == iter) {
      assert(false);

      return;
    } else {
      iter->second.verified_ = verified;
      iter->second.verifiedTimeStamp_ = verifiedTimestamp;      
      braveledger_bat_helper::saveToJson(iter->second, stringifiedPublisher);
    }
    if (!level_db_) {
      assert(false);

      return;
    }

    // Save the publisher to the database
    leveldb::Status status = level_db_->Put(leveldb::WriteOptions(), publisher, stringifiedPublisher);
    assert(status.ok());
  }
  synopsisNormalizerInternal();
}

void BatPublishers::setPublisherTimestampVerified(const std::string& publisher,
    const uint64_t& verifiedTimestamp, const bool& verified) {

  auto runnable = braveledger_bat_helper::bat_mem_fun_binder(*this, &BatPublishers::setPublisherTimestampVerifiedInternal, publisher, verifiedTimestamp, verified);
  braveledger_bat_helper::PostTask(runnable);
}

void BatPublishers::setPublisherFavIconInternal(const std::string& publisher, const std::string& favicon_url) {
  std::string stringifiedPublisher;
  std::lock_guard<std::mutex> guard(publishers_map_mutex_);
  std::map<std::string, braveledger_bat_helper::PUBLISHER_ST>::iterator iter = publishers_.find(publisher);
  if (publishers_.end() == iter) {
    braveledger_bat_helper::PUBLISHER_ST publisher_st;
    publisher_st.favicon_url_ = favicon_url;
    publishers_[publisher] = publisher_st;    
    braveledger_bat_helper::saveToJson(publisher_st, stringifiedPublisher);
  } else {
    iter->second.favicon_url_ = favicon_url;    
    braveledger_bat_helper::saveToJson(iter->second, stringifiedPublisher);
  }
  if (!level_db_) {
    assert(false);

    return;
  }

  // Save the publisher to the database
  leveldb::Status status = level_db_->Put(leveldb::WriteOptions(), publisher, stringifiedPublisher);
  assert(status.ok());
}

void BatPublishers::setPublisherFavIcon(const std::string& publisher, const std::string& favicon_url) {

  auto runnable = braveledger_bat_helper::bat_mem_fun_binder(*this, &BatPublishers::setPublisherFavIconInternal, publisher, favicon_url);
  braveledger_bat_helper::PostTask(runnable);
}

void BatPublishers::setPublisherIncludeInternal(const std::string& publisher, const bool& include) {
  {
    std::string stringifiedPublisher;
    std::lock_guard<std::mutex> guard(publishers_map_mutex_);
    std::map<std::string, braveledger_bat_helper::PUBLISHER_ST>::iterator iter = publishers_.find(publisher);
    if (publishers_.end() == iter) {
      braveledger_bat_helper::PUBLISHER_ST publisher_st;
      publisher_st.exclude_ = !include;
      publishers_[publisher] = publisher_st;      
      braveledger_bat_helper::saveToJson(publisher_st, stringifiedPublisher);
    } else {
      iter->second.exclude_ = !include;      
      braveledger_bat_helper::saveToJson(iter->second, stringifiedPublisher);
    }
    if (!level_db_) {
      assert(false);

      return;
    }

    // Save the publisher to the database
    leveldb::Status status = level_db_->Put(leveldb::WriteOptions(), publisher, stringifiedPublisher);
    assert(status.ok());
  }
  synopsisNormalizerInternal();
}

void BatPublishers::setPublisherInclude(const std::string& publisher, const bool& include) {

  auto runnable = braveledger_bat_helper::bat_mem_fun_binder(*this, &BatPublishers::setPublisherIncludeInternal, publisher, include);
  braveledger_bat_helper::PostTask(runnable);
}

void BatPublishers::setPublisherDeletedInternal(const std::string& publisher, const bool& deleted) {
  {
    std::string stringifiedPublisher;
    std::lock_guard<std::mutex> guard(publishers_map_mutex_);
    std::map<std::string, braveledger_bat_helper::PUBLISHER_ST>::iterator iter = publishers_.find(publisher);
    if (publishers_.end() == iter) {
      braveledger_bat_helper::PUBLISHER_ST publisher_st;
      publisher_st.deleted_ = deleted;
      publishers_[publisher] = publisher_st;      
      braveledger_bat_helper::saveToJson(publisher_st, stringifiedPublisher);
    } else {
      iter->second.deleted_ = deleted;      
      braveledger_bat_helper::saveToJson(iter->second, stringifiedPublisher);
    }
    if (!level_db_) {
      assert(false);
      return;
    }

    // Save the publisher to the database
    leveldb::Status status = level_db_->Put(leveldb::WriteOptions(), publisher, stringifiedPublisher);
    assert(status.ok());
  }
  synopsisNormalizerInternal();
}

void BatPublishers::setPublisherDeleted(const std::string& publisher, const bool& deleted) {
  auto runnable = braveledger_bat_helper::bat_mem_fun_binder(*this, &BatPublishers::setPublisherDeletedInternal, publisher, deleted);
  braveledger_bat_helper::PostTask(runnable);
}

void BatPublishers::setPublisherPinPercentageInternal(const std::string& publisher, const bool& pinPercentage) {
  {
    std::string stringifiedPublisher;
    std::lock_guard<std::mutex> guard(publishers_map_mutex_);
    std::map<std::string, braveledger_bat_helper::PUBLISHER_ST>::iterator iter = publishers_.find(publisher);
    if (publishers_.end() == iter) {
      braveledger_bat_helper::PUBLISHER_ST publisher_st;
      publisher_st.pinPercentage_ = pinPercentage;
      publishers_[publisher] = publisher_st;      
      braveledger_bat_helper::saveToJson(publisher_st, stringifiedPublisher);
    } else {
      iter->second.pinPercentage_ = pinPercentage;      
      braveledger_bat_helper::saveToJson(iter->second, stringifiedPublisher);
    }
    if (!level_db_) {
      assert(false);
      return;
    }

    // Save the publisher to the database
    leveldb::Status status = level_db_->Put(leveldb::WriteOptions(), publisher, stringifiedPublisher);
    assert(status.ok());
  }
  synopsisNormalizerInternal();
}

void BatPublishers::setPublisherPinPercentage(const std::string& publisher, const bool& pinPercentage) {
  auto runnable = braveledger_bat_helper::bat_mem_fun_binder(*this, &BatPublishers::setPublisherPinPercentageInternal, publisher, pinPercentage);
  braveledger_bat_helper::PostTask(runnable);
}

void BatPublishers::setPublisherMinVisitTime(const uint64_t& duration) { // In milliseconds
  state_.min_pubslisher_duration_ = duration; //TODO: conversion from 'const uint64_t' to 'unsigned int', possible loss of data
  braveledger_bat_helper::savePublisherState(state_);
  synopsisNormalizer();
}

void BatPublishers::setPublisherMinVisits(const unsigned int& visits) {
  state_.min_visits_ = visits;
  braveledger_bat_helper::savePublisherState(state_);
  synopsisNormalizer();
}

void BatPublishers::setPublisherAllowNonVerified(const bool& allow) {
  state_.allow_non_verified_ = allow;
  braveledger_bat_helper::savePublisherState(state_);
  synopsisNormalizer();
}

std::vector<braveledger_bat_helper::PUBLISHER_DATA_ST> BatPublishers::getPublishersData() {
  std::vector<braveledger_bat_helper::PUBLISHER_DATA_ST> res;

  std::lock_guard<std::mutex> guard(publishers_map_mutex_);
  for (std::map<std::string, braveledger_bat_helper::PUBLISHER_ST>::const_iterator iter = publishers_.begin(); iter != publishers_.end(); iter++) {
    braveledger_bat_helper::PUBLISHER_DATA_ST publisherData;
    publisherData.publisherKey_ = iter->first;
    publisherData.publisher_ = iter->second;
    // TODO check all of that
    if (iter->second.duration_ >= braveledger_ledger::_milliseconds_day) {
      publisherData.daysSpent_ = std::max((int)std::lround((double)iter->second.duration_ / (double)braveledger_ledger::_milliseconds_day), 1);
    } else if (iter->second.duration_ >= braveledger_ledger::_milliseconds_hour) {
      publisherData.hoursSpent_ = std::max((int)((double)iter->second.duration_ / (double)braveledger_ledger::_milliseconds_hour), 1);
      publisherData.minutesSpent_ = std::lround((double)(iter->second.duration_ % braveledger_ledger::_milliseconds_hour) / (double)braveledger_ledger::_milliseconds_minute);
    } else if (iter->second.duration_ >= braveledger_ledger::_milliseconds_minute) {
      publisherData.minutesSpent_ = std::max((int)((double)iter->second.duration_ / (double)braveledger_ledger::_milliseconds_minute), 1);
      publisherData.secondsSpent_ = std::lround((double)(iter->second.duration_ % braveledger_ledger::_milliseconds_minute) / (double)braveledger_ledger::_milliseconds_second);
    } else {
      publisherData.secondsSpent_ = std::max((int)std::lround((double)iter->second.duration_ / (double)braveledger_ledger::_milliseconds_second), 1);
    }
    res.push_back(publisherData);
  }

  return res;
}

bool BatPublishers::isPublisherVisible(const braveledger_bat_helper::PUBLISHER_ST& publisher_st) {
  if (publisher_st.deleted_ || (!state_.allow_non_verified_ && !publisher_st.verified_)) {
    return false;
  }

  return publisher_st.score_ > 0 &&
    publisher_st.duration_ >= state_.min_pubslisher_duration_ &&
    publisher_st.visits_ >= state_.min_visits_;
}

void BatPublishers::synopsisNormalizerInternal() {

  LOG(ERROR)<<"BatPublishers::synopsisNormalizerInternal";
  std::lock_guard<std::mutex> guard(publishers_map_mutex_);
  double totalScores = 0.0;
  for (std::map<std::string, braveledger_bat_helper::PUBLISHER_ST>::const_iterator iter = publishers_.begin(); iter != publishers_.end(); iter++) {
    if (!isPublisherVisible(iter->second)) {
      continue;
    }
    totalScores += iter->second.score_;
  }
  std::vector<unsigned int> percents;
  std::vector<double> weights;
  std::vector<double> realPercents;
  std::vector<double> roundoffs;
  unsigned int totalPercents = 0;
  for (std::map<std::string, braveledger_bat_helper::PUBLISHER_ST>::iterator iter = publishers_.begin(); iter != publishers_.end(); iter++) {
    if (!isPublisherVisible(iter->second)) {
      //LOG(ERROR) << "!!!not visible " << iter->first;
      continue;
    }
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
    if (!isPublisherVisible(iter->second)) {
      continue;
    }
    iter->second.percent_ = percents[currentValue];
    iter->second.weight_ = weights[currentValue];
    currentValue++;
  }
}

void BatPublishers::synopsisNormalizer() {
  auto runnable = braveledger_bat_helper::bat_mem_fun_binder(*this, &BatPublishers::synopsisNormalizerInternal);
  braveledger_bat_helper::PostTask(runnable);
}

std::vector<braveledger_bat_helper::WINNERS_ST> BatPublishers::winners(const unsigned int& ballots) {
  std::vector<braveledger_bat_helper::WINNERS_ST> res;

  std::vector<braveledger_bat_helper::PUBLISHER_DATA_ST> top = topN();
  unsigned int totalVotes = 0;
  std::vector<unsigned int> votes;
  // TODO there is underscore.shuffle
  for (size_t i = 0; i < top.size(); i++) {
    LOG(ERROR) << "!!!name == " << top[i].publisherKey_ << ", score == " << top[i].publisher_.score_;
    if (top[i].publisher_.percent_ <= 0) {
      continue;
    }
    braveledger_bat_helper::WINNERS_ST winner;
    winner.votes_ = (unsigned int)std::lround((double)top[i].publisher_.percent_ * (double)ballots / 100.0);
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

std::vector<braveledger_bat_helper::PUBLISHER_DATA_ST> BatPublishers::topN() {
  std::vector<braveledger_bat_helper::PUBLISHER_DATA_ST> res;

  std::lock_guard<std::mutex> guard(publishers_map_mutex_);
  for (std::map<std::string, braveledger_bat_helper::PUBLISHER_ST>::const_iterator iter = publishers_.begin(); iter != publishers_.end(); iter++) {
    if (0 == iter->second.score_
        || state_.min_pubslisher_duration_ > iter->second.duration_
        || state_.min_visits_ > iter->second.visits_) {
      continue;
    }
    braveledger_bat_helper::PUBLISHER_DATA_ST publisherData;
    publisherData.publisherKey_ = iter->first;
    publisherData.publisher_ = iter->second;
    res.push_back(publisherData);
  }

  std::sort(res.begin(), res.end());

  return res;
}

bool BatPublishers::isEligableForContribution(const braveledger_bat_helper::PUBLISHER_DATA_ST& publisherData) {
  return !publisherData.publisher_.exclude_ && isPublisherVisible(publisherData.publisher_);
}

// courtesy of @dimitry-xyz: https://github.com/brave/ledger/issues/2#issuecomment-221752002
double BatPublishers::concaveScore(const uint64_t& duration) {
  return (std::sqrt(b2_ + a4_ * duration) - b_) / (double)a2_;
}

} //namespace braveledger_bat_publisher
