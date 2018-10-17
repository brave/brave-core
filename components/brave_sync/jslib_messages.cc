/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_sync/jslib_messages.h"
#include "brave/components/brave_sync/values_conv.h"
#include "base/logging.h"
#include "base/values.h"

namespace brave_sync {

namespace jslib {

Site::Site() {
  ;
}

Site::Site(const base::Value *value) {
  FromValue(value);
}

Site::~Site() {
  ;
}

std::unique_ptr<Site> Site::Clone(const Site& site) {
   auto ret_val = std::make_unique<Site>();
   ret_val->location = site.location;
   ret_val->title = site.title;
   ret_val->customTitle = site.customTitle;
   ret_val->creationTime = site.creationTime;
   ret_val->favicon = site.favicon;
   return  ret_val;
}

void Site::FromValue(const base::Value *site_value) {
  DCHECK(site_value);
  DCHECK(site_value->is_dict());

  const base::Value *location_value = site_value->FindKeyOfType("location", base::Value::Type::STRING);
  DCHECK(location_value);
  this->location = location_value->GetString();
  DCHECK(!this->location.empty());

  const base::Value *title_value = site_value->FindKeyOfType("title", base::Value::Type::STRING);
  DCHECK(title_value);
  this->title = title_value->GetString();

  const base::Value *custom_title_value = site_value->FindKeyOfType("customTitle", base::Value::Type::STRING);
  DCHECK(custom_title_value);
  this->customTitle = custom_title_value->GetString();

  this->lastAccessedTime = ExtractTimeFieldFromDict(site_value, "lastAccessedTime");
  //this->lastAccessedTime may be empty
  this->creationTime = ExtractTimeFieldFromDict(site_value, "creationTime");
  //this->creationTime may be empty

  const base::Value *favicon_value = site_value->FindKeyOfType("favicon", base::Value::Type::STRING);
  DCHECK(favicon_value);
  this->favicon = favicon_value->GetString();
}

Bookmark::Bookmark() : isFolder(false), hideInToolbar(false) {
  ;
}

Bookmark::Bookmark(const base::Value *value) : isFolder(false), hideInToolbar(false) {
  FromValue(value);
}

Bookmark::~Bookmark() {
  ;
}

std::unique_ptr<Bookmark> Bookmark::Clone(const Bookmark& bookmark) {
   auto ret_val = std::make_unique<Bookmark>();
   ret_val->site = std::move(*Site::Clone(bookmark.site));
   ret_val->isFolder = bookmark.isFolder;
   ret_val->parentFolderObjectId = bookmark.parentFolderObjectId;
   ret_val->fields = bookmark.fields;
   ret_val->hideInToolbar = bookmark.hideInToolbar;
   ret_val->order = bookmark.order;
   return  ret_val;
}

void Bookmark::FromValue(const base::Value *bookmark_value) {
  DCHECK(bookmark_value);
  DCHECK(bookmark_value->is_dict());

  const base::Value *site_value = bookmark_value->FindKeyOfType("site", base::Value::Type::DICTIONARY);
  DCHECK(site_value);
  this->site.FromValue(site_value);

  const base::Value *is_folder_value = bookmark_value->FindKeyOfType("isFolder", base::Value::Type::BOOLEAN);
  DCHECK(is_folder_value);
  this->isFolder = is_folder_value->GetBool();

  this->parentFolderObjectId = ExtractIdFieldFromDictOrList(bookmark_value, "parentFolderObjectId");

  const base::Value *hide_in_toolbar_value = bookmark_value->FindKeyOfType("hideInToolbar", base::Value::Type::BOOLEAN);
  if (hide_in_toolbar_value) {
    this->hideInToolbar = hide_in_toolbar_value->GetBool();
  }

  const base::Value *order_value = bookmark_value->FindKeyOfType("order", base::Value::Type::STRING);
  if (order_value) {
    this->order = order_value->GetString();
  }
}

SiteSetting::SiteSetting() : zoomLevel(1.0f), shieldsUp(true),
  adControl(SiteSetting::AdControl::ADC_INVALID),
  cookieControl(SiteSetting::CookieControl::CC_INVALID),
  safeBrowsing(true), noScript(false), httpsEverywhere(true),
  fingerprintingProtection(false), ledgerPayments(false),
  ledgerPaymentsShown(false) {
  ;
}

SiteSetting::SiteSetting(const base::Value *value) : zoomLevel(1.0f), shieldsUp(true),
  adControl(SiteSetting::AdControl::ADC_INVALID),
  cookieControl(SiteSetting::CookieControl::CC_INVALID),
  safeBrowsing(true), noScript(false), httpsEverywhere(true),
  fingerprintingProtection(false), ledgerPayments(false),
  ledgerPaymentsShown(false) {
  FromValue(value);
}

SiteSetting::~SiteSetting() {
  ;
}

std::unique_ptr<SiteSetting> SiteSetting::Clone(const SiteSetting& site_setting) {
   auto ret_val = std::make_unique<SiteSetting>();
   ret_val->hostPattern = site_setting.hostPattern;
   ret_val->zoomLevel = site_setting.zoomLevel;
   ret_val->shieldsUp = site_setting.shieldsUp;
   ret_val->adControl = site_setting.adControl;
   ret_val->cookieControl = site_setting.cookieControl;
   ret_val->safeBrowsing = site_setting.safeBrowsing;
   ret_val->noScript = site_setting.noScript;
   ret_val->httpsEverywhere = site_setting.httpsEverywhere;
   ret_val->fingerprintingProtection = site_setting.fingerprintingProtection;
   ret_val->ledgerPayments = site_setting.ledgerPayments;
   ret_val->ledgerPaymentsShown = site_setting.ledgerPaymentsShown;
   ret_val->fields = site_setting.fields;
   return  ret_val;
}

void SiteSetting::FromValue(const base::Value *site_setting_value) {
  DCHECK(site_setting_value);
  DCHECK(site_setting_value->is_dict());

  const base::Value *host_pattern_value = site_setting_value->FindKeyOfType("hostPattern", base::Value::Type::STRING);
  DCHECK(host_pattern_value);
  this->hostPattern = host_pattern_value->GetString();

  const base::Value *zoom_level_value = site_setting_value->FindKeyOfType("zoomLevel", base::Value::Type::DOUBLE);
  DCHECK(zoom_level_value);
  this->zoomLevel = zoom_level_value->GetDouble();

  this->shieldsUp = ExtractBool(site_setting_value, "shieldsUp");

  this->adControl = ExtractEnum<SiteSetting::AdControl>(site_setting_value, "adControl",
    SiteSetting::AdControl::ADC_MIN, SiteSetting::AdControl::ADC_MAX, SiteSetting::AdControl::ADC_INVALID);
  this->cookieControl = ExtractEnum<SiteSetting::CookieControl>(site_setting_value, "cookieControl",
    SiteSetting::CookieControl::CC_MIN, SiteSetting::CookieControl::CC_MAX, SiteSetting::CookieControl::CC_INVALID);

  this->safeBrowsing = ExtractBool(site_setting_value, "safeBrowsing");
  this->noScript =  ExtractBool(site_setting_value, "noScript");
  this->httpsEverywhere = ExtractBool(site_setting_value, "httpsEverywhere");
  this->fingerprintingProtection = ExtractBool(site_setting_value, "fingerprintingProtection");
  this->ledgerPayments = ExtractBool(site_setting_value, "ledgerPayments");
  this->ledgerPaymentsShown = ExtractBool(site_setting_value, "ledgerPaymentsShown");
}

Device::Device() {
  ;
}

Device::Device(const base::Value *value) {
  FromValue(value);
}

Device::~Device() {
  ;
}

std::unique_ptr<Device> Device::Clone(const Device& device) {
   auto ret_val = std::make_unique<Device>();
   ret_val->name = device.name;
   return  ret_val;
}

void Device::FromValue(const base::Value *device_value) {
  DCHECK(device_value);
  DCHECK(device_value->is_dict());

  const base::Value *name_value = device_value->FindKeyOfType("name", base::Value::Type::STRING);
  DCHECK(name_value);
  this->name = name_value->GetString();
}

SyncRecord::SyncRecord() : action(SyncRecord::Action::A_INVALID) {
  ;
}

SyncRecord::SyncRecord(const base::Value *value) : action(SyncRecord::Action::A_INVALID) {
  FromValue(value);
}

SyncRecord::~SyncRecord() {
  ;
}

std::unique_ptr<SyncRecord> SyncRecord::Clone(const SyncRecord& record) {
   auto ret_val = std::make_unique<SyncRecord>();

   ret_val->action = record.action;
   ret_val->deviceId = record.deviceId;
   ret_val->objectId = record.objectId;
   ret_val->objectData = record.objectData;
   if (record.has_bookmark()) {
     ret_val->SetBookmark(Bookmark::Clone(record.GetBookmark()));
   } else if (record.has_historysite()) {
     ret_val->SetHistorySite(Site::Clone(record.GetHistorySite()));
   } else if (record.has_sitesetting()) {
     ret_val->SetSiteSetting(SiteSetting::Clone(record.GetSiteSetting()));
   } else if (record.has_device()) {
     ret_val->SetDevice(Device::Clone(record.GetDevice()));
   }

   ret_val->syncTimestamp = record.syncTimestamp;

   return  ret_val;
}

bool SyncRecord::has_bookmark() const {
  return bookmark_ != nullptr;
}

bool SyncRecord::has_historysite() const {
  return history_site_ != nullptr;
}

bool SyncRecord::has_sitesetting() const {
  return site_setting_ != nullptr;
}

bool SyncRecord::has_device() const {
  return device_ != nullptr;
}

const Bookmark& SyncRecord::GetBookmark() const {
  DCHECK(has_bookmark());
  return *bookmark_.get();
}

const Site& SyncRecord::GetHistorySite() const {
  DCHECK(has_historysite());
  return *history_site_.get();
}

const SiteSetting& SyncRecord::GetSiteSetting() const {
  DCHECK(has_sitesetting());
  return *site_setting_.get();
}

const Device& SyncRecord::GetDevice() const {
  DCHECK(has_device());
  return *device_.get();
}

void SyncRecord::FromValue(const base::Value *sync_record) {
  this->action = static_cast<SyncRecord::Action>(GetIntAction(*sync_record));
  this->deviceId = ExtractDeviceIdFromDict(sync_record);
  this->objectId = ExtractObjectIdFromDict(sync_record);

  this->syncTimestamp = ExtractTimeFieldFromDict(sync_record, "syncTimestamp");

  const base::Value *bookmark = sync_record->FindKeyOfType("bookmark", base::Value::Type::DICTIONARY);
  const base::Value *historySite = sync_record->FindKeyOfType("historySite", base::Value::Type::DICTIONARY);
  const base::Value *siteSetting = sync_record->FindKeyOfType("siteSetting", base::Value::Type::DICTIONARY);
  const base::Value *device = sync_record->FindKeyOfType("device", base::Value::Type::DICTIONARY);

  DCHECK((bookmark && !historySite && !siteSetting && !device) ||
    (!bookmark && historySite && !siteSetting && !device) ||
    (!bookmark && !historySite && siteSetting && !device) ||
    (!bookmark && !historySite && !siteSetting && device) );

  if (bookmark) {
    bookmark_.reset(new Bookmark(bookmark));
  } else if (historySite) {
    history_site_.reset(new Site(historySite));
  } else if (siteSetting) {
    site_setting_.reset(new SiteSetting(siteSetting));
  } else if (device) {
    device_.reset(new Device(device));
  } else {
    NOTREACHED();
  }
}

void SyncRecord::SetBookmark(std::unique_ptr<Bookmark> bookmark) {
  DCHECK(!has_bookmark() && !has_historysite() && !has_sitesetting() && !has_device());
  bookmark_ = std::move(bookmark);
}

void SyncRecord::SetHistorySite(std::unique_ptr<Site> history_site) {
  DCHECK(!has_bookmark() && !has_historysite() && !has_sitesetting() && !has_device());
  history_site_ = std::move(history_site);
}

void SyncRecord::SetSiteSetting(std::unique_ptr<SiteSetting> site_setting) {
  DCHECK(!has_bookmark() && !has_historysite() && !has_sitesetting() && !has_device());
  site_setting_ = std::move(site_setting);
}

void SyncRecord::SetDevice(std::unique_ptr<Device> device) {
  DCHECK(!has_bookmark() && !has_historysite() && !has_sitesetting() && !has_device());
  device_ = std::move(device);
}

} // jslib

} // namespace brave_sync
