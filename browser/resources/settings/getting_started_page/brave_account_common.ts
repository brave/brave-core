/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export function OnEyeIconClicked(event: Event) {
  event.preventDefault()
  const target = event.target as Element
  const isShowing = target.getAttribute('name') === 'eye-on'
  target.setAttribute('name', isShowing ? 'eye-off' : 'eye-on')
  target.parentElement!.setAttribute('type', isShowing ? 'password' : 'text')
}

const emailRegexp = /^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,4}$/
export { emailRegexp }
