/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const addSiteCosmeticFilter = async (origin: string, cssfilter: string) => {
  chrome.storage.local.get('cosmeticFilterList', (storeData = {}) => {
    let storeList = Object.assign({}, storeData.cosmeticFilterList)
    if (storeList[origin] === undefined || storeList[origin].length === 0) { // nothing in filter list for origin
      storeList[origin] = [{
        'filter': cssfilter,
        'isIdempotent': isIdempotent(cssfilter),
        'applied': false
      }]
    } else { // add entry
      storeList[origin].push({
        'filter': cssfilter,
        'isIdempotent': isIdempotent(cssfilter),
        'applied': false
      })
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

export const applyCSSCosmeticFilters = (tabId: number, hostname: string) => {
  chrome.storage.local.get('cosmeticFilterList', (storeData = {}) => {
    if (!storeData.cosmeticFilterList) {
      if (process.env.NODE_ENV === 'shields_development') {
        console.log('applySiteFilters: no cosmetic filter store yet')
      }
      return
    }
    if (storeData.cosmeticFilterList[hostname] !== undefined) {
      storeData.cosmeticFilterList[hostname].map((rule: any) => { // if the filter hasn't been applied once before, apply it and set the corresponding filter to true
        if (process.env.NODE_ENV === 'shields_development') {
          console.log('applying filter', rule.filter)
        }
        chrome.tabs.insertCSS(tabId, { // https://github.com/brave/brave-browser/wiki/Cosmetic-Filtering
          code: `${rule.filter} {display:none!important;}`,
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

function isIdempotent (str: String) {
  // if string contains a non-idempotent string, the selector is not idempotent
  //   https://www.w3schools.com/cssref/css_selectors.asp

  let nonIdempotentStrings = [
    ':active',
    '::after',
    '::before',
    ':checked',
    ':default',
    ':disabled',
    ':empty',
    ':enabled',
    ':first-child',
    '::first-letter',
    '::first-line',
    ':first-of-type',
    ':focus',
    ':hover',
    ':last-child',
    ':last-of-type',
    ':nth-child',
    ':nth-last-child',
    ':nth-last-of-type',
    ':nth-of-type']

  for (let i = 0; i < nonIdempotentStrings.length; i++) {
    if (str.includes(nonIdempotentStrings[i])) {
      return true
    }
  }
  return false
}
