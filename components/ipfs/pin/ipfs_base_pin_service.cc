#include "brave/components/ipfs/pin/ipfs_base_pin_service.h"

namespace ipfs {

IpfsBaseJob::IpfsBaseJob() {}

IpfsBaseJob::~IpfsBaseJob() {}

IpfsCIDListJob::IpfsCIDListJob(const std::vector<std::string> items,
                               JobFinishedCallback callback)
    : items_(items), callback_(std::move(callback)) {}

IpfsCIDListJob::~IpfsCIDListJob() {}

void IpfsCIDListJob::Start() {
  Continue();
}

void IpfsCIDListJob::Cancel() {
  canceled_ = true;
}

void IpfsCIDListJob::Fail() {
  NotifyJobFinished(false, 0);
}

void IpfsCIDListJob::Continue() {
  // LOG(ERROR) << "XXXZZZ OnAddPinResult " << result;
  if (canceled_) {
    NotifyJobFinished(false, 0);
  }
  if (counter_ < items_.size()) {
    DoWork(items_.at(counter_++));
  } else {
    NotifyJobFinished(true, 0);
  }
}

void IpfsCIDListJob::NotifyJobFinished(bool result, int error_code) {
  std::move(callback_).Run(result);
}

using JobFinishedCallback = base::OnceCallback<void(bool)>;

IpfsBasePinService::IpfsBasePinService(IpfsService* ipfs_service)
    : ipfs_service_(ipfs_service) {}

IpfsBasePinService::~IpfsBasePinService() {}

void IpfsBasePinService::AddJob(std::unique_ptr<IpfsBaseJob> job) {
  jobs_.push(std::move(job));
  if (!current_job_) {
    DoNextJob();
  }
}

void IpfsBasePinService::DoNextJob() {
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

void IpfsBasePinService::OnJobDone(bool result) {
  current_job_.reset();
  DoNextJob();
}

bool IpfsBasePinService::AwaitUntilDaemonStart() {
  if (ipfs_service_->IsDaemonLaunched()) {
    return false;
  }
  ipfs_service_->LaunchDaemon(base::BindOnce(
      &IpfsBasePinService::OnDaemonStarted, base::Unretained(this)));
  return true;
}

void IpfsBasePinService::OnDaemonStarted(bool result) {
  DoNextJob();
}

}  // namespace ipfs
