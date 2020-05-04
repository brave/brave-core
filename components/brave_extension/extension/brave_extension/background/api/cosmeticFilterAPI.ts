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
export const injectClassIdStylesheet = (tabId: number, classes: string[], ids: string[], exceptions: string[]) => {
  chrome.braveShields.hiddenClassIdSelectors(classes, ids, exceptions, (selectors, forceHideSelectors) => {
    informTabOfCosmeticRulesToConsider(tabId, selectors)

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
export const applyAdblockCosmeticFilters = (tabId: number, hostname: string) => {
  chrome.braveShields.hostnameCosmeticResources(hostname, async (resources) => {
    if (chrome.runtime.lastError) {
      console.warn('Unable to get cosmetic filter data for the current host', chrome.runtime.lastError)
      return
    }

    informTabOfCosmeticRulesToConsider(tabId, resources.hide_selectors)

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

    shieldsPanelActions.cosmeticFilterRuleExceptions(tabId, resources.exceptions, resources.injected_script || '')
  })
}

// User generated cosmetic filtering below
export const applyCSSCosmeticFilters = (tabId: number, hostname: string) => {
  chrome.storage.local.get('cosmeticFilterList', (storeData = {}) => {
    if (!storeData.cosmeticFilterList) {
      if (process.env.NODE_ENV === 'shields_development') {
        console.log('applySiteFilters: no cosmetic filter store yet')
      }
      return
    }
    if (storeData.cosmeticFilterList[hostname] !== undefined) {
      storeData.cosmeticFilterList[hostname].map((rule: string) => {
        if (process.env.NODE_ENV === 'shields_development') {
          console.log('applying rule', rule)
        }
        chrome.tabs.insertCSS(tabId, { // https://github.com/brave/brave-browser/wiki/Cosmetic-Filtering
          code: `${rule} {display: none !important;}`,
          cssOrigin: 'user',
          runAt: 'document_start'
        })
      })
    }
  })
}

export const removeAllFilters = () => {
  chrome.storage.local.set({ 'cosmeticFilterList': {} })
}

export const addSiteCosmeticFilter = async (origin: string, cssfilter: string) => {
  chrome.storage.local.get('cosmeticFilterList', (storeData = {}) => {
    let storeList = Object.assign({}, storeData.cosmeticFilterList)
    if (storeList[origin] === undefined || storeList[origin].length === 0) { // nothing in filter list for origin
      storeList[origin] = [cssfilter]
    } else { // add entry
      storeList[origin].push(cssfilter)
    }
    chrome.storage.local.set({ 'cosmeticFilterList': storeList })
  })
}

export const removeSiteFilter = (origin: string) => {
  chrome.storage.local.get('cosmeticFilterList', (storeData = {}) => {
    let storeList = Object.assign({}, storeData.cosmeticFilterList)
    delete storeList[origin]
    chrome.storage.local.set({ 'cosmeticFilterList': storeList })
  })
}
