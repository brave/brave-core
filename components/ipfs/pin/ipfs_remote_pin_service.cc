#include "brave/components/ipfs/pin/ipfs_remote_pin_service.h"

namespace ipfs {

using JobFinishedCallback = base::OnceCallback<void(bool)>;

IPFSRemotePinService::IPFSRemotePinService(IpfsService* ipfs_service)
    : ipfs_service_(ipfs_service) {}

IPFSRemotePinService::~IPFSRemotePinService() {}

void IPFSRemotePinService::AddPins(const std::string& service_name,
                                   const std::string& prefix,
                                   const std::vector<std::string>& cids,
                                   AddPinCallback callback) {
  LOG(ERROR) << "XXXZZZ Add pins";
  std::unique_ptr<IpfsPinServiceRemoteAddJob> add_job =
      std::make_unique<IpfsPinServiceRemoteAddJob>(
          ipfs_service_, service_name, prefix, cids,
          base::BindOnce(&IPFSRemotePinService::OnAddJobFinished,
                         base::Unretained(this), std::move(callback)));
  AddJob(std::move(add_job));
}

void IPFSRemotePinService::RemovePins(const std::string& service_name,
                                      const std::string& prefix,
                                      const std::vector<std::string>& cids,
                                      RemovePinsCallback callback) {
  //    std::unique_ptr<IpfsPinServiceAddJob> add_job =
  //        std::make_unique<IpfsPinServiceAddJob>(
  //            ipfs_service_, service_name, prefix, cids,
  //            base::BindOnce(&IPFSRemotePinService::OnAddJobFinished,
  //                           base::Unretained(this), std::move(callback)));
  //    AddJob(std::move(add_job));
}

void IPFSRemotePinService::GetPinStatus(const std::string& service_name,
                                        const std::string& prefix,
                                        const std::vector<std::string>& cids,
                                        GetPinStatusCallback callback) {
  ipfs_service_->StartDaemonAndLaunch(base::BindOnce(
      [](IpfsService* ipfs_service, const std::string& service_name,
         const std::string& prefix, const std::vector<std::string>& cids,
         GetPinStatusCallback callback) {
        //                    ipfs_service_->GetRemotePins(service_name,
        //                        name, std::move(callback));
      },
      ipfs_service_, service_name, prefix, cids, std::move(callback)));
}

void IPFSRemotePinService::AddRemotePinService(
    const std::string& name,
    const std::string& endpoint,
    const std::string& key,
    AddRemotePinServiceCallback callback) {
  LOG(ERROR) << "XXXZZZ Add remote pin service";
  ipfs_service_->StartDaemonAndLaunch(base::BindOnce(
      [](IpfsService* ipfs_service, const std::string& name,
         const std::string& endpoint, const std::string& key,
         AddRemotePinServiceCallback callback) {
        LOG(ERROR) << "XXXZZZ Add remote pin service internal";
        ipfs_service->AddRemotePinService(name, endpoint, key,
                                          std::move(callback));
      },
      ipfs_service_, name, endpoint, key, std::move(callback)));
}

void IPFSRemotePinService::RemoveRemotePinService(
    const std::string& name,
    RemoveRemotePinServiceCallback callback) {
  LOG(ERROR) << "XXXZZZ Add remote pin service";

  ipfs_service_->StartDaemonAndLaunch(base::BindOnce(
      [](IpfsService* ipfs_service, const std::string& name,
         AddRemotePinServiceCallback callback) {
        ipfs_service->RemoveRemotePinService(name, std::move(callback));
      },
      ipfs_service_, name, std::move(callback)));
}

void IPFSRemotePinService::GetRemotePinServices(
    GetRemotePinServicesCallback callback) {
  ipfs_service_->StartDaemonAndLaunch(base::BindOnce(
      [](IpfsService* ipfs_service, GetRemotePinServicesCallback callback) {
        LOG(ERROR) << "XXXZZZ daemon launched";
        ipfs_service->GetRemotePinServices(false, std::move(callback));
      },
      ipfs_service_, std::move(callback)));
}

void IPFSRemotePinService::OnAddJobFinished(AddPinCallback client_callback,
                                            bool result) {
  LOG(ERROR) << "XXXZZZ Add pins finished " << result;
  current_job_.reset();
  std::move(client_callback).Run(result);
  DoNextJob();
}

// void IPFSRemotePinService::RemovePins(const std::string& type,
//                                 const std::string& group_subpath,
//                                 RemovePinsCallback callback) {}

void IPFSRemotePinService::AddJob(std::unique_ptr<IpfsBaseJob> job) {
  jobs_.push(std::move(job));
  if (!current_job_) {
    DoNextJob();
  }
}

void IPFSRemotePinService::DoNextJob() {
  LOG(ERROR) << "XXXZZZ do next job";
  if (jobs_.empty()) {
    return;
  }
  if (AwaitUntilDaemonStart()) {
    return;
  }
  LOG(ERROR) << "XXXZZZ do next job 1";

  current_job_ = std::move(jobs_.front());
  jobs_.pop();
  LOG(ERROR) << "XXXZZZ do next job 2";

  current_job_->Start();
}

void IPFSRemotePinService::OnJobDone(bool result) {
  DoNextJob();
}

bool IPFSRemotePinService::AwaitUntilDaemonStart() {
  if (ipfs_service_->IsDaemonLaunched()) {
    return false;
  }
  ipfs_service_->StartDaemonAndLaunch(base::BindOnce(
      &IPFSRemotePinService::OnDaemonStarted, base::Unretained(this)));
  return true;
}

void IPFSRemotePinService::OnDaemonStarted() {
  DoNextJob();
}

}  // namespace ipfs
