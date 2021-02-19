/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_IPFS_IPFS_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_SERVICES_IPFS_IPFS_SERVICE_IMPL_H_

#include <memory>

#include "base/process/process.h"
#include "base/threading/thread.h"
#include "brave/components/services/ipfs/public/mojom/ipfs_service.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace ipfs {

class IpfsServiceImpl : public mojom::IpfsService {
 public:
  explicit IpfsServiceImpl(mojo::PendingReceiver<mojom::IpfsService> receiver);
  ~IpfsServiceImpl() override;

 private:
  // mojom::IpfsService
  void Launch(mojom::IpfsConfigPtr config, LaunchCallback callback) override;
  void Shutdown() override;
  void SetCrashHandler(SetCrashHandlerCallback callback) override;
  void MonitorChild();
  void Cleanup();

  base::Process ipfs_process_;
  mojo::Receiver<mojom::IpfsService> receiver_;
  SetCrashHandlerCallback crash_handler_callback_;
  std::unique_ptr<base::Thread> child_monitor_thread_;
  bool in_shutdown_ = false;

  DISALLOW_COPY_AND_ASSIGN(IpfsServiceImpl);
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_SERVICES_IPFS_IPFS_SERVICE_IMPL_H_
