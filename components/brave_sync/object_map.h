/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_OBJ_MAP_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_OBJ_MAP_H_

#include <string>
#include <memory>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/sequence_checker.h"

namespace leveldb {
  class DB;
}

namespace brave_sync {
class Bookmarks;

namespace storage {

// Map works in two directions:
// 1. local_id => {object_id, order, api_version}
// 2. object_id => local_id

class ObjectMap {
public:
  ObjectMap(const base::FilePath &profile_path);
  ~ObjectMap();

  // Local ids both of bookmarks and history are just int64_t and can be the same.
  enum Type {
    Unset = 0,
    Bookmark = 1,
    History = 2
  };

  void SetApiVersion(const std::string &api_version);

  std::string GetLocalIdByObjectId(const Type &type, const std::string &object_id);

  std::string GetObjectIdByLocalId(const Type &type, const std::string &localId);
  std::string GetSpecialJSONByLocalId(const std::string &localId);

  std::string GetOrderByObjectId(const Type &type, const std::string &object_id);
  std::string GetOrderByLocalObjectId(const Type &type, const std::string &localId);

  void SaveObjectId(
    const Type &type,
    const std::string &localId,
    const std::string &objectId);

  void SaveObjectIdAndOrder(
    const Type &type,
    const std::string &localId,
    const std::string &objectId,
    const std::string &order);

  void SaveSpecialJson(
    const std::string &localId,
    const std::string &specialJSON);

  void UpdateOrderByLocalObjectId(
    const Type &type,
    const std::string &localId,
    const std::string &order);

  void CreateOrderByLocalObjectId(
    const Type &type,
    const std::string &localId,
    const std::string &objectId,
    const std::string &order);

  void DeleteByLocalId(const Type &type, const std::string &localId);
  void Close();
  void CloseDBHandle();
  void ResetSync(const std::string& key);
  void DestroyDB();

private:
  void SaveObjectIdRawJson(
    const std::string &localId,
    const std::string &objectIdJSON,
    const std::string &objectId);
  std::string GetRawJsonByLocalId(const std::string &localId);

  bool GetParsedDataByLocalId(const Type &type, const std::string &localId, std::string *object_id, std::string *order, std::string *api_version);

  void CreateOpenDatabase();
  void TraceAll();

  void SplitRawLocalId(const std::string &raw_local_id, std::string &local_id, Type &read_type);
  std::string ComposeRawLocalId(const Type &type, const std::string &localId);

  base::FilePath profile_path_;
  std::unique_ptr<leveldb::DB> level_db_;
  std::string api_version_;

  DISALLOW_COPY_AND_ASSIGN(ObjectMap);
  SEQUENCE_CHECKER(sequence_checker_);
};

} // namespace storage
} // namespace brave_sync

#endif //BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_OBJ_MAP_H_
