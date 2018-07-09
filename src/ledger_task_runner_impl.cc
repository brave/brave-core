/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ledger_task_runner_impl.h"

namespace bat_ledger {

  LedgerTaskRunnerImpl::LedgerTaskRunnerImpl(Task task) : task_(task) {}
  LedgerTaskRunnerImpl::~LedgerTaskRunnerImpl() {}

  void LedgerTaskRunnerImpl::Run() {
    task_();
  }

}  // namespace ledger
