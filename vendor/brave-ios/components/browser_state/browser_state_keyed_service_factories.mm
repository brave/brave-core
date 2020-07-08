#include "brave/vendor/brave-ios/components/browser_state/browser_state_keyed_service_factories.h"

#include "base/logging.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "brave/vendor/brave-ios/components/bookmarks/bookmark_model_factory.h"
#include "brave/vendor/brave-ios/components/bookmarks/startup_task_runner_service_factory.h"
#include "brave/vendor/brave-ios/components/bookmark_sync_service/bookmark_undo_service_factory.h"

//#include "ios/chrome/browser/history/history_service_factory.h"
//#include "ios/chrome/browser/sync/profile_sync_service_factory.h"
//#include "ios/chrome/browser/sync/sync_setup_service_factory.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

void EnsureBrowserStateKeyedServiceFactoriesBuilt() {
  ios::BookmarkModelFactory::GetInstance();
  ios::BookmarkUndoServiceFactory::GetInstance();
  //ios::HistoryServiceFactory::GetInstance();
  //ios::InMemoryURLIndexFactory::GetInstance();
  ios::StartupTaskRunnerServiceFactory::GetInstance();
  //ProfileSyncServiceFactory::GetInstance();
  //SyncSetupServiceFactory::GetInstance();
}

