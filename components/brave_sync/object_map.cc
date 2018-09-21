/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_sync/object_map.h"

#include <string>

#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/path_service.h"
#include "base/task/post_task.h"
#include "base/threading/thread.h"
#include "base/threading/thread_checker.h"
#include "base/values.h"
#include "brave/components/brave_sync/debug.h"
#include "brave/components/brave_sync/value_debug.h"
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

void ObjectMap::SetApiVersion(const std::string &api_version) {
  DCHECK(!api_version.empty());
  DCHECK(api_version_.empty());
  api_version_ = api_version;
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
    TraceAll();
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
  std::string object_id;

  bool ret = GetParsedDataByLocalId(localId, &object_id, nullptr, nullptr);

  if (!ret) {
    return "";
  }

  return object_id;
}

bool ObjectMap::GetParsedDataByLocalId(
  const std::string &localId,
  std::string *object_id, std::string *order, std::string *api_version) {
  std::string json = GetRawJsonByLocalId(localId);

  LOG(ERROR) << "TAGAB ObjectMap::GetParsedDataByLocalId: json=" << json;

  if (json.empty()) {
    return false;
  }

  // Parse JSON
  int error_code_out = 0;
  std::string error_msg_out;
  int error_line_out = 0;
  int error_column_out = 0;
  std::unique_ptr<base::Value> val = base::JSONReader::ReadAndReturnError(
    json,
    base::JSONParserOptions::JSON_PARSE_RFC,
    &error_code_out,
    &error_msg_out,
    &error_line_out,
    &error_column_out);

  LOG(ERROR) << "TAGAB ObjectMap::GetParsedDataByLocalId: val.get()="<<val.get();
  LOG(ERROR) << "TAGAB ObjectMap::GetParsedDataByLocalId: error_msg_out="<<error_msg_out;

  if (!val) {
    return false;
  }

  LOG(ERROR) << "TAGAB ObjectMap::GetParsedDataByLocalId: val=" << brave::debug::ToPrintableString(*val);

  DCHECK(val->is_list());
  DCHECK(val->GetList().size() == 1);

  if (!val->is_list() || val->GetList().size() != 1) {
    return false;
  }

  const auto &val_entry = val->GetList().at(0);

  //std::string json = "[{\"objectId\": \"" + objectId + "\", \"order\": \"" + order + "\", \"apiVersion\": \"" + api_version_ + "\"}]";
  if (object_id) {
    *object_id = val_entry.FindKey("objectId")->GetString();
  }
  if (order) {
    *order = val_entry.FindKey("order")->GetString();
  }
  if (api_version) {
    *api_version = val_entry.FindKey("apiVersion")->GetString();
  }

  return true;
}

void ObjectMap::UpdateOrderByLocalObjectId(
  const std::string &localId,
  const std::string &new_order) {
  LOG(ERROR) << "TAGAB ObjectMap::UpdateOrderByLocalObjectId";
  LOG(ERROR) << "TAGAB localId="<<localId;
  LOG(ERROR) << "TAGAB new_order="<<new_order;
  std::string object_id;
  std::string old_order;
  bool ret = GetParsedDataByLocalId(
    localId,
    &object_id, &old_order, nullptr);
  DCHECK(ret);
  LOG(ERROR) << "TAGAB object_id="<<object_id;
  LOG(ERROR) << "TAGAB old_order="<<old_order;
  if (object_id.empty()) {
    return;
  }

  SaveObjectIdAndOrder(localId, object_id, new_order);
}

std::string ObjectMap::GetRawJsonByLocalId(const std::string &localId) {
  CreateOpenDatabase();
  if (nullptr == level_db_) {
      return "";
  }

  std::string json_value;
  leveldb::Status db_status = level_db_->Get(leveldb::ReadOptions(), localId, &json_value);

  if (!db_status.ok()) {
    LOG(ERROR) << "sync level db get error " << db_status.ToString();
  }

  return json_value;
}

void ObjectMap::SaveObjectIdRawJson(
  const std::string &localId,
  const std::string &objectIdJSON,
  const std::string &objectId) {
  LOG(ERROR) << "TAGAB brave_sync::ObjectMap::SaveObjectIdRawJson - enter";

  CreateOpenDatabase();
  if (nullptr == level_db_) {
    LOG(ERROR) << "TAGAB brave_sync::ObjectMap::SaveObjectIdRawJson nullptr == level_db_ ???";
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
  LOG(ERROR) << "TAGAB brave_sync::ObjectMap::SaveObjectIdRawJson - DONE";
}

std::string ObjectMap::GetSpecialJSONByLocalId(const std::string &localId) {
  std::string json = GetRawJsonByLocalId(localId);
  return json;
}

std::string ObjectMap::GetOrderByObjectId(const std::string &object_id) {
  std::string local_id = GetLocalIdByObjectId(object_id);
  std::string order;
  std::string object_id_saved;
  bool ret = GetParsedDataByLocalId(local_id, &object_id_saved, &order, nullptr);

  if (!ret) {
    return "";
  }

  DCHECK(object_id_saved == object_id);

  return order;
}

std::string ObjectMap::GetOrderByLocalObjectId(const std::string &localId) {
  std::string order;
  std::string object_id_saved;
  bool ret = GetParsedDataByLocalId(localId, &object_id_saved, &order, nullptr);

  if (!ret) {
    return "";
  }

  return order;
}

void ObjectMap::SaveObjectId(
  const std::string &localId,
  const std::string &objectId) {
  DCHECK(!api_version_.empty()); // Not sure how to manage this. For now this is just a passthorugh from "OnGetInitData"
                                 // Android:  private String mApiVersion = "0"; just hardcoded

  std::string json = "[{\"objectId\": \"" + objectId + "\", apiVersion\": \"" + api_version_ + "\"}]";
  SaveObjectIdRawJson(localId, json, objectId);
}

void ObjectMap::SaveObjectIdAndOrder(
  const std::string &localId,
  const std::string &objectId,
  const std::string &order) {
  DCHECK(!api_version_.empty());
  std::string json = "[{\"objectId\": \"" + objectId + "\", \"order\": \"" + order + "\", \"apiVersion\": \"" + api_version_ + "\"}]";
  SaveObjectIdRawJson(localId, json, objectId);
}

void ObjectMap::SaveSpecialJson(
  const std::string &localId,
  const std::string &specialJSON) {
  SaveObjectIdRawJson(localId, specialJSON, "");
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
  DCHECK(profile_);

  LOG(ERROR) << "TAGAB brave_sync::ObjectMap::ResetObjects";
  CloseDBHandle();

  base::FilePath dbFilePath = profile_->GetPath().Append(DB_FILE_NAME);
  LOG(ERROR) << "TAGAB ResetObjects dbFilePath=" << dbFilePath;

  leveldb::Status db_status = leveldb::DestroyDB(dbFilePath.value(), leveldb::Options());
  if (!db_status.ok()) {
    LOG(ERROR) << "sync level db destroy error " << db_status.ToString();
    DCHECK(false);
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
