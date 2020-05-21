// TODO(darkdh): Find out why browser->window()->IsVisible() is false when
// running `RestoreTabsToBrowser` and upstream test doesn't even run session
// restore
#define ShouldPollWhenIntervalExpiredAcrossRestarts DISABLED_ShouldPollWhenIntervalExpiredAcrossRestarts
#include "../../../../../../../chrome/browser/sync/test/integration/single_client_polling_sync_test.cc"
#undef ShouldPollWhenIntervalExpiredAcrossRestarts
