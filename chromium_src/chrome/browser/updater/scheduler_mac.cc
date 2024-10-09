/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/callback.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "brave/browser/mac/keystone_glue.h"
#include "brave/browser/mac_features.h"
#include "chrome/browser/updater/browser_updater_client.h"
#include "chrome/browser/updater/browser_updater_client_util.h"
#include "chrome/common/chrome_features.h"
#include "chrome/updater/updater_scope.h"
#include "chrome/updater/util/mac_util.h"

#define DoPeriodicTasks DoPeriodicTasks_ChromiumImpl

#include "src/chrome/browser/updater/scheduler_mac.cc"

#undef DoPeriodicTasks

namespace updater {

namespace {

void CheckProcessExit(base::Process process, base::OnceClosure callback) {
  if (!process.IsValid() ||
      process.WaitForExitWithTimeout(base::TimeDelta(), nullptr)) {
    std::move(callback).Run();
  } else {
    base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&CheckProcessExit, std::move(process),
                       std::move(callback)),
        base::Minutes(1));
  }
}

}  // namespace

void DoPeriodicTasks(base::OnceClosure callback) {
  if (brave::ShouldUseOmaha4()) {
    DoPeriodicTasks_ChromiumImpl(std::move(callback));
  } else if (keystone_glue::KeystoneEnabled()) {
    // The registration framework doesn't provide a mechanism to ask Keystone to
    // just do its normal routine tasks, so instead launch the agent directly.
    // The agent can be in one of four places depending on the age and mode of
    // Keystone.
    for (UpdaterScope scope : {UpdaterScope::kSystem, UpdaterScope::kUser}) {
      std::optional<base::FilePath> keystone_path =
          GetKeystoneFolderPath(scope);
      if (!keystone_path) {
        continue;
      }
      for (const std::string& folder : {"Helpers", "Resources"}) {
        base::FilePath agent_path =
            keystone_path->Append("Contents")
                .Append(folder)
                .Append(
                    "GoogleSoftwareUpdateAgent.app/Contents/MacOS/"
                    "GoogleSoftwareUpdateAgent");
        if (base::PathExists(agent_path)) {
          CheckProcessExit(
              base::LaunchProcess(base::CommandLine(agent_path), {}),
              std::move(callback));
          return;
        }
      }
    }
    std::move(callback).Run();
  }
}

}  // namespace updater
