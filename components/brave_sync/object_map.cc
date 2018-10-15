/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_sync/object_map.h"

#include <string>

#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/files/file_path.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/path_service.h"
#include "base/task/post_task.h"
#include "base/threading/thread.h"
#include "base/threading/thread_checker.h"
#include "base/values.h"
#include "brave/components/brave_sync/jslib_const.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_paths.h"
#include "third_party/leveldatabase/src/include/leveldb/db.h"

namespace brave_sync {
namespace storage {

static const char DB_FILE_NAME[] = "brave_sync_db";

ObjectMap::ObjectMap(const base::FilePath &profile_path) {
  DETACH_FROM_SEQUENCE(sequence_checker_);

  DCHECK(!profile_path.empty());
  profile_path_ = profile_path;
}

ObjectMap::~ObjectMap() {
  Close();
}

void ObjectMap::SetApiVersion(const std::string &api_version) {
  DCHECK(!api_version.empty());
  DCHECK(api_version_.empty());
  api_version_ = api_version;
}

void ObjectMap::CreateOpenDatabase() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (nullptr == level_db_) {
    DCHECK(!profile_path_.empty());
    base::FilePath dbFilePath = profile_path_.Append(DB_FILE_NAME);

    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::DB *level_db_raw = nullptr;
    leveldb::Status status = leveldb::DB::Open(options,
                                               dbFilePath.value().c_str(),
                                               &level_db_raw);
    if (!status.ok() || !level_db_raw) {
      if (level_db_raw) {
        delete level_db_raw;
      }
      return;
    }
    level_db_.reset(level_db_raw);
  }
}

std::string ObjectMap::GetLocalIdByObjectId(const Type &type, const std::string &objectId) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!objectId.empty());
  CreateOpenDatabase();
  if (nullptr == level_db_) {
    return "";
  }

  std::string value;
  leveldb::Status db_status = level_db_->Get(leveldb::ReadOptions(),
                                             objectId,
                                             &value);

  std::string local_id;
  Type read_type = Unset;
  if (value.empty()) {
    return "";
  }

  SplitRawLocalId(value, local_id, read_type);
  DCHECK(type == read_type);

  return local_id;
}

std::string ObjectMap::GetObjectIdByLocalId(const Type &type,
                                            const std::string &localId) {
  std::string object_id;

  bool ret = GetParsedDataByLocalId(
      type, localId, &object_id, nullptr, nullptr);

  if (!ret) {
    return "";
  }

  return object_id;
}

bool ObjectMap::GetParsedDataByLocalId(
  const Type &type,
  const std::string &localId,
  std::string *object_id, std::string *order, std::string *api_version) {
  std::string raw_local_id = ComposeRawLocalId(type, localId);
  std::string json = GetRawJsonByLocalId(raw_local_id);

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

  if (!val) {
    return false;
  }

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
  const Type &type,
  const std::string &localId,
  const std::string &new_order) {
  std::string object_id;
  std::string old_order;
  bool ret = GetParsedDataByLocalId(
      type,
      localId,
      &object_id,
      &old_order,
      nullptr);
  DCHECK(ret);
  if (object_id.empty())
    return;

  SaveObjectIdAndOrder(type, localId, object_id, new_order);
}

void ObjectMap::CreateOrderByLocalObjectId(
  const Type &type,
  const std::string &localId,
  const std::string &objectId,
  const std::string &order) {
  DCHECK(!localId.empty());
  DCHECK(!objectId.empty());
  DCHECK(!order.empty());

  SaveObjectIdAndOrder(type, localId, objectId, order);
}

std::string ObjectMap::GetRawJsonByLocalId(const std::string &localId) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CreateOpenDatabase();
  if (nullptr == level_db_) {
      return "";
  }

  std::string json_value;
  leveldb::Status db_status = level_db_->Get(leveldb::ReadOptions(), localId, &json_value);

  return json_value;
}

void ObjectMap::SaveObjectIdRawJson(
  const std::string &raw_local_id,
  const std::string &objectIdJSON,
  const std::string &objectId) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CreateOpenDatabase();

  if (nullptr == level_db_)
    return;

  leveldb::Status db_status = level_db_->Put(leveldb::WriteOptions(), raw_local_id, objectIdJSON);

  if (0 != objectId.size())
    db_status = level_db_->Put(leveldb::WriteOptions(), objectId, raw_local_id);
}

std::string ObjectMap::GetSpecialJSONByLocalId(const std::string &localId) {
  std::string json = GetRawJsonByLocalId(localId);
  return json;
}

std::string ObjectMap::GetOrderByObjectId(const Type &type, const std::string &object_id) {
  std::string local_id = GetLocalIdByObjectId(type, object_id);
  std::string order;
  std::string object_id_saved;
  bool ret = GetParsedDataByLocalId(type, local_id, &object_id_saved, &order, nullptr);

  if (!ret) {
    return "";
  }

  DCHECK(object_id_saved == object_id);

  return order;
}

std::string ObjectMap::GetOrderByLocalObjectId(const Type &type, const std::string &localId) {
  std::string order;
  std::string object_id_saved;
  bool ret = GetParsedDataByLocalId(type, localId, &object_id_saved, &order, nullptr);

  if (!ret) {
    return "";
  }

  return order;
}

void ObjectMap::SaveObjectId(
  const Type &type,
  const std::string &localId,
  const std::string &objectId) {
  DCHECK(!api_version_.empty()); // Not sure how to manage this. For now this is just a passthorugh from "OnGetInitData"
                                 // Android:  private String mApiVersion = "0"; just hardcoded

  std::string json = "[{\"objectId\": \"" + objectId + "\", apiVersion\": \"" + api_version_ + "\"}]";
  SaveObjectIdRawJson(ComposeRawLocalId(type, localId), json, objectId);
}

void ObjectMap::SaveObjectIdAndOrder(
  const Type &type,
  const std::string &localId,
  const std::string &objectId,
  const std::string &order) {
  DCHECK(!api_version_.empty());
  std::string json = "[{\"objectId\": \"" + objectId + "\", \"order\": \"" + order + "\", \"apiVersion\": \"" + api_version_ + "\"}]";
  SaveObjectIdRawJson(ComposeRawLocalId(type, localId), json, objectId);
}

void ObjectMap::SaveSpecialJson(
  const std::string &localId,
  const std::string &specialJSON) {
  SaveObjectIdRawJson(localId, specialJSON, "");
}

void ObjectMap::DeleteByLocalId(const Type &type, const std::string &localId) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CreateOpenDatabase();
  if (nullptr == level_db_) {
      return;
  }

  std::string raw_local_id = ComposeRawLocalId(type, localId);

  std::string object_id;
  bool got_parsed = GetParsedDataByLocalId(type, localId, &object_id, nullptr, nullptr);

  leveldb::Status db_status = level_db_->Delete(leveldb::WriteOptions(), raw_local_id);

  if (got_parsed && !object_id.empty()) {
    db_status = level_db_->Delete(leveldb::WriteOptions(), object_id);
  }
}

std::set<std::string> ObjectMap::SaveGetDeleteNotSyncedRecords(
  const Type &type,
  const std::string &action,
  const std::vector<std::string> &local_ids,
  const NotSyncedRecordsOperation &operation) {
  // recordType: "BOOKMARKS" | "HISTORY_SITES" | "PREFERENCES"
  // action: "0" | "1" | "2"
  std::string recordType;
  switch(type) {
    case Bookmark:
      recordType = "BOOKMARKS";
      break;
    case History:
      recordType = "HISTORY_SITES";
      break;
    default:
      NOTREACHED();
  }

  std::string key = recordType + action;
  std::set<std::string> existing_list = GetNotSyncedRecords(key);

  if (operation == GetItems) {
    return existing_list;
  } else if (operation == AddItems) {
    for (const auto & id: local_ids) {
      existing_list.insert(id);
    }
  } else if (operation == DeleteItems) {
    bool list_changed = false;
    bool clear_local_db = (action == jslib_const::DELETE_RECORD); // "2"
    for (const auto & id: local_ids) {
      size_t items_removed = existing_list.erase(id);
      if (!list_changed) {
        list_changed = (items_removed != 0);
      }
      // Delete corresponding objectIds
      if (clear_local_db && (items_removed != 0)) {
        DeleteByLocalId(type, id);
      }
    }
  } else {
    NOTREACHED();
  }

  SaveNotSyncedRecords(key, existing_list);

  return std::set<std::string>();
}

std::set<std::string> ObjectMap::GetNotSyncedRecords(const std::string &key) {
  std::string raw = GetRawJsonByLocalId(key);
  std::set<std::string> list = DeserializeList(raw);
  return list;
}

void ObjectMap::SaveNotSyncedRecords(const std::string &key, const std::set<std::string> &existing_list) {
  std::string raw = SerializeList(existing_list);
  SaveObjectIdRawJson(key, raw, std::string());
}

std::set<std::string> ObjectMap::DeserializeList(const std::string &raw) {
  // Parse JSON
  int error_code_out = 0;
  std::string error_msg_out;
  int error_line_out = 0;
  int error_column_out = 0;
  std::unique_ptr<base::Value> list = base::JSONReader::ReadAndReturnError(
    raw,
    base::JSONParserOptions::JSON_PARSE_RFC,
    &error_code_out,
    &error_msg_out,
    &error_line_out,
    &error_column_out);

  if (!list) {
    return std::set<std::string>();
  }

  std::set<std::string> ret;
  DCHECK(list->is_list());

  for (const auto &val : list->GetList() ) {
    ret.insert(val.GetString());
  }

  return ret;
}

std::string ObjectMap::SerializeList(const std::set<std::string> &existing_list) {
  using base::Value;
  auto list = std::make_unique<Value>(Value::Type::LIST);
  for (const auto & item: existing_list) {
    list->GetList().push_back(base::Value(item));
  }

  std::string json;
  bool result = base::JSONWriter::WriteWithOptions(
    *list,
    0,
    &json);

  DCHECK(result);
  return json;
}

void ObjectMap::Close() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  level_db_.reset();
}

void ObjectMap::CloseDBHandle() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  level_db_.reset();
}

void ObjectMap::DestroyDB() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!profile_path_.empty());

  CloseDBHandle();

  base::FilePath dbFilePath = profile_path_.Append(DB_FILE_NAME);

  leveldb::Status db_status = leveldb::DestroyDB(dbFilePath.value(), leveldb::Options());
  if (!db_status.ok()) {
    DCHECK(false);
  }
  api_version_.clear();
}

void ObjectMap::ResetSync(const std::string& key) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CreateOpenDatabase();
  if (nullptr == level_db_) {
      return;
  }
  level_db_->Delete(leveldb::WriteOptions(), key);
}

void ObjectMap::SplitRawLocalId(const std::string &raw_local_id, std::string &local_id, Type &read_type) {
  if (raw_local_id.empty()) {
    local_id = "";
    read_type = Unset;
    return;
  }

  char object_type_char = raw_local_id.at(0);
  switch (object_type_char) {
    case 'b':
      read_type = Bookmark;
      local_id.assign(raw_local_id.begin() + 1, raw_local_id.end());
    break;
    case 'h':
      read_type = History;
      local_id.assign(raw_local_id.begin() + 1, raw_local_id.end());
    break;
    default:
      read_type = Unset;
      local_id.assign(raw_local_id);
    break;
  }
}

std::string ObjectMap::ComposeRawLocalId(const ObjectMap::Type &type, const std::string &localId) {
  switch (type) {
    case Unset:
      return localId;
    break;
    case Bookmark:
      return 'b' + localId;
    break;
    case History:
      return 'h' + localId;
    break;
    default:
      NOTREACHED();
  }
}

std::string ObjectMap::ToString(const Type &type) {
  switch (type) {
    case Unset:
      return "Unset";
    break;
    case Bookmark:
      return "Bookmark";
    break;
    case History:
      return "History";
    break;
    default:
      NOTREACHED();
  }
}

std::string ObjectMap::ToString(const NotSyncedRecordsOperation &operation) {
  switch (operation) {
    case GetItems:
      return "GetItems";
    break;
    case AddItems:
      return "AddItems";
    break;
    case DeleteItems:
      return "DeleteItems";
    break;
    default:
      NOTREACHED();
  }
}

} // namespace storage
} // namespace brave_sync
