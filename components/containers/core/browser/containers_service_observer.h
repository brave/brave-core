// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_CONTAINERS_SERVICE_OBSERVER_H_
#define BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_CONTAINERS_SERVICE_OBSERVER_H_

#include "base/observer_list_types.h"

namespace containers {

// Observes `ContainersService` for changes to the user-editable containers list
// (backed by prefs).
class ContainersServiceObserver : public base::CheckedObserver {
 public:
  // Called when the synced containers list pref changes.
  virtual void OnContainersListChanged() {}
};

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_CONTAINERS_SERVICE_OBSERVER_H_
