// We don't have one client with scrypt and another using PBKDF2
#define TwoClientCustomPassphraseSyncTestScryptEnabledInPreTest DISABLED_TwoClientCustomPassphraseSyncTestScryptEnabledInPreTest
#include "../../../../../../../chrome/browser/sync/test/integration/two_client_custom_passphrase_sync_test.cc"
#undef TwoClientCustomPassphraseSyncTestScryptEnabledInPreTest
