// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
export function getPreference (key: string) {
  return new Promise<chrome.settingsPrivate.PrefObject>(function (resolve) {
    chrome.settingsPrivate.getPref(key, pref => resolve(pref))
  })
}

export function setPreference (key: string, value: any) {
  return new Promise<boolean>(function (resolve) {
    chrome.settingsPrivate.setPref(key, value, null, resolve)
  })
}
