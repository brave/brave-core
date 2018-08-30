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

std::unique_ptr<brave_sync::jslib::Site> FromExtSite(const extensions::api::brave_sync::Site &ext_site) {
  auto site = std::make_unique<brave_sync::jslib::Site>();

  site->location = ext_site.location;
  site->title = ext_site.title;
  site->customTitle = ext_site.custom_title;
  site->lastAccessedTime = base::Time::FromJsTime(ext_site.last_accessed_time);
  site->creationTime = base::Time::FromJsTime(ext_site.creation_time);
  site->favicon = ext_site.favicon;

  return site;
}

std::unique_ptr<brave_sync::jslib::Device> FromExtDevice(const extensions::api::brave_sync::Device &ext_device) {
  auto device = std::make_unique<brave_sync::jslib::Device>();
  device->name = ext_device.name;
  return device;
}

std::unique_ptr<brave_sync::jslib::SiteSetting> FromExtSiteSetting(const extensions::api::brave_sync::SiteSetting &ext_site_setting) {
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

// void FromExtSite(jslib::Site &site, const extensions::api::brave_sync::Site &ext_site) {
//   site.location = ext_site.location;
//   site.title = ext_site.title;
//   site.customTitle = ext_site.custom_title;
//   site.lastAccessedTime = base::Time::FromJsTime(ext_site.last_accessed_time);
//   site.creationTime = base::Time::FromJsTime(ext_site.creation_time);
//   site.favicon = ext_site.favicon;
// }

std::unique_ptr<jslib::Bookmark> FromExtBookmark(const extensions::api::brave_sync::Bookmark &ext_bookmark) {
  auto bookmark = std::make_unique<jslib::Bookmark>();

  //FromExtSite(bookmark->site, ext_bookmark.site);
  bookmark->site = std::move(*FromExtSite(ext_bookmark.site));

  bookmark->isFolder = ext_bookmark.is_folder;
  if (ext_bookmark.parent_folder_object_id) {
    bookmark->parentFolderObjectId = StrFromCharArray(*ext_bookmark.parent_folder_object_id);
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

  return bookmark;
}

std::unique_ptr<extensions::api::brave_sync::Site> FromLibSite(const jslib::Site &lib_site) {
  auto ext_site = std::make_unique<extensions::api::brave_sync::Site>();

  ext_site->location = lib_site.location;
  ext_site->title = lib_site.title;
  ext_site->custom_title = lib_site.customTitle;
  ext_site->last_accessed_time = 0;//lib_site.lastAccessedTime.ToJsTime();
  ext_site->creation_time = 0;//lib_site.creationTime.ToJsTime();
  ext_site->favicon = lib_site.favicon;

  return ext_site;
}

std::unique_ptr<extensions::api::brave_sync::Bookmark> FromLibBookmark(const jslib::Bookmark &lib_bookmark) {
  auto ext_bookmark = std::make_unique<extensions::api::brave_sync::Bookmark>();

  ext_bookmark->site = std::move(*FromLibSite(lib_bookmark.site));

  ext_bookmark->is_folder = lib_bookmark.isFolder;
  // if (!lib_bookmark.parentFolderObjectId.empty()) {
  //   ext_bookmark->parent_folder_object_id.reset(new std::vector<char>(CharVecFromString(lib_bookmark.parentFolderObjectId)));
  //   ext_bookmark->parent_folder_object_id_str.reset(new std::string(lib_bookmark.parentFolderObjectId));
  // }

  if (!lib_bookmark.fields.empty()) {
    ext_bookmark->fields.reset(new std::vector<std::string>(lib_bookmark.fields));
  }

  ext_bookmark->hide_in_toolbar.reset(new bool(lib_bookmark.hideInToolbar));

  if (!lib_bookmark.order.empty()) {
    ext_bookmark->order.reset(new std::string(lib_bookmark.order));
  }

  return ext_bookmark;
}

std::unique_ptr<extensions::api::brave_sync::SiteSetting> FromLibSiteSetting(const jslib::SiteSetting &lib_site_setting) {
  auto ext_site_setting = std::make_unique<extensions::api::brave_sync::SiteSetting>();

  ext_site_setting->host_pattern = lib_site_setting.hostPattern;

  ext_site_setting->zoom_level.reset(new double(lib_site_setting.zoomLevel));
  ext_site_setting->shields_up.reset(new bool (lib_site_setting.shieldsUp));
  //ext_site_setting->ad_control = lib_site_setting.adControl;
  //ext_site_setting->cookie_control = lib_site_setting.cookieControl;
  //DCHECK(false);
  ext_site_setting->safe_browsing.reset(new bool(lib_site_setting.safeBrowsing));
  ext_site_setting->no_script.reset(new bool(lib_site_setting.noScript));
  ext_site_setting->https_everywhere.reset(new bool(lib_site_setting.httpsEverywhere));
  ext_site_setting->fingerprinting_protection.reset(new bool(lib_site_setting.fingerprintingProtection));
  ext_site_setting->ledger_payments.reset(new bool(lib_site_setting.ledgerPayments));
  ext_site_setting->ledger_payments_shown.reset(new bool(lib_site_setting.ledgerPaymentsShown));
  if (!lib_site_setting.fields.empty()) {
    ext_site_setting->fields.reset(new std::vector<std::string>(lib_site_setting.fields));
  }

  return ext_site_setting;
}

std::unique_ptr<extensions::api::brave_sync::Device> FromLibDevice(const jslib::Device &lib_device) {
  auto ext_device = std::make_unique<extensions::api::brave_sync::Device>();
  ext_device->name = lib_device.name;
  return ext_device;
}

std::unique_ptr<extensions::api::brave_sync::SyncRecord2> FromLibSyncRecord(const brave_sync::SyncRecordPtr &lib_record) {
  DCHECK(lib_record);
  std::unique_ptr<extensions::api::brave_sync::SyncRecord2> ext_record = std::make_unique<extensions::api::brave_sync::SyncRecord2>();

  ext_record->action = static_cast<int>(lib_record->action);
  ext_record->device_id = CharVecFromString(lib_record->deviceId);
  ext_record->object_id = CharVecFromString(lib_record->objectId);

  // Workaround, because properties device_id and object_id somehow are empty in js code after passing Browser=>Extension
  ext_record->device_id_str.reset(new std::string(lib_record->deviceId));
  ext_record->object_id_str.reset(new std::string(lib_record->objectId));

  ext_record->object_data = lib_record->objectData;
  //ext_record->sync_timestamp = 0;//lib_record->syncTimestamp.ToJsTime();
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

brave_sync::SyncRecordPtr FromExtSyncRecord(const extensions::api::brave_sync::SyncRecord2 &ext_record) {
//LOG(ERROR) << "" << ext_record.object_id[0];
  brave_sync::SyncRecordPtr record = std::make_unique<brave_sync::jslib::SyncRecord>();

  record->action = ConvertEnum<brave_sync::jslib::SyncRecord::Action>(ext_record.action,
    brave_sync::jslib::SyncRecord::Action::A_MIN,
    brave_sync::jslib::SyncRecord::Action::A_MAX,
    brave_sync::jslib::SyncRecord::Action::A_INVALID);

  record->deviceId = StrFromCharArray(ext_record.device_id);
  record->objectId = StrFromCharArray(ext_record.object_id);
  record->objectData = ext_record.object_data;
  if (ext_record.sync_timestamp) {
    record->syncTimestamp = base::Time::FromJsTime(*ext_record.sync_timestamp);
  }

  DCHECK((ext_record.bookmark && !ext_record.history_site && !ext_record.site_setting && !ext_record.device) ||
    (!ext_record.bookmark && ext_record.history_site && !ext_record.site_setting && !ext_record.device) ||
    (!ext_record.bookmark && !ext_record.history_site && ext_record.site_setting && !ext_record.device) ||
    (!ext_record.bookmark && !ext_record.history_site && !ext_record.site_setting && ext_record.device) );

  if (ext_record.bookmark) {
    std::unique_ptr<brave_sync::jslib::Bookmark> bookmark = FromExtBookmark(*ext_record.bookmark);
    record->SetBookmark(std::move(bookmark));
  } else if (ext_record.history_site) {
    std::unique_ptr<brave_sync::jslib::Site> history_site = FromExtSite(*ext_record.history_site);
    record->SetHistorySite(std::move(history_site));
  } else if (ext_record.site_setting) {
    std::unique_ptr<brave_sync::jslib::SiteSetting> site_setting = FromExtSiteSetting(*ext_record.site_setting);
    record->SetSiteSetting(std::move(site_setting));
  } else if (ext_record.device) {
    std::unique_ptr<brave_sync::jslib::Device> device = FromExtDevice(*ext_record.device);
    record->SetDevice(std::move(device));
  }
  return record;
}

void ConvertSyncRecords(const std::vector<extensions::api::brave_sync::SyncRecord2> &records_extension,
  std::vector<brave_sync::SyncRecordPtr> &records) {
  DCHECK(records.empty());

  for (const extensions::api::brave_sync::SyncRecord2 &ext_record : records_extension) {
//LOG(ERROR) << "" << ext_record.object_id[0];
    brave_sync::SyncRecordPtr record = FromExtSyncRecord(ext_record);
    records.emplace_back(std::move(record));
  }
}

void ConvertResolvedPairs(const SyncRecordAndExistingList &records_and_existing_objects,
  std::vector<extensions::api::brave_sync::RecordAndExistingObject> &records_and_existing_objects_ext) {

  DCHECK(records_and_existing_objects_ext.empty());

  for (const SyncRecordAndExistingPtr &src : records_and_existing_objects) {
LOG(ERROR) << "" << src->first->objectId;
    std::unique_ptr<extensions::api::brave_sync::RecordAndExistingObject> dest =
      std::make_unique<extensions::api::brave_sync::RecordAndExistingObject>();

    dest->server_record = std::move(*FromLibSyncRecord(src->first));

    if (src->second) {
      dest->local_record = FromLibSyncRecord(src->second);
    }

    records_and_existing_objects_ext.emplace_back(std::move(*dest));
  }
}

void ConvertSyncRecordsFromLibToExt(const std::vector<brave_sync::SyncRecordPtr> &records,
  std::vector<extensions::api::brave_sync::SyncRecord2> &records_extension) {
  DCHECK(records_extension.empty());
  LOG(ERROR) << "TAGAB ConvertSyncRecordsFromLibToExt=============";

  for (const brave_sync::SyncRecordPtr &src : records) {
    LOG(ERROR) << "TAGAB SRC:";
    LOG(ERROR) << "TAGAB src->action="<<src->action;
    LOG(ERROR) << "TAGAB src->deviceId ="<<src->deviceId;
    LOG(ERROR) << "TAGAB src->objectId ="<<src->objectId;
    LOG(ERROR) << "TAGAB src->objectData ="<<src->objectData;
    if (src->has_bookmark()) {
      LOG(ERROR) << "TAGAB src->GetBookmark().site.location ="<<src->GetBookmark().site.location;
      LOG(ERROR) << "TAGAB src->GetBookmark().site.title ="<<src->GetBookmark().site.title;
      LOG(ERROR) << "TAGAB src->GetBookmark().site.customTitle ="<<src->GetBookmark().site.customTitle;
      LOG(ERROR) << "TAGAB src->GetBookmark().site.lastAccessedTime ="<<src->GetBookmark().site.lastAccessedTime;
      LOG(ERROR) << "TAGAB src->GetBookmark().site.creationTime ="<<src->GetBookmark().site.creationTime;
      LOG(ERROR) << "TAGAB src->GetBookmark().site.favicon ="<<src->GetBookmark().site.favicon;
      LOG(ERROR) << "TAGAB src->GetBookmark().isFolder ="<<src->GetBookmark().isFolder;
      LOG(ERROR) << "TAGAB src->GetBookmark().parentFolderObjectId ="<<src->GetBookmark().parentFolderObjectId;
      LOG(ERROR) << "TAGAB src->GetBookmark().fields.size() ="<<src->GetBookmark().fields.size();
      LOG(ERROR) << "TAGAB src->GetBookmark().hideInToolbar ="<<src->GetBookmark().hideInToolbar;
      LOG(ERROR) << "TAGAB src->GetBookmark().order ="<<src->GetBookmark().order;
    }
    LOG(ERROR) << "TAGAB src->syncTimestamp ="<<src->syncTimestamp;

    std::unique_ptr<extensions::api::brave_sync::SyncRecord2> dest = FromLibSyncRecord(src);

    LOG(ERROR) << "TAGAB DEST:";
    LOG(ERROR) << "TAGAB dest->action=" << dest->action;
    LOG(ERROR) << "TAGAB dest->device_id.size()=" << dest->device_id.size();
    LOG(ERROR) << "TAGAB dest->object_id.size()=" << dest->object_id.size();
    LOG(ERROR) << "TAGAB dest->object_data=" << dest->object_data;
    LOG(ERROR) << "TAGAB dest->sync_timestamp=" << dest->sync_timestamp;
    if (dest->bookmark) {
      LOG(ERROR) << "TAGAB dest->bookmark->site.location=" << dest->bookmark->site.location;
      LOG(ERROR) << "TAGAB dest->bookmark->site.title=" << dest->bookmark->site.title;
      LOG(ERROR) << "TAGAB dest->bookmark->site.custom_title=" << dest->bookmark->site.custom_title;
      LOG(ERROR) << "TAGAB dest->bookmark->site.last_accessed_time=" << dest->bookmark->site.last_accessed_time;
      LOG(ERROR) << "TAGAB dest->bookmark->site.creation_time=" << dest->bookmark->site.creation_time;
      LOG(ERROR) << "TAGAB dest->bookmark->site.favicon=" << dest->bookmark->site.favicon;

      LOG(ERROR) << "TAGAB dest->bookmark->is_folder=" << dest->bookmark->is_folder;
      if (dest->bookmark->parent_folder_object_id) {
        LOG(ERROR) << "TAGAB dest->bookmark->parent_folder_object_id.size()=" << dest->bookmark->parent_folder_object_id->size();
      }
      if (dest->bookmark->fields) {
        LOG(ERROR) << "TAGAB dest->bookmark->fields->size()=" << dest->bookmark->fields->size();
      }
      if (dest->bookmark->hide_in_toolbar) {
        LOG(ERROR) << "TAGAB *dest->bookmark->hide_in_toolbar=" << *dest->bookmark->hide_in_toolbar;
      }
      LOG(ERROR) << "TAGAB dest->bookmark->=order" << dest->bookmark->order;
    }

    records_extension.emplace_back(std::move(*dest));
  }

  LOG(ERROR) << "TAGAB ConvertSyncRecordsFromLibToExt=============<<<<";
}


} // namespace brave_sync
