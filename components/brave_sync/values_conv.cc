/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/values_conv.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/values.h"
#include "brave/components/brave_sync/brave_sync_settings.h"
#include "brave/components/brave_sync/brave_sync_devices.h"
#include "brave/components/brave_sync/brave_sync_jslib_const.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "brave/components/brave_sync/value_debug.h"

namespace brave_sync {

using base::Value;

std::unique_ptr<Value> BraveSyncSettingsToValue(BraveSyncSettings *brave_sync_settings) {
  CHECK(brave_sync_settings);
  auto result = std::make_unique<Value>(Value::Type::DICTIONARY);

  result->SetKey("this_device_name", Value(brave_sync_settings->this_device_name_));
  result->SetKey("sync_this_device", Value(brave_sync_settings->sync_this_device_));
  result->SetKey("sync_bookmarks", Value(brave_sync_settings->sync_bookmarks_));
  result->SetKey("sync_settings", Value(brave_sync_settings->sync_settings_));
  result->SetKey("sync_history", Value(brave_sync_settings->sync_history_));

  result->SetKey("sync_configured", Value(brave_sync_settings->sync_configured_));

  return result;
}

std::string ExtractObjectIdFromDict(const base::Value *val) {
  CHECK(val);
  DCHECK(val->is_dict());
  LOG(ERROR) << "TAGAB ExtractObjectId val=" << brave::debug::ToPrintableString(*val);
  if (!val->is_dict()) {
    return "";
  }
  const base::Value *p_object_id = val->FindKey("objectId");
  LOG(ERROR) << "TAGAB ExtractObjectId p_object_data="<<p_object_id;
  DCHECK(p_object_id);
  if (p_object_id == nullptr) {
    return "";
  }

  //iterate
  std::string object_id;
  for (int i = 0; i < 16; ++i) {
      const base::Value *p_byte = p_object_id->FindKey({base::IntToString(i)});
      if (p_byte == nullptr) {
        return "";
      }
      object_id += base::IntToString(p_byte->GetInt());
      if (i != 15) {object_id += ", ";}
  }

  return object_id;
}

std::string ExtractObjectIdFromList(const base::Value *val) {
  CHECK(val);
  DCHECK(val->is_dict());
  LOG(ERROR) << "TAGAB ExtractObjectId val=" << brave::debug::ToPrintableString(*val);
  if (!val->is_dict()) {
    return "";
  }
  const base::Value *p_object_id = val->FindKey("objectId");
  LOG(ERROR) << "TAGAB ExtractObjectId p_object_data="<<p_object_id;
  DCHECK(p_object_id);
  if (p_object_id == nullptr) {
    return "";
  }

  DCHECK(p_object_id->is_list());

  std::string object_id;
  for (size_t i = 0; i < p_object_id->GetList().size(); ++i) {
      const base::Value &p_byte = p_object_id->GetList().at(i);
      DCHECK(p_byte.is_int());

      object_id += base::IntToString(p_byte.GetInt());
      if (i != p_object_id->GetList().size() - 1) {object_id += ", ";}
  }

  return object_id;
}

namespace {
  //anonymous

  std::unique_ptr<base::Value> CreateSyncRecordValue(
    int action, // kActionCreate/kActionUpdate/kActionDelete 0/1/2
    const std::string &device_id,
    const std::string &object_id,
    const std::string &object_data_name, //"bookmark"|"historySite"|"siteSetting"|"device"
    std::unique_ptr<base::Value> object_data // oneof Bookmark Site SiteSetting Device
  ) {
  auto value = std::make_unique<base::Value>(base::Value::Type::DICTIONARY);

  value->SetKey("action", base::Value(action));

  value->SetKey("deviceId", std::move(*BytesListFromString(device_id)));
  value->SetKey("objectId", std::move(*BytesListFromString(object_id)));

  value->SetKey(object_data_name, std::move(*object_data));

  return value;
}

  std::unique_ptr<base::Value> CreateBookmarkObjectDataValue(
    std::unique_ptr<base::Value> site,
    bool isFolder,
    const std::string &parentFolderObjectId,
    //repeated string fields = 6;
    bool hideInToolbar,
    const std::string &order
  ) {
    auto value = std::make_unique<base::Value>(base::Value::Type::DICTIONARY);
    /*
    name=bookmark
    TYPE=DICTIONARY
    [
       name=isFolder
       TYPE=BOOLEAN VALUE=false
       name=order
       TYPE=STRING VALUE=<2.1.1>
       name=parentFolderObjectId
       TYPE=LIST
       [
       ]
       name=site
       TYPE=DICTIONARY
       [
       ]
    ]
    */
    value->SetKey("site", std::move(*site));
    value->SetKey("isFolder", base::Value(isFolder));
    value->SetKey("parentFolderObjectId", base::Value(parentFolderObjectId));
    //repeated string fields = 6;
    value->SetKey("hideInToolbar", base::Value(hideInToolbar));
    value->SetKey("order", base::Value(order));

    return value;
  }

  std::unique_ptr<base::Value> CreateSiteValue(
    const std::string &location,
    const std::string &title,
    const std::string &customTitle,
    const uint64_t &lastAccessedTime,
    const uint64_t &creationTime,
    const std::string &favicon
  ) {
    auto value = std::make_unique<base::Value>(base::Value::Type::DICTIONARY);
    /*
    name=site
    TYPE=DICTIONARY
    [
       name=creationTime
       TYPE=INTEGER VALUE=0
       name=customTitle
       TYPE=STRING VALUE=<>
       name=favicon
       TYPE=STRING VALUE=<>
       name=lastAccessedTime
       TYPE=INTEGER VALUE=0
       name=location
       TYPE=STRING VALUE=<https://www.google.com/>
       name=title
       TYPE=STRING VALUE=<Google>
    ]
    */
    value->SetKey("location", base::Value(location));
    value->SetKey("title", base::Value(title));
    value->SetKey("customTitle", base::Value(customTitle));
    value->SetKey("lastAccessedTime", base::Value(base::checked_cast<double>(lastAccessedTime))); // there is no int64 Value, try with double, it is need to js to accept this
    value->SetKey("creationTime", base::Value(base::checked_cast<double>(creationTime)));
    value->SetKey("favicon", base::Value(favicon));
    return value;
  }

  // std::unique_ptr<base::Value> CreateHistorySiteObjectDataValue(
  // );
  //
  // std::unique_ptr<base::Value> CreateSiteSettingObjectDataValue(
  // );
  //
  // std::unique_ptr<base::Value> CreateDeviceObjectDataValue(
  // );

} // namespace



// exported
std::unique_ptr<base::Value> CreateBookmarkSyncRecordValue(
  int action, // kActionCreate/kActionUpdate/kActionDelete 0/1/2
  const std::string &device_id,
  const std::string &object_id,
  //object data - site
  const std::string &location,
  const std::string &title,
  const std::string &customTitle,
  const uint64_t &lastAccessedTime,
  const uint64_t &creationTime,
  const std::string &favicon,
  //object data - bookmark
  bool isFolder,
  const std::string &parentFolderObjectId,
  //repeated string fields = 6;
  bool hideInToolbar,
  const std::string &order) {

  DCHECK(!device_id.empty());
  DCHECK(!object_id.empty());

  std::unique_ptr<base::Value> site_value = CreateSiteValue(
    location,
    title,
    customTitle,
    lastAccessedTime,
    creationTime,
    favicon
  );

  std::unique_ptr<base::Value> bookmark_value = CreateBookmarkObjectDataValue(
      std::move(site_value),
      isFolder,
      parentFolderObjectId,
      //repeated string fields = 6;
      hideInToolbar,
      order
    );

  std::unique_ptr<base::Value> sync_record_value = CreateSyncRecordValue(
    action, // kActionCreate/kActionUpdate/kActionDelete 0/1/2
    device_id,
    object_id,
    "bookmark",
    std::move(bookmark_value) // oneof Bookmark Site SiteSetting Device
  );

    return sync_record_value;
}

std::unique_ptr<base::Value> VecToListValue(const std::vector<char> &v) {
  auto lv = std::make_unique<base::Value>(base::Value::Type::LIST);
  for (const char &i : v) {
    lv->GetList().emplace_back(i);
  }
  return lv;
}

std::unique_ptr<base::Value> BytesListFromString(const std::string &data_string) {
  auto value = std::make_unique<base::Value>(base::Value::Type::LIST);

  std::vector<std::string> splitted = SplitString(
      data_string,
      ", ",
      base::WhitespaceHandling::TRIM_WHITESPACE,
      base::SplitResult::SPLIT_WANT_NONEMPTY);

  for (size_t i = 0; i < splitted.size(); ++i) {
    int output = 0;
    bool b = base::StringToInt(splitted[i], &output);
    CHECK(b);
    value->GetList().emplace_back(base::Value(output));
  }

  return value;
}

std::unique_ptr<base::Value> SingleIntStrToListValue(const std::string &data_string) {
  auto value = std::make_unique<base::Value>(base::Value::Type::LIST);
  int output = 0;
  bool b = base::StringToInt(data_string, &output);
  CHECK(b);
  value->GetList().emplace_back(base::Value(output));

  return value;
}

std::unique_ptr<base::Value> BlobFromString(const std::string &data_string) {
  std::vector<std::string> splitted = SplitString(
      data_string,
      ", ",
      base::WhitespaceHandling::TRIM_WHITESPACE,
      base::SplitResult::SPLIT_WANT_NONEMPTY);

  base::Value::BlobStorage vec;
  vec.reserve(splitted.size());
  for (size_t i = 0; i < splitted.size(); ++i) {
    int output = 0;
    bool b = base::StringToInt(splitted[i], &output);
    CHECK(b);
    unsigned char byte = base::checked_cast<unsigned char>(output);
    vec.push_back(byte);
  }

  return std::make_unique<base::Value>(vec);
}

std::unique_ptr<base::Value> BlobFromSingleIntStr(const std::string &data_string) {
 int output = 0;
 bool b = base::StringToInt(data_string, &output);
 CHECK(b);
 unsigned char byte = base::checked_cast<unsigned char>(output);
 base::Value::BlobStorage vec({byte});
 return std::make_unique<base::Value>(vec);
}

std::string GetAction(const base::Value &sync_record) {
  CHECK(sync_record.is_dict());

  const base::Value* val_action = sync_record.FindKey("action");
  DCHECK(val_action != nullptr);
  if (val_action == nullptr) {
    return "";
  }
  DCHECK(val_action->is_int());
  if (!val_action->is_int()) {
    return "";
  }

  int iaction = val_action->GetInt();
  DCHECK(iaction == jslib_const::kActionCreate ||
    iaction == jslib_const::kActionUpdate || iaction == jslib_const::kActionDelete);

  std::string action = base::IntToString(iaction);
  CHECK(action == jslib_const::CREATE_RECORD || action == jslib_const::CREATE_RECORD
    || action == jslib_const::DELETE_RECORD);
  return action;
}

std::string ExtractBookmarkLocation(const base::Value *sync_record) {
  LOG(ERROR) << "TAGAB ExtractBookmarkLocation: sync_record=" << sync_record;
  CHECK(sync_record);
  CHECK(sync_record->is_dict());

  const base::Value *val_location = sync_record->FindPathOfType({"bookmark", "site", "location"}, base::Value::Type::STRING);
  LOG(ERROR) << "TAGAB ExtractBookmarkLocation: val_location=" << val_location;
  DCHECK(val_location);
  if (val_location == nullptr) {
    return "";
  }

  std::string location = val_location->GetString();
  LOG(ERROR) << "TAGAB ExtractBookmarkLocation: location=" << location;
  return location;
}

std::string ExtractBookmarkTitle(const base::Value *sync_record) {
  LOG(ERROR) << "TAGAB ExtractBookmarkTitle: sync_record=" << sync_record;
  CHECK(sync_record);
  CHECK(sync_record->is_dict());

  const base::Value *val_title = sync_record->FindPathOfType({"bookmark", "site", "title"}, base::Value::Type::STRING);
  LOG(ERROR) << "TAGAB ExtractBookmarkTitle: val_title=" << val_title;
  DCHECK(val_title);
  if (val_title == nullptr) {
    return "";
  }

  std::string title = val_title->GetString();
  LOG(ERROR) << "TAGAB ExtractBookmarkTitle: title=" << title;
  return title;
}


} // namespace brave_sync
