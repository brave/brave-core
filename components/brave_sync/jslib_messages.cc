/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/jslib_messages.h"

#include <iostream>
#include <utility>

#include "brave/components/brave_sync/jslib_const.h"
#include "brave/components/brave_sync/values_conv.h"
#include "base/logging.h"
#include "base/values.h"

namespace brave_sync {

namespace jslib {

Site::Site() = default;

Site::Site(const Site& site) {
  location = site.location;
  title = site.title;
  customTitle = site.customTitle;
  creationTime = site.creationTime;
  favicon = site.favicon;
}

Site::~Site() = default;

std::unique_ptr<Site> Site::Clone(const Site& site) {
  return std::make_unique<Site>(site);
}

bool Site::Matches(const Site& site) const {
  if (location == site.location &&
      title == site.title &&
      customTitle == site.customTitle &&
      favicon == site.favicon)
    return true;
  return false;
}

std::string Site::TryGetNonEmptyTitle() const {
  return !title.empty() ? title : customTitle;
}

MetaInfo::MetaInfo() = default;

MetaInfo::MetaInfo(const MetaInfo& metaInfo) {
  key = metaInfo.key;
  value = metaInfo.value;
}

MetaInfo::~MetaInfo() = default;

std::unique_ptr<MetaInfo> MetaInfo::Clone(const MetaInfo& metaInfo) {
  return std::make_unique<MetaInfo>(metaInfo);
}

Bookmark::Bookmark() : isFolder(false), hideInToolbar(false) {}

Bookmark::Bookmark(const Bookmark& bookmark) {
  site = bookmark.site;
  isFolder = bookmark.isFolder;
  parentFolderObjectId = bookmark.parentFolderObjectId;
  fields = bookmark.fields;
  hideInToolbar = bookmark.hideInToolbar;
  order = bookmark.order;
  metaInfo = bookmark.metaInfo;
}

Bookmark::~Bookmark() = default;

std::unique_ptr<Bookmark> Bookmark::Clone(const Bookmark& bookmark) {
  return std::make_unique<Bookmark>(bookmark);
}

bool Bookmark::Matches(const Bookmark& bookmark) const {
  if (site.Matches(bookmark.site) &&
      isFolder == bookmark.isFolder &&
      parentFolderObjectId == bookmark.parentFolderObjectId &&
      hideInToolbar == bookmark.hideInToolbar &&
      order == bookmark.order)
    return true;
  return false;
}

SiteSetting::SiteSetting() : zoomLevel(1.0f), shieldsUp(true),
  adControl(SiteSetting::AdControl::ADC_INVALID),
  cookieControl(SiteSetting::CookieControl::CC_INVALID),
  safeBrowsing(true), noScript(false), httpsEverywhere(true),
  fingerprintingProtection(false), ledgerPayments(false),
  ledgerPaymentsShown(false) {}

SiteSetting::~SiteSetting() = default;

std::unique_ptr<SiteSetting> SiteSetting::Clone(
    const SiteSetting& site_setting) {
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
  return ret_val;
}

Device::Device() = default;

Device::~Device() = default;

std::unique_ptr<Device> Device::Clone(const Device& device) {
  auto ret_val = std::make_unique<Device>();
  ret_val->name = device.name;
  ret_val->deviceIdV2 = device.deviceIdV2;
  return ret_val;
}

SyncRecord::SyncRecord() : action(SyncRecord::Action::A_INVALID) {}

SyncRecord::~SyncRecord() = default;

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

  return ret_val;
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

Bookmark* SyncRecord::mutable_bookmark() {
  DCHECK(has_bookmark());
  return bookmark_.get();
}

bool SyncRecord::Matches(const SyncRecord& record) const {
  if (action == record.action && deviceId == record.deviceId &&
      objectId == record.objectId && objectData == record.objectData &&
      has_bookmark() == record.has_bookmark() &&
      has_historysite() == record.has_historysite() &&
      has_sitesetting() == record.has_sitesetting() &&
      has_device() == record.has_device()) {
    if (objectData == jslib_const::SyncObjectData_BOOKMARK)
      return GetBookmark().Matches(record.GetBookmark());
    return true;
  }
  return false;
}

void SyncRecord::SetBookmark(std::unique_ptr<Bookmark> bookmark) {
  DCHECK(!has_bookmark() && !has_historysite() && !has_sitesetting() &&
         !has_device());
  bookmark_ = std::move(bookmark);
}

void SyncRecord::SetHistorySite(std::unique_ptr<Site> history_site) {
  DCHECK(!has_bookmark() && !has_historysite() && !has_sitesetting() &&
         !has_device());
  history_site_ = std::move(history_site);
}

void SyncRecord::SetSiteSetting(std::unique_ptr<SiteSetting> site_setting) {
  DCHECK(!has_bookmark() && !has_historysite() && !has_sitesetting() &&
         !has_device());
  site_setting_ = std::move(site_setting);
}

void SyncRecord::SetDevice(std::unique_ptr<Device> device) {
  DCHECK(!has_bookmark() && !has_historysite() && !has_sitesetting() &&
         !has_device());
  device_ = std::move(device);
}

std::ostream& operator<<(std::ostream& out, const Site& site) {
  out << "location=" << site.location << ", ";
  out << "title=" << site.title << ", ";
  out << "customTitle=" << site.customTitle << ", ";
  out << "creationTime=" << site.creationTime << ", ";
  out << "favicon=" << site.favicon;
  return out;
}

}  // namespace jslib

}  // namespace brave_sync
