#include "brave/components/ipfs/pin/ipfs_pin_service_add_job.h"

namespace ipfs {

IpfsPinServiceRemoteAddJob::IpfsPinServiceRemoteAddJob(
    IpfsService* ipfs_service,
    const std::string& service_name,
    const std::string& path,
    const std::vector<std::string>& items,
    JobFinishedCallback callback)
    : IpfsCIDListJob(items, std::move(callback)),
      ipfs_service_(ipfs_service),
      service_name_(service_name),
      path_(path) {}

IpfsPinServiceRemoteAddJob::~IpfsPinServiceRemoteAddJob() {}

void IpfsPinServiceRemoteAddJob::DoWork(const std::string& cid) {
  std::string name = path_ + "/" + cid;
  LOG(ERROR) << "XXXZZZ ExecuteAddNextPin " + cid + " " + name;

  ipfs_service_->AddRemotePin(
      "/ipfs/" + cid, service_name_, name, false,
      base::BindOnce(&IpfsPinServiceRemoteAddJob::OnAddPinResult,
                     base::Unretained(this)));
}

void IpfsPinServiceRemoteAddJob::OnAddPinResult(bool result) {
  LOG(ERROR) << "XXXZZZ OnAddPinResult " << result;
  if (!result) {
    Fail();
    NotifyJobFinished(false, 0);
  } else {
    Continue();
  }
}

}  // namespace ipfs
