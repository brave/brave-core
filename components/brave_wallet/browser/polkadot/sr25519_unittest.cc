#include "brave/components/brave_wallet/browser/polkadot/rust/sr25519.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(PolkdadotSR25519, HelloWorld) {
  polkadot::CxxPolkadotTest x;
  EXPECT_EQ(test_fn(x), 1234);
}

}  // namespace brave_wallet
