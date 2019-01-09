/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/base_local_data_files_observer.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/task_runner_util.h"
#include "base/task/post_task.h"
#include "base/threading/thread_restrictions.h"

namespace brave_shields {

BaseLocalDataFilesObserver::BaseLocalDataFilesObserver() { }

BaseLocalDataFilesObserver::~BaseLocalDataFilesObserver() { }

}  // namespace brave_shields
