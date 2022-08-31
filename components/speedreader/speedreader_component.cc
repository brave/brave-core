/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_component.h"

#include <utility>

#include "base/command_line.h"
#include "base/files/file_path_watcher.h"
#include "base/task/bind_post_task.h"
#include "base/task/task_runner_util.h"
#include "base/task/thread_pool.h"
#include "base/threading/sequenced_task_runner_handle.h"

namespace speedreader {

namespace {

constexpr base::FilePath::CharType kDatFileVersion[] = FILE_PATH_LITERAL("1");

constexpr base::FilePath::CharType kStylesheetFileName[] =
    FILE_PATH_LITERAL("content-stylesheet.css");

constexpr const char kSpeedreaderStylesheet[] = "speedreader-stylesheet";

constexpr char kComponentName[] = "Brave SpeedReader Updater";
constexpr char kComponentId[] = "jicbkmdloagakknpihibphagfckhjdih";
constexpr char kComponentPublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA3j/+grwCsrYVA99oDHa+E9z5edPIV"
    "3J+lzld3X7K8wfJXbSauGf2DSxW0UEh+MqkkcIK/66Kkd4veuWqnUCAGXUzrHVy/N6kksDkrS"
    "cOlpKT9zfyIvLc/4nmiyPCSc5c7UrDVUwZnIUBBpEHiwkpiM4pujeJkZSl5783RWIDRN92GDB"
    "dHMdD97JH3bPp3SCTmfAAHzzYUAHUSrOAfodD8qWkfWT19VigseIqwK6dH30uFgaZIOwU9uJV"
    "2Ts/TDEddNv8eV7XbwQdL1HUEoFj+RXDq1CuQJjvQdc7YRmy0WGV0GIXu0lAFOQ6D/Z/rjtOe"
    "//2uc4zIkviMcUlrvHaJwIDAQAB";

base::FilePathWatcher* CreateAndStartFilePathWatcher(
    const base::FilePath& watch_path,
    const base::FilePathWatcher::Callback& callback) {
  auto watcher = std::make_unique<base::FilePathWatcher>();
  if (!watcher->Watch(watch_path, base::FilePathWatcher::Type::kNonRecursive,
                      callback)) {
    return nullptr;
  }

  return watcher.release();
}

}  // namespace

SpeedreaderComponent::SpeedreaderComponent(Delegate* delegate)
    : brave_component_updater::BraveComponent(delegate) {
  // Register component
  Register(kComponentName, kComponentId, kComponentPublicKey);
}

SpeedreaderComponent::~SpeedreaderComponent() {
  if (file_watcher_) {
    watch_task_runner_->DeleteSoon(FROM_HERE, file_watcher_);
  }
}

void SpeedreaderComponent::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void SpeedreaderComponent::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void SpeedreaderComponent::OnComponentReady(const std::string& component_id,
                                            const base::FilePath& install_dir,
                                            const std::string& manifest) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          kSpeedreaderStylesheet)) {
    stylesheet_path_ =
        base::CommandLine::ForCurrentProcess()->GetSwitchValuePath(
            kSpeedreaderStylesheet);
    watch_task_runner_ =
        base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()});

    base::PostTaskAndReplyWithResult(
        watch_task_runner_.get(), FROM_HERE,
        base::BindOnce(
            &CreateAndStartFilePathWatcher, stylesheet_path_,
            base::BindPostTask(
                base::SequencedTaskRunnerHandle::Get(),
                base::BindRepeating(&SpeedreaderComponent::OnFileChanged,
                                    weak_factory_.GetWeakPtr()))),
        base::BindOnce(&SpeedreaderComponent::OnWatcherStarted,
                       weak_factory_.GetWeakPtr()));
  } else {
    stylesheet_path_ =
        install_dir.Append(kDatFileVersion).Append(kStylesheetFileName);
  }

  for (Observer& observer : observers_) {
    observer.OnStylesheetReady(stylesheet_path_);
  }
}

void SpeedreaderComponent::OnFileChanged(const base::FilePath& path,
                                         bool error) {
  DCHECK_EQ(path, stylesheet_path_);
  for (Observer& observer : observers_) {
    observer.OnStylesheetReady(stylesheet_path_);
  }
}

void SpeedreaderComponent::OnWatcherStarted(
    base::FilePathWatcher* file_watcher) {
  file_watcher_ = file_watcher;
}

}  // namespace speedreader
