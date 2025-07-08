#include "brave/components/brave_wallet/browser/polkadot/rust/sr25519.h"

#include <memory>

namespace brave_wallet {

class HDKeySr25519Impl : public HDKeySr25519 {
 public:
  explicit HDKeySr25519Impl(rust::Box<polkadot::CxxSr25519KeyPair> impl);

 private:
  rust::Box<polkadot::CxxSr25519KeyPair> impl_;
};

HDKeySr25519Impl::HDKeySr25519Impl(rust::Box<polkadot::CxxSr25519KeyPair> impl)
    : impl_(std::move(impl)) {}

std::unique_ptr<HDKeySr25519> HDKeySr25519::GenerateFromSeed(
    base::span<const uint8_t> seed) {
  rust::Slice<uint8_t const> bytes{seed.data(), seed.size()};
  auto mk = polkadot::generate_sr25519_keypair_from_seed(bytes);
  if (mk->is_ok()) {
    return std::make_unique<HDKeySr25519Impl>(mk->unwrap());
  }
  return nullptr;
}

}  // namespace brave_wallet
