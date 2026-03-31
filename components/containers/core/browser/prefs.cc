// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/prefs.h"

#include <string_view>
#include <utility>

#include "brave/components/containers/core/browser/pref_names.h"
#include "brave/components/containers/core/common/features.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace containers {

namespace {

mojom::ContainerPtr ContainerFromDict(const base::DictValue& dict) {
  auto* id = dict.FindString("id");
  auto* name = dict.FindString("name");
  auto icon = dict.FindInt("icon");
  auto background_color = dict.FindInt("background_color");
  if (!id || !name || !icon.has_value() || !background_color.has_value()) {
    LOG(ERROR) << "Container is missing id, name, icon, or background_color";
    return nullptr;
  }

  return mojom::Container::New(*id, *name, static_cast<mojom::Icon>(*icon),
                               *background_color);
}

base::DictValue ContainerToDict(const mojom::ContainerPtr& container) {
  return base::DictValue()
      .Set("id", container->id)
      .Set("name", container->name)
      .Set("icon", std::to_underlying(container->icon))
      .Set("background_color", static_cast<int>(container->background_color));
}

std::vector<mojom::ContainerPtr> GetContainersFromList(
    const base::ListValue& list) {
  std::vector<mojom::ContainerPtr> containers;
  for (const auto& container : list) {
    if (!container.is_dict()) {
      LOG(ERROR) << "Container is not a dictionary";
      continue;
    }

    if (auto parsed = ContainerFromDict(container.GetDict())) {
      containers.push_back(std::move(parsed));
    }
  }
  return containers;
}

}  // namespace

std::vector<mojom::ContainerPtr> GetContainersFromPrefs(
    const PrefService& prefs) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));
  return GetContainersFromList(prefs.GetList(prefs::kContainersList));
}

mojom::ContainerPtr GetContainerFromPrefs(const PrefService& prefs,
                                          std::string_view id) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));
  for (auto& container : GetContainersFromPrefs(prefs)) {
    if (container->id == id) {
      return std::move(container);
    }
  }
  return nullptr;
}

void SetContainersToPrefs(const std::vector<mojom::ContainerPtr>& containers,
                          PrefService& prefs) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));
  base::ListValue list;
  for (const auto& container : containers) {
    list.Append(ContainerToDict(container));
  }
  prefs.SetList(prefs::kContainersList, std::move(list));
}

std::vector<mojom::ContainerPtr> GetLocallyUsedContainersFromPrefs(
    const PrefService& prefs) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));
  std::vector<mojom::ContainerPtr> containers;
  for (const auto item : prefs.GetDict(prefs::kLocallyUsedContainers)) {
    if (!item.second.is_dict()) {
      LOG(ERROR) << "Used container snapshot is not a dictionary";
      continue;
    }

    if (auto parsed = ContainerFromDict(item.second.GetDict())) {
      containers.push_back(std::move(parsed));
    }
  }
  return containers;
}

mojom::ContainerPtr GetLocallyUsedContainerFromPrefs(const PrefService& prefs,
                                                     std::string_view id) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));
  if (const auto* value =
          prefs.GetDict(prefs::kLocallyUsedContainers).FindDict(id)) {
    return ContainerFromDict(*value);
  }
  return nullptr;
}

void SetLocallyUsedContainerToPrefs(const mojom::ContainerPtr& container,
                                    PrefService& prefs) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));
  CHECK(container);
  ScopedDictPrefUpdate update(&prefs, prefs::kLocallyUsedContainers);
  update->Set(container->id, ContainerToDict(container));
}

bool HasLocallyUsedContainerInPrefs(const PrefService& prefs,
                                    std::string_view id) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));
  return prefs.GetDict(prefs::kLocallyUsedContainers).contains(id);
}

void RemoveLocallyUsedContainerFromPrefs(std::string_view id,
                                         PrefService& prefs) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));
  if (!prefs.GetDict(prefs::kLocallyUsedContainers).contains(id)) {
    return;
  }

  ScopedDictPrefUpdate update(&prefs, prefs::kLocallyUsedContainers);
  update->Remove(id);
}

}  // namespace containers
