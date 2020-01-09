// Notify the background script as soon as the content script has loaded.
// chrome.tabs.insertCSS may sometimes fail to inject CSS in a newly navigated
// page when using the chrome.webNavigation API.
// See: https://bugs.chromium.org/p/chromium/issues/detail?id=331654#c15
// The RenderView should always be ready when the content script begins, so
// this message is used to trigger CSS insertion instead.
chrome.runtime.sendMessage({
  type: 'contentScriptsLoaded'
})

const parseDomain = require('parse-domain')

const queriedIds = new Set()
const queriedClasses = new Set()

let notYetQueriedClasses: string[] = []
let notYetQueriedIds: string[] = []

let backgroundReady: boolean = false

const fetchNewClassIdRules = function () {
  // Only let the backend know that we've found new classes and id attributes
  // if  the back end has told us its ready to go and we have at least one new
  // class or id to query for.
  if (backgroundReady) {
    if (notYetQueriedClasses.length !== 0 || notYetQueriedIds.length !== 0) {
      chrome.runtime.sendMessage({
        type: 'hiddenClassIdSelectors',
        classes: notYetQueriedClasses,
        ids: notYetQueriedIds
      })
      notYetQueriedClasses = []
      notYetQueriedIds = []
    }
  } else {
    setTimeout(fetchNewClassIdRules, 100)
  }
}

const handleMutations = function (mutations: any[]) {
  for (const aMutation of mutations) {
    if (aMutation.type === 'attribute') {
      // Since we're filtering for attribute modifications, we can be certain
      // that the targets are always HTMLElements, and never TextNode.
      const changedElm = aMutation.target
      switch (aMutation.attributeName) {
        case 'class':
          for (const aClassName of changedElm.classList.values()) {
            if (queriedClasses.has(aClassName) === false) {
              notYetQueriedClasses.push(aClassName)
              queriedClasses.add(aClassName)
            }
          }
          break

        case 'id':
          const mutatedId = changedElm.id
          if (queriedIds.has(mutatedId) === false) {
            notYetQueriedIds.push(mutatedId)
            queriedIds.add(mutatedId)
          }
          break
      }
    } else if (aMutation.addedNodes.length > 0) {
      for (const element of aMutation.addedNodes) {
        const id = element.id
        if (id && !queriedIds.has(id)) {
          notYetQueriedIds.push(id)
          queriedIds.add(id)
        }
        const classList: any = element.classList
        if (classList) {
          for (const className of classList.values()) {
            if (className && !queriedClasses.has(className)) {
              notYetQueriedClasses.push(className)
              queriedClasses.add(className)
            }
          }
        }
      }
    }
  }

  fetchNewClassIdRules()
}

const cosmeticObserver = new MutationObserver(handleMutations)
let observerConfig = {
  subtree: true,
  childList: true,
  attributeFilter: ['id', 'class']
}
cosmeticObserver.observe(document.documentElement, observerConfig)

const _parseDomainCache = Object.create(null)
const getParsedDomain = (aDomain: any) => {
  const cacheResult = _parseDomainCache[aDomain]
  if (cacheResult !== undefined) {
    return cacheResult
  }

  const newResult = parseDomain(aDomain)
  _parseDomainCache[aDomain] = newResult
  return newResult
}

const _parsedCurrentDomain = getParsedDomain(window.location.host)
const isFirstPartyUrl = (url: string): boolean => {
  if (url.startsWith('/')) {
    return true
  }

  const parsedTargetDomain = getParsedDomain(url)
  if (!parsedTargetDomain) {
    // If we cannot determine the party-ness of the resource,
    // consider it first-party.
    console.debug(`Unable to determine party-ness of "${url}"`)
    return false
  }

  return (
    _parsedCurrentDomain.tld === parsedTargetDomain.tld &&
    _parsedCurrentDomain.domain === parsedTargetDomain.domain
  )
}

interface IsFirstPartyQueryResult {
  foundFirstPartyResource: boolean,
  foundThirdPartyResource: boolean,
  foundKnownThirdPartyAd: boolean
}

/**
 * Determine whether a given subtree should be considered as "first party" content.
 *
 * Uses the following process in making this determination.
 *   - If the subtree contains any first party resources, the subtree is first party.
 *   - If the subtree contains no remote resources, the subtree is first party.
 *   - Otherwise, its 3rd party.
 *
 * Note that any instances of "url(" or escape characters in style attributes are
 * automatically treated as third-party URLs.  These patterns and special cases
 * were generated from looking at patterns in ads with resources in the style
 * attribute.
 *
 * Similarly, an empty srcdoc attribute is also considered third party, since many
 * third party ads clear this attribute in practice.
 *
 * Finally, special case some ids we know are used only for third party ads.
 */
const isSubTreeFirstParty = (elm: Element, possibleQueryResult?: IsFirstPartyQueryResult): boolean => {
  let queryResult: IsFirstPartyQueryResult
  let isTopLevel: boolean

  if (possibleQueryResult) {
    queryResult = possibleQueryResult
    isTopLevel = false
  } else {
    queryResult = {
      foundFirstPartyResource: false,
      foundThirdPartyResource: false,
      foundKnownThirdPartyAd: false
    }
    isTopLevel = true
  }

  if (elm.getAttribute) {
    if (elm.hasAttribute('id')) {
      const elmId = elm.getAttribute('id') as string
      if (elmId.startsWith('google_ads_iframe_') ||
          elmId.startsWith('div-gpt-ad') ||
          elmId.startsWith('adfox_')) {
        queryResult.foundKnownThirdPartyAd = true
        return false
      }
    }

    if (elm.hasAttribute('src')) {
      const elmSrc = elm.getAttribute('src') as string
      const elmSrcIsFirstParty = isFirstPartyUrl(elmSrc)
      if (elmSrcIsFirstParty === true) {
        queryResult.foundFirstPartyResource = true
        return true
      }
      queryResult.foundThirdPartyResource = true
    }

    if (elm.hasAttribute('style')) {
      const elmStyle = elm.getAttribute('style') as string
      if (elmStyle.includes('url(') ||
          elmStyle.includes('//')) {
        queryResult.foundThirdPartyResource = true
      }
    }

    if (elm.hasAttribute('srcdoc')) {
      const elmSrcDoc = elm.getAttribute('srcdoc') as string
      if (elmSrcDoc.trim() === '') {
        queryResult.foundThirdPartyResource = true
      }
    }
  }

  if (elm.firstChild) {
    isSubTreeFirstParty(elm.firstChild as Element, queryResult)
    if (queryResult.foundKnownThirdPartyAd === true) {
      return false
    }
    if (queryResult.foundFirstPartyResource === true) {
      return true
    }
  }

  if (elm.nextSibling) {
    isSubTreeFirstParty(elm.nextSibling as Element, queryResult)
    if (queryResult.foundKnownThirdPartyAd === true) {
      return false
    }
    if (queryResult.foundFirstPartyResource === true) {
      return true
    }
  }

  if (isTopLevel === false) {
    return (queryResult.foundThirdPartyResource === false)
  }

  const foundText = (elm as HTMLElement).innerText
  return (
    queryResult.foundThirdPartyResource === false &&
    foundText.trim().length > 0
  )
}

const hideSelectors = (selectors: string[]) => {
  if (selectors.length === 0) {
    return
  }

  chrome.runtime.sendMessage({
    type: 'hideThirdPartySelectors',
    selectors
  })
}

const alreadyHiddenSelectors = new Set()
const alreadyHiddenThirdPartySubTrees = new WeakSet()
const allSelectorsSet = new Set()
const firstRunQueue = new Set()
const secondRunQueue = new Set()
const finalRunQueue = new Set()
const allQueues = [firstRunQueue, secondRunQueue, finalRunQueue]
const numQueues = allQueues.length
const pumpIntervalMs = 50
const maxWorkSize = 50
let queueIsSleeping = false

const pumpCosmeticFilterQueues = () => {
  if (queueIsSleeping) {
    return
  }

  let didPumpAnything = false
  for (let queueIndex = 0; queueIndex < numQueues; queueIndex += 1) {
    const currentQueue = allQueues[queueIndex]
    const nextQueue = allQueues[queueIndex + 1]
    if (currentQueue.size === 0) {
      continue
    }

    const currentWorkLoad = Array.from(currentQueue.values()).slice(0, maxWorkSize)
    const comboSelector = currentWorkLoad.join(',')
    const matchingElms = document.querySelectorAll(comboSelector)
    const selectorsToHide = []

    for (const aMatchingElm of Array.from(matchingElms)) {
      if (alreadyHiddenThirdPartySubTrees.has(aMatchingElm)) {
        continue
      }
      const elmSubtreeIsFirstParty = isSubTreeFirstParty(aMatchingElm)
      if (elmSubtreeIsFirstParty === false) {
        for (const selector of currentWorkLoad) {
          if (aMatchingElm.matches(selector) && !alreadyHiddenSelectors.has(selector)) {
            selectorsToHide.push(selector)
            alreadyHiddenSelectors.add(selector)
          }
        }
        alreadyHiddenThirdPartySubTrees.add(aMatchingElm)
      }
    }

    hideSelectors(selectorsToHide)

    for (const aUsedSelector of currentWorkLoad) {
      currentQueue.delete(aUsedSelector)
      if (nextQueue) {
        nextQueue.add(aUsedSelector)
      }
    }

    didPumpAnything = true
    break
  }

  if (didPumpAnything) {
    queueIsSleeping = true
    setTimeout(() => {
      queueIsSleeping = false
      pumpCosmeticFilterQueues()
    }, pumpIntervalMs)
  }
}

chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
  const action = typeof msg === 'string' ? msg : msg.type
  switch (action) {
    case 'cosmeticFilteringBackgroundReady': {
      backgroundReady = true
      break
    }

    case 'cosmeticFilterConsiderNewRules': {
      const { hideRules } = msg
      for (const aHideRule of hideRules) {
        if (allSelectorsSet.has(aHideRule)) {
          continue
        }
        allSelectorsSet.add(aHideRule)
        firstRunQueue.add(aHideRule)
      }
      pumpCosmeticFilterQueues()
      break
    }
  }
})
