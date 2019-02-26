#include "brave/components/brave_sync/client/brave_sync_client_impl.h"
#include "../../../../../chrome/browser/sync/chrome_sync_client.cc"

namespace browser_sync {

brave_sync::BraveSyncClient* ChromeSyncClient::GetBraveSyncClient() {
  return brave_sync_client_.get();
}

}   // namespace browser_sync
