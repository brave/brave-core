/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_LEDGER_IO_TASK_RUNNER_IMPL_
#define BAT_LEDGER_LEDGER_IO_TASK_RUNNER_IMPL_

#include <string>
#include <functional>

#include "bat/ledger/ledger_task_runner.h"

namespace bat_ledger {

class LedgerTaskRunnerImpl : public ledger::LedgerTaskRunner {
 public:
  using Task = std::function<void (void)>;

  LedgerTaskRunnerImpl(Task task);
  ~LedgerTaskRunnerImpl() override;

  void Run() override;

 private:
  Task task_;
};

}  // namespace ledger

#endif  // BAT_LEDGER_LEDGER_IO_TASK_RUNNER_IMPL_
