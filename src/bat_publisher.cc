/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat_publisher.h"
#include "leveldb/db.h"
#include "base/sequenced_task_runner.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/files/file_util.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/browser_process.h"
#include "static_values.h"
#include <cmath>
#include <algorithm>

#include "logging.h"

/* foo.bar.example.com
   QLD = ‘bar’
   RLD = ‘foo.bar’
   SLD = ‘example.com’
   TLD = ‘com’

  search.yahoo.co.jp
   QLD = ‘search’
   RLD = ‘search’
   SLD = ‘yahoo.co.jp’
   TLD = ‘co.jp’
*/

static bool winners_votes_compare(const WINNERS_ST& first, const WINNERS_ST& second){
    return (first.votes_ < second.votes_);
}


namespace bat_publisher {

BatPublisher::BatPublisher():
  level_db_(nullptr) {
  calcScoreConsts();
}

BatPublisher::~BatPublisher() {
  if (nullptr != level_db_) {
    delete level_db_;
  }
}

void BatPublisher::calcScoreConsts() {
  a_ = 1.0 / (ledger::_d * 2.0) - state_.min_pubslisher_duration_;
  a2_ = a_ * 2;
  a4_ = a2_ * 2;
  b_ = state_.min_pubslisher_duration_ - a_;
  b2_ = b_ * b_;
}

void BatPublisher::openPublishersDB() {
  base::FilePath dbFilePath;
  base::PathService::Get(base::DIR_HOME, &dbFilePath);
  dbFilePath = dbFilePath.Append(PUBLISHERS_DB_NAME);

  leveldb::Options options;
  options.create_if_missing = true;
  leveldb::Status status = leveldb::DB::Open(options, dbFilePath.value().c_str(), &level_db_);
  if (!status.ok() || !level_db_) {
      if (level_db_) {
          delete level_db_;
          level_db_ = nullptr;
      }

      LOG(ERROR) << "openPublishersDB level db open error " << dbFilePath.value().c_str();
  }
}

void BatPublisher::loadPublishers() {
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
    PUBLISHER_ST publisher_st;
    BatHelper::getJSONPublisher(it->value().ToString(), publisher_st);
    publishers_[publisher] = publisher_st;
  }
  assert(it->status().ok());  // Check for any errors found during the scan
  delete it;
}

void BatPublisher::loadStateCallback(bool result, const PUBLISHER_STATE_ST& state) {
  if (!result) {
    return;
  }
  state_ = state;
  calcScoreConsts();
}

void BatPublisher::initSynopsis() {
  BatHelper::loadPublisherState(base::Bind(&BatPublisher::loadStateCallback,
    base::Unretained(this)));
  scoped_refptr<base::SequencedTaskRunner> task_runner =
     base::CreateSequencedTaskRunnerWithTraits(
         {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  task_runner->PostTask(FROM_HERE, base::Bind(&BatPublisher::loadPublishers, base::Unretained(this)));
}

void BatPublisher::saveVisitInternal(const std::string& publisher, const uint64_t& duration,
    BatPublisher::SaveVisitCallback callback) {
  double currentScore = concaveScore(duration);

  std::string stringifiedPublisher;
  uint64_t verifiedTimestamp = 0;
  {
    std::lock_guard<std::mutex> guard(publishers_map_mutex_);
    std::map<std::string, PUBLISHER_ST>::iterator iter = publishers_.find(publisher);
    if (publishers_.end() == iter) {
      PUBLISHER_ST publisher_st;
      publisher_st.duration_ = duration;
      publisher_st.score_ = currentScore;
      publisher_st.visits_ = 1;
      publishers_[publisher] = publisher_st;
      stringifiedPublisher = BatHelper::stringifyPublisher(publisher_st);
    } else {
      iter->second.duration_ += duration;
      iter->second.score_ += currentScore;
      iter->second.visits_ += 1;
      verifiedTimestamp = iter->second.verifiedTimeStamp_;
      stringifiedPublisher = BatHelper::stringifyPublisher(iter->second);
    }
    if (!level_db_) {
      assert(false);

      return;
    }

    // Save the publisher to the database
    leveldb::Status status = level_db_->Put(leveldb::WriteOptions(), publisher, stringifiedPublisher);
    assert(status.ok());
  }
  callback.Run(publisher, verifiedTimestamp);
  synopsisNormalizerInternal();
}

void BatPublisher::saveVisit(const std::string& publisher, const uint64_t& duration,
    BatPublisher::SaveVisitCallback callback, bool ignoreMinTime) {
  if (!ignoreMinTime && duration < state_.min_pubslisher_duration_) {
    return;
  }

  // TODO checks if the publisher verified, disabled and etc

  scoped_refptr<base::SequencedTaskRunner> task_runner =
     base::CreateSequencedTaskRunnerWithTraits(
         {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  task_runner->PostTask(FROM_HERE, base::Bind(&BatPublisher::saveVisitInternal, base::Unretained(this),
    publisher, duration, callback));
}

void BatPublisher::setPublisherTimestampVerifiedInternal(const std::string& publisher,
    const uint64_t& verifiedTimestamp, const bool& verified) {
  {
    std::string stringifiedPublisher;
    std::lock_guard<std::mutex> guard(publishers_map_mutex_);
    std::map<std::string, PUBLISHER_ST>::iterator iter = publishers_.find(publisher);
    if (publishers_.end() == iter) {
      assert(false);

      return;
    } else {
      iter->second.verified_ = verified;
      iter->second.verifiedTimeStamp_ = verifiedTimestamp;
      stringifiedPublisher = BatHelper::stringifyPublisher(iter->second);
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

void BatPublisher::setPublisherTimestampVerified(const std::string& publisher,
    const uint64_t& verifiedTimestamp, const bool& verified) {
  scoped_refptr<base::SequencedTaskRunner> task_runner =
     base::CreateSequencedTaskRunnerWithTraits(
         {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  task_runner->PostTask(FROM_HERE, base::Bind(&BatPublisher::setPublisherTimestampVerifiedInternal,
    base::Unretained(this), publisher, verifiedTimestamp, verified));
}

void BatPublisher::setPublisherFavIconInternal(const std::string& publisher, const std::string& favicon_url) {
  std::string stringifiedPublisher;
  std::lock_guard<std::mutex> guard(publishers_map_mutex_);
  std::map<std::string, PUBLISHER_ST>::iterator iter = publishers_.find(publisher);
  if (publishers_.end() == iter) {
    PUBLISHER_ST publisher_st;
    publisher_st.favicon_url_ = favicon_url;
    publishers_[publisher] = publisher_st;
    stringifiedPublisher = BatHelper::stringifyPublisher(publisher_st);
  } else {
    iter->second.favicon_url_ = favicon_url;
    stringifiedPublisher = BatHelper::stringifyPublisher(iter->second);
  }
  if (!level_db_) {
    assert(false);

    return;
  }

  // Save the publisher to the database
  leveldb::Status status = level_db_->Put(leveldb::WriteOptions(), publisher, stringifiedPublisher);
  assert(status.ok());
}

void BatPublisher::setPublisherFavIcon(const std::string& publisher, const std::string& favicon_url) {
  scoped_refptr<base::SequencedTaskRunner> task_runner =
     base::CreateSequencedTaskRunnerWithTraits(
         {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  task_runner->PostTask(FROM_HERE, base::Bind(&BatPublisher::setPublisherFavIconInternal,
    base::Unretained(this), publisher, favicon_url));
}

void BatPublisher::setPublisherIncludeInternal(const std::string& publisher, const bool& include) {
  {
    std::string stringifiedPublisher;
    std::lock_guard<std::mutex> guard(publishers_map_mutex_);
    std::map<std::string, PUBLISHER_ST>::iterator iter = publishers_.find(publisher);
    if (publishers_.end() == iter) {
      PUBLISHER_ST publisher_st;
      publisher_st.exclude_ = !include;
      publishers_[publisher] = publisher_st;
      stringifiedPublisher = BatHelper::stringifyPublisher(publisher_st);
    } else {
      iter->second.exclude_ = !include;
      stringifiedPublisher = BatHelper::stringifyPublisher(iter->second);
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

void BatPublisher::setPublisherInclude(const std::string& publisher, const bool& include) {
  scoped_refptr<base::SequencedTaskRunner> task_runner =
     base::CreateSequencedTaskRunnerWithTraits(
         {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  task_runner->PostTask(FROM_HERE, base::Bind(&BatPublisher::setPublisherIncludeInternal,
    base::Unretained(this), publisher, include));
}

void BatPublisher::setPublisherDeletedInternal(const std::string& publisher, const bool& deleted) {
  {
    std::string stringifiedPublisher;
    std::lock_guard<std::mutex> guard(publishers_map_mutex_);
    std::map<std::string, PUBLISHER_ST>::iterator iter = publishers_.find(publisher);
    if (publishers_.end() == iter) {
      PUBLISHER_ST publisher_st;
      publisher_st.deleted_ = deleted;
      publishers_[publisher] = publisher_st;
      stringifiedPublisher = BatHelper::stringifyPublisher(publisher_st);
    } else {
      iter->second.deleted_ = deleted;
      stringifiedPublisher = BatHelper::stringifyPublisher(iter->second);
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

void BatPublisher::setPublisherDeleted(const std::string& publisher, const bool& deleted) {
  scoped_refptr<base::SequencedTaskRunner> task_runner =
     base::CreateSequencedTaskRunnerWithTraits(
         {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  task_runner->PostTask(FROM_HERE, base::Bind(&BatPublisher::setPublisherDeletedInternal,
    base::Unretained(this), publisher, deleted));
}

void BatPublisher::setPublisherPinPercentageInternal(const std::string& publisher, const bool& pinPercentage) {
  {
    std::string stringifiedPublisher;
    std::lock_guard<std::mutex> guard(publishers_map_mutex_);
    std::map<std::string, PUBLISHER_ST>::iterator iter = publishers_.find(publisher);
    if (publishers_.end() == iter) {
      PUBLISHER_ST publisher_st;
      publisher_st.pinPercentage_ = pinPercentage;
      publishers_[publisher] = publisher_st;
      stringifiedPublisher = BatHelper::stringifyPublisher(publisher_st);
    } else {
      iter->second.pinPercentage_ = pinPercentage;
      stringifiedPublisher = BatHelper::stringifyPublisher(iter->second);
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

void BatPublisher::setPublisherPinPercentage(const std::string& publisher, const bool& pinPercentage) {
  scoped_refptr<base::SequencedTaskRunner> task_runner =
     base::CreateSequencedTaskRunnerWithTraits(
         {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  task_runner->PostTask(FROM_HERE, base::Bind(&BatPublisher::setPublisherPinPercentageInternal,
    base::Unretained(this), publisher, pinPercentage));
}

void BatPublisher::setPublisherMinVisitTime(const uint64_t& duration) { // In milliseconds
  state_.min_pubslisher_duration_ = duration;
  BatHelper::savePublisherState(state_);
  synopsisNormalizer();
}

void BatPublisher::setPublisherMinVisits(const unsigned int& visits) {
  state_.min_visits_ = visits;
  BatHelper::savePublisherState(state_);
  synopsisNormalizer();
}

void BatPublisher::setPublisherAllowNonVerified(const bool& allow) {
  state_.allow_non_verified_ = allow;
  BatHelper::savePublisherState(state_);
  synopsisNormalizer();
}

std::vector<PUBLISHER_DATA_ST> BatPublisher::getPublishersData() {
  std::vector<PUBLISHER_DATA_ST> res;

  std::lock_guard<std::mutex> guard(publishers_map_mutex_);
  for (std::map<std::string, PUBLISHER_ST>::const_iterator iter = publishers_.begin(); iter != publishers_.end(); iter++) {
    PUBLISHER_DATA_ST publisherData;
    publisherData.publisherKey_ = iter->first;
    publisherData.publisher_ = iter->second;
    // TODO check all of that
    if (iter->second.duration_ >= ledger::_milliseconds_day) {
      publisherData.daysSpent_ = std::max((int)std::lround((double)iter->second.duration_ / (double)ledger::_milliseconds_day), 1);
    } else if (iter->second.duration_ >= ledger::_milliseconds_hour) {
      publisherData.hoursSpent_ = std::max((int)((double)iter->second.duration_ / (double)ledger::_milliseconds_hour), 1);
      publisherData.minutesSpent_ = std::lround((double)(iter->second.duration_ % ledger::_milliseconds_hour) / (double)ledger::_milliseconds_minute);
    } else if (iter->second.duration_ >= ledger::_milliseconds_minute) {
      publisherData.minutesSpent_ = std::max((int)((double)iter->second.duration_ / (double)ledger::_milliseconds_minute), 1);
      publisherData.secondsSpent_ = std::lround((double)(iter->second.duration_ % ledger::_milliseconds_minute) / (double)ledger::_milliseconds_second);
    } else {
      publisherData.secondsSpent_ = std::max((int)std::lround((double)iter->second.duration_ / (double)ledger::_milliseconds_second), 1);
    }
    res.push_back(publisherData);
  }

  return res;
}

bool BatPublisher::isPublisherVisible(const PUBLISHER_ST& publisher_st) {
  if (publisher_st.deleted_ || (!state_.allow_non_verified_ && !publisher_st.verified_)) {
    return false;
  }

  return publisher_st.score_ > 0 &&
    publisher_st.duration_ >= state_.min_pubslisher_duration_ &&
    publisher_st.visits_ >= state_.min_visits_;
}

void BatPublisher::synopsisNormalizerInternal() {
  std::lock_guard<std::mutex> guard(publishers_map_mutex_);
  double totalScores = 0.0;
  for (std::map<std::string, PUBLISHER_ST>::const_iterator iter = publishers_.begin(); iter != publishers_.end(); iter++) {
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
  for (std::map<std::string, PUBLISHER_ST>::iterator iter = publishers_.begin(); iter != publishers_.end(); iter++) {
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
  for (std::map<std::string, PUBLISHER_ST>::iterator iter = publishers_.begin(); iter != publishers_.end(); iter++) {
    if (!isPublisherVisible(iter->second)) {
      continue;
    }
    iter->second.percent_ = percents[currentValue];
    iter->second.weight_ = weights[currentValue];
    currentValue++;
  }
}

void BatPublisher::synopsisNormalizer() {
  scoped_refptr<base::SequencedTaskRunner> task_runner =
     base::CreateSequencedTaskRunnerWithTraits(
         {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  task_runner->PostTask(FROM_HERE, base::Bind(&BatPublisher::synopsisNormalizerInternal,
    base::Unretained(this)));
}

std::vector<WINNERS_ST> BatPublisher::winners(const unsigned int& ballots) {
  std::vector<WINNERS_ST> res;

  std::vector<PUBLISHER_DATA_ST> top = topN();
  unsigned int totalVotes = 0;
  std::vector<unsigned int> votes;
  // TODO there is underscore.shuffle
  for (size_t i = 0; i < top.size(); i++) {
    LOG(ERROR) << "!!!name == " << top[i].publisherKey_ << ", score == " << top[i].publisher_.score_;
    if (top[i].publisher_.percent_ <= 0) {
      continue;
    }
    WINNERS_ST winner;
    winner.votes_ = (unsigned int)std::lround((double)top[i].publisher_.percent_ * (double)ballots / 100.0);
    totalVotes += winner.votes_;
    winner.publisher_data_ = top[i];
    res.push_back(winner);
  }
  if (res.size()) {
    while (totalVotes > ballots) {
      std::vector<WINNERS_ST>::iterator max = std::max_element(res.begin(), res.end(), winners_votes_compare);
      (max->votes_)--;
      totalVotes--;
    }
  }

  return res;
}

std::vector<PUBLISHER_DATA_ST> BatPublisher::topN() {
  std::vector<PUBLISHER_DATA_ST> res;

  std::lock_guard<std::mutex> guard(publishers_map_mutex_);
  for (std::map<std::string, PUBLISHER_ST>::const_iterator iter = publishers_.begin(); iter != publishers_.end(); iter++) {
    if (0 == iter->second.score_
        || state_.min_pubslisher_duration_ > iter->second.duration_
        || state_.min_visits_ > iter->second.visits_) {
      continue;
    }
    PUBLISHER_DATA_ST publisherData;
    publisherData.publisherKey_ = iter->first;
    publisherData.publisher_ = iter->second;
    res.push_back(publisherData);
  }

  std::sort(res.begin(), res.end());

  return res;
}

bool BatPublisher::isEligableForContribution(const PUBLISHER_DATA_ST& publisherData) {
  return !publisherData.publisher_.exclude_ && isPublisherVisible(publisherData.publisher_);
}

// courtesy of @dimitry-xyz: https://github.com/brave/ledger/issues/2#issuecomment-221752002
double BatPublisher::concaveScore(const uint64_t& duration) {
  return (std::sqrt(b2_ + a4_ * duration) - b_) / (double)a2_;
}

}
