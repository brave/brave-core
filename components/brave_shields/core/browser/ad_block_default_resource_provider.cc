// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_default_resource_provider.h"

#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "brave/components/brave_component_updater/browser/component_contents_reader.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_installer.h"

namespace {

constexpr char kAdBlockResourcesFilename[] = "resources.json";

}  // namespace

namespace brave_shields {

AdBlockDefaultResourceProvider::AdBlockDefaultResourceProvider(
    component_updater::ComponentUpdateService* cus) {
  // Can be nullptr in unit tests
  if (!cus) {
    return;
  }

  RegisterAdBlockDefaultResourceComponent(
      cus,
      base::BindRepeating(&AdBlockDefaultResourceProvider::OnComponentReady,
                          weak_factory_.GetWeakPtr()));
}

AdBlockDefaultResourceProvider::~AdBlockDefaultResourceProvider() = default;

base::FilePath AdBlockDefaultResourceProvider::GetResourcesPath() {
  if (!component_reader_) {
    return base::FilePath();
  }

  return component_reader_->GetComponentRootDeprecated().AppendASCII(
      kAdBlockResourcesFilename);
}

void AdBlockDefaultResourceProvider::OnComponentReady(
    std::unique_ptr<component_updater::ComponentContentsReader> reader) {
  component_reader_ = std::move(reader);

  // Load the resources (as a string)
  LoadResources(
      base::BindOnce(&AdBlockDefaultResourceProvider::NotifyResourcesLoaded,
                     weak_factory_.GetWeakPtr()));
}

void AdBlockDefaultResourceProvider::LoadResources(
    base::OnceCallback<void(const std::string& resources_json)> cb) {
  if (!component_reader_) {
    // If the component is not ready yet, run the callback with empty resources
    // to avoid blocking filter data loads.
    std::move(cb).Run("[]");
    return;
  }

  auto handle_file_content = base::BindOnce(
      [](base::OnceCallback<void(const std::string& resources_json)> cb,
         std::optional<std::string> content) {
        std::move(cb).Run(std::move(content).value_or("[]"));
      },
      std::move(cb));

  component_reader_->GetFileAsString(
      base::FilePath::FromASCII(kAdBlockResourcesFilename),
      std::move(handle_file_content));
}

}  // namespace brave_shields
