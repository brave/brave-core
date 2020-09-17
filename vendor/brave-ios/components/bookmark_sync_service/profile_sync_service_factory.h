#ifndef profile_sync_service_factory_h
#define profile_sync_service_factory_h

#include <memory>

#include "base/macros.h"
#include "base/no_destructor.h"
#include "brave/vendor/brave-ios/components/keyed_service/browser_state_keyed_service_factory.h"

class ChromeBrowserState;

namespace syncer {
class ProfileSyncService;
class SyncService;
}  // namespace syncer

namespace brave {
// Singleton that owns all SyncServices and associates them with
// ChromeBrowserState.
class ProfileSyncServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  static syncer::SyncService* GetForBrowserState(
      ChromeBrowserState* browser_state);

  static syncer::SyncService* GetForBrowserStateIfExists(
      ChromeBrowserState* browser_state);

  static syncer::ProfileSyncService* GetAsProfileSyncServiceForBrowserState(
      ChromeBrowserState* browser_state);

  static syncer::ProfileSyncService*
  GetAsProfileSyncServiceForBrowserStateIfExists(
      ChromeBrowserState* browser_state);

  static ProfileSyncServiceFactory* GetInstance();

 private:
  friend class base::NoDestructor<ProfileSyncServiceFactory>;

  ProfileSyncServiceFactory();
  ~ProfileSyncServiceFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
};
}
#endif
