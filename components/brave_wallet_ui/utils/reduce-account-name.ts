// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
export const reduceAccountDisplayName = (name: string, maxLength: number) => {
  if (!name) {
    return ''
  } else {
    if (name.length > maxLength) {
      const sliced = name.slice(0, maxLength - 2)
      const reduced = sliced.concat('..')
      return reduced
    } else {
      return name
    }
  }
}
