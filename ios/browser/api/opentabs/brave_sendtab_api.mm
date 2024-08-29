/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/opentabs/brave_sendtab_api.h"

#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/api/opentabs/brave_sendtab_observer.h"
#include "brave/ios/browser/api/opentabs/sendtab_model_listener_ios.h"
#include "components/send_tab_to_self/send_tab_to_self_model.h"
#include "components/send_tab_to_self/send_tab_to_self_sync_service.h"
#include "components/send_tab_to_self/target_device_info.h"
#include "net/base/apple/url_conversions.h"

namespace brave {
namespace ios {
TargetDeviceType DeviceTypeFromSyncDeviceType(
    syncer::DeviceInfo::FormFactor deviceType) {
  switch (deviceType) {
    case syncer::DeviceInfo::FormFactor::kUnknown:
    case syncer::DeviceInfo::FormFactor::kAutomotive:
    case syncer::DeviceInfo::FormFactor::kWearable:
    case syncer::DeviceInfo::FormFactor::kTv:
      return TargetDeviceTypeUnset;
    case syncer::DeviceInfo::FormFactor::kDesktop:
      return TargetDeviceTypePC;
    case syncer::DeviceInfo::FormFactor::kPhone:
      return TargetDeviceTypeMobile;
    case syncer::DeviceInfo::FormFactor::kTablet:
      return TargetDeviceTypeTablet;
  }
}
}  // namespace ios
}  // namespace brave

#pragma mark - IOSSendTabTargetDevice

@implementation IOSSendTabTargetDevice

- (instancetype)initWithFullName:(NSString*)fullName
                       shortName:(NSString*)shortName
                      deviceName:(NSString*)deviceName
                         cacheId:(NSString*)cacheId
                      deviceType:(TargetDeviceType)deviceType
                 lastUpdatedTime:(NSDate*)lastUpdatedTime {
  if ((self = [super init])) {
    self.fullName = fullName;
    self.shortName = shortName;
    self.deviceName = deviceName;
    self.cacheId = cacheId;
    self.deviceType = deviceType;
    self.lastUpdatedTime = lastUpdatedTime;
  }

  return self;
}

- (id)copyWithZone:(NSZone*)zone {
  IOSSendTabTargetDevice* targetDevice =
      [[[self class] allocWithZone:zone] init];

  if (targetDevice) {
    targetDevice.fullName = self.fullName;
    targetDevice.shortName = self.shortName;
    targetDevice.deviceName = self.deviceName;
    targetDevice.cacheId = self.cacheId;
    targetDevice.deviceType = self.deviceType;
    targetDevice.lastUpdatedTime = self.lastUpdatedTime;
  }

  return targetDevice;
}

@end

#pragma mark - BraveSendTabAPI

@interface BraveSendTabAPI () {
  // SendTab Sync Service is needed in order to send session data to
  // different devices - receive device information
  raw_ptr<send_tab_to_self::SendTabToSelfSyncService> sendtab_sync_service_;

  // Model to send current tab to other devices
  raw_ptr<send_tab_to_self::SendTabToSelfModel> send_tab_to_self_model_;
}
@end

@implementation BraveSendTabAPI

- (instancetype)initWithSyncService:
    (send_tab_to_self::SendTabToSelfSyncService*)syncService {
  if ((self = [super init])) {
    sendtab_sync_service_ = syncService;
    send_tab_to_self_model_ = sendtab_sync_service_->GetSendTabToSelfModel();
  }
  return self;
}

- (void)dealloc {
  sendtab_sync_service_ = nullptr;
}

- (id<SendTabToSelfModelStateListener>)addObserver:
    (id<SendTabToSelfModelStateObserver>)observer {
  return [[SendTabToSelfModelListenerImpl alloc] init:observer
                                   sendTabToSelfModel:send_tab_to_self_model_];
}

- (void)removeObserver:(id<SendTabToSelfModelStateListener>)observer {
  [observer destroy];
}

- (NSArray<IOSSendTabTargetDevice*>*)getListOfSyncedDevices {
  DCHECK(sendtab_sync_service_);

  NSMutableArray<IOSSendTabTargetDevice*>* targetDeviceList =
      [[NSMutableArray alloc] init];

  // The list of devices with thier names, cache_guids, device types,
  // and active times.
  std::vector<send_tab_to_self::TargetDeviceInfo> target_device_list_ =
      sendtab_sync_service_->GetSendTabToSelfModel()
          ->GetTargetDeviceInfoSortedList();

  for (const auto& device : target_device_list_) {
    IOSSendTabTargetDevice* targetDevice = [[IOSSendTabTargetDevice alloc]
        initWithFullName:base::SysUTF8ToNSString(device.full_name)
               shortName:base::SysUTF8ToNSString(device.short_name)
              deviceName:base::SysUTF8ToNSString(device.device_name)
                 cacheId:base::SysUTF8ToNSString(device.cache_guid)
              deviceType:brave::ios::DeviceTypeFromSyncDeviceType(
                             device.form_factor)
         lastUpdatedTime:device.last_updated_timestamp.ToNSDate()];

    [targetDeviceList addObject:targetDevice];
  }

  return [targetDeviceList copy];
}

- (void)sendActiveTabToDevice:(NSString*)deviceID
                     tabTitle:(NSString*)tabTitle
                    activeURL:(NSURL*)activeURL {
  GURL url = net::GURLWithNSURL(activeURL);
  std::string title = base::SysNSStringToUTF8(tabTitle);
  std::string target_device = base::SysNSStringToUTF8(deviceID);

  send_tab_to_self_model_->AddEntry(url, title, target_device);
}

@end
