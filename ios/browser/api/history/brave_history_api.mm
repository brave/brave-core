/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/history/brave_history_api.h"

#include <optional>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/api/history/brave_history_observer.h"
#include "brave/ios/browser/api/history/history_driver_ios.h"
#include "brave/ios/browser/api/history/history_service_listener_ios.h"
#include "components/history/core/browser/browsing_history_service.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/keyed_service/core/service_access_type.h"
#include "ios/chrome/browser/history/model/history_service_factory.h"
#include "ios/chrome/browser/history/model/web_history_service_factory.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/sync/model/sync_service_factory.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"
#include "net/base/apple/url_conversions.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

history::WebHistoryService* WebHistoryServiceGetter(
    base::WeakPtr<ProfileIOS> profile) {
  DCHECK(profile.get())
      << "Getter should not be called after ProfileIOS destruction.";
  return ios::WebHistoryServiceFactory::GetForBrowserState(profile.get());
}

}  // anonymous namespace

DomainMetricTypeIOS const DomainMetricTypeIOSNoMetric =
    static_cast<history::DomainMetricType>(
        history::DomainMetricType::kNoMetric);
DomainMetricTypeIOS const DomainMetricTypeIOSLast1DayMetric =
    static_cast<history::DomainMetricType>(
        history::DomainMetricType::kEnableLast1DayMetric);
DomainMetricTypeIOS const DomainMetricTypeIOSLast7DayMetric =
    static_cast<history::DomainMetricType>(
        history::DomainMetricType::kEnableLast7DayMetric);
DomainMetricTypeIOS const DomainMetricTypeIOSLast28DayMetric =
    static_cast<history::DomainMetricType>(
        history::DomainMetricType::kEnableLast28DayMetric);

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

#pragma mark - IOSHistorySearchOptions

@implementation IOSHistorySearchOptions

- (instancetype)init {
  return [self initWithMaxCount:0
                       hostOnly:NO
              duplicateHandling:HistoryDuplicateHandlingIOSRemoveAll
                      beginDate:nil
                        endDate:nil];
}

- (instancetype)initWithMaxCount:(NSUInteger)maxCount
               duplicateHandling:
                   (HistoryDuplicateHandlingIOS)duplicateHandling {
  return [self initWithMaxCount:maxCount
                       hostOnly:NO
              duplicateHandling:duplicateHandling
                      beginDate:nil
                        endDate:nil];
}

- (instancetype)initWithMaxCount:(NSUInteger)maxCount
                        hostOnly:(BOOL)hostOnly
               duplicateHandling:(HistoryDuplicateHandlingIOS)duplicateHandling
                       beginDate:(nullable NSDate*)beginDate
                         endDate:(nullable NSDate*)endDate {
  if ((self = [super init])) {
    self.maxCount = maxCount;
    self.hostOnly = hostOnly;
    self.duplicateHandling = duplicateHandling;
    self.beginDate = beginDate;
    self.endDate = endDate;
  }
  return self;
}
@end

#pragma mark - BraveHistoryAPI

@interface BraveHistoryAPI () {
  // History Service for adding and querying
  raw_ptr<history::HistoryService> history_service_;
  // WebhistoryService for delete operations
  raw_ptr<history::WebHistoryService> web_history_service_;
  // Tracker for history requests.
  base::CancelableTaskTracker tracker_;

  // Provides dependencies and funnels callbacks from BrowsingHistoryService.
  std::unique_ptr<HistoryDriverIOS> _browsingHistoryDriver;
  // Abstraction to communicate with HistoryService and WebHistoryService.
  std::unique_ptr<history::BrowsingHistoryService> _browsingHistoryService;
}
@property(nonatomic, strong) void (^query_completion)(NSArray<IOSHistoryNode*>*)
    ;
@end

@implementation BraveHistoryAPI {
  raw_ptr<ProfileIOS> _mainBrowserState;  // NOT OWNED
}

- (instancetype)initWithBrowserState:(ProfileIOS*)mainBrowserState {
  if ((self = [super init])) {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    _mainBrowserState = mainBrowserState;

    history_service_ = ios::HistoryServiceFactory::GetForProfile(
        _mainBrowserState, ServiceAccessType::EXPLICIT_ACCESS);
    web_history_service_ =
        ios::WebHistoryServiceFactory::GetForBrowserState(_mainBrowserState);

    _browsingHistoryDriver =
        std::make_unique<HistoryDriverIOS>(base::BindRepeating(
            &WebHistoryServiceGetter, _mainBrowserState->AsWeakPtr()));

    _browsingHistoryService = std::make_unique<history::BrowsingHistoryService>(
        _browsingHistoryDriver.get(),
        ios::HistoryServiceFactory::GetForProfile(
            _mainBrowserState, ServiceAccessType::EXPLICIT_ACCESS),
        SyncServiceFactory::GetForBrowserState(_mainBrowserState));
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

- (void)addHistory:(IOSHistoryNode*)history {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);

  // Important! Only Typed URL is being synced in core side
  history::HistoryAddPageArgs args = history::HistoryAddPageArgs(
      /*url*/ net::GURLWithNSURL(history.url),
      /*time*/ base::Time::FromNSDate(history.dateAdded),
      /*context_id=*/0,
      /*nav_entry_id=*/0, /*local_navigation_id=*/std::nullopt,
      /*referrer=*/GURL(),
      /*redirect_list*/ history::RedirectList(),
      /*transition*/ ui::PAGE_TRANSITION_TYPED,
      /*hidden=*/false, /*visit_source*/ history::VisitSource::SOURCE_BROWSED,
      /*did_replace_entry=*/false, /*consider_for_ntp_most_visited=*/true,
      /*title*/ base::SysNSStringToUTF16(history.title),
      /*opener*/ std::nullopt,
      /*bookmark_id*/ std::nullopt);

  history_service_->AddPage(args);
}

- (void)removeHistoryForNode:(IOSHistoryNode*)node {
  [self removeHistoryForNodes:@[ node ]];
}

- (void)removeHistoryForNodes:(NSArray<IOSHistoryNode*>*)nodes {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);

  // Delete items from Browser History and from synced devices
  std::vector<history::BrowsingHistoryService::HistoryEntry> entries;
  for (IOSHistoryNode* history in nodes) {
    history::BrowsingHistoryService::HistoryEntry entry;
    entry.url = net::GURLWithNSURL(history.url);
    entry.all_timestamps.insert(base::Time::FromNSDate(history.dateAdded)
                                    .ToDeltaSinceWindowsEpoch()
                                    .InMicroseconds());
    entries.push_back(entry);
  }
  _browsingHistoryService->RemoveVisits(entries);
}

- (void)removeAllWithCompletion:(void (^)())completion {
  // Deletes entire History and from all synced devices
  __weak BraveHistoryAPI* weak_history_api = self;
  auto delete_history = ^(void (^callback)()) {
    BraveHistoryAPI* historyAPI = weak_history_api;
    if (!historyAPI) {
      callback();
      return;
    }

    DCHECK_CURRENTLY_ON(web::WebThread::UI);

    historyAPI->history_service_->DeleteLocalAndRemoteHistoryBetween(
        historyAPI->web_history_service_, base::Time::Min(), base::Time::Max(),
        /*app_id*/ std::nullopt, base::BindOnce(callback),
        &historyAPI->tracker_);
  };

  web::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(delete_history, completion));
}

- (void)searchWithQuery:(NSString*)queryArg
                options:(IOSHistorySearchOptions*)searchOptionsArg
             completion:
                 (void (^)(NSArray<IOSHistoryNode*>* historyResults))callback {
  __weak BraveHistoryAPI* weak_history_api = self;
  auto search_with_query = ^(NSString* query,
                             IOSHistorySearchOptions* searchOptions,
                             void (^completion)(NSArray<IOSHistoryNode*>*)) {
    BraveHistoryAPI* historyAPI = weak_history_api;
    if (!historyAPI) {
      completion(@[]);
      return;
    }

    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    std::u16string queryString = !query || [query length] == 0
                                     ? std::u16string()
                                     : base::SysNSStringToUTF16(query);

    // Creating fetch options for querying history
    history::QueryOptions options;
    options.max_count = static_cast<int>(searchOptions.maxCount);
    options.host_only = searchOptions.hostOnly;

    if (searchOptions.beginDate) {
      options.begin_time = base::Time::FromNSDate(searchOptions.beginDate);
    }
    if (searchOptions.endDate) {
      options.end_time = base::Time::FromNSDate(searchOptions.endDate);
    }

    switch (searchOptions.duplicateHandling) {
      case HistoryDuplicateHandlingIOSRemoveAll:
        options.duplicate_policy =
            history::QueryOptions::DuplicateHandling::REMOVE_ALL_DUPLICATES;
        break;
      case HistoryDuplicateHandlingIOSRemovePerDay:
        options.duplicate_policy =
            history::QueryOptions::DuplicateHandling::REMOVE_DUPLICATES_PER_DAY;
        break;
      case HistoryDuplicateHandlingIOSKeepAll:
        options.duplicate_policy =
            history::QueryOptions::DuplicateHandling::KEEP_ALL_DUPLICATES;
        break;
    }
    options.matching_algorithm =
        query_parser::MatchingAlgorithm::ALWAYS_PREFIX_SEARCH;

    historyAPI->history_service_->QueryHistory(
        queryString, options, base::BindOnce(^(history::QueryResults results) {
          NSMutableArray<IOSHistoryNode*>* historyNodes =
              [[NSMutableArray alloc] init];
          for (const auto& result : results) {
            IOSHistoryNode* historyNode = [[IOSHistoryNode alloc]
                initWithURL:net::NSURLWithGURL(result.url())
                      title:base::SysUTF16ToNSString(result.title())
                  dateAdded:result.visit_time().ToNSDate()];
            [historyNodes addObject:historyNode];
          }

          completion(historyNodes);
        }),
        &historyAPI->tracker_);
  };

  web::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(search_with_query, queryArg, searchOptionsArg, callback));
}

- (void)fetchDomainDiversityForType:(DomainMetricTypeIOS)type
                         completion:(void (^)(NSInteger count))completion {
  __weak BraveHistoryAPI* weak_history_api = self;
  auto fetchDomainDiversity =
      ^(DomainMetricTypeIOS metricType, void (^callback)(NSInteger)) {
        BraveHistoryAPI* historyAPI = weak_history_api;
        if (!historyAPI) {
          callback(0);
          return;
        }
        // At the moment we'll never use this API other than to fetch the past 7
        // days worth of unique domains at the current time so hard-coding now &
        // 1
        historyAPI->history_service_->GetDomainDiversity(
            base::Time::Now(), /*number_of_days_to_report*/ 1,
            static_cast<history::DomainMetricType>(type),
            base::BindOnce(
                ^(std::pair<history::DomainDiversityResults,
                            history::DomainDiversityResults> metrics) {
                  if (!metrics.first.empty()) {
                    callback(0);
                    return;
                  }
                  auto& metric = metrics.first.front();
                  NSInteger value = 0;
                  switch (metricType) {
                    case DomainMetricTypeIOSNoMetric:
                      break;
                    case DomainMetricTypeIOSLast1DayMetric:
                      if (metric.one_day_metric) {
                        value = metric.one_day_metric.value().count;
                      }
                      break;
                    case DomainMetricTypeIOSLast7DayMetric:
                      if (metric.seven_day_metric) {
                        value = metric.seven_day_metric.value().count;
                      }
                      break;
                    case DomainMetricTypeIOSLast28DayMetric:
                      if (metric.twenty_eight_day_metric) {
                        value = metric.twenty_eight_day_metric.value().count;
                      }
                      break;
                  }
                  callback(value);
                }),
            &historyAPI->tracker_);
      };
  web::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(fetchDomainDiversity, type, completion));
}

@end
