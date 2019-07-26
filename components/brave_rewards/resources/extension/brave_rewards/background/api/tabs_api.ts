/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const getTabData = (tabId: number) =>
  chrome.tabs.get(tabId, (tab: chrome.tabs.Tab) => {
    onTabData(tab)
  })

export const onTabData = (tab: chrome.tabs.Tab, activeTabIsLoadingTriggered?: boolean) => {
  const rewardsPanelActions = require('../actions/rewardsPanelActions').default

  console.log('In onTabRetrieved...')
  if (tab.id && tab.active && tab.url) {
    console.log('Checking for Twitter regex...')
    const regex = /^https?:\/\/mobile.twitter\.com\/(#!\/)?([^\/]+)(\/\w+)*$/
    const match = regex.exec(tab.url)
    if (match && match.length > 2) {
      console.log('Matched Twitter regex...')
      const screenName = match[2]
      console.log('Making call to getProfileUrl...')

      const msg = { type: 'getProfileUrl', screenName }

      chrome.tabs.sendMessage(tab.id, msg, response => {
        console.log('sendMessage', response)
        const profileUrl = response.profileUrl

        if (profileUrl) {
          console.log(`Changing url to ${profileUrl}`)
          tab.url = profileUrl
        }

        rewardsPanelActions.onTabRetrieved(tab, activeTabIsLoadingTriggered)
      })
      return
    }
  }

  rewardsPanelActions.onTabRetrieved(tab, activeTabIsLoadingTriggered)
}
