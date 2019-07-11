/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/greaselion/browser/greaselion_service_impl.h"

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/sequenced_task_runner.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "brave/components/greaselion/browser/greaselion_download_service.h"
#include "brave/components/greaselion/browser/greaselion_extension_converter.h"
#include "extensions/common/extension.h"
#include "extensions/common/file_util.h"

namespace greaselion {

GreaselionServiceImpl::GreaselionServiceImpl(
    GreaselionDownloadService* download_service,
    const base::FilePath& install_directory,
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : download_service_(download_service),
      install_directory_(install_directory),
      all_rules_installed_successfully_(true),
      task_runner_(std::move(task_runner)),
      weak_factory_(this) {
  for (int i = FIRST_FEATURE; i != LAST_FEATURE; i++)
    state_[static_cast<GreaselionFeature>(i)] = false;
  UpdateInstalledExtensions();
}

GreaselionServiceImpl::~GreaselionServiceImpl() = default;

void GreaselionServiceImpl::UpdateInstalledExtensions() {
  std::vector<std::unique_ptr<GreaselionRule>>* rules =
      download_service_->rules();
  for (const auto& rule : *rules) {
    if (rule->Matches(state_)) {
      // Convert script file to component extension. This must run on extension
      // file task runner, which was passed in.
      base::PostTaskAndReplyWithResult(
          task_runner_.get(), FROM_HERE,
          base::BindOnce(
              &greaselion::ConvertGreaselionRuleToExtensionOnTaskRunner, rule,
              install_directory_),
          base::BindOnce(&GreaselionServiceImpl::Install,
                         weak_factory_.GetWeakPtr()));
    }
  }
}

void GreaselionServiceImpl::Install(
    scoped_refptr<extensions::Extension> extension) {
  if (!extension.get()) {
    all_rules_installed_successfully_ = false;
    LOG(ERROR) << "Could not load Greaselion script";
  } else {
    base::PostTaskAndReplyWithResult(
        task_runner_.get(), FROM_HERE,
        base::BindOnce(&extensions::file_util::InstallExtension,
                       extension->path(), extension->id(),
                       extension->VersionString(), install_directory_),
        base::BindOnce(&GreaselionServiceImpl::PostInstall,
                       weak_factory_.GetWeakPtr()));
  }
#if 0
  pending_scripts_ -= 1;
  if (!pending_scripts_)
    for (Observer& observer : observers_)
      observer.OnRuleReady(this, all_scripts_loaded_successfully_);
#endif
}

void GreaselionServiceImpl::PostInstall(const base::FilePath& extension_path) {
  // TODO this should call a new method that adds the extension to a
  // map of extension IDs that is keyed on extension_path so we can
  // uninstall all extensions in UpdateInstalledExtensions
}

#if 0
bool GreaselionServiceImpl::ScriptsFor(const GURL& primary_url,
                                       std::vector<std::string>* scripts) {
  bool any = false;
  std::vector<std::unique_ptr<GreaselionRule>>* rules =
    download_service_->rules();
  scripts->clear();
  for (const auto& rule : *rules) {
    if (rule->Matches(primary_url, state_)) {
      rule->Populate(scripts);
      any = true;
    }
  }
  return any;
}
#endif

void GreaselionServiceImpl::SetFeatureEnabled(GreaselionFeature feature,
                                              bool enabled) {
  DCHECK(feature >= 0 && feature < LAST_FEATURE);
  state_[feature] = enabled;
  UpdateInstalledExtensions();
}

}  // namespace greaselion
