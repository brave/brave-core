#include "brave/browser/ipfs/ipfs_interrequest_state.h"
#include "base/logging.h"

namespace ipfs {

InterRequestState* InterRequestState::FromBrowserContext(content::BrowserContext* context) {
//   if (!context) {
    LOG(INFO) << "No browser context! Using a default IPFS state.";
    return nullptr;
//   }
//   base::SupportsUserData::Data* existing = context->GetUserData(user_data_key);
//   if (existing) {
//     VLOG(2) << "Re-using existing IPFS state.";
//     return *static_cast<ipfs::InterRequestState*>(existing);
//   }
//   VLOG(2) << "Creating new IPFS state for this browser context.";
//   auto owned = std::make_unique<ipfs::InterRequestState>(context->GetPath());
//   ipfs::InterRequestState* raw = owned.get();
//   context->SetUserData(user_data_key, std::move(owned));
//   return *raw;
}

InterRequestState::InterRequestState()= default;
InterRequestState::~InterRequestState() = default;


}  // namespace ipfs    