//
//  BraveBrowserStateManagerImpl.cpp
//  base/third_party/double_conversion:double_conversion
//
//  Created by brandon on 2020-05-12.
//

#include "brave/vendor/brave-ios/components/browser_state/brave_browser_state_manager.h"
#include "brave/vendor/brave-ios/components/context/brave_application_context.h"

#include <stdint.h>
#include <utility>

#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/metrics/histogram_macros.h"
#include "base/path_service.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/threading/scoped_blocking_call.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/browser_state/browser_state_info_cache.h"
#include "chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/off_the_record_chrome_browser_state_impl.h"
#include "ios/chrome/browser/chrome_constants.h"
#include "ios/chrome/browser/chrome_paths.h"
#include "ios/chrome/browser/pref_names.h"

namespace {

int64_t ComputeFilesSize(const base::FilePath& directory,
                         const base::FilePath::StringType& pattern) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::MAY_BLOCK);
  int64_t running_size = 0;
  base::FileEnumerator iter(directory, false, base::FileEnumerator::FILES,
                            pattern);
  while (!iter.Next().empty())
    running_size += iter.GetInfo().GetSize();
  return running_size;
}

// Simple task to log the size of the browser state at |path|.
void BrowserStateSizeTask(const base::FilePath& path) {
  const int64_t kBytesInOneMB = 1024 * 1024;

  int64_t size = ComputeFilesSize(path, FILE_PATH_LITERAL("*"));
  int size_MB = static_cast<int>(size / kBytesInOneMB);
  UMA_HISTOGRAM_COUNTS_10000("Profile.TotalSize", size_MB);

  size = ComputeFilesSize(path, FILE_PATH_LITERAL("History"));
  size_MB = static_cast<int>(size / kBytesInOneMB);
  UMA_HISTOGRAM_COUNTS_10000("Profile.HistorySize", size_MB);

  size = ComputeFilesSize(path, FILE_PATH_LITERAL("History*"));
  size_MB = static_cast<int>(size / kBytesInOneMB);
  UMA_HISTOGRAM_COUNTS_10000("Profile.TotalHistorySize", size_MB);

  size = ComputeFilesSize(path, FILE_PATH_LITERAL("Cookies"));
  size_MB = static_cast<int>(size / kBytesInOneMB);
  UMA_HISTOGRAM_COUNTS_10000("Profile.CookiesSize", size_MB);

  size = ComputeFilesSize(path, FILE_PATH_LITERAL("Bookmarks"));
  size_MB = static_cast<int>(size / kBytesInOneMB);
  UMA_HISTOGRAM_COUNTS_10000("Profile.BookmarksSize", size_MB);

  size = ComputeFilesSize(path, FILE_PATH_LITERAL("Favicons"));
  size_MB = static_cast<int>(size / kBytesInOneMB);
  UMA_HISTOGRAM_COUNTS_10000("Profile.FaviconsSize", size_MB);

  size = ComputeFilesSize(path, FILE_PATH_LITERAL("Top Sites"));
  size_MB = static_cast<int>(size / kBytesInOneMB);
  UMA_HISTOGRAM_COUNTS_10000("Profile.TopSitesSize", size_MB);

  size = ComputeFilesSize(path, FILE_PATH_LITERAL("Visited Links"));
  size_MB = static_cast<int>(size / kBytesInOneMB);
  UMA_HISTOGRAM_COUNTS_10000("Profile.VisitedLinksSize", size_MB);

  size = ComputeFilesSize(path, FILE_PATH_LITERAL("Web Data"));
  size_MB = static_cast<int>(size / kBytesInOneMB);
  UMA_HISTOGRAM_COUNTS_10000("Profile.WebDataSize", size_MB);

  size = ComputeFilesSize(path, FILE_PATH_LITERAL("Extension*"));
  size_MB = static_cast<int>(size / kBytesInOneMB);
  UMA_HISTOGRAM_COUNTS_10000("Profile.ExtensionSize", size_MB);
}

// Gets the user data directory.
base::FilePath GetUserDataDir() {
  base::FilePath user_data_dir;
  bool result = base::PathService::Get(ios::DIR_USER_DATA, &user_data_dir);
  DCHECK(result);
  return user_data_dir;
}

}  // namespace

namespace brave {
BraveBrowserStateManager::BraveBrowserStateManager() {}

BraveBrowserStateManager::~BraveBrowserStateManager() {
  for (const auto& pair : browser_states_) {
    ChromeBrowserStateImpl* browser_state = pair.second.get();
    ActiveStateManager::FromBrowserState(browser_state)->SetActive(false);
    if (!browser_state->HasOffTheRecordChromeBrowserState())
      continue;

    web::BrowserState* otr_browser_state =
        browser_state->GetOffTheRecordChromeBrowserState();
    if (!ActiveStateManager::ExistsForBrowserState(otr_browser_state))
      continue;
    ActiveStateManager::FromBrowserState(otr_browser_state)->SetActive(false);
  }
}

ChromeBrowserState* BraveBrowserStateManager::GetLastUsedBrowserState() {
  return GetBrowserState(GetLastUsedBrowserStateDir(GetUserDataDir()));
}

ChromeBrowserState* BraveBrowserStateManager::GetBrowserState(
    const base::FilePath& path) {
  // If the browser state is already loaded, just return it.
  auto iter = browser_states_.find(path);
  if (iter != browser_states_.end()) {
    DCHECK(iter->second.get());
    return iter->second.get();
  }
    
  scoped_refptr<base::SequencedTaskRunner> io_task_runner =
      base::CreateSequencedTaskRunner(
          {base::ThreadPool(), base::TaskShutdownBehavior::BLOCK_SHUTDOWN,
           base::MayBlock()});

  std::unique_ptr<BraveBrowserState> browser_state_impl(
      new BraveBrowserState(io_task_runner, path));
  DCHECK(!browser_state_impl->IsOffTheRecord());

  std::pair<ChromeBrowserStateImplPathMap::iterator, bool> insert_result =
      browser_states_.insert(
          std::make_pair(path, std::move(browser_state_impl)));
  DCHECK(insert_result.second);
  DCHECK(insert_result.first != browser_states_.end());

  DoFinalInit(insert_result.first->second.get());
  return insert_result.first->second.get();
}

base::FilePath BraveBrowserStateManager::GetLastUsedBrowserStateDir(
    const base::FilePath& user_data_dir) {
  PrefService* local_state = GetApplicationContext()->GetLocalState();
  DCHECK(local_state);
  std::string last_used_browser_state_name =
      local_state->GetString(prefs::kBrowserStateLastUsed);
  if (last_used_browser_state_name.empty())
    last_used_browser_state_name = kIOSChromeInitialBrowserState;
  return user_data_dir.AppendASCII(last_used_browser_state_name);
}

BrowserStateInfoCache*
BraveBrowserStateManager::GetBrowserStateInfoCache() {
  if (!browser_state_info_cache_) {
    browser_state_info_cache_.reset(new BrowserStateInfoCache(
        GetApplicationContext()->GetLocalState(), GetUserDataDir()));
  }
  return browser_state_info_cache_.get();
}

std::vector<ChromeBrowserState*>
BraveBrowserStateManager::GetLoadedBrowserStates() {
  std::vector<ChromeBrowserState*> loaded_browser_states;
  for (const auto& pair : browser_states_)
    loaded_browser_states.push_back(pair.second.get());
  return loaded_browser_states;
}

void BraveBrowserStateManager::DoFinalInit(
    ChromeBrowserState* browser_state) {
  DoFinalInitForServices(browser_state);
  AddBrowserStateToCache(browser_state);
}

void BraveBrowserStateManager::DoFinalInitForServices(
    ChromeBrowserState* browser_state) {
    
}

void BraveBrowserStateManager::AddBrowserStateToCache(
    ChromeBrowserState* browser_state) {
  DCHECK(!browser_state->IsOffTheRecord());
  BrowserStateInfoCache* cache = GetBrowserStateInfoCache();
  if (browser_state->GetStatePath().DirName() != cache->GetUserDataDir())
    return;
    
  size_t browser_state_index =
      cache->GetIndexOfBrowserStateWithPath(browser_state->GetStatePath());
  if (browser_state_index != std::string::npos) {
      
    return;
  }
  cache->AddBrowserState(browser_state->GetStatePath(), account_info.gaia,
                         username);
}
}
