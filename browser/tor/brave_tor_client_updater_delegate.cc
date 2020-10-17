/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/brave_tor_client_updater_delegate.h"

#include "base/files/file_util.h"
#include "base/task/post_task.h"
#include "base/task_runner.h"
#include "brave/browser/tor/tor_profile_service_factory.h"

namespace tor {

namespace {
void DeleteDir(const base::FilePath& path) {
  base::DeletePathRecursively(path);
}
}  // namespace

BraveTorClientUpdaterDelegate::BraveTorClientUpdaterDelegate(
    const base::FilePath& user_data_dir)
    : user_data_dir_(user_data_dir),
      task_runner_(base::CreateSequencedTaskRunner(
          {base::ThreadPool(), base::MayBlock()})) {}

BraveTorClientUpdaterDelegate::~BraveTorClientUpdaterDelegate() = default;

void BraveTorClientUpdaterDelegate::Cleanup(const char* component_id) {
  DCHECK(!user_data_dir_.empty());
  base::FilePath tor_component_dir = user_data_dir_.AppendASCII(component_id);
  task_runner_->PostTask(FROM_HERE,
                            base::BindOnce(&DeleteDir, tor_component_dir));
}

bool BraveTorClientUpdaterDelegate::IsTorDisabled() {
  return TorProfileServiceFactory::IsTorDisabled();
}

}  // namespace tor
