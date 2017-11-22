/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {bindActionCreators} from 'redux'
import store from '../store'
import * as shieldsPanelActions from '../../actions/shieldsPanelActions'
const actions = bindActionCreators(shieldsPanelActions, store.dispatch)

chrome.braveShields.onBlocked.addListener(function(detail) {
  actions.resourceBlocked(detail)
})

const getShieldSettingsForTabData = (tabData) => {
  if (tabData == null) {
    return Promise.reject(new Error('No tab specified'))
  }
  const url = new window.URL(tabData.url)
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
      trackingProtection: details[1].setting,
      tabId: tabData.id
    }
  })
}

const getActiveTabData = () => {
  return chrome.tabs.queryAsync({'active': true, 'lastFocusedWindow': true})
    .then((tabs) => {
      return (tabs.length && tabs[0]) || undefined
    })
}

export const updateShieldsSettings = () =>
  getActiveTabData()
    .then(getShieldSettingsForTabData)
    .then((details) => {
      actions.shieldsPanelDataUpdated(details)
    })
    .catch((e) => {
      console.error('updateShieldsSettings:', e)
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
