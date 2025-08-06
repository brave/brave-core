// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/components/prefs/pref_change_registrar_bridge.h"

#include <memory>
#include <string>

#include "base/apple/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/components/prefs/pref_service_bridge_impl.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"

@interface PrefChangeRegistrarBridge () {
  std::unique_ptr<PrefChangeRegistrar> _registrar;
}
@end

@implementation PrefChangeRegistrarBridge

- (instancetype)init {
  if ((self = [super init])) {
    _registrar = std::make_unique<PrefChangeRegistrar>();
  }
  return self;
}

- (instancetype)initWithPrefService:(id<PrefServiceBridge>)prefService {
  if ((self = [self init])) {
    [self initializeWithPrefService:prefService];
  }
  return self;
}

- (void)initializeWithPrefService:(id<PrefServiceBridge>)prefService {
  PrefServiceBridgeImpl* holder =
      base::apple::ObjCCastStrict<PrefServiceBridgeImpl>(prefService);
  _registrar->Init(holder.prefService);
}

- (void)reset {
  _registrar->Reset();
}

- (void)addObserverForPath:(NSString*)path
                  callback:(void (^)(NSString* prefPath))callback {
  // Add observer using the named callback version
  _registrar->Add(base::SysNSStringToUTF8(path),
                  base::BindRepeating(^(const std::string& changedPath) {
                    callback(base::SysUTF8ToNSString(changedPath));
                  }));
}

- (void)removeObserverForPath:(NSString*)path {
  _registrar->Remove(base::SysNSStringToUTF8(path));
}

- (void)removeAllObservers {
  _registrar->RemoveAll();
}

- (BOOL)isEmpty {
  return _registrar->IsEmpty();
}

- (BOOL)isObservedPath:(NSString*)path {
  return _registrar->IsObserved(base::SysNSStringToUTF8(path));
}

@end
