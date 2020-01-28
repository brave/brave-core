/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlists/browser/playlists_db_controller.h"

#include "base/files/file_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/playlists/browser/playlists_types.h"
#include "third_party/leveldatabase/env_chromium.h"
#include "third_party/leveldatabase/src/include/leveldb/db.h"
#include "third_party/leveldatabase/src/include/leveldb/iterator.h"
#include "third_party/leveldatabase/src/include/leveldb/options.h"
#include "third_party/leveldatabase/src/include/leveldb/slice.h"
#include "third_party/leveldatabase/src/include/leveldb/status.h"

namespace brave_playlists {

PlaylistsDBController::PlaylistsDBController(const base::FilePath& db_path)
    : db_path_(db_path) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

PlaylistsDBController::~PlaylistsDBController() = default;

bool PlaylistsDBController::Put(const std::string& key,
                                const std::string& value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(initialized_);

  if (!initialized_)
    return false;

  leveldb::Status status = db_->Put(leveldb::WriteOptions(), key, value);
  return status.ok();
}

std::string PlaylistsDBController::Get(const std::string& key) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(initialized_);

  if (!initialized_)
    return std::string();

  std::string value;
  leveldb::Status status = db_->Get(leveldb::ReadOptions(), key, &value);
  if (status.ok())
    return value;

  return std::string();
}

std::vector<std::string> PlaylistsDBController::GetAll() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(initialized_);

  std::vector<std::string> playlists;
  if (!initialized_)
    return playlists;

  std::unique_ptr<leveldb::Iterator> it(
      db_->NewIterator(leveldb::ReadOptions()));
  for (it->SeekToFirst(); it->Valid(); it->Next())
    playlists.push_back(it->value().ToString());
  DCHECK(it->status().ok());
  return playlists;
}

bool PlaylistsDBController::Del(const std::string& key) {
  DCHECK(initialized_);

  if (!initialized_)
    return false;

  leveldb::Status status = db_->Delete(leveldb::WriteOptions(), key);
  return status.ok();
}

bool PlaylistsDBController::DeleteAll() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(initialized_);

  if (!initialized_)
    return false;

  // Delete db file and re-init.
  db_.reset();
  base::DeleteFile(db_path_, true);
  return Init();
}

bool PlaylistsDBController::Init() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!db_);
  initialized_ = false;

  leveldb_env::Options options;
  options.create_if_missing = true;

  const std::string path =
#if defined(OS_WIN)
      base::UTF16ToUTF8(db_path_.value());
#else
      db_path_.value();
#endif
  leveldb::Status status = leveldb_env::OpenDB(options, path, &db_);

  if (status.IsCorruption()) {
    VLOG(1) << "Deleting corrupt database";
    base::DeleteFile(db_path_, true);
    status = leveldb_env::OpenDB(options, path, &db_);
  }
  if (status.ok()) {
    CHECK(db_);
    initialized_ = true;
    return true;
  }
  VLOG(2) << "Unable to open " << path << ": " << status.ToString();
  return false;
}

}  // namespace brave_playlists
