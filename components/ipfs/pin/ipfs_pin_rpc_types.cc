#include "brave/components/ipfs/pin/ipfs_pin_rpc_types.h"

namespace ipfs {

RemotePinStatus::RemotePinStatus() {}

RemotePinStatus::~RemotePinStatus() {}

AddRemotePinResult::AddRemotePinResult() {}

AddRemotePinResult::~AddRemotePinResult() {}

GetRemotePinResult::GetRemotePinResult() {}

GetRemotePinResult::~GetRemotePinResult() {}

GetRemotePinResult::GetRemotePinResult(const GetRemotePinResult& result) {}

GetRemotePinServicesResult::GetRemotePinServicesResult() {}

GetRemotePinServicesResult::~GetRemotePinServicesResult() {}

GetRemotePinServicesResult::GetRemotePinServicesResult(
    const GetRemotePinServicesResult&) = default;

RemotePinServiceItem::RemotePinServiceItem() {}

RemotePinServiceItem::~RemotePinServiceItem() {}

RemotePinServiceItem::RemotePinServiceItem(const RemotePinServiceItem&) =
    default;

AddPinResult::AddPinResult() {}

AddPinResult::~AddPinResult() {}

AddPinResult::AddPinResult(const AddPinResult&) {}

}  // namespace ipfs
