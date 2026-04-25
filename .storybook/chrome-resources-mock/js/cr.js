// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 *
 * @param {string} eventName
 * @param {Function} handler
 */
export function addWebUiListener(eventName, handler) {
  console.log('addWebUIListener', eventName, handler)
}

/**
 *
 * @param {string} message
 * @param  {...any} args
 */
export function sendWithPromise(message, ...args) {
  return new Promise(() => console.log('sendWithPromise', message, args))
}
