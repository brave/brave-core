/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import shieldsPanelActions from '../actions/shieldsPanelActions'

const informTabOfCosmeticRulesToConsider = (tabId: number, selectors: string[]) => {
  if (selectors.length !== 0) {
    const message = {
      type: 'cosmeticFilterConsiderNewSelectors',
      selectors
    }
    const options = {
      frameId: 0
    }
    chrome.tabs.sendMessage(tabId, message, options)
  }
}

// Fires when content-script calls hiddenClassIdSelectors
export const injectClassIdStylesheet = (tabId: number, classes: string[], ids: string[], exceptions: string[], hide1pContent: boolean) => {
  chrome.braveShields.hiddenClassIdSelectors(classes, ids, exceptions, (selectors, forceHideSelectors) => {
    if (hide1pContent) {
      forceHideSelectors.push(...selectors)
    } else {
      informTabOfCosmeticRulesToConsider(tabId, selectors)
    }

    if (forceHideSelectors.length > 0) {
      const forceHideStylesheet = forceHideSelectors.join(',') + '{display:none!important;}\n'

      chrome.tabs.insertCSS(tabId, {
        code: forceHideStylesheet,
        cssOrigin: 'user',
        runAt: 'document_start'
      })
    }
  })
}

// Fires on content-script loaded
export const applyAdblockCosmeticFilters = (tabId: number, frameId: number, url: string, hide1pContent: boolean) => {
  chrome.braveShields.urlCosmeticResources(url, async (resources) => {
    if (chrome.runtime.lastError) {
      console.warn('Unable to get cosmetic filter data for the current host', chrome.runtime.lastError)
      return
    }

    if (frameId === 0) {
      if (hide1pContent) {
        resources.force_hide_selectors.push(...resources.hide_selectors)
      } else {
        informTabOfCosmeticRulesToConsider(tabId, resources.hide_selectors)
      }

      let styledStylesheet = ''
      if (resources.force_hide_selectors.length > 0) {
        styledStylesheet += resources.force_hide_selectors.join(',') + '{display:none!important;}\n'
      }
      for (const selector in resources.style_selectors) {
        styledStylesheet += selector + '{' + resources.style_selectors[selector].join(';') + ';}\n'
      }
      chrome.tabs.insertCSS(tabId, {
        code: styledStylesheet,
        cssOrigin: 'user',
        runAt: 'document_start'
      })
    }

    shieldsPanelActions.cosmeticFilterRuleExceptions(tabId, frameId, resources.exceptions, resources.injected_script || '', resources.generichide)
  })
}

export const addSiteCosmeticFilter = (host: string, cssSelector: string) => {
  chrome.braveShields.addSiteCosmeticFilter(host, cssSelector)
}

export const openFilterManagementPage = () => {
  chrome.braveShields.openFilterManagementPage()
}
