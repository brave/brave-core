/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/publisher_info_backend.h"

#include <codecvt>
#include <memory>
#include <string>
#include <vector>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/leveldatabase/env_chromium.h"
#include "third_party/leveldatabase/src/include/leveldb/db.h"
#include "third_party/leveldatabase/src/include/leveldb/iterator.h"
#include "third_party/leveldatabase/src/include/leveldb/options.h"
#include "third_party/leveldatabase/src/include/leveldb/slice.h"
#include "third_party/leveldatabase/src/include/leveldb/status.h"

namespace brave_rewards {

PublisherInfoBackend::PublisherInfoBackend(const base::FilePath& path) :
    path_(path) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

PublisherInfoBackend::~PublisherInfoBackend() {}

bool PublisherInfoBackend::Put(const std::string& key,
                               const std::string& value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bool initialized = EnsureInitialized();
  DCHECK(initialized);

  if (!initialized)
    return false;

  leveldb::WriteOptions options;
  leveldb::Status status = db_->Put(options, key, value);
  if (status.ok())
    return true;

  return false;
}

bool PublisherInfoBackend::Get(const std::string& lookup,
                               std::string* value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bool initialized = EnsureInitialized();
  DCHECK(initialized);

  if (!initialized)
    return false;

  leveldb::ReadOptions options;
  leveldb::Status status = db_->Get(options, lookup, value);
  if (status.ok())
    return true;

  return false;
}

bool PublisherInfoBackend::Search(const std::vector<std::string>& prefixes,
              uint32_t start, uint32_t limit,
              std::vector<std::string>& results) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bool initialized = EnsureInitialized();
  DCHECK(initialized);

  if (!initialized)
    return false;

  leveldb::ReadOptions options;
  std::unique_ptr<leveldb::Iterator> db_it(db_->NewIterator(options));

  uint32_t count = 0;
  uint32_t position = 0;
  for (std::vector<std::string>::const_iterator prefix =
        prefixes.begin(); prefix != prefixes.end(); ++prefix) {
    auto slice = leveldb::Slice(*prefix);
    for (db_it->Seek(slice);
          count < limit && db_it->Valid();
          db_it->Next()) {
      if (!db_it->value().starts_with(slice)) {
        db_it->SeekToLast();
        continue;
      }
      if (position++ < start)
        continue;
      else
        count++;
      results.push_back(db_it->value().ToString());
    }
  }

  return true;
}

bool PublisherInfoBackend::Load(uint32_t start,
                                uint32_t limit,
                                std::vector<std::string>& results) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bool initialized = EnsureInitialized();
  DCHECK(initialized);

  if (!initialized)
    return false;

  leveldb::ReadOptions options;
  std::unique_ptr<leveldb::Iterator> db_it(db_->NewIterator(options));

  uint32_t count = 0;
  uint32_t position = 0;
  for (db_it->SeekToFirst(); count < limit && db_it->Valid(); db_it->Next()) {
    if (position++ < start)
      continue;
    else
      count++;
    results.push_back(db_it->value().ToString());
  }

  return true;
}

bool PublisherInfoBackend::EnsureInitialized() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (db_.get())
    return true;

  leveldb_env::Options options;
  options.create_if_missing = true;
#if defined(OS_WIN)
  std::string path = base::UTF16ToUTF8(path_.value());
#else
  std::string path = path_.value();
#endif
  leveldb::Status status = leveldb_env::OpenDB(options, path, &db_);

  if (status.IsCorruption()) {
    LOG(ERROR) << "Deleting corrupt database";
    base::DeleteFile(path_, true);
    status = leveldb_env::OpenDB(options, path, &db_);
  }
  if (status.ok()) {
    CHECK(db_);
    return true;
  }
  LOG(WARNING) << "Unable to open " << path << ": "
               << status.ToString();
  return false;
}

}  // namespace brave_rewards
