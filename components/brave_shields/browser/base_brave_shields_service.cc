/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/base_brave_shields_service.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/brave_shields/browser/brave_component_installer.h"
#include "chrome/browser/browser_process.h"

namespace brave_shields {

BaseBraveShieldsService::BaseBraveShieldsService(
    const std::string& updater_name,
    const std::string& updater_id,
    const std::string& updater_base64_public_key)
    : initialized_(false),
      task_runner_(
          base::CreateSequencedTaskRunnerWithTraits({base::MayBlock()})),
      updater_name_(updater_name),
      updater_id_(updater_id),
      updater_base64_public_key_(updater_base64_public_key) {
  base::Closure registered_callback =
      base::Bind(&BaseBraveShieldsService::OnComponentRegistered,
                 base::Unretained(this), updater_id_);
  ReadyCallback ready_callback =
      base::Bind(&BaseBraveShieldsService::OnComponentReady,
                 base::Unretained(this), updater_id_);
  brave::RegisterComponent(g_browser_process->component_updater(),
      updater_name_, updater_base64_public_key_,
      registered_callback, ready_callback);
}

BaseBraveShieldsService::~BaseBraveShieldsService() {
}

bool BaseBraveShieldsService::IsInitialized() const {
  return initialized_;
}

void BaseBraveShieldsService::InitShields() {
  if (Init()) {
    std::lock_guard<std::mutex> guard(initialized_mutex_);
    initialized_ = true;
  }
}

bool BaseBraveShieldsService::Start() {
  if (initialized_) {
    return true;
  }

  InitShields();
  return false;
}

void BaseBraveShieldsService::Stop() {
  std::lock_guard<std::mutex> guard(initialized_mutex_);
  Cleanup();
  initialized_ = false;
}

bool BaseBraveShieldsService::ShouldStartRequest(const GURL& url,
    content::ResourceType resource_type,
    const std::string& tab_host) {
  return true;
}

}  // namespace brave_shields
