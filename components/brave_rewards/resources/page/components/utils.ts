/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export function convertBalance (tokens: number, rate: number): string {
  if (tokens === 0) {
    return '0.00'
  }

  const converted = tokens * rate

  if (isNaN(converted)) {
    return '0.00'
  }

  return converted.toFixed(2)
}
