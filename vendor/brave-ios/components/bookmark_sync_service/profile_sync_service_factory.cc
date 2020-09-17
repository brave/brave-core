#include "ios/chrome/browser/sync/profile_sync_service_factory.h"

#include <utility>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "base/task/post_task.h"
#include "base/time/time.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/invalidation/impl/invalidation_switches.h"
#include "components/invalidation/impl/profile_invalidation_provider.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/network_time/network_time_tracker.h"
#include "components/sync/driver/profile_sync_service.h"
#include "components/sync/driver/startup_controller.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/driver/sync_util.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/autofill/personal_data_manager_factory.h"
#include "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#include "ios/chrome/browser/bookmarks/bookmark_sync_service_factory.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/favicon/favicon_service_factory.h"
#include "ios/chrome/browser/history/history_service_factory.h"


#include "ios/chrome/browser/sync/device_info_sync_service_factory.h"
#include "ios/chrome/browser/sync/ios_chrome_sync_client.h"
#include "ios/chrome/browser/sync/ios_user_event_service_factory.h"
#include "ios/chrome/browser/sync/model_type_store_service_factory.h"
#include "ios/chrome/browser/sync/session_sync_service_factory.h"
#include "ios/chrome/browser/undo/bookmark_undo_service_factory.h"
#include "ios/chrome/common/channel_info.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace {

void UpdateNetworkTimeOnUIThread(base::Time network_time,
                                 base::TimeDelta resolution,
                                 base::TimeDelta latency,
                                 base::TimeTicks post_time) {
  GetApplicationContext()->GetNetworkTimeTracker()->UpdateNetworkTime(
      network_time, resolution, latency, post_time);
}

void UpdateNetworkTime(const base::Time& network_time,
                       const base::TimeDelta& resolution,
                       const base::TimeDelta& latency) {
  base::PostTask(FROM_HERE, {web::WebThread::UI},
                 base::BindOnce(&UpdateNetworkTimeOnUIThread, network_time,
                                resolution, latency, base::TimeTicks::Now()));
}

}  // namespace

// static
ProfileSyncServiceFactory* ProfileSyncServiceFactory::GetInstance() {
  static base::NoDestructor<ProfileSyncServiceFactory> instance;
  return instance.get();
}

// static
syncer::SyncService* ProfileSyncServiceFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  if (!switches::IsSyncAllowedByFlag())
    return nullptr;

  return static_cast<syncer::SyncService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
syncer::SyncService* ProfileSyncServiceFactory::GetForBrowserStateIfExists(
    ChromeBrowserState* browser_state) {
  if (!switches::IsSyncAllowedByFlag())
    return nullptr;

  return static_cast<syncer::SyncService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, false));
}

// static
syncer::ProfileSyncService*
ProfileSyncServiceFactory::GetAsProfileSyncServiceForBrowserState(
    ChromeBrowserState* browser_state) {
  return static_cast<syncer::ProfileSyncService*>(
      GetForBrowserState(browser_state));
}

// static
syncer::ProfileSyncService*
ProfileSyncServiceFactory::GetAsProfileSyncServiceForBrowserStateIfExists(
    ChromeBrowserState* browser_state) {
  return static_cast<syncer::ProfileSyncService*>(
      GetForBrowserStateIfExists(browser_state));
}

ProfileSyncServiceFactory::ProfileSyncServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "ProfileSyncService",
          BrowserStateDependencyManager::GetInstance()) {

  DependsOn(ConsentAuditorFactory::GetInstance());
  DependsOn(DeviceInfoSyncServiceFactory::GetInstance());
  DependsOn(ios::BookmarkModelFactory::GetInstance());
  DependsOn(ios::BookmarkSyncServiceFactory::GetInstance());
  DependsOn(ios::BookmarkUndoServiceFactory::GetInstance());
  DependsOn(ios::HistoryServiceFactory::GetInstance());
}

ProfileSyncServiceFactory::~ProfileSyncServiceFactory() {}

std::unique_ptr<KeyedService>
ProfileSyncServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  ChromeBrowserState* browser_state =
      ChromeBrowserState::FromBrowserState(context);
    
  syncer::ProfileSyncService::InitParams init_params;
  init_params.identity_manager =
      IdentityManagerFactory::GetForBrowserState(browser_state);
  init_params.start_behavior = syncer::ProfileSyncService::MANUAL_START;
  init_params.sync_client =
      std::make_unique<IOSChromeSyncClient>(browser_state);
  init_params.network_time_update_callback = base::Bind(&UpdateNetworkTime);
  init_params.url_loader_factory = browser_state->GetSharedURLLoaderFactory();
  init_params.network_connection_tracker =
      GetApplicationContext()->GetNetworkConnectionTracker();
  init_params.channel = ::GetChannel();
  init_params.debug_identifier = browser_state->GetDebugName();
  init_params.autofill_enable_account_wallet_storage =
      base::FeatureList::IsEnabled(
          autofill::features::kAutofillEnableAccountWalletStorage);

  auto* fcm_invalidation_provider =
      IOSChromeProfileInvalidationProviderFactory::GetForBrowserState(
          browser_state);
  if (fcm_invalidation_provider) {
    init_params.invalidations_identity_providers.push_back(
        fcm_invalidation_provider->GetIdentityProvider());
  }

  // This code should stay here until all invalidation client are
  // migrated from deprecated invalidation  infructructure.
  // Since invalidations will work only if ProfileSyncService calls
  // SetActiveAccountId for all identity providers.
  auto* deprecated_invalidation_provider =
      IOSChromeDeprecatedProfileInvalidationProviderFactory::GetForBrowserState(
          browser_state);
  if (deprecated_invalidation_provider) {
    init_params.invalidations_identity_providers.push_back(
        deprecated_invalidation_provider->GetIdentityProvider());
  }

  auto pss =
      std::make_unique<syncer::ProfileSyncService>(std::move(init_params));
  pss->Initialize();

  // Hook PSS into PersonalDataManager (a circular dependency).
  autofill::PersonalDataManager* pdm =
      autofill::PersonalDataManagerFactory::GetForBrowserState(browser_state);
  pdm->OnSyncServiceInitialized(pss.get());

  return pss;
}
