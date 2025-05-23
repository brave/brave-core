// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/prefs.h"

#include <utility>

namespace containers {

// Container structure in prefs:
// {
//   "containers": [
//     {
//       "id": "1",
//       "name": "Container 1"
//     }
//   ]
// }

namespace {

template <typename Callback>
void ForEachValidContainer(const PrefService& prefs, Callback&& callback) {
  for (const auto& container : prefs.GetList(prefs::kContainersList)) {
    if (!container.is_dict()) {
      continue;
    }
    const base::Value::Dict& dict = container.GetDict();
    auto* id = dict.FindString("id");
    auto* name = dict.FindString("name");
    if (!id || !name) {
      continue;
    }
    if (!std::forward<Callback>(callback)(*id, *name)) {
      break;
    }
  }
}

}  // namespace

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterListPref(prefs::kContainersList);
}

std::vector<mojom::ContainerPtr> GetContainersList(const PrefService& prefs) {
  std::vector<mojom::ContainerPtr> containers;
  ForEachValidContainer(prefs,
                        [&](const std::string& id, const std::string& name) {
                          containers.push_back(mojom::Container::New(id, name));
                          return true;
                        });
  return containers;
}

void SetContainersList(PrefService& prefs,
                       const std::vector<mojom::ContainerPtr>& containers) {
  base::Value::List list;
  for (const auto& container : containers) {
    list.Append(base::Value::Dict()
                    .Set("id", container->id)
                    .Set("name", container->name));
  }
  prefs.SetList(prefs::kContainersList, std::move(list));
}

std::string GetContainerName(const PrefService& prefs,
                             const std::string& container_id) {
  std::string container_name;
  ForEachValidContainer(prefs,
                        [&](const std::string& id, const std::string& name) {
                          if (id == container_id) {
                            container_name = name;
                            return false;
                          }
                          return true;
                        });
  return container_name;
}

}  // namespace containers
