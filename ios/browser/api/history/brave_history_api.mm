/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/history/brave_history_api.h"
#include "brave/ios/browser/api/history/brave_history_observer.h"
#include "brave/ios/browser/api/history/brave_browsing_history_driver.h"

#include "base/compiler_specific.h"
#include "base/containers/adapters.h"
#include "base/strings/sys_string_conversions.h"
#include "base/guid.h"
#include "base/bind.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/sequenced_task_runner.h"

#include "components/browsing_data/core/browsing_data_utils.h"
#include "components/history/core/browser/browsing_history_service.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/sync/driver/sync_service.h"

#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#include "ios/chrome/browser/browsing_data/browsing_data_remover.h"
#include "ios/chrome/browser/browsing_data/browsing_data_remover_factory.h"
#include "ios/chrome/browser/browsing_data/browsing_data_remove_mask.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/history/history_service_factory.h"
#include "ios/chrome/browser/history/web_history_service_factory.h"
#include "ios/chrome/browser/sync/profile_sync_service_factory.h"
#include "ios/web/public/thread/web_thread.h"
#include "ios/web/public/thread/web_task_traits.h"

#include "net/base/mac/url_conversions.h"
#include "url/gurl.h"
#include "ui/base/page_transition_types.h"

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
  date_added_ = base::Time::FromDoubleT([dateAdded timeIntervalSince1970]);
}

- (NSDate*)dateAdded {
  return [NSDate dateWithTimeIntervalSince1970:date_added_.ToDoubleT()];
}
@end

#pragma mark - BraveHistoryAPI

@interface BraveHistoryAPI ()<BraveHistoryDriverDelegate>  {
  // Browser State Used to retreive History Driver - History Service and Web History Service
  ChromeBrowserState* browser_state_;
  // History Browsing Service Driver
  std::unique_ptr<BraveBrowsingHistoryDriver> browsing_history_driver_;
  // History Browsing Service
  std::unique_ptr<history::BrowsingHistoryService> browsing_history_service_;
  // History Service for adding and querying
  history::HistoryService* history_service_ ;
  // WebhistoryService for remove and elete operations
  history::WebHistoryService* web_history_service_;
  // Tracker for history requests.
  base::CancelableTaskTracker tracker_;
}
@property (nonatomic, strong) void(^query_completion)(NSArray<IOSHistoryNode*>*);
@end

@implementation BraveHistoryAPI

/// Shared Singleton Instance for History API
+ (instancetype)sharedHistoryAPI {
  static BraveHistoryAPI* instance = nil;
  static dispatch_once_t onceToken;

  dispatch_once(&onceToken, ^{
    instance = [[BraveHistoryAPI alloc] init];
  });

  return instance;
}

/// Constructor which is setting up services 
/// like HistoryService / SyncService / BrowsingHistoryService / WebHistoryService
- (instancetype)init {
  if ((self = [super init])) {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    ios::ChromeBrowserStateManager* browserStateManager =
        GetApplicationContext()->GetChromeBrowserStateManager();
    browser_state_ =
        browserStateManager->GetLastUsedBrowserState();
    browsing_history_driver_ = std::make_unique<BraveBrowsingHistoryDriver>(
        browser_state_,
        self);
    history_service_ = ios::HistoryServiceFactory::GetForBrowserState(
        browser_state_, ServiceAccessType::EXPLICIT_ACCESS);
    syncer::SyncService* sync_service =
        ProfileSyncServiceFactory::GetForBrowserState(browser_state_);
    browsing_history_service_ =
        std::make_unique<history::BrowsingHistoryService>(
              browsing_history_driver_.get(),
              history_service_,
              sync_service);
    
    web_history_service_ = ios::WebHistoryServiceFactory::GetForBrowserState(browser_state_);
    DCHECK(history_service_);
    DCHECK(browsing_history_service_);
  }
  return self;
}

- (void)dealloc {
  browser_state_ = nil;
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

- (bool)isLoaded {
  // Triggers backend to load if it hasn't already, and then returns whether
  // it's finished loading.
  return history_service_->BackendLoaded();
}

- (void)addHistory:(IOSHistoryNode*)history {
  // Default Add History method with Typed Transition Type
  // Add History is only called when a tyoped_url is added on iOS side
  [self addHistory: history pageTransition: BraveHistoryTransitionType_TYPED];
}

- (void)addHistory:(IOSHistoryNode*)history pageTransition:(BraveHistoryTransitionType)pageTransition {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  DCHECK(history_service_->backend_loaded());

  history::HistoryAddPageArgs args;
  args.url = net::GURLWithNSURL(history.url);
  args.time = base::Time::FromDoubleT([history.dateAdded timeIntervalSince1970]);
  args.redirects = history::RedirectList();
  // Important! Only Typed Transtion is being synced in core side
  args.transition = pageTransition == BraveHistoryTransitionType_TYPED ? ui::PAGE_TRANSITION_TYPED : ui::PAGE_TRANSITION_LINK;
  args.hidden = false;
  args.visit_source = history::VisitSource::SOURCE_BROWSED;
  args.consider_for_ntp_most_visited = true;
  args.title = base::SysNSStringToUTF16(history.title);
  args.floc_allowed = false; // Disable Floc - Not allow tracking

  history_service_->AddPage(args);
}

- (void)removeHistory:(IOSHistoryNode*)history {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  DCHECK(history_service_->backend_loaded());

  // Deletes a specific URL using history service and web history service
  history_service_->DeleteLocalAndRemoteUrl(web_history_service_,
                                           net::GURLWithNSURL(history.url));
}

- (void)removeAllWithCompletion:(void(^)())completion {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  DCHECK(history_service_->backend_loaded());

  // Deletes entire History and from all synced devices
  history_service_->DeleteLocalAndRemoteHistoryBetween(web_history_service_,
                                                      base::Time::Min(),
                                                      base::Time::Max(),
                                                      base::BindOnce([](std::function<void()> completion){
                                                        completion();
                                                      }, completion),
                                                      &tracker_);
}

- (void)searchWithQuery:(NSString*)query maxCount:(NSUInteger)maxCount
                                       completion:(void(^)(NSArray<IOSHistoryNode*>* historyResults))completion {
  __weak BraveHistoryAPI* weak_history_api = self;

  auto search_with_query =
      ^(NSString* query, NSUInteger maxCount,
        void(^completion)(NSArray<IOSHistoryNode*>* historyResults)) {

        BraveHistoryAPI* history_api = weak_history_api;
        if (!history_api) {
          completion(@[]);
          return;
        }

        DCHECK(history_api->history_service_->backend_loaded());

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

        history_api->_query_completion = completion;

        history_api->browsing_history_service_->QueryHistory(queryString, options);
      };

  base::PostTask(
      FROM_HERE, {web::WebThread::UI},
      base::BindOnce(search_with_query, query, maxCount, completion));
}

#pragma mark - BraveHistoryDriverDelegate

- (void)historyQueryWasCompletedWithResults:
    (const std::vector<history::BrowsingHistoryService::HistoryEntry>&)results
                   queryResultsInfo:(const history::BrowsingHistoryService::
                                         QueryResultsInfo&)queryResultsInfo
                        continuationClosure:(base::OnceClosure)continuationClosure {
  
  if (_query_completion) {
    continuationClosure.Reset();
    
    NSMutableArray<IOSHistoryNode*>* historyNodes = [[NSMutableArray alloc] init];

    for (const auto& result : results) {
      IOSHistoryNode *historyNode = [[IOSHistoryNode alloc] initWithURL:net::NSURLWithGURL(result.url)
                                                                  title:base::SysUTF16ToNSString(result.title)
                                                              dateAdded:[NSDate dateWithTimeIntervalSince1970:
                                                                            result.time.ToDoubleT()]];
      [historyNodes addObject:historyNode];
    }

    _query_completion(historyNodes);
    _query_completion = nullptr;
  } else {
    // Pagination
    std::move(continuationClosure).Run();
  }
}

- (void)historyWasDeleted {}

@end
