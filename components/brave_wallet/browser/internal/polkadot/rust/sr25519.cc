#include "brave/components/brave_wallet/browser/internal/polkadot/rust/sr25519.h"

#include <memory>

#include "brave/components/brave_wallet/browser/internal/polkadot/rust/lib.rs.h"

namespace brave_wallet::schnorrkel {

class SchnorrkelKeyPairImpl : public SchnorrkelKeyPair {
 public:
  explicit SchnorrkelKeyPairImpl(rust::Box<CxxSchnorrkelKeyPair> impl);

  ~SchnorrkelKeyPairImpl() override;
  SR25519PublicKey GetPublicKey() override;

 private:
  rust::Box<CxxSchnorrkelKeyPair> impl_;
};

SchnorrkelKeyPairImpl::~SchnorrkelKeyPairImpl() = default;

SchnorrkelKeyPairImpl::SchnorrkelKeyPairImpl(
    rust::Box<CxxSchnorrkelKeyPair> impl)
    : impl_(std::move(impl)) {}

std::unique_ptr<SchnorrkelKeyPair> SchnorrkelKeyPair::GenerateFromSeed(
    base::span<const uint8_t> seed) {
  rust::Slice<uint8_t const> bytes{seed.data(), seed.size()};
  auto mk = generate_sr25519_keypair_from_seed(bytes);
  if (mk->is_ok()) {
    return std::make_unique<SchnorrkelKeyPairImpl>(mk->unwrap());
  }
  return nullptr;
}

SR25519PublicKey SchnorrkelKeyPairImpl::GetPublicKey() {
  return impl_->get_public_key();
}

}  // namespace brave_wallet::schnorrkel
