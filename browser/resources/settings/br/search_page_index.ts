// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  RegisterPolymerPrototypeModification
} from 'chrome://resources/brave/polymer_overriding.js'
import { routes } from '../route.js'
import type { Route } from '../router.js'

// Modify the prototype to handle route changes
RegisterPolymerPrototypeModification({
  'settings-search-page-index': (prototype) => {
    const oldCurrentRouteChanged = prototype.currentRouteChanged
    prototype.currentRouteChanged = function (newRoute: Route) {
      if (newRoute === routes.DEFAULT_SEARCH ||
          newRoute === routes.PRIVATE_SEARCH) {
        this.$.viewManager.switchView('parent', 'no-animation', 'no-animation')
      } else {
        oldCurrentRouteChanged.call(this, newRoute)
      }
    }
  }
})
