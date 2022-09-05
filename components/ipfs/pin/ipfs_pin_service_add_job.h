#include "brave/components/ipfs/ipfs_service.h"

#include "brave/components/ipfs/pin/ipfs_base_pin_service.h"

#include <vector>

namespace ipfs {

class IpfsPinServiceRemoteAddJob : public IpfsCIDListJob {
 public:
  IpfsPinServiceRemoteAddJob(IpfsService* ipfs_service,
                             const std::string& service_name,
                             const std::string& path,
                             const std::vector<std::string>& items,
                             JobFinishedCallback callback);

  ~IpfsPinServiceRemoteAddJob() override;

 protected:
  void DoWork(const std::string& cid) override;

 private:
  void OnAddPinResult(bool);
  IpfsService* ipfs_service_;

  std::string service_name_;
  std::string path_;
};

}  // namespace ipfs
