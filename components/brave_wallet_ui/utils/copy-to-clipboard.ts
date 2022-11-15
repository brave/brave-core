// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
export const copyToClipboard = async (data: string) => {
  try {
    await navigator.clipboard?.writeText(data)
  } catch (e) {
    console.log(`Could not copy address ${e.toString()}`)
  }
}

export const clearClipboard = () => {
  return copyToClipboard('')
}
