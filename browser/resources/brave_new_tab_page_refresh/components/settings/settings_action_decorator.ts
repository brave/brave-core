/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { NewTabActions } from '../../state/new_tab_state'
import { useNewTabActions } from '../../context/new_tab_context'

// Wraps the specified actions object such that any action call will be
// proceeded by a call to `reportCustomizeDialogEdited`.
export function useActionsWithMetrics<A extends Object>(actions: A): A {
  return decorateActionsWithMetrics(useNewTabActions(), actions)
}

export function decorateActionsWithMetrics<A extends Object>(
  newTabActions: NewTabActions,
  actions: A,
): A {
  return new Proxy(actions, {
    get(target, prop, receiver) {
      const value = Reflect.get(target, prop, receiver)
      if (typeof value === 'function') {
        return function (...args: any[]) {
          newTabActions.reportCustomizeDialogEdited()
          return value.apply(this, args)
        }
      }
      return value
    },
  })
}
