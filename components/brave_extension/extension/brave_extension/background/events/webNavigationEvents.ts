/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Actions
import actions from '../actions/webNavigationActions'
import settingsActions from '../actions/settingsActions'

chrome.webNavigation.onBeforeNavigate.addListener(function ({ tabId, url, frameId }: chrome.webNavigation.WebNavigationParentedCallbackDetails) {
  const isMainFrame: boolean = frameId === 0
  actions.onBeforeNavigate(tabId, url, isMainFrame)
})

let shouldRequestSettingsData = true
chrome.webNavigation.onCommitted.addListener(function ({ tabId, url, frameId }: chrome.webNavigation.WebNavigationTransitionCallbackDetails) {
  const isMainFrame: boolean = frameId === 0
  actions.onCommitted(tabId, url, isMainFrame)
  if (shouldRequestSettingsData) {
    // check whether or not the settings store should update based on settings changes.
    // this action is needed in the onCommitted phase for edge cases such as when after Brave is re-launched
    settingsActions.fetchAndDispatchSettings()
    // this request only needs to perform once
    shouldRequestSettingsData = false
  }
})

chrome.webNavigation.onErrorOccurred.addListener(function ({ error, tabId, frameId }) {
  const isMainFrame: boolean = frameId === 0
  actions.onErrorOccurred(error, tabId, isMainFrame)
})
