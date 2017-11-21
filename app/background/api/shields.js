/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {bindActionCreators} from 'redux'
import store from '../store'
import * as shieldsPanelActions from '../../actions/shieldsPanelActions'
const actions = bindActionCreators(shieldsPanelActions, store.dispatch)

const getShieldSettingsForURL = (urlStr) => {
  if (urlStr == null) {
    return Promise.reject(new Error('No URL specified'))
  }
  const url = new window.URL(urlStr)
  const origin = url.origin
  const hostname = url.hostname
  return Promise.all([
    chrome.contentSettings.braveAdBlock.getAsync({primaryUrl: origin}),
    chrome.contentSettings.braveTrackingProtection.getAsync({primaryUrl: origin})
  ]).then((details) => {
    return {
      origin,
      hostname,
      adBlock: details[0].setting,
      trackingProtection: details[1].setting
    }
  })
}

const getActiveTabURL = () => {
  return chrome.tabs.queryAsync({'active': true, 'lastFocusedWindow': true})
    .then((tabs) => {
      return (tabs.length && tabs[0].url) || undefined
    })
}

export const updateShieldsSettings = () =>
  getActiveTabURL()
    .then(getShieldSettingsForURL)
    .then((details) => {
      actions.shieldsPanelDataUpdated(details)
    })

export const setAllowAdBlock = (origin, setting) => {
  chrome.contentSettings.braveAdBlock.setAsync({
    primaryPattern: origin + '/*',
    setting
  }).then(() => {
    updateShieldsSettings()
  })
}

export const setAllowTrackingProtection = (origin, setting) => {
  chrome.contentSettings.braveTrackingProtection.setAsync({
    primaryPattern: origin + '/*',
    setting
  }).then(() => {
    updateShieldsSettings()
  })
}
