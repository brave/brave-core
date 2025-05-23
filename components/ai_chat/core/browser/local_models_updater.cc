/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/local_models_updater.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "components/component_updater/component_updater_paths.h"

namespace base {
class Version;
}  // namespace base

namespace ai_chat {

namespace {
constexpr base::FilePath::CharType kComponentInstallDir[] =
    FILE_PATH_LITERAL("AIChatLocalModels");
constexpr base::FilePath::CharType kDeprecatedComponentInstallDir[] =
    FILE_PATH_LITERAL("LeoLocalModels");

base::FilePath GetComponentDir() {
  base::FilePath components_dir =
      base::PathService::CheckedGet(component_updater::DIR_COMPONENT_USER);

  return components_dir.Append(kComponentInstallDir);
}

base::FilePath GetDeprecatedComponentDir() {
  base::FilePath components_dir =
      base::PathService::CheckedGet(component_updater::DIR_COMPONENT_USER);

  return components_dir.Append(kDeprecatedComponentInstallDir);
}

void DeleteComponent() {
  if (base::PathExists(GetDeprecatedComponentDir())) {
    base::DeletePathRecursively(GetDeprecatedComponentDir());
  }
  if (base::PathExists(GetComponentDir())) {
    base::DeletePathRecursively(GetComponentDir());
  }
}

}  // namespace

void ManageLocalModelsComponentRegistration(
    component_updater::ComponentUpdateService* cus) {
  // Delete the component - its not required anymore.
  DeleteComponent();
}

}  // namespace ai_chat
