#include "brave/ios/browser/browser_state/browser_state_keyed_service_factories.h"

#include "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#include "ios/chrome/browser/bookmarks/startup_task_runner_service_factory.h"
#include "ios/chrome/browser/history/history_service_factory.h"
#include "ios/chrome/browser/signin/identity_manager_factory.h"
#include "ios/chrome/browser/sync/profile_sync_service_factory.h"
#include "ios/chrome/browser/sync/sync_setup_service_factory.h"
#include "ios/chrome/browser/undo/bookmark_undo_service_factory.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

void EnsureBrowserStateKeyedServiceFactoriesBuilt() {
  ios::BookmarkModelFactory::GetInstance();
  ios::BookmarkUndoServiceFactory::GetInstance();
  ios::HistoryServiceFactory::GetInstance();
  //ios::InMemoryURLIndexFactory::GetInstance();
  ios::StartupTaskRunnerServiceFactory::GetInstance();
  IdentityManagerFactory::GetInstance();
  ProfileSyncServiceFactory::GetInstance();
  SyncSetupServiceFactory::GetInstance();
}
