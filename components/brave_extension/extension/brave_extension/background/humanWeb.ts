// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

declare const CLIQZ: any // this is a global injected by humanweb
const HUMAN_WEB_PREF_KEY = 'brave.human_web_enabled'

const toggleHumanWeb = (enabled: boolean) => {
  const cliqzPrefs = CLIQZ.app.prefs
  if (enabled) {
    cliqzPrefs.set('modules.human-web.enabled', true)
    cliqzPrefs.set('modules.hpnv2.enabled', true)
  } else {
    cliqzPrefs.set('modules.human-web.enabled', false)
    cliqzPrefs.set('modules.hpnv2.enabled', false)
  }
}

chrome.settingsPrivate.onPrefsChanged.addListener((prefs) => {
  const pref = prefs.find(p => p.key === HUMAN_WEB_PREF_KEY)
  if (pref && pref.type === chrome.settingsPrivate.PrefType.BOOLEAN) {
    toggleHumanWeb(pref.value)
  }
})

chrome.settingsPrivate.getPref(HUMAN_WEB_PREF_KEY, (pref) => {
  if (pref && pref.type === chrome.settingsPrivate.PrefType.BOOLEAN) {
    toggleHumanWeb(pref.value)
  }
})
