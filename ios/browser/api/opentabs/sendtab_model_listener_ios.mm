/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/opentabs/sendtab_model_listener_ios.h"

#include "base/check.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/api/opentabs/brave_opentabs_api.h"
#include "brave/ios/browser/api/opentabs/brave_sendtab_api.h"
#include "brave/ios/browser/api/opentabs/brave_sendtab_observer.h"
#include "components/send_tab_to_self/send_tab_to_self_entry.h"
#include "components/send_tab_to_self/send_tab_to_self_model.h"
#include "components/send_tab_to_self/send_tab_to_self_model_observer.h"
#include "net/base/apple/url_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace brave {
namespace ios {

SendTabToSelfModelListenerIOS::SendTabToSelfModelListenerIOS(
    id<SendTabToSelfModelStateObserver> observer,
    send_tab_to_self::SendTabToSelfModel* model)
    : observer_(observer), model_(model) {
  DCHECK(observer_);
  DCHECK(model_);
  model_->AddObserver(this);
}

SendTabToSelfModelListenerIOS::~SendTabToSelfModelListenerIOS() {
  DCHECK(model_);
  model_->RemoveObserver(this);
}

void SendTabToSelfModelListenerIOS::SendTabToSelfModelLoaded() {
  if ([observer_ respondsToSelector:@selector(sendTabToSelfModelLoaded)]) {
    [observer_ sendTabToSelfModelLoaded];
  }
}

void SendTabToSelfModelListenerIOS::EntriesAddedRemotely(
    const std::vector<const send_tab_to_self::SendTabToSelfEntry*>&
        new_entries) {
  NSMutableArray<IOSOpenDistantTab*>* entries = [[NSMutableArray alloc] init];

  for (const send_tab_to_self::SendTabToSelfEntry* entry : new_entries) {
    IOSOpenDistantTab* distantTab = [[IOSOpenDistantTab alloc]
        initWithURL:net::NSURLWithGURL(entry->GetURL())
              title:base::SysUTF8ToNSString(entry->GetTitle())
              tabId:0
         sessionTag:base::SysUTF8ToNSString(entry->GetGUID())];
    [entries addObject:distantTab];
  }

  if ([observer_
          respondsToSelector:@selector(sendTabToSelfEntriesAddedRemotely:)]) {
    [observer_ sendTabToSelfEntriesAddedRemotely:[entries copy]];
  }
}

void SendTabToSelfModelListenerIOS::EntriesRemovedRemotely(
    const std::vector<std::string>& guids) {
  if ([observer_
          respondsToSelector:@selector(sendTabToSelfEntriesRemovedRemotely)]) {
    [observer_ sendTabToSelfEntriesRemovedRemotely];
  }
}

void SendTabToSelfModelListenerIOS::EntriesOpenedRemotely(
    const std::vector<const send_tab_to_self::SendTabToSelfEntry*>&
        opened_entries) {
  if ([observer_
          respondsToSelector:@selector(sendTabToSelfEntriesOpenedRemotely)]) {
    [observer_ sendTabToSelfEntriesOpenedRemotely];
  }
}

}  // namespace ios
}  // namespace brave

@interface SendTabToSelfModelListenerImpl () {
  std::unique_ptr<brave::ios::SendTabToSelfModelListenerIOS> observer_;
  raw_ptr<send_tab_to_self::SendTabToSelfModel> model_;
}
@end

@implementation SendTabToSelfModelListenerImpl
- (instancetype)init:(id<SendTabToSelfModelStateObserver>)observer
    sendTabToSelfModel:(void*)model {
  if ((self = [super init])) {
    observer_ = std::make_unique<brave::ios::SendTabToSelfModelListenerIOS>(
        observer, static_cast<send_tab_to_self::SendTabToSelfModel*>(model));

    model_ = static_cast<send_tab_to_self::SendTabToSelfModel*>(model);
  }
  return self;
}

- (void)dealloc {
  [self destroy];
}

- (void)destroy {
  observer_.reset();
}
@end
