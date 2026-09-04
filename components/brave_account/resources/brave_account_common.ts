/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  AsyncDirective,
  directive,
  nothing,
} from '//resources/lit/v3_0/lit.rollup.js'

// Custom directive that freezes the previously rendered value when `freeze`
// is true. Similar to Lit's `noChange` (not exported by Chromium's Lit wrapper
// from //third_party/lit/v3_0/lit.ts), but instead of preventing the update,
// it reuses the last rendered value.
class FreezeWhenDirective extends AsyncDirective {
  private previousValue: unknown = nothing

  render(freeze: boolean, value: unknown): unknown {
    return freeze ? this.previousValue : (this.previousValue = value)
  }
}

export const freezeWhen = directive(FreezeWhenDirective)

// Focuses a <leo-input>'s underlying native <input>.
// On iOS WebKit, focusing via the shadow root's delegatesFocus leaves the
// caret painted behind the placeholder (no selection is established).
// Setting an explicit (empty) selection range forces WebKit to lay the caret
// out correctly.
export function focusLeoInput(leoInput: Element | null | undefined) {
  const input = leoInput?.shadowRoot?.querySelector('input')
  if (!input) {
    // Fall back to host focus if the inner input isn't available yet.
    ;(leoInput as HTMLElement | null | undefined)?.focus()
    return
  }
  input.focus()
  input.setSelectionRange(input.value.length, input.value.length)
}
