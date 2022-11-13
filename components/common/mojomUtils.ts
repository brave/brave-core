// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as mojo from 'gen/mojo/public/mojom/base/time.mojom.m.js'

/**
 * Converts a mojo time to a JS time.
 */
export function mojoTimeToJSDate (mojoTime: mojo.Time): Date {
  // The JS Date() is based off of the number of milliseconds since the
  // UNIX epoch (1970-01-01 00::00:00 UTC), while |internalValue| of the
  // base::Time (represented in mojom.Time) represents the number of
  // microseconds since the Windows FILETIME epoch (1601-01-01 00:00:00 UTC).
  // This computes the final JS time by computing the epoch delta and the
  // conversion from microseconds to milliseconds.
  const windowsEpoch = Date.UTC(1601, 0, 1, 0, 0, 0, 0)
  const unixEpoch = Date.UTC(1970, 0, 1, 0, 0, 0, 0)
  // |epochDeltaInMs| equals to base::Time::kTimeTToMicrosecondsOffset.
  const epochDeltaInMs = unixEpoch - windowsEpoch
  const timeInMs = Number(mojoTime.internalValue) / 1000

  return new Date(timeInMs - epochDeltaInMs)
}

/**
 * Converts a mojo time to a JS time.
 * @param {!mojoBase.mojom.TimeDelta} mojoTime
 * @return {!Date}
 */
export function mojoTimeDeltaToJSDate (mojoTime: mojo.TimeDelta) {
  return new Date(Number(mojoTime.microseconds) / 1000)
}
