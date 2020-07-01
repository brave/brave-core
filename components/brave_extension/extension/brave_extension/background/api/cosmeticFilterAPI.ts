/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import shieldsPanelActions from '../actions/shieldsPanelActions'
import { UrlCosmeticResourcesType } from '../../types/adblock/adblockTypes'

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

export const getAdblockCosmeticFiltersKey = (tabId: number, frameId: number) => {
  return `adblockCosmeticFilter-${tabId}-${frameId}`
}

export const fetchAdblockCosmeticFilters = (url: string, tabId: number, frameId: number, hide1pContent: boolean) => {
  chrome.braveShields.urlCosmeticResources(url, async (resources) => {
    if (chrome.runtime.lastError) {
      shieldsPanelActions.cosmeticFilterResourcesReady(tabId, frameId, hide1pContent, undefined)
      console.warn('Unable to get cosmetic filter data for the current host', chrome.runtime.lastError)
      return
    }

    shieldsPanelActions.cosmeticFilterResourcesReady(tabId, frameId, hide1pContent, resources)
  })
}

export const applyAdblockCosmeticFiltersWithResources = (tabId: number, frameId: number, hide1pContent: boolean, resources: UrlCosmeticResourcesType) => {
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

  let message: { type: string, scriptlet: string, hideOptions?: { hide1pContent: boolean, generichide: boolean } } = {
    type: 'cosmeticFilteringBackgroundReady',
    scriptlet: resources.injected_script || '',
    hideOptions: undefined
  }
  if (frameId === 0) {
    // Non-scriptlet cosmetic filters are only applied on the top-level frame
    message.hideOptions = {
      hide1pContent: hide1pContent,
      generichide: resources.generichide
    }
  }
  chrome.tabs.sendMessage(tabId, message, {
    frameId: frameId
  })

  // setTimeout is used to prevent calling another Redux function immediately
  setTimeout(() => shieldsPanelActions.cosmeticFilterRuleExceptions(tabId, frameId, resources.exceptions, resources.injected_script || '', resources.generichide), 0)
}

// Fires on content-script loaded
export const applyAdblockCosmeticFilters = (tabId: number, frameId: number, url: string, hide1pContent: boolean) => {
  chrome.braveShields.urlCosmeticResources(url, async (resources) => {
    if (chrome.runtime.lastError) {
      console.warn('Unable to get cosmetic filter data for the current host', chrome.runtime.lastError)
      return
    }
    applyAdblockCosmeticFiltersWithResources(tabId, frameId, hide1pContent, resources)
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
