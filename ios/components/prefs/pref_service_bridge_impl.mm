// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/components/prefs/pref_service_bridge_impl.h"

#include "base/files/file_path.h"
#include "base/strings/sys_string_conversions.h"
#include "base/time/time.h"
#include "brave/base/apple/base_value_bridge+private.h"
#include "components/prefs/pref_service.h"

@implementation PrefServiceBridgeImpl

- (instancetype)initWithPrefService:(raw_ptr<PrefService>)prefService {
  if ((self = [super init])) {
    _prefService = prefService;
    DCHECK(_prefService);
  }
  return self;
}

- (void)commitPendingWrite {
  self.prefService->CommitPendingWrite();
}

- (BOOL)isManagedPreferenceForPath:(NSString*)path {
  return self.prefService->IsManagedPreference(base::SysNSStringToUTF8(path));
}

- (BOOL)boolForPath:(NSString*)path {
  return self.prefService->GetBoolean(base::SysNSStringToUTF8(path));
}

- (NSInteger)integerForPath:(NSString*)path {
  return self.prefService->GetInteger(base::SysNSStringToUTF8(path));
}

- (double)doubleForPath:(NSString*)path {
  return self.prefService->GetDouble(base::SysNSStringToUTF8(path));
}

- (NSString*)stringForPath:(NSString*)path {
  const std::string& str =
      self.prefService->GetString(base::SysNSStringToUTF8(path));
  return base::SysUTF8ToNSString(str);
}

- (NSString*)filePathForPath:(NSString*)path {
  base::FilePath filePath =
      self.prefService->GetFilePath(base::SysNSStringToUTF8(path));
  return base::SysUTF8ToNSString(filePath.value());
}

- (BaseValueBridge*)valueForPath:(NSString*)path {
  const base::Value& value =
      self.prefService->GetValue(base::SysNSStringToUTF8(path));
  return [[BaseValueBridge alloc] initWithValue:value.Clone()];
}

- (NSDictionary<NSString*, BaseValueBridge*>*)dictionaryForPath:
    (NSString*)path {
  const base::DictValue& dict =
      self.prefService->GetDict(base::SysNSStringToUTF8(path));
  return brave::NSDictionaryFromBaseValueDict(dict.Clone());
}

- (NSArray<BaseValueBridge*>*)listForPath:(NSString*)path {
  const base::ListValue& list =
      self.prefService->GetList(base::SysNSStringToUTF8(path));
  return brave::NSArrayFromBaseValue(base::Value(list.Clone()));
}

- (void)setBool:(BOOL)value forPath:(NSString*)path {
  self.prefService->SetBoolean(base::SysNSStringToUTF8(path), value);
}

- (void)setInteger:(NSInteger)value forPath:(NSString*)path {
  self.prefService->SetInteger(base::SysNSStringToUTF8(path),
                               static_cast<int>(value));
}

- (void)setDouble:(double)value forPath:(NSString*)path {
  self.prefService->SetDouble(base::SysNSStringToUTF8(path), value);
}

- (void)setString:(NSString*)value forPath:(NSString*)path {
  self.prefService->SetString(base::SysNSStringToUTF8(path),
                              base::SysNSStringToUTF8(value));
}

- (void)setFilePath:(NSString*)value forPath:(NSString*)path {
  base::FilePath filePath(base::SysNSStringToUTF8(value));
  self.prefService->SetFilePath(base::SysNSStringToUTF8(path), filePath);
}

- (void)setValue:(BaseValueBridge*)value forPath:(NSString*)path {
  self.prefService->Set(base::SysNSStringToUTF8(path), [value value]);
}

- (void)setDictionary:(NSDictionary<NSString*, BaseValueBridge*>*)dict
              forPath:(NSString*)path {
  base::DictValue baseDict = brave::BaseValueDictFromNSDictionary(dict);
  self.prefService->SetDict(base::SysNSStringToUTF8(path), std::move(baseDict));
}

- (void)setList:(NSArray<BaseValueBridge*>*)list forPath:(NSString*)path {
  base::ListValue baseList = brave::BaseValueListFromNSArray(list);
  self.prefService->SetList(base::SysNSStringToUTF8(path), std::move(baseList));
}

- (int64_t)int64ForPath:(NSString*)path {
  return self.prefService->GetInt64(base::SysNSStringToUTF8(path));
}

- (void)setInt64:(int64_t)value forPath:(NSString*)path {
  self.prefService->SetInt64(base::SysNSStringToUTF8(path), value);
}

- (uint64_t)uint64ForPath:(NSString*)path {
  return self.prefService->GetUint64(base::SysNSStringToUTF8(path));
}

- (void)setUint64:(uint64_t)value forPath:(NSString*)path {
  self.prefService->SetUint64(base::SysNSStringToUTF8(path), value);
}

- (NSDate*)timeForPath:(NSString*)path {
  base::Time time = self.prefService->GetTime(base::SysNSStringToUTF8(path));
  return time.ToNSDate();
}

- (void)setTime:(NSDate*)time forPath:(NSString*)path {
  base::Time baseTime = base::Time::FromNSDate(time);
  self.prefService->SetTime(base::SysNSStringToUTF8(path), baseTime);
}

- (NSTimeInterval)timeDeltaForPath:(NSString*)path {
  base::TimeDelta delta =
      self.prefService->GetTimeDelta(base::SysNSStringToUTF8(path));
  return delta.InSecondsF();
}

- (void)setTimeDelta:(NSTimeInterval)delta forPath:(NSString*)path {
  base::TimeDelta baseDelta = base::Seconds(delta);
  self.prefService->SetTimeDelta(base::SysNSStringToUTF8(path), baseDelta);
}

- (void)clearPrefForPath:(NSString*)path {
  self.prefService->ClearPref(base::SysNSStringToUTF8(path));
}

- (nullable BaseValueBridge*)userPrefValueForPath:(NSString*)path {
  const base::Value* value =
      self.prefService->GetUserPrefValue(base::SysNSStringToUTF8(path));
  if (!value) {
    return nil;
  }
  return [[BaseValueBridge alloc] initWithValue:value->Clone()];
}

- (BOOL)hasPrefPath:(NSString*)path {
  return self.prefService->HasPrefPath(base::SysNSStringToUTF8(path));
}

@end
