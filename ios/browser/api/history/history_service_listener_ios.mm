/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/history/history_service_listener_ios.h"

#include "base/check.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/api/history/brave_history_api.h"
#include "brave/ios/browser/api/history/brave_history_observer.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_service_observer.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface IOSHistoryNode (Private)
- (instancetype)initWithURL:(NSURL*)url
                      title:(NSString* _Nullable)title
                  dateAdded:(NSDate* _Nullable)dateAdded;
@end

namespace brave {
namespace ios {

HistoryServiceListenerIOS::HistoryServiceListenerIOS(
    id<HistoryServiceObserver> observer,
    history::HistoryService* service)
    : observer_(observer), service_(service) {
  DCHECK(observer_);
  DCHECK(service_);
  service_->AddObserver(this);
}

HistoryServiceListenerIOS::~HistoryServiceListenerIOS() {
  DCHECK(service_);
  service_->RemoveObserver(this);
}

void HistoryServiceListenerIOS::OnHistoryServiceLoaded(
    history::HistoryService* service) {
  if ([observer_ respondsToSelector:@selector(historyServiceLoaded)]) {
    [observer_ historyServiceLoaded];
  }
}

void HistoryServiceListenerIOS::HistoryServiceBeingDeleted(
    history::HistoryService* service) {
  if ([observer_ respondsToSelector:@selector(historyServiceBeingDeleted)]) {
    [observer_ historyServiceBeingDeleted];
  }
}

void HistoryServiceListenerIOS::OnURLVisited(
    history::HistoryService* history_service,
    const history::URLRow& url_row,
    const history::VisitRow& new_visit) {
  IOSHistoryNode* historyNode = [[IOSHistoryNode alloc]
      initWithURL:net::NSURLWithGURL(url_row.url())
            title:base::SysUTF16ToNSString(url_row.title())
        dateAdded:new_visit.visit_time.ToNSDate()];

  if ([observer_ respondsToSelector:@selector(historyNodeVisited:)]) {
    [observer_ historyNodeVisited:historyNode];
  }
}

void HistoryServiceListenerIOS::OnURLsModified(
    history::HistoryService* history_service,
    const history::URLRows& changed_urls) {
  NSMutableArray<IOSHistoryNode*>* nodes = [[NSMutableArray alloc] init];
  for (const history::URLRow& row : changed_urls) {
    IOSHistoryNode* node = [[IOSHistoryNode alloc]
        initWithURL:net::NSURLWithGURL(row.url())
              title:base::SysUTF16ToNSString(row.title())
          dateAdded:row.last_visit().ToNSDate()];
    [nodes addObject:node];
  }

  if ([observer_ respondsToSelector:@selector(historyNodesModified:)]) {
    [observer_ historyNodesModified:nodes];
  }
}

void HistoryServiceListenerIOS::OnHistoryDeletions(
    history::HistoryService* history_service,
    const history::DeletionInfo& deletion_info) {
  bool isAllHistory = false;
  NSMutableArray<IOSHistoryNode*>* nodes = [[NSMutableArray alloc] init];

  if (deletion_info.IsAllHistory()) {
    isAllHistory = true;
  } else {
    for (const history::URLRow& row : deletion_info.deleted_rows()) {
      IOSHistoryNode* node = [[IOSHistoryNode alloc]
          initWithURL:net::NSURLWithGURL(row.url())
                title:base::SysUTF16ToNSString(row.title())
            dateAdded:row.last_visit().ToNSDate()];
      [nodes addObject:node];
    }
  }

  if ([observer_ respondsToSelector:@selector(historyNodesDeleted:
                                                     isAllHistory:)]) {
    [observer_ historyNodesDeleted:nodes isAllHistory:isAllHistory];
  }
}

}  // namespace ios
}  // namespace brave

@interface HistoryServiceListenerImpl () {
  std::unique_ptr<brave::ios::HistoryServiceListenerIOS> observer_;
  raw_ptr<history::HistoryService> history_service_;
}
@end

@implementation HistoryServiceListenerImpl
- (instancetype)init:(id<HistoryServiceObserver>)observer
      historyService:(void*)service {
  if ((self = [super init])) {
    observer_ = std::make_unique<brave::ios::HistoryServiceListenerIOS>(
        observer, static_cast<history::HistoryService*>(service));

    history_service_ = static_cast<history::HistoryService*>(service);
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
