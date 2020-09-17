//
//  BraveBrowserState.cpp
//  Sources
//
//  Created by brandon on 2020-05-06.
//

#include "brave/vendor/brave-ios/components/browser_state/brave_browser_state.h"
#include "brave/vendor/brave-ios/components/keyed_service/browser_state_dependency_manager.h"
#include "brave/vendor/brave-ios/components/user_prefs/user_prefs.h"

#include <memory>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/thread_restrictions.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/sync_preferences/pref_service_syncable.h"

#include "ios/chrome/browser/chrome_constants.h"
#include "ios/chrome/browser/chrome_paths_internal.h"
#include "ios/chrome/browser/file_metadata_util.h"
//#include "ios/chrome/browser/net/ios_chrome_url_request_context_getter.h"
#include "ios/chrome/browser/pref_names.h"
#include "ios/chrome/browser/prefs/browser_prefs.h"
#include "ios/chrome/browser/prefs/ios_chrome_pref_service_factory.h"

#include "brave/vendor/brave-ios/components/bookmarks/bookmark_model_factory.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
//#error "This file requires ARC support."
#endif

namespace {
const char kBrowserStateIsChromeBrowserState[] = "IsChromeBrowserState";
}

namespace brave {
bool EnsureBrowserStateDirectoriesCreated(const base::FilePath& path,
                                          const base::FilePath& otr_path,
                                          const base::FilePath& cache_path) {
  base::ThreadRestrictions::ScopedAllowIO allow_io_to_create_directory;

  if (!base::PathExists(path) && !base::CreateDirectory(path))
    return false;
  if (!base::PathExists(otr_path) && !base::CreateDirectory(otr_path))
    return false;
  SetSkipSystemBackupAttributeToItem(otr_path, true);
  if (!base::PathExists(cache_path) && !base::CreateDirectory(cache_path))
    return false;
  return true;
}

base::FilePath GetCachePath(const base::FilePath& base) {
  return base.Append(kIOSChromeCacheDirname); //TODO: Change from string back to enum
}

BraveBrowserState::BraveBrowserState(
    scoped_refptr<base::SequencedTaskRunner> io_task_runner,
    const base::FilePath& path)
    : ChromeBrowserState(std::move(io_task_runner)),
      state_path_(path),
      pref_registry_(new user_prefs::PrefRegistrySyncable),
      io_data_(new BraveBrowserStateIOData::Handle(this)) {
  otr_state_path_ = state_path_.Append(FILE_PATH_LITERAL("OTR"));

  base::FilePath base_cache_path;
  ios::GetUserCacheDirectory(state_path_, &base_cache_path);

  bool directories_created = EnsureBrowserStateDirectoriesCreated(
      state_path_, otr_state_path_, base_cache_path);
  DCHECK(directories_created);

  RegisterBrowserStatePrefs(pref_registry_.get());
  BrowserStateDependencyManager::GetInstance()
      ->RegisterBrowserStatePrefsForServices(pref_registry_.get());

  prefs_ = CreateBrowserStatePrefs(state_path_, GetIOTaskRunner().get(),
                                   pref_registry_);
  // Register on BrowserState.
  user_prefs::UserPrefs::Set(this, prefs_.get());

  // Migrate obsolete prefs.
  PrefService* local_state = GetApplicationContext()->GetLocalState();
  MigrateObsoleteLocalStatePrefs(local_state);
  MigrateObsoleteBrowserStatePrefs(prefs_.get());

  BrowserStateDependencyManager::GetInstance()->CreateBrowserStateServices(
      this);

  base::FilePath cookie_path = state_path_.Append(kIOSChromeCookieFilename);
  base::FilePath cache_path = GetCachePath(base_cache_path);
  int cache_max_size = 0;

  io_data_->Init(cookie_path, cache_path, cache_max_size, state_path_);

  bookmarks::BookmarkModel* model =
      brave::BookmarkModelFactory::GetForBrowserState(this);
  model->AddObserver(new bookmarks::BookmarkModelLoadedObserver(this));

  send_tab_to_self::SendTabToSelfClientServiceFactory::GetForBrowserState(this);
}

BraveBrowserState::~BraveBrowserState() {
  BrowserStateDependencyManager::GetInstance()->DestroyBrowserStateServices(
      this);
//  if (pref_proxy_config_tracker_)
//    pref_proxy_config_tracker_->DetachFromPrefService();
  DestroyOffTheRecordChromeBrowserState();
}

ChromeBrowserState* BraveBrowserState::GetOriginalChromeBrowserState() {
  return this;
}

ChromeBrowserState*
BraveBrowserState::GetOffTheRecordChromeBrowserState() {
  if (!otr_state_) {
    otr_state_.reset(new OffTheRecordChromeBrowserStateImpl(
        GetIOTaskRunner(), this, otr_state_path_));
  }

  return otr_state_.get();
}

bool BraveBrowserState::HasOffTheRecordChromeBrowserState() const {
  return !!otr_state_;
}

void BraveBrowserState::DestroyOffTheRecordChromeBrowserState() {
  otr_state_.reset();
}

PrefService* BraveBrowserState::GetPrefs() {
  DCHECK(prefs_);  // Should explicitly be initialized.
  return prefs_.get();
}

bool BraveBrowserState::IsOffTheRecord() const {
  return false;
}

base::FilePath BraveBrowserState::GetStatePath() const {
  return state_path_;
}

void BraveBrowserState::SetOffTheRecordChromeBrowserState(
    std::unique_ptr<ChromeBrowserState> otr_state) {
  DCHECK(!otr_state_);
  otr_state_ = std::move(otr_state);
}

PrefService* BraveBrowserState::GetOffTheRecordPrefs() {
  DCHECK(prefs_);
  if (!otr_prefs_) {
    otr_prefs_ = CreateIncognitoBrowserStatePrefs(prefs_.get());
  }
  return otr_prefs_.get();
}

BraveBrowserStateIOData* BraveBrowserState::GetIOData() {
  return io_data_->io_data();
}
}
