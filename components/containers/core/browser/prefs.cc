// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/prefs.h"

#include <utility>

#include "base/types/cxx23_to_underlying.h"
#include "brave/components/containers/core/browser/pref_names.h"
#include "brave/components/containers/core/common/features.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "third_party/skia/include/core/SkColor.h"

namespace containers {

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(
      prefs::kContainersDict, user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
}

std::vector<mojom::ContainerPtr> GetContainersFromPrefs(
    const PrefService& prefs) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));
  std::vector<mojom::ContainerPtr> containers;
  for (const auto container_it : prefs.GetDict(prefs::kContainersDict)) {
    if (!container_it.second.is_dict()) {
      LOG(ERROR) << "Container is not a dictionary";
      continue;
    }

    const auto& id = container_it.first;
    const base::Value::Dict& dict = container_it.second.GetDict();
    auto* name = dict.FindString("name");
    auto icon = dict.FindInt("icon");
    auto background_color = dict.FindInt("background_color");
    if (!name || !icon.has_value() || !background_color.has_value()) {
      LOG(ERROR) << "Container is missing id, name, icon, or background_color";
      continue;
    }

    containers.push_back(mojom::Container::New(
        id, *name, static_cast<mojom::Icon>(*icon), *background_color));
  }

  std::ranges::sort(containers, std::less<>{}, &mojom::Container::name);
  return containers;
}

void SetContainersToPrefs(const std::vector<mojom::ContainerPtr>& containers,
                          PrefService& prefs) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));
  base::Value::Dict dict;
  for (const auto& container : containers) {
    dict.Set(container->id,
             base::Value::Dict()
                 .Set("name", container->name)
                 .Set("icon", base::to_underlying(container->icon))
                 .Set("background_color",
                      static_cast<int>(container->background_color)));
  }
  prefs.SetDict(prefs::kContainersDict, std::move(dict));
}

}  // namespace containers
