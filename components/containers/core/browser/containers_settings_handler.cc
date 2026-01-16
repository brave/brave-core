// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/containers_settings_handler.h"

#include <algorithm>
#include <utility>

#include "base/uuid.h"
#include "brave/components/containers/core/browser/pref_names.h"
#include "brave/components/containers/core/browser/prefs.h"
#include "third_party/re2/src/re2/re2.h"

namespace containers {

namespace {

// Returns true if the container name is valid.
bool IsContainerNameValid(std::string_view name) {
  // A string that is not empty and does not contain only whitespace.
  return re2::RE2::FullMatch(name, re2::RE2("^.*\\S.*$"));
}

bool IsIconValid(mojom::Icon icon) {
  // Valid icons are in the range of defined mojom::Icon values.
  return icon >= mojom::Icon::kMinValue && icon <= mojom::Icon::kMaxValue;
}

bool IsBackgroundColorValid(SkColor color) {
  return SkColorGetA(color) == SK_AlphaOPAQUE;
}

}  // namespace

ContainersSettingsHandler::ContainersSettingsHandler(PrefService* prefs)
    : prefs_(prefs) {
  DCHECK(prefs_);
  pref_change_registrar_.Init(prefs);
  // Watch for external changes to containers list (e.g. sync, other windows)
  pref_change_registrar_.Add(
      prefs::kContainersDict,
      base::BindRepeating(&ContainersSettingsHandler::OnContainersChanged,
                          base::Unretained(this)));
}

ContainersSettingsHandler::~ContainersSettingsHandler() {}

void ContainersSettingsHandler::BindUI(
    mojo::PendingRemote<mojom::ContainersSettingsUI> ui) {
  DCHECK(!ui_);
  ui_.Bind(std::move(ui));
}

void ContainersSettingsHandler::GetContainers(GetContainersCallback callback) {
  std::move(callback).Run(GetContainersFromPrefs(*prefs_));
}

void ContainersSettingsHandler::AddContainer(mojom::ContainerPtr container,
                                             AddContainerCallback callback) {
  if (!container->id.empty()) {
    std::move(callback).Run(mojom::ContainerOperationError::kIdShouldBeEmpty);
    return;
  }

  if (auto error = ValidateEditableContainerProperties(container)) {
    std::move(callback).Run(*error);
    return;
  }

  auto containers = GetContainersFromPrefs(*prefs_);
  container->id = base::Uuid::GenerateRandomV4().AsLowercaseString();
  containers.push_back(std::move(container));
  SetContainersToPrefs(std::move(containers), *prefs_);
  std::move(callback).Run(std::nullopt);
}

void ContainersSettingsHandler::UpdateContainer(
    mojom::ContainerPtr container,
    UpdateContainerCallback callback) {
  if (container->id.empty()) {
    std::move(callback).Run(mojom::ContainerOperationError::kIdShouldBeSet);
    return;
  }

  if (auto error = ValidateEditableContainerProperties(container)) {
    std::move(callback).Run(*error);
    return;
  }

  auto containers = GetContainersFromPrefs(*prefs_);
  auto it = std::ranges::find(containers, container->id, &mojom::Container::id);
  if (it == containers.end()) {
    std::move(callback).Run(mojom::ContainerOperationError::kNotFound);
    return;
  }
  *it = std::move(container);
  SetContainersToPrefs(std::move(containers), *prefs_);
  std::move(callback).Run(std::nullopt);
}

void ContainersSettingsHandler::RemoveContainer(
    const std::string& id,
    RemoveContainerCallback callback) {
  if (id.empty()) {
    std::move(callback).Run(mojom::ContainerOperationError::kIdShouldBeSet);
    return;
  }

  auto containers = GetContainersFromPrefs(*prefs_);
  auto it = std::ranges::find(containers, id, &mojom::Container::id);
  if (it == containers.end()) {
    std::move(callback).Run(mojom::ContainerOperationError::kNotFound);
    return;
  }

  containers.erase(it);
  SetContainersToPrefs(std::move(containers), *prefs_);

  std::move(callback).Run(std::nullopt);
}

// static
std::optional<mojom::ContainerOperationError>
ContainersSettingsHandler::ValidateEditableContainerProperties(
    const mojom::ContainerPtr& container) {
  if (!IsContainerNameValid(container->name)) {
    return mojom::ContainerOperationError::kInvalidName;
  }

  if (!IsIconValid(container->icon)) {
    return mojom::ContainerOperationError::kInvalidIcon;
  }

  if (!IsBackgroundColorValid(container->background_color)) {
    return mojom::ContainerOperationError::kInvalidBackgroundColor;
  }

  return std::nullopt;
}

void ContainersSettingsHandler::OnContainersChanged() {
  // Notify UI about container list changes (from this window or others).
  if (ui_) {
    ui_->OnContainersChanged(GetContainersFromPrefs(*prefs_));
  }
}

}  // namespace containers
