/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { LoginError, RegisterError } from './brave_account.mojom-webui.js'

export type Error =
  | { flow: 'login'; details: LoginError }
  | { flow: 'register'; details: RegisterError }

export function makeFocusHandler(setFocused: (focused: boolean) => void) {
  return (detail: { innerEvent: Event }) => {
    setFocused(detail.innerEvent.type === 'focus')
  }
}

export function onToggleVisibility(e: CustomEvent) {
  ;(e.currentTarget as Element).setAttribute(
    'type',
    e.detail.show ? 'text' : 'password',
  )
}
