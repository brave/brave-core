/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export function formatUnixEpochToLocalTime(epoch: number) {
  const date = new Date(epoch * 1000) // Convert seconds to milliseconds.
  return date.toLocaleString()
}

export function uniqueTableRows<T>(data: T[]): T[] {
  return Array.from(new Set(data.map((item) => JSON.stringify(item))))
    .map((item) => JSON.parse(item))
}
