#include "brave/components/brave_sync/brave_profile_sync_service.h"
using brave_sync::BraveProfileSyncService;
#include "../../../../../chrome/browser/sync/profile_sync_service_factory.cc"

// static
BraveProfileSyncService*
ProfileSyncServiceFactory::GetAsBraveProfileSyncServiceForProfile(Profile* profile) {
  return static_cast<BraveProfileSyncService*>(GetForProfile(profile));
}
