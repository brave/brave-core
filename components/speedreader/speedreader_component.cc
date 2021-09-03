/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_component.h"

#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "brave/components/speedreader/speedreader_switches.h"
#include "url/gurl.h"

namespace speedreader {

namespace {

constexpr base::FilePath::CharType kDatFileVersion[] = FILE_PATH_LITERAL("1");

constexpr base::FilePath::CharType kDatFileName[] =
    FILE_PATH_LITERAL("speedreader-updater.dat");

constexpr base::FilePath::CharType kStylesheetFileName[] =
    FILE_PATH_LITERAL("content-stylesheet.css");

constexpr char kComponentName[] = "Brave SpeedReader Updater";
constexpr char kComponentId[] = "jicbkmdloagakknpihibphagfckhjdih";
constexpr char kComponentPublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA3j/+grwCsrYVA99oDHa+E9z5edPIV"
    "3J+lzld3X7K8wfJXbSauGf2DSxW0UEh+MqkkcIK/66Kkd4veuWqnUCAGXUzrHVy/N6kksDkrS"
    "cOlpKT9zfyIvLc/4nmiyPCSc5c7UrDVUwZnIUBBpEHiwkpiM4pujeJkZSl5783RWIDRN92GDB"
    "dHMdD97JH3bPp3SCTmfAAHzzYUAHUSrOAfodD8qWkfWT19VigseIqwK6dH30uFgaZIOwU9uJV"
    "2Ts/TDEddNv8eV7XbwQdL1HUEoFj+RXDq1CuQJjvQdc7YRmy0WGV0GIXu0lAFOQ6D/Z/rjtOe"
    "//2uc4zIkviMcUlrvHaJwIDAQAB";

}  // namespace

SpeedreaderComponent::SpeedreaderComponent(Delegate* delegate)
    : brave_component_updater::BraveComponent(delegate) {
  const auto* cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch(speedreader::kSpeedreaderWhitelistPath)) {
    user_provided_whitelist_ = true;
    const base::FilePath whitelist_path(
        cmd_line->GetSwitchValuePath(speedreader::kSpeedreaderWhitelistPath));
    VLOG(2) << "Speedreader whitelist from " << whitelist_path;

    // Notify the `OnWhitelistFileReady` method synchronously.
    OnWhitelistFileReady(whitelist_path, false /* error */);

    // Watch the provided file for changes.
    whitelist_path_watcher_ = std::make_unique<base::FilePathWatcher>();
    if (!whitelist_path_watcher_->Watch(
            whitelist_path, base::FilePathWatcher::Type::kNonRecursive,
            base::BindRepeating(&SpeedreaderComponent::OnWhitelistFileReady,
                                weak_factory_.GetWeakPtr()))) {
      LOG(ERROR) << "Speedreader could not watch filesystem for changes"
                 << " at path " << whitelist_path.LossyDisplayName();
    }
  }

  if (cmd_line->HasSwitch(speedreader::kSpeedreaderStylesheet)) {
    user_provided_stylesheet_ = true;
    const base::FilePath stylesheet_path(
        cmd_line->GetSwitchValuePath(speedreader::kSpeedreaderStylesheet));
    VLOG(2) << "Speedreader stylesheet from " << stylesheet_path;

    // Notify the `OnStylesheetReady` method synchronously.
    OnStylesheetFileReady(stylesheet_path, false /* error */);

    stylesheet_path_watcher_ = std::make_unique<base::FilePathWatcher>();
    if (!stylesheet_path_watcher_->Watch(
            stylesheet_path, base::FilePathWatcher::Type::kNonRecursive,
            base::BindRepeating(&SpeedreaderComponent::OnStylesheetFileReady,
                                weak_factory_.GetWeakPtr()))) {
      LOG(ERROR) << "Speedreader could not watch filesystem for changes"
                 << " at path " << stylesheet_path.LossyDisplayName();
    }
  }

  // Register component
  Register(kComponentName, kComponentId, kComponentPublicKey);
}

SpeedreaderComponent::~SpeedreaderComponent() = default;

void SpeedreaderComponent::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void SpeedreaderComponent::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void SpeedreaderComponent::OnStylesheetFileReady(const base::FilePath& path,
                                                 bool error) {
  if (error) {
    LOG(ERROR) << __func__
               << "Speedreader got an error watching for file changes."
               << " Stopping watching.";
    stylesheet_path_watcher_.reset();
    return;
  }

  stylesheet_path_ = path;
  for (Observer& observer : observers_)
    observer.OnStylesheetReady(stylesheet_path_);
}

void SpeedreaderComponent::OnWhitelistFileReady(const base::FilePath& path,
                                                bool error) {
  if (error) {
    LOG(ERROR) << __func__
               << "Speedreader got an error watching for file changes."
               << " Stopping watching.";
    whitelist_path_watcher_.reset();
    return;
  }

  whitelist_path_ = path;
  for (Observer& observer : observers_)
    observer.OnWhitelistReady(whitelist_path_);
}

void SpeedreaderComponent::OnComponentReady(const std::string& component_id,
                                            const base::FilePath& install_dir,
                                            const std::string& manifest) {
  stylesheet_path_ =
      install_dir.Append(kDatFileVersion).Append(kStylesheetFileName);
  whitelist_path_ = install_dir.Append(kDatFileVersion).Append(kDatFileName);

  for (Observer& observer : observers_) {
    if (!user_provided_whitelist_)
      observer.OnWhitelistReady(whitelist_path_);
    if (!user_provided_stylesheet_)
      observer.OnStylesheetReady(stylesheet_path_);
  }
}

}  // namespace speedreader
