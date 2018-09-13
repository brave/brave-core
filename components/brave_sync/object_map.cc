/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
 #include "brave/components/brave_sync/object_map.h"

#include <string>

#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/task/post_task.h"
#include "base/threading/thread.h"
#include "base/threading/thread_checker.h"
#include "brave/components/brave_sync/debug.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_paths.h"
#include "third_party/leveldatabase/src/include/leveldb/db.h"

namespace brave_sync {
namespace storage {

static const char DB_FILE_NAME[] = "brave_sync_db";

ObjectMap::ObjectMap(Profile *profile) : profile_(nullptr) {
  LOG(ERROR) << "TAGAB brave_sync::ObjectMap::ObjectMap CTOR profile="<<profile;
  LOG(ERROR) << "TAGAB brave_sync::ObjectMap::ObjectMap CTOR profile->GetPath()="<<profile->GetPath();

  DETACH_FROM_SEQUENCE(sequence_checker_);

  DCHECK(profile);
  profile_ = profile;
}

ObjectMap::~ObjectMap() {
  LOG(ERROR) << "TAGAB brave_sync::ObjectMap::ObjectMap DTOR";
  Close();
}

void ObjectMap::TraceAll() {
  LOG(ERROR) << "TAGAB brave_sync::ObjectMap::TraceAll:-----------------------";
  leveldb::Iterator* it = level_db_->NewIterator(leveldb::ReadOptions());
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    LOG(ERROR) << "<" << it->key().ToString() << ">: <" << it->value().ToString() << ">";
  }
  DCHECK(it->status().ok());  // Check for any errors found during the scan
  delete it;
  LOG(ERROR) << "TAGAB brave_sync::ObjectMap::TraceAll:^----------------------";
}

void ObjectMap::CreateOpenDatabase() {
  LOG(ERROR) << "TAGAB brave_sync::ObjectMap::CreateOpenDatabase, DCHECK_CALLED_ON_VALID_SEQUENCE " << GetThreadInfoString();
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (nullptr == level_db_) {
    DCHECK(profile_);

    base::FilePath dbFilePath = profile_->GetPath().Append(DB_FILE_NAME);
    LOG(ERROR) << "TAGAB ObjectMap::CreateOpenDatabase dbFilePath=" << dbFilePath;
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::DB *level_db_raw = nullptr;
    leveldb::Status status = leveldb::DB::Open(options, dbFilePath.value().c_str(), &level_db_raw);
    LOG(ERROR) << "TAGAB ObjectMap::CreateOpenDatabase status=" << status.ToString();
    if (!status.ok() || !level_db_raw) {
      if (level_db_raw) {
        delete level_db_raw;
      }

      LOG(ERROR) << "sync level db open error " << DB_FILE_NAME;
      return;
    }
    level_db_.reset(level_db_raw);
    LOG(ERROR) << "TAGAB DB opened";
    //TraceAll();
  }
}

std::string ObjectMap::GetLocalIdByObjectId(const std::string &objectId) {
  CreateOpenDatabase();
  if (nullptr == level_db_) {
      return "";
  }

  std::string value;
  leveldb::Status db_status = level_db_->Get(leveldb::ReadOptions(), objectId, &value);
  if (!db_status.ok()) {
    LOG(ERROR) << "sync level db get error " << db_status.ToString();
  }

  return value;
}

std::string ObjectMap::GetObjectIdByLocalId(const std::string &localId) {
  CreateOpenDatabase();
  if (nullptr == level_db_) {
      return "";
  }

  std::string value;
  leveldb::Status db_status = level_db_->Get(leveldb::ReadOptions(), localId, &value);

  if (!db_status.ok()) {
    LOG(ERROR) << "sync level db get error " << db_status.ToString();
  }

  return value;
}

void ObjectMap::SaveObjectId(
  const std::string &localId,
  const std::string &objectIdJSON, //may be an order or empty
  const std::string &objectId) {
  LOG(ERROR) << "TAGAB brave_sync::ObjectMap::SaveObjectId - enter";

  CreateOpenDatabase();
  if (nullptr == level_db_) {
    LOG(ERROR) << "TAGAB brave_sync::ObjectMap::SaveObjectId nullptr == level_db_ ???";
    return;
  }

  leveldb::Status db_status = level_db_->Put(leveldb::WriteOptions(), localId, objectIdJSON);
  if (!db_status.ok()) {
    LOG(ERROR) << "sync level db put error " << db_status.ToString();
  }

  if (0 != objectId.size()) {
      db_status = level_db_->Put(leveldb::WriteOptions(), objectId, localId);
      if (!db_status.ok()) {
        LOG(ERROR) << "sync level db put error " << db_status.ToString();
      }
  }
  LOG(ERROR) << "TAGAB brave_sync::ObjectMap::SaveObjectId - DONE";
}

void ObjectMap::DeleteByLocalId(const std::string &localId) {
  CreateOpenDatabase();
  if (nullptr == level_db_) {
      return;
  }

  std::string value;
  leveldb::Status db_status = level_db_->Get(leveldb::ReadOptions(), localId, &value);
  if (!db_status.ok()) {
    LOG(ERROR) << "sync level db get error " << db_status.ToString();
  }

  db_status = level_db_->Delete(leveldb::WriteOptions(), localId);
  if (!db_status.ok()) {
    LOG(ERROR) << "sync level db delete error " << db_status.ToString();
  }
  db_status = level_db_->Delete(leveldb::WriteOptions(), value);
  if (!db_status.ok()) {
    LOG(ERROR) << "sync level db delete error " << db_status.ToString();
  }
}

void ObjectMap::Close() {
  LOG(ERROR) << "TAGAB brave_sync::ObjectMap::Close, DCHECK_CALLED_ON_VALID_SEQUENCE " << GetThreadInfoString();
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  level_db_.reset();
}

void ObjectMap::CloseDBHandle() {
  LOG(ERROR) << "TAGAB brave_sync::ObjectMap::CloseDBHandle, DCHECK_CALLED_ON_VALID_SEQUENCE " << GetThreadInfoString();
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  level_db_.reset();
}

void ObjectMap::DestroyDB() {
  LOG(ERROR) << "TAGAB brave_sync::ObjectMap::DestroyDB, DCHECK_CALLED_ON_VALID_SEQUENCE " << GetThreadInfoString();
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  LOG(ERROR) << "TAGAB brave_sync::ObjectMap::ResetObjects";
  CloseDBHandle();
  base::FilePath app_data_path;
  bool success = base::PathService::Get(chrome::DIR_USER_DATA, &app_data_path);
  CHECK(success);
  base::FilePath dbFilePath = app_data_path.Append(DB_FILE_NAME);
  LOG(ERROR) << "TAGAB ResetObjects dbFilePath=" << dbFilePath;
  leveldb::Status db_status = leveldb::DestroyDB(dbFilePath.value(), leveldb::Options());
  if (!db_status.ok()) {
    LOG(ERROR) << "sync level db destroy error " << db_status.ToString();
  }
}

void ObjectMap::ResetSync(const std::string& key) {
  CreateOpenDatabase();
  if (nullptr == level_db_) {
      return;
  }
  level_db_->Delete(leveldb::WriteOptions(), key);
}

} // namespace storage
} // namespace brave_sync
