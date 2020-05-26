// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

const getRandomBytes = (bytes: number) => {
  const QUOTA = 65536
  const intArr = new Uint8Array(bytes)

  for (let i = 0; i < bytes; i += QUOTA) {
    crypto.getRandomValues(intArr.subarray(i, i + Math.min(bytes - i, QUOTA)))
  }

  return intArr
}

const getRandomBase64 = (bytes: number) => {
  return btoa(String.fromCharCode.apply(null, getRandomBytes(bytes)))
    .replace(/\+/g, '-')
    .replace(/\//g, '_')
    .replace(/\=/g, '')
}

export default getRandomBase64
