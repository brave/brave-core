// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { LOCAL_STORAGE_KEYS } from '../common/constants/local-storage-keys'

export const parseJSONFromLocalStorage = <T = any> (
  storageString: keyof typeof LOCAL_STORAGE_KEYS,
  fallback: T
): T => {
  try {
    return JSON.parse(
      window.localStorage.getItem(LOCAL_STORAGE_KEYS[storageString]) || ''
    ) as T
  } catch (e) {
    return fallback
  }
}
