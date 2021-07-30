/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CHILD_PROCESS_MONITOR_CHILD_PROCESS_MONITOR_H_
#define BRAVE_COMPONENTS_CHILD_PROCESS_MONITOR_CHILD_PROCESS_MONITOR_H_

#include <memory>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/process/process.h"
#include "base/sequence_checker.h"
#include "base/threading/thread.h"

namespace brave {

// This class can only be used once per utility process to monitor the lifetime
// of external process, ex. IpfsServiceImpl and TorLauncherImpl
class ChildProcessMonitor {
 public:
  ChildProcessMonitor();
  ~ChildProcessMonitor();
  ChildProcessMonitor(const ChildProcessMonitor&) = delete;
  ChildProcessMonitor& operator=(const ChildProcessMonitor&) = delete;

  void Start(base::Process child,
             base::OnceCallback<void(base::ProcessId)> callback);

 private:
  void OnChildCrash(base::OnceCallback<void(base::ProcessId)> callback,
                    base::ProcessId pid);

  base::Process child_process_;
  std::unique_ptr<base::Thread> child_monitor_thread_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<ChildProcessMonitor> weak_ptr_factory_{this};
};

}  // namespace brave
#endif  // BRAVE_COMPONENTS_CHILD_PROCESS_MONITOR_CHILD_PROCESS_MONITOR_H_
