/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/history/brave_history_api.h"

#include "base/bind.h"
#include "base/strings/sys_string_conversions.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "ios/chrome/browser/history/history_service_factory.h"
#include "ios/chrome/browser/history/web_history_service_factory.h"
#include "ios/web/public/thread/web_thread.h"
#include "net/base/mac/url_conversions.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

#include "brave/ios/browser/api/history/brave_history_observer.h"
#include "brave/ios/browser/api/history/history_service_listener_ios.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#pragma mark - IOSHistoryNode

@interface IOSHistoryNode () {
  std::u16string title_;
  GURL gurl_;
  base::Time date_added_;
}
@end

@implementation IOSHistoryNode

- (instancetype)initWithURL:(NSURL*)url
                      title:(NSString* _Nullable)title
                  dateAdded:(NSDate* _Nullable)dateAdded {
  if ((self = [super init])) {
    [self setUrl:url];

    if (title) {
      [self setTitle:title];
    }

    if (dateAdded) {
      [self setDateAdded:dateAdded];
    }
  }

  return self;
}

- (void)setUrl:(NSURL*)url {
  gurl_ = net::GURLWithNSURL(url);
}

- (NSURL*)url {
  return net::NSURLWithGURL(gurl_);
}

- (void)setTitle:(NSString*)title {
  title_ = base::SysNSStringToUTF16(title);
}

- (NSString*)title {
  return base::SysUTF16ToNSString(title_);
}

- (void)setDateAdded:(NSDate*)dateAdded {
  date_added_ = base::Time::FromNSDate(dateAdded);
}

- (NSDate*)dateAdded {
  return date_added_.ToNSDate();
}
@end

#pragma mark - BraveHistoryAPI

@interface BraveHistoryAPI () {
  // History Service for adding and querying
  history::HistoryService* history_service_;
  // WebhistoryService for delete operations
  history::WebHistoryService* web_history_service_;
  // Tracker for history requests.
  base::CancelableTaskTracker tracker_;
}
@property(nonatomic, strong) void (^query_completion)(NSArray<IOSHistoryNode*>*)
    ;
@end

@implementation BraveHistoryAPI

- (instancetype)initWithHistoryService:(history::HistoryService*)historyService
                     webHistoryService:
                         (history::WebHistoryService*)webHistoryService {
  if ((self = [super init])) {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    history_service_ = historyService;
    web_history_service_ = webHistoryService;
  }
  return self;
}

- (void)dealloc {
  history_service_ = nil;
  web_history_service_ = nil;
}

- (id<HistoryServiceListener>)addObserver:(id<HistoryServiceObserver>)observer {
  return [[HistoryServiceListenerImpl alloc] init:observer
                                   historyService:history_service_];
}

- (void)removeObserver:(id<HistoryServiceListener>)observer {
  [observer destroy];
}

- (bool)isBackendLoaded {
  // Triggers backend to load if it hasn't already, and then returns whether
  // it's finished loading.
  return history_service_->BackendLoaded();
}

- (void)addHistory:(IOSHistoryNode*)history isURLTyped:(BOOL)isURLTyped {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  DCHECK(history_service_->backend_loaded());

  // Important! Only Typed URL is being synced in core side
  // Disable Floc - Not allow tracking!! 
  history::HistoryAddPageArgs args = history::HistoryAddPageArgs(
      /*url*/ net::GURLWithNSURL(history.url),
      /*time*/ base::Time::FromNSDate(history.dateAdded),
      /*context_id=*/nullptr,
      /*nav_entry_id=*/0, /*referrer=*/GURL(),
      /*redirect_list*/ history::RedirectList(),
      /*transition*/
      isURLTyped ? ui::PAGE_TRANSITION_TYPED : ui::PAGE_TRANSITION_LINK,
      /*hidden=*/false, /*visit_source*/ history::VisitSource::SOURCE_BROWSED,
      /*did_replace_entry=*/false, /*consider_for_ntp_most_visited=*/true,
      /*floc_allowed=*/false,
      /*title*/ base::SysNSStringToUTF16(history.title));

  history_service_->AddPage(args);
}

- (void)removeHistory:(IOSHistoryNode*)history {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  DCHECK(history_service_->backend_loaded());

  // Deletes a specific URL using history service and web history service
  history_service_->DeleteLocalAndRemoteUrl(web_history_service_,
                                            net::GURLWithNSURL(history.url));
}

- (void)removeAllWithCompletion:(void (^)())completion {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  DCHECK(history_service_->backend_loaded());

  // Deletes entire History and from all synced devices
  __weak BraveHistoryAPI* weakSelf = self;
  history_service_->DeleteLocalAndRemoteHistoryBetween(
      web_history_service_, base::Time::Min(), base::Time::Max(),
      base::BindOnce(
          ^(std::function<void()> completion) {
            if (!weakSelf)
              return;
            completion();
          },
          completion),
      &tracker_);
}

- (void)searchWithQuery:(NSString*)query
               maxCount:(NSUInteger)maxCount
             completion:(void (^)(NSArray<IOSHistoryNode*>* historyResults))
                            completion {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  DCHECK(history_service_->backend_loaded());

  // Check Query is empty for Fetching all history
  // The entered query can be nil or empty String
  BOOL fetchAllHistory = !query || [query length] == 0;
  std::u16string queryString =
      fetchAllHistory ? std::u16string() : base::SysNSStringToUTF16(query);

  // Creating fetch options for querying history
  history::QueryOptions options;
  options.duplicate_policy =
      fetchAllHistory ? history::QueryOptions::REMOVE_DUPLICATES_PER_DAY
                      : history::QueryOptions::REMOVE_ALL_DUPLICATES;
  options.max_count = fetchAllHistory ? 0 : static_cast<int>(maxCount);
  options.matching_algorithm =
      query_parser::MatchingAlgorithm::ALWAYS_PREFIX_SEARCH;

  __weak BraveHistoryAPI* weakSelf = self;
  history_service_->QueryHistory(
      queryString, options,
      base::BindOnce(
          ^(std::function<void(NSArray<IOSHistoryNode*>*)> completion,
            history::QueryResults results) {
            BraveHistoryAPI* historyAPI = weakSelf;
            if (!historyAPI) {
              completion(@[]);
              return;
            }

            completion([historyAPI onHistoryResults:std::move(results)]);
          },
          completion),
      &tracker_);
}

- (NSArray<IOSHistoryNode*>*)onHistoryResults:(history::QueryResults)results {
  NSMutableArray<IOSHistoryNode*>* historyNodes = [[NSMutableArray alloc] init];

  for (const auto& result : results) {
    IOSHistoryNode* historyNode = [[IOSHistoryNode alloc]
        initWithURL:net::NSURLWithGURL(result.url())
              title:base::SysUTF16ToNSString(result.title())
          dateAdded:result.visit_time().ToNSDate()];
    [historyNodes addObject:historyNode];
  }

  return [historyNodes copy];
}

@end
