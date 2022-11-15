/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export default function performSideEffectLoader (storage: NewTab.State) {
  type SideEffectFunction = (currentState: NewTab.State) => void

  return function (fn: SideEffectFunction): void {
    window.setTimeout(() => fn(storage), 0)
  }
}
