// brave sync doesn't have pause state
#define SyncPausedState DISABLED_SyncPausedState
#define ShouldTrackDeletionsInSyncPausedState DISABLED_ShouldTrackDeletionsInSyncPausedState
#define ShouldRecordNigoriConfigurationWithInvalidatedCredentials DISABLED_ShouldRecordNigoriConfigurationWithInvalidatedCredentials
#include "../../../../../../../chrome/browser/sync/test/integration/sync_auth_test.cc"
#undef SyncPausedState
#undef ShouldTrackDeletionsInSyncPausedState
#undef ShouldRecordNigoriConfigurationWithInvalidatedCredentials
