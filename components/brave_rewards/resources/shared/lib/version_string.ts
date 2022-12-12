/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

function splitVersion (version: string) {
  return (version || 'invalid').split(/\./g).map((part) => {
    const n = Number(part || 'invalid')
    if (isNaN(n) || n < 0) {
      throw new Error(`Invalid version string "${version}"`)
    }
    return n
  })
}

// Compares two version strings. Returns 0 if the version strings refer to the
// same version, a positive number if the first version is "later", and a
// negative number if the first version is "earlier". Adapted from
// `base::Version::CompareTo`
export function compareVersionStrings (version1: string, version2: string) {
  const components1 = splitVersion(version1)
  const components2 = splitVersion(version2)

  const count = Math.min(components1.length, components2.length)
  for (let i = 0; i < count; ++i) {
    if (components1[i] > components2[i]) {
      return 1
    }
    if (components1[i] < components2[i]) {
      return -1
    }
  }

  if (components1.length > components2.length) {
    for (let i = count; i < components1.length; ++i) {
      if (components1[i] > 0) {
        return 1
      }
    }
  } else if (components1.length < components2.length) {
    for (let i = count; i < components2.length; ++i) {
      if (components2[i] > 0) {
        return -1
      }
    }
  }

  return 0
}
