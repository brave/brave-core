/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

#ifndef BRAVE_IOS_BROWSER_API_HISTORY_HISTORY_SERVICE_LISTENER_IOS_H_
#define BRAVE_IOS_BROWSER_API_HISTORY_HISTORY_SERVICE_LISTENER_IOS_H_

#include "brave/ios/browser/api/history/brave_history_observer.h"
#include "components/history/core/browser/history_service_observer.h"

@interface HistoryServiceListenerImpl : NSObject <HistoryServiceListener>
- (instancetype)init:(id<HistoryServiceObserver>)observer
      historyService:(void*)service;
@end

namespace brave {
namespace ios {
class HistoryServiceListenerIOS : public history::HistoryServiceObserver {
 public:
  explicit HistoryServiceListenerIOS(id<HistoryServiceObserver> observer,
                                     history::HistoryService* service);
  ~HistoryServiceListenerIOS() override;

 private:
  // HistoryServiceListener implementation.
  void OnHistoryServiceLoaded(history::HistoryService* service) override;
  void HistoryServiceBeingDeleted(history::HistoryService* service) override;
  void OnURLVisited(history::HistoryService* service,
                    ui::PageTransition transition,
                    const history::URLRow& row,
                    const history::RedirectList& redirects,
                    base::Time visit_time) override;
  void OnURLsModified(history::HistoryService* history_service,
                      const history::URLRows& changed_urls) override;
  void OnURLsDeleted(history::HistoryService* history_service,
                     const history::DeletionInfo& deletion_info) override;

  id<HistoryServiceObserver> observer_;
  history::HistoryService* service_;  // NOT OWNED
};

}  // namespace ios
}  // namespace brave

#endif  // BRAVE_IOS_BROWSER_API_HISTORY_HISTORY_SERVICE_LISTENER_IOS_H_
