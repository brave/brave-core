/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export type CustomEventName = 'ntp-open-news-feed-settings'

export function dispatchCustomEvent(name: CustomEventName) {
  window.dispatchEvent(new CustomEvent(name))
}

export function addCustomEventListener(
  name: CustomEventName,
  listener: () => void,
) {
  window.addEventListener(name, listener)
  return () => window.removeEventListener(name, listener)
}
