/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export function onEyeIconClicked(event: Event) {
  event.preventDefault()
  let type = 'password'
  const target = event.target as Element
  target.setAttribute('name',
    target.getAttribute('name') === 'eye-off'
      ? (type = 'text', 'eye-on')
      : 'eye-off')
  target.parentElement!.setAttribute('type', type)
}

export function isEmailValid(input: string) {
  if (!input) {
    // input is falsy
    return false
  }

  if (!/^[\x20-\x7F]+$/.test(input)) {
    // input contains characters outside printable ASCII
    return false
  }

  const index = input.lastIndexOf('@')
  if (index === -1) {
    // there's no @ sign in input
    return false
  }

  if (index === 0 || index === input.length - 1) {
    // there are no characters either before or after the last @ sign
    return false
  }

  return true
}
