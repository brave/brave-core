#include "brave/components/brave_wallet/browser/polkadot/rust/sr25519.h"

namespace brave_wallet {
int test_fn(brave_wallet::polkadot::CxxPolkadotTest x) {
  return polkadot::rawr(x);
}
}  // namespace brave_wallet
