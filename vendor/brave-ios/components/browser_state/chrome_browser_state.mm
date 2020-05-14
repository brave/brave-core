//
//  chrome_browser_state.cpp
//  base/third_party/double_conversion:double_conversion
//
//  Created by brandon on 2020-05-07.
//

#include "brave/vendor/brave-ios/components/browser_state/chrome_browser_state.h"

#include "base/files/file_util.h"
#include "base/mac/foundation_util.h"
#include "base/path_service.h"
#include "base/sequenced_task_runner.h"
#include "base/supports_user_data.h"
#include "base/task/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/prefs/pref_service.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "components/user_prefs/user_prefs.h"
#include "ios/chrome/browser/prefs/ios_chrome_pref_service_factory.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"
#include "ios/web/shell/shell_url_request_context_getter.h"
#include "net/url_request/url_request_context_getter.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

const base::FilePath::CharType kProductDirName[] = FILE_PATH_LITERAL("Brave");
const base::FilePath::CharType kIOSChromeInitialBrowserState[] =
    FILE_PATH_LITERAL("Default");

namespace {

bool GetDefaultUserDataDirectory(base::FilePath* result) {
  if (!base::mac::GetUserDirectory(NSApplicationSupportDirectory,
                                                 result)) {
    NOTREACHED();
    return false;
  }

  if (!base::PathExists(*result) && !base::CreateDirectory(*result))
    return false;

  *result = result->Append(kProductDirName);
  return true;
}

base::FilePath GetUserDataDir() {
  base::FilePath user_data_dir;
  bool result = GetDefaultUserDataDirectory(&user_data_dir);
  DCHECK(result);
  return user_data_dir;
}

// Returns a bool indicating whether the necessary directories were able to be
// created (or already existed).
bool EnsureBrowserStateDirectoriesCreated(const base::FilePath& path) {
  // Create the browser state directory synchronously otherwise we would need to
  // sequence every otherwise independent I/O operation inside the browser state
  // directory with this operation. base::CreateDirectory() should be a
  // lightweight I/O operation and avoiding the headache of sequencing all
  // otherwise unrelated I/O after this one justifies running it on the main
  // thread.
  base::ThreadRestrictions::ScopedAllowIO allow_io_to_create_directory;

  if (!base::PathExists(path) && !base::CreateDirectory(path))
    return false;

  return true;
}

// All ChromeBrowserState will store a dummy base::SupportsUserData::Data
// object with this key. It can be used to check that a web::BrowserState
// is effectively a ChromeBrowserState when converting.
const char kBrowserStateIsChromeBrowserState[] = "IsChromeBrowserState";
}

ChromeBrowserState::ChromeBrowserState(const base::FilePath& name)
    : state_path_(GetUserDataDir().Append(name)),
      io_task_runner_(base::CreateSequencedTaskRunner(
          {base::ThreadPool(), base::TaskShutdownBehavior::BLOCK_SHUTDOWN,
           base::MayBlock()})),
      pref_registry_(new user_prefs::PrefRegistrySyncable) {
  DCHECK(io_task_runner_);
  SetUserData(kBrowserStateIsChromeBrowserState,
              std::make_unique<base::SupportsUserData::Data>());

  bool directories_created = EnsureBrowserStateDirectoriesCreated(state_path_);
  DCHECK(directories_created);

  // RegisterBrowserStatePrefs(pref_registry_.get());
  BrowserStateDependencyManager::GetInstance()
      ->RegisterBrowserStatePrefsForServices(pref_registry_.get());

  prefs_ = CreateBrowserStatePrefs(state_path_, GetIOTaskRunner().get(),
                                   pref_registry_);
  // Register on BrowserState.
  user_prefs::UserPrefs::Set(this, prefs_.get());

  BrowserStateDependencyManager::GetInstance()->CreateBrowserStateServices(
      this);

  // Listen for bookmark model load, to bootstrap the sync service.
  // TODO(bridiver)
  // bookmarks::BookmarkModel* model =
  //     ios::BookmarkModelFactory::GetForBrowserState(this);
  // model->AddObserver(new BookmarkModelLoadedObserver(this));
}

ChromeBrowserState::~ChromeBrowserState() {}

// static
ChromeBrowserState* ChromeBrowserState::FromBrowserState(
    web::BrowserState* browser_state) {
  if (!browser_state)
    return nullptr;

  // Check that the BrowserState is a ChromeBrowserState. It should always
  // be true in production and during tests as the only BrowserState that
  // should be used in ios/chrome inherits from ChromeBrowserState.
  DCHECK(browser_state->GetUserData(kBrowserStateIsChromeBrowserState));
  return static_cast<ChromeBrowserState*>(browser_state);
}

net::URLRequestContextGetter* ChromeBrowserState::GetRequestContext() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  if (!request_context_getter_) {
    request_context_getter_ =
        base::WrapRefCounted(CreateRequestContext());
  }
  return request_context_getter_.get();
}

net::URLRequestContextGetter* ChromeBrowserState::CreateRequestContext() {
  return new web::ShellURLRequestContextGetter(
      GetStatePath(), this,
      base::CreateSingleThreadTaskRunner({web::WebThread::IO}));
}

bool ChromeBrowserState::IsOffTheRecord() const {
  return false;
}

base::FilePath ChromeBrowserState::GetStatePath() const {
  return state_path_;
}

scoped_refptr<base::SequencedTaskRunner> ChromeBrowserState::GetIOTaskRunner() {
  return io_task_runner_;
}

PrefService* ChromeBrowserState::GetPrefs() {
  DCHECK(prefs_);  // Should explicitly be initialized.
  return prefs_.get();
}
