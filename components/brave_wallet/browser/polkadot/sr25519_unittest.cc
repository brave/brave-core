#include "brave/components/brave_wallet/browser/polkadot/rust/sr25519.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(PolkdadotSR25519, GenerateFromSeed) {
  auto kpresult = HDKeySr25519::GenerateFromSeed({});
  EXPECT_FALSE(kpresult);

  kpresult = HDKeySr25519::GenerateFromSeed({1, 2, 3, 4});
  EXPECT_FALSE(kpresult);

  kpresult = HDKeySr25519::GenerateFromSeed({
      157, 97,  177, 157, 239, 253, 90,  96,  186, 132, 074,
      244, 146, 236, 044, 196, 68,  073, 197, 105, 123, 050,
      105, 025, 112, 59,  172, 003, 28,  174, 127, 96,
  });
  EXPECT_TRUE(kpresult);
}

}  // namespace brave_wallet
