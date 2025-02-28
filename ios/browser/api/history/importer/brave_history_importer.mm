/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/history/importer/brave_history_importer.h"

#include <vector>

#include "base/apple/foundation_util.h"
#include "base/base_paths.h"
#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/path_service.h"
#include "base/stl_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "brave/ios/browser/api/history/importer/history_json_reader.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/keyed_service/core/service_access_type.h"
#include "ios/chrome/browser/history/model/history_service_factory.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/profile/profile_manager_ios.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

namespace history {
void AddHistoryItems(const std::vector<history::URLRow>& history_items) {
  std::vector<ProfileIOS*> profiles =
      GetApplicationContext()->GetProfileManager()->GetLoadedProfiles();
  ProfileIOS* last_used_profile = profiles.at(0);
  ios::HistoryServiceFactory::GetForProfile(last_used_profile,
                                            ServiceAccessType::EXPLICIT_ACCESS)
      ->AddPagesWithDetails(history_items, VisitSource::SOURCE_SAFARI_IMPORTED);
}
}  // namespace history

@implementation BraveImportedHistory
- (instancetype)initFromChromiumImportedHistory:(const history::URLRow&)entry {
  if ((self = [super init])) {
    _url = net::NSURLWithGURL(entry.url());
    _title = base::SysUTF16ToNSString(entry.title());
    _visitCount = entry.visit_count();
    _lastVisitDate =
        [NSDate dateWithTimeIntervalSince1970:entry.last_visit()
                                                  .InSecondsFSinceUnixEpoch()];
  }
  return self;
}

- (history::URLRow)toChromiumImportedHistory {
  history::URLRow entry = history::URLRow(net::GURLWithNSURL(self.url));
  entry.set_title(base::SysNSStringToUTF16(self.title));
  entry.set_visit_count(self.visitCount);
  entry.set_last_visit(base::Time::FromSecondsSinceUnixEpoch(
      [self.lastVisitDate timeIntervalSince1970]));
  return entry;
}
@end

@interface BraveHistoryImporter () {
  scoped_refptr<base::SequencedTaskRunner> import_thread_;
}
@property(atomic) bool cancelled;  // atomic
@end

@implementation BraveHistoryImporter
- (instancetype)init {
  if ((self = [super init])) {
    self.cancelled = false;

    import_thread_ = base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
         base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
  }
  return self;
}

- (void)dealloc {
  [self cancel];
}

- (void)cancel {
  self.cancelled = true;
}

- (void)importFromFile:(NSString*)filePath
       automaticImport:(bool)automaticImport
          withListener:
              (void (^)(BraveHistoryImporterState,
                        NSArray<BraveImportedHistory*>* _Nullable))listener {
  __weak BraveHistoryImporter* weakSelf = self;

  auto start_import = ^{
    // Import cancelled as the importer has been deallocated
    __strong BraveHistoryImporter* importer = weakSelf;
    if (!importer) {
      listener(BraveHistoryImporterStateStarted, nullptr);
      listener(BraveHistoryImporterStateCancelled, nullptr);
      return;
    }

    base::FilePath source_file_path = base::apple::NSStringToFilePath(filePath);

    listener(BraveHistoryImporterStateStarted, nullptr);
    std::vector<history::URLRow> history_items;
    history_json_reader::ImportHistoryFile(
        base::BindRepeating(^{
          return [importer isImporterCancelled];
        }),
        base::BindRepeating(^(const GURL& url) {
          return [importer canImportURL:url];
        }),
        source_file_path, &history_items);

    if (!history_items.empty() && ![importer isImporterCancelled]) {
      if (automaticImport) {
        // Import into the Profile/ProfileIOS on the main-thread.
        web::GetUIThreadTaskRunner({})->PostTask(
            FROM_HERE, base::BindOnce(
                           ^(std::vector<history::URLRow> items) {
                             history::AddHistoryItems(items);
                             listener(BraveHistoryImporterStateAutoCompleted,
                                      nullptr);
                           },
                           std::move(history_items)));
      } else {
        listener(BraveHistoryImporterStateCompleted,
                 [importer convertToIOSImportedHistory:history_items]);
      }
    } else {
      listener(BraveHistoryImporterStateCancelled, nullptr);
    }
  };

  // Run the importer on the sequenced task runner.
  import_thread_->PostTask(FROM_HERE, base::BindOnce(start_import));
}

- (void)importFromArray:(NSArray<BraveImportedHistory*>*)historyItems
           withListener:(void (^)(BraveHistoryImporterState))listener {
  __weak BraveHistoryImporter* weakSelf = self;

  auto start_import = ^{
    // Import cancelled as the importer has been deallocated
    __strong BraveHistoryImporter* importer = weakSelf;
    if (!importer) {
      listener(BraveHistoryImporterStateStarted);
      listener(BraveHistoryImporterStateCancelled);
      return;
    }

    listener(BraveHistoryImporterStateStarted);
    history::AddHistoryItems(
        [importer convertToChromiumImportedHistory:historyItems]);
    listener(BraveHistoryImporterStateCompleted);
  };

  // Import into the Profile/ProfileIOS on the main-thread.
  import_thread_->PostTask(FROM_HERE, base::BindOnce(start_import));
}

// MARK: - Private

- (bool)isImporterCancelled {
  return self.cancelled;
}

// Returns true if |url| has a valid scheme that we allow to import. We
// filter out the URL with a unsupported scheme.
- (bool)canImportURL:(const GURL&)url {
  // The URL is not valid.
  if (!url.is_valid()) {
    return false;
  }

  // Filter out the URLs with unsupported schemes.
  const char* const kInvalidSchemes[] = {"wyciwyg", "place", "about", "chrome"};
  for (const char* scheme : kInvalidSchemes) {
    if (url.SchemeIs(scheme)) {
      return false;
    }
  }

  return true;
}

// Converts an array of Chromium imported history to iOS imported history.
- (NSArray<BraveImportedHistory*>*)convertToIOSImportedHistory:
    (const std::vector<history::URLRow>&)historyItems {
  NSMutableArray<BraveImportedHistory*>* results =
      [[NSMutableArray alloc] init];
  for (const auto& historyItem : historyItems) {
    BraveImportedHistory* imported_history_item = [[BraveImportedHistory alloc]
        initFromChromiumImportedHistory:historyItem];
    [results addObject:imported_history_item];
  }
  return results;
}

// Converts an array of iOS imported history to Chromium imported history.
- (std::vector<history::URLRow>)convertToChromiumImportedHistory:
    (NSArray<BraveImportedHistory*>*)historyItems {
  std::vector<history::URLRow> results;
  for (BraveImportedHistory* historyItem in historyItems) {
    results.push_back([historyItem toChromiumImportedHistory]);
  }
  return results;
}
@end
