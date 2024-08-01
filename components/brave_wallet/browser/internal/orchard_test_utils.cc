#include "brave/components/brave_wallet/browser/internal/orchard_test_utils.h"

#include "base/memory/ptr_util.h"

namespace brave_wallet {

OrchardTestUtils::OrchardTestUtils() {
  orchard_test_utils_impl_ = orchard::OrchardTestUtils::Create();
}

OrchardTestUtils::~OrchardTestUtils() {}

OrchardCommitmentValue OrchardTestUtils::CreateMockCommitmentValue(
    uint32_t position,
    uint32_t rseed) {
  return orchard_test_utils_impl_->CreateMockCommitmentValue(position, rseed);
}

}  // namespace brave_wallet
