#include "brave/ios/browser/browser_state/browser_state_keyed_service_factories.h"

#include "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#include "ios/chrome/browser/bookmarks/startup_task_runner_service_factory.h"
#include "ios/chrome/browser/history/history_service_factory.h"
#include "ios/chrome/browser/signin/identity_manager_factory.h"
#include "ios/chrome/browser/sync/profile_sync_service_factory.h"
//#include "ios/chrome/browser/sync/session_sync_service_factory.h"
#include "ios/chrome/browser/sync/sync_setup_service_factory.h"
#include "ios/chrome/browser/undo/bookmark_undo_service_factory.h"
#include "base/command_line.h"
#include "components/sync/base/model_type.h"
#include "components/browser_sync/browser_sync_switches.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

void SetupSyncServiceTypes() {
  //const char kDisableSyncTypes[] = "disable-sync-types";
  syncer::ModelTypeSet disabledTypes = syncer::ModelTypeSet(syncer::TYPED_URLS, syncer::PASSWORDS, syncer::PROXY_TABS, syncer::AUTOFILL, syncer::PREFERENCES, syncer::READING_LIST);
    
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  command_line->RemoveSwitch(switches::kDisableSyncTypes);
  command_line->AppendSwitchASCII(switches::kDisableSyncTypes, syncer::ModelTypeSetToString(disabledTypes));
    
//  SyncSetupService* service = SyncSetupServiceFactory::GetForBrowserState(browser_state);
//  service->SetDataTypeEnabled(syncer::BOOKMARKS, true);
//  service->SetDataTypeEnabled(syncer::TYPED_URLS, false);
//  service->SetDataTypeEnabled(syncer::PASSWORDS, false);
//  service->SetDataTypeEnabled(syncer::PROXY_TABS, false);
//  service->SetDataTypeEnabled(syncer::AUTOFILL, false);
//  service->SetDataTypeEnabled(syncer::PREFERENCES, false);
//  service->SetDataTypeEnabled(syncer::READING_LIST, false);
}

void EnsureBrowserStateKeyedServiceFactoriesBuilt() {
  ios::BookmarkModelFactory::GetInstance();
  ios::BookmarkUndoServiceFactory::GetInstance();
  ios::HistoryServiceFactory::GetInstance();
  //ios::InMemoryURLIndexFactory::GetInstance();
  ios::StartupTaskRunnerServiceFactory::GetInstance();
  IdentityManagerFactory::GetInstance();
  ProfileSyncServiceFactory::GetInstance();
  //SessionSyncServiceFactory::GetInstance();
  SyncSetupServiceFactory::GetInstance();
    
  SetupSyncServiceTypes();
}
