/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

#ifndef BRAVE_IOS_BROWSER_API_OPENTABS_SENDTAB_MODEL_LISTENER_IOS_H_
#define BRAVE_IOS_BROWSER_API_OPENTABS_SENDTAB_MODEL_LISTENER_IOS_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/ios/browser/api/opentabs/brave_sendtab_observer.h"
#include "components/send_tab_to_self/send_tab_to_self_entry.h"
#include "components/send_tab_to_self/send_tab_to_self_model.h"
#include "components/send_tab_to_self/send_tab_to_self_model_observer.h"

@interface SendTabToSelfModelListenerImpl
    : NSObject <SendTabToSelfModelStateListener>
- (instancetype)init:(id<SendTabToSelfModelStateObserver>)observer
    sendTabToSelfModel:(void*)model;
@end

namespace brave {
namespace ios {

class SendTabToSelfModelListenerIOS
    : public send_tab_to_self::SendTabToSelfModelObserver {
 public:
  explicit SendTabToSelfModelListenerIOS(
      id<SendTabToSelfModelStateObserver> observer,
      send_tab_to_self::SendTabToSelfModel* model);
  ~SendTabToSelfModelListenerIOS() override;

 private:
  // SendTabToSelfModelListener implementation.
  void SendTabToSelfModelLoaded() override;
  void EntriesAddedRemotely(
      const std::vector<const send_tab_to_self::SendTabToSelfEntry*>&
          new_entries) override;
  void EntriesRemovedRemotely(const std::vector<std::string>& guids) override;
  void EntriesOpenedRemotely(
      const std::vector<const send_tab_to_self::SendTabToSelfEntry*>&
          opened_entries) override;

  id<SendTabToSelfModelStateObserver> observer_;
  raw_ptr<send_tab_to_self::SendTabToSelfModel> model_;
};

}  // namespace ios
}  // namespace brave

#endif  // BRAVE_IOS_BROWSER_API_OPENTABS_SENDTAB_MODEL_LISTENER_IOS_H_
