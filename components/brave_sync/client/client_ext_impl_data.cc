/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/client/client_ext_impl_data.h"

#include "brave/common/extensions/api/brave_sync.h"
#include "brave/components/brave_sync/client/client_data.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "brave/components/brave_sync/values_conv.h"

namespace brave_sync {

void ConvertConfig(const brave_sync::client_data::Config &config,
  extensions::api::brave_sync::Config &config_extension) {
  config_extension.api_version = config.api_version;
  config_extension.server_url = config.server_url;
  config_extension.debug = config.debug;
}

std::unique_ptr<brave_sync::jslib::Site> FromExtSite(
    const extensions::api::brave_sync::Site &ext_site) {
  auto site = std::make_unique<brave_sync::jslib::Site>();

  site->location = ext_site.location;
  site->title = ext_site.title;
  site->customTitle = ext_site.custom_title;
  site->lastAccessedTime = base::Time::FromJsTime(ext_site.last_accessed_time);
  site->creationTime = base::Time::FromJsTime(ext_site.creation_time);
  site->favicon = ext_site.favicon;

  return site;
}

std::unique_ptr<brave_sync::jslib::Device> FromExtDevice(
    const extensions::api::brave_sync::Device &ext_device) {
  auto device = std::make_unique<brave_sync::jslib::Device>();
  device->name = ext_device.name;
  return device;
}

std::unique_ptr<brave_sync::jslib::SiteSetting> FromExtSiteSetting(
    const extensions::api::brave_sync::SiteSetting &ext_site_setting) {
  auto site_setting = std::make_unique<brave_sync::jslib::SiteSetting>();

  site_setting->hostPattern = ext_site_setting.host_pattern;

  #define CHECK_AND_ASSIGN(FIELDNAME_LIB, FIELDNAME_EXT) \
  if (ext_site_setting.FIELDNAME_EXT) {   \
    site_setting->FIELDNAME_LIB = *ext_site_setting.FIELDNAME_EXT;  \
  }
  CHECK_AND_ASSIGN(zoomLevel, zoom_level);
  CHECK_AND_ASSIGN(shieldsUp, shields_up);
  //DCHECK(false);
  // site_setting-> = ext_site_setting.ad_control;
  // site_setting-> = ext_site_setting.cookie_control;
  CHECK_AND_ASSIGN(safeBrowsing, safe_browsing);
  CHECK_AND_ASSIGN(noScript, no_script);
  CHECK_AND_ASSIGN(httpsEverywhere, https_everywhere);
  CHECK_AND_ASSIGN(fingerprintingProtection, fingerprinting_protection);
  CHECK_AND_ASSIGN(ledgerPayments, ledger_payments);
  CHECK_AND_ASSIGN(ledgerPaymentsShown, ledger_payments_shown);
  #undef CHECK_AND_ASSIGN

  return site_setting;
}

std::unique_ptr<std::vector<brave_sync::jslib::MetaInfo>> FromExtMetaInfo(
  const std::vector<extensions::api::brave_sync::MetaInfo>& ext_meta_info) {
  auto meta_info = std::make_unique<std::vector<brave_sync::jslib::MetaInfo>>();

  for (auto& ext_meta : ext_meta_info) {
    brave_sync::jslib::MetaInfo meta;
    meta.key = ext_meta.key;
    meta.value = ext_meta.value;
    meta_info->push_back(meta);
  }

  return meta_info;
}

std::unique_ptr<jslib::Bookmark> FromExtBookmark(
    const extensions::api::brave_sync::Bookmark &ext_bookmark) {
  auto bookmark = std::make_unique<jslib::Bookmark>();

  bookmark->site = std::move(*FromExtSite(ext_bookmark.site));

  bookmark->isFolder = ext_bookmark.is_folder;
  if (ext_bookmark.parent_folder_object_id) {
    bookmark->parentFolderObjectId =
        StrFromUnsignedCharArray(*ext_bookmark.parent_folder_object_id);
  }
  if (ext_bookmark.fields) {
    bookmark->fields = *ext_bookmark.fields;
  }
  if (ext_bookmark.hide_in_toolbar) {
    bookmark->hideInToolbar = *ext_bookmark.hide_in_toolbar;
  }
  if (ext_bookmark.order) {
    bookmark->order = *ext_bookmark.order;
  }
  if (ext_bookmark.meta_info) {
    bookmark->metaInfo = std::move(*FromExtMetaInfo(*ext_bookmark.meta_info));
  }

  return bookmark;
}

std::unique_ptr<extensions::api::brave_sync::Site> FromLibSite(
    const jslib::Site &lib_site) {
  auto ext_site = std::make_unique<extensions::api::brave_sync::Site>();

  ext_site->location = lib_site.location;
  ext_site->title = lib_site.title;
  ext_site->custom_title = lib_site.customTitle;
  ext_site->last_accessed_time = 0;//lib_site.lastAccessedTime.ToJsTime();
  ext_site->creation_time = 0;//lib_site.creationTime.ToJsTime();
  ext_site->favicon = lib_site.favicon;

  return ext_site;
}

std::unique_ptr<std::vector<extensions::api::brave_sync::MetaInfo>>
FromLibMetaInfo(const std::vector<jslib::MetaInfo>& lib_metaInfo) {
  auto ext_meta_info =
    std::make_unique<std::vector<extensions::api::brave_sync::MetaInfo>>();

  for (auto& metaInfo : lib_metaInfo) {
    auto meta_info = std::make_unique<extensions::api::brave_sync::MetaInfo>();
    meta_info->key = metaInfo.key;
    meta_info->value = metaInfo.value;
    ext_meta_info->push_back(std::move(*meta_info));
  }

  return ext_meta_info;
}

std::unique_ptr<extensions::api::brave_sync::Bookmark> FromLibBookmark(
    const jslib::Bookmark &lib_bookmark) {
  auto ext_bookmark = std::make_unique<extensions::api::brave_sync::Bookmark>();

  ext_bookmark->site = std::move(*FromLibSite(lib_bookmark.site));

  ext_bookmark->is_folder = lib_bookmark.isFolder;
  if (!lib_bookmark.parentFolderObjectId.empty()) {
    ext_bookmark->parent_folder_object_id.reset(
        new std::vector<unsigned char>(
            UCharVecFromString(lib_bookmark.parentFolderObjectId)));
    ext_bookmark->parent_folder_object_id_str.reset(
        new std::string(lib_bookmark.parentFolderObjectId));
  }

  if (!lib_bookmark.prevObjectId.empty()) {
    ext_bookmark->prev_object_id.reset(
        new std::vector<unsigned char>(
            UCharVecFromString(lib_bookmark.prevObjectId)));
    ext_bookmark->prev_object_id_str.reset(
        new std::string(lib_bookmark.prevObjectId));
  }

  if (!lib_bookmark.fields.empty()) {
    ext_bookmark->fields.reset(
        new std::vector<std::string>(lib_bookmark.fields));
  }

  ext_bookmark->hide_in_toolbar.reset(new bool(lib_bookmark.hideInToolbar));

  ext_bookmark->order.reset(new std::string(lib_bookmark.order));

  ext_bookmark->prev_order.reset(new std::string(lib_bookmark.prevOrder));

  ext_bookmark->next_order.reset(new std::string(lib_bookmark.nextOrder));

  ext_bookmark->parent_order.reset(new std::string(lib_bookmark.parentOrder));

  if (!lib_bookmark.metaInfo.empty()) {
    ext_bookmark->meta_info = FromLibMetaInfo(lib_bookmark.metaInfo);
  }

  return ext_bookmark;
}

std::unique_ptr<extensions::api::brave_sync::SiteSetting> FromLibSiteSetting(
    const jslib::SiteSetting &lib_site_setting) {
  auto ext_site_setting =
      std::make_unique<extensions::api::brave_sync::SiteSetting>();

  ext_site_setting->host_pattern = lib_site_setting.hostPattern;

  ext_site_setting->zoom_level.reset(new double(lib_site_setting.zoomLevel));
  ext_site_setting->shields_up.reset(new bool (lib_site_setting.shieldsUp));
  //ext_site_setting->ad_control = lib_site_setting.adControl;
  //ext_site_setting->cookie_control = lib_site_setting.cookieControl;
  //DCHECK(false);
  ext_site_setting->safe_browsing.reset(
      new bool(lib_site_setting.safeBrowsing));
  ext_site_setting->no_script.reset(new bool(lib_site_setting.noScript));
  ext_site_setting->https_everywhere.reset(
      new bool(lib_site_setting.httpsEverywhere));
  ext_site_setting->fingerprinting_protection.reset(
      new bool(lib_site_setting.fingerprintingProtection));
  ext_site_setting->ledger_payments.reset(
      new bool(lib_site_setting.ledgerPayments));
  ext_site_setting->ledger_payments_shown.reset(
      new bool(lib_site_setting.ledgerPaymentsShown));
  if (!lib_site_setting.fields.empty()) {
    ext_site_setting->fields.reset(
        new std::vector<std::string>(lib_site_setting.fields));
  }

  return ext_site_setting;
}

std::unique_ptr<extensions::api::brave_sync::Device> FromLibDevice(
    const jslib::Device &lib_device) {
  auto ext_device = std::make_unique<extensions::api::brave_sync::Device>();
  ext_device->name = lib_device.name;
  return ext_device;
}

std::unique_ptr<extensions::api::brave_sync::SyncRecord> FromLibSyncRecord(
    const brave_sync::SyncRecordPtr &lib_record) {
  DCHECK(lib_record);
  std::unique_ptr<extensions::api::brave_sync::SyncRecord> ext_record =
      std::make_unique<extensions::api::brave_sync::SyncRecord>();

  ext_record->action = static_cast<int>(lib_record->action);
  ext_record->device_id = UCharVecFromString(lib_record->deviceId);
  ext_record->object_id = UCharVecFromString(lib_record->objectId);

  // Workaround, because properties device_id and object_id somehow are empty
  // in js code after passing Browser=>Extension
  ext_record->device_id_str.reset(new std::string(lib_record->deviceId));
  ext_record->object_id_str.reset(new std::string(lib_record->objectId));

  ext_record->object_data = lib_record->objectData;
  ext_record->sync_timestamp.reset(
    new double(lib_record->syncTimestamp.ToJsTime()));
  if (lib_record->has_bookmark()) {
    ext_record->bookmark = FromLibBookmark(lib_record->GetBookmark());
  } else if (lib_record->has_historysite()) {
    ext_record->history_site = FromLibSite(lib_record->GetHistorySite());
  } else if (lib_record->has_sitesetting()) {
    ext_record->site_setting = FromLibSiteSetting(lib_record->GetSiteSetting());
  } else if (lib_record->has_device()) {
    ext_record->device = FromLibDevice(lib_record->GetDevice());
  }

  return ext_record;
}

brave_sync::SyncRecordPtr FromExtSyncRecord(
    const extensions::api::brave_sync::SyncRecord &ext_record) {
  brave_sync::SyncRecordPtr record = std::make_unique<brave_sync::jslib::SyncRecord>();

  record->action = ConvertEnum<brave_sync::jslib::SyncRecord::Action>(ext_record.action,
    brave_sync::jslib::SyncRecord::Action::A_MIN,
    brave_sync::jslib::SyncRecord::Action::A_MAX,
    brave_sync::jslib::SyncRecord::Action::A_INVALID);

  record->deviceId = StrFromUnsignedCharArray(ext_record.device_id);
  record->objectId = StrFromUnsignedCharArray(ext_record.object_id);
  record->objectData = ext_record.object_data;
  if (ext_record.sync_timestamp) {
    record->syncTimestamp = base::Time::FromJsTime(*ext_record.sync_timestamp);
  }

  DCHECK((ext_record.bookmark &&
          !ext_record.history_site &&
          !ext_record.site_setting && !ext_record.device) ||
        (!ext_record.bookmark && ext_record.history_site &&
          !ext_record.site_setting &&
          !ext_record.device) ||
        (!ext_record.bookmark &&
          !ext_record.history_site &&
          ext_record.site_setting &&
          !ext_record.device) ||
        (!ext_record.bookmark &&
          !ext_record.history_site &&
          !ext_record.site_setting &&
          ext_record.device));

  if (ext_record.bookmark) {
    std::unique_ptr<brave_sync::jslib::Bookmark> bookmark =
        FromExtBookmark(*ext_record.bookmark);
    record->SetBookmark(std::move(bookmark));
  } else if (ext_record.history_site) {
    std::unique_ptr<brave_sync::jslib::Site> history_site =
        FromExtSite(*ext_record.history_site);
    record->SetHistorySite(std::move(history_site));
  } else if (ext_record.site_setting) {
    std::unique_ptr<brave_sync::jslib::SiteSetting> site_setting =
        FromExtSiteSetting(*ext_record.site_setting);
    record->SetSiteSetting(std::move(site_setting));
  } else if (ext_record.device) {
    std::unique_ptr<brave_sync::jslib::Device> device =
        FromExtDevice(*ext_record.device);
    record->SetDevice(std::move(device));
  }
  return record;
}

void ConvertSyncRecords(
    const std::vector<extensions::api::brave_sync::SyncRecord>& ext_records,
  std::vector<brave_sync::SyncRecordPtr> &records) {
  DCHECK(records.empty());

  for (const extensions::api::brave_sync::SyncRecord &ext_record : ext_records) {
    brave_sync::SyncRecordPtr record = FromExtSyncRecord(ext_record);
    records.emplace_back(std::move(record));
  }
}

void ConvertResolvedPairs(
    const SyncRecordAndExistingList& records_and_existing_objects,
    std::vector<extensions::api::brave_sync::RecordAndExistingObject>&
        records_and_existing_objects_ext) {

  DCHECK(records_and_existing_objects_ext.empty());

  for (const SyncRecordAndExistingPtr &src : records_and_existing_objects) {
    DCHECK(src->first.get() != nullptr);
    std::unique_ptr<extensions::api::brave_sync::RecordAndExistingObject> dest =
      std::make_unique<extensions::api::brave_sync::RecordAndExistingObject>();

    dest->server_record = std::move(*FromLibSyncRecord(src->first));

    if (src->second) {
      dest->local_record = FromLibSyncRecord(src->second);
    }

    records_and_existing_objects_ext.emplace_back(std::move(*dest));
  }
}

void ConvertSyncRecordsFromLibToExt(
    const std::vector<brave_sync::SyncRecordPtr>& records,
    std::vector<extensions::api::brave_sync::SyncRecord>& records_extension) {
  DCHECK(records_extension.empty());

  for (const brave_sync::SyncRecordPtr &src : records) {
    std::unique_ptr<extensions::api::brave_sync::SyncRecord> dest =
        FromLibSyncRecord(src);
    records_extension.emplace_back(std::move(*dest));
  }
}

} // namespace brave_sync
