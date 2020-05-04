// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

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

// Start looking for things to unhide before at most this long after
// the backend script is up and connected (eg backgroundReady = true),
// or sooner if the thread is idle.
const maxTimeMSBeforeStart = 2500

// The cutoff for text ads.  If something has only text in it, it needs to have
// this many, or more, characters.  Similarly, require it to have a non-trivial
// number of words in it, to look like an actual text ad.
const minAdTextChars = 30
const minAdTextWords = 5

const queriedIds = new Set<string>()
const queriedClasses = new Set<string>()

// Each of these get setup once the mutation observer starts running.
let notYetQueriedClasses: string[]
let notYetQueriedIds: string[]
let cosmeticObserver: MutationObserver | undefined = undefined

const allSelectorsToRules = new Map<string, number>()
const cosmeticStyleSheet = new CSSStyleSheet()

function injectScriptlet (text: string) {
  let script
  try {
    script = document.createElement('script')
    const textnode: Text = document.createTextNode(text)
    script.appendChild(textnode);
    (document.head || document.documentElement).appendChild(script)
  } catch (ex) {
    /* Unused catch */
  }
  if (script) {
    if (script.parentNode) {
      script.parentNode.removeChild(script)
    }
    script.textContent = ''
  }
}

/**
 * Provides a new function which can only be scheduled once at a time.
 *
 * @param onIdle function to run when the thread is less busy
 * @param timeout max time to wait. at or after this time the function will be run regardless of thread noise
 */
function idleize (onIdle: Function, timeout: number) {
  let idleId: number | undefined = undefined
  return function WillRunOnIdle () {
    if (idleId !== undefined) {
      return
    }
    idleId = window.requestIdleCallback(() => {
      idleId = undefined
      onIdle()
    }, { timeout })
  }
}

function isRelativeUrl (url: string): boolean {
  return (
    !url.startsWith('//') &&
    !url.startsWith('http://') &&
    !url.startsWith('https://')
  )
}

function isElement (node: Node): boolean {
  return (node.nodeType === 1)
}

function asElement (node: Node): Element | null {
  return isElement(node) ? node as Element : null
}

function isHTMLElement (node: Node): boolean {
  return ('innerText' in node)
}

function asHTMLElement (node: Node): HTMLElement | null {
  return isHTMLElement(node) ? node as HTMLElement : null
}

const fetchNewClassIdRules = function () {
  if ((!notYetQueriedClasses || notYetQueriedClasses.length === 0) &&
      (!notYetQueriedIds || notYetQueriedIds.length === 0)) {
    return
  }
  chrome.runtime.sendMessage({
    type: 'hiddenClassIdSelectors',
    classes: notYetQueriedClasses || [],
    ids: notYetQueriedIds || []
  })
  notYetQueriedClasses = []
  notYetQueriedIds = []
}

const handleMutations: MutationCallback = function (mutations: MutationRecord[]) {
  for (const aMutation of mutations) {
    if (aMutation.type === 'attributes') {
      // Since we're filtering for attribute modifications, we can be certain
      // that the targets are always HTMLElements, and never TextNode.
      const changedElm = aMutation.target as Element
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
      for (const node of aMutation.addedNodes) {
        const element = asElement(node)
        if (!element) {
          continue
        }
        const id = element.id
        if (id && !queriedIds.has(id)) {
          notYetQueriedIds.push(id)
          queriedIds.add(id)
        }
        const classList = element.classList
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

const _parseDomainCache = Object.create(null)
const getParsedDomain = (aDomain: string) => {
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
  if (isRelativeUrl(url)) {
    return true
  }

  const parsedTargetDomain = getParsedDomain(url)
  if (!parsedTargetDomain) {
    // If we cannot determine the party-ness of the resource,
    // consider it first-party.
    console.debug(`Cosmetic filtering: Unable to determine party-ness of "${url}"`)
    return false
  }

  return (
    _parsedCurrentDomain.tld === parsedTargetDomain.tld &&
    _parsedCurrentDomain.domain === parsedTargetDomain.domain
  )
}

const isAdText = (text: string): boolean => {
  const trimmedText = text.trim()
  if (trimmedText.length < minAdTextChars) {
    return false
  }
  if (trimmedText.split(' ').length < minAdTextWords) {
    return false
  }
  return true
}

interface IsFirstPartyQueryResult {
  foundFirstPartyResource: boolean,
  foundThirdPartyResource: boolean,
  foundKnownThirdPartyAd: boolean
}

/**
 * Determine whether a subtree should be considered as "first party" content.
 *
 * Uses the following process in making this determination.
 *   - If the subtree contains any first party resources, the subtree is 1p.
 *   - If the subtree contains no remote resources, the subtree is first party.
 *   - Otherwise, its 3rd party.
 *
 * Note that any instances of "url(" or escape characters in style attributes
 * are automatically treated as third-party URLs.  These patterns and special
 * cases were generated from looking at patterns in ads with resources in the
 * style attribute.
 *
 * Similarly, an empty srcdoc attribute is also considered 3p, since many
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

  if (queryResult.foundThirdPartyResource) {
    return false
  }
  const htmlElement = asHTMLElement(elm)
  if (!htmlElement || isAdText(htmlElement.innerText) === false) {
    return false
  }
  return true
}

const unhideSelectors = (selectors: Set<string>) => {
  if (selectors.size === 0) {
    return
  }
  // Find selectors we have a rule index for
  const rulesToRemove = Array.from(selectors)
    .map(selector =>
      allSelectorsToRules.get(selector)
    )
    .filter(i => i !== undefined)
    .sort()
    .reverse()
  // Delete the rules
  let lastIdx: number = allSelectorsToRules.size - 1
  for (const ruleIdx of rulesToRemove) {
    cosmeticStyleSheet.deleteRule(ruleIdx)
  }
  // Re-sync the indexes
  // TODO: Sync is hard, just re-build by iterating through the StyleSheet rules.
  const ruleLookup = Array.from(allSelectorsToRules.entries())
  let countAtLastHighest = rulesToRemove.length
  for (let i = lastIdx; i > 0; i--) {
    const [selector, oldIdx] = ruleLookup[i]
    // Is this one we removed?
    if (rulesToRemove.includes(i)) {
      allSelectorsToRules.delete(selector)
      countAtLastHighest--
      if (countAtLastHighest === 0) {
        break
      }
      continue
    }
    if (oldIdx !== i) {
      // Probably out of sync
      console.error('Cosmetic Filters: old index did not match lookup index', { selector, oldIdx, i })
    }
    allSelectorsToRules.set(selector, oldIdx - countAtLastHighest)
  }
}

const alreadyUnhiddenSelectors = new Set<string>()
const alreadyKnownFirstPartySubtrees = new WeakSet()
// All new selectors go in `firstRunQueue`
const firstRunQueue = new Set<string>()
// Third party matches go in the second and third queues.
const secondRunQueue = new Set<string>()
// Once a selector gets in to this queue, it's only evaluated for 1p content one
// more time.
const finalRunQueue = new Set<string>()
const allQueues = [firstRunQueue, secondRunQueue, finalRunQueue]
const numQueues = allQueues.length
const pumpIntervalMinMs = 40
const pumpIntervalMaxMs = 1000
const maxWorkSize = 60
let queueIsSleeping = false

/**
 * Go through each of the 3 queues, only take 50 items from each one
 * 1. Take 50 selects from the first queue with any items
 * 2. Determine partyness of matched element:
 *   - If any are 3rd party, keep 'hide' rule and check again later in next queue.
 *   - If any are 1st party, remove 'hide' rule and never check selector again.
 * 3. If we're looking at the 3rd queue, don't requeue any selectors.
 */
const pumpCosmeticFilterQueues = () => {
  if (queueIsSleeping === true) {
    return
  }

  let didPumpAnything = false
  // For each "pump", walk through each queue until we find selectors
  // to evaluate. This means that nothing in queue N+1 will be evaluated
  // until queue N is completely empty.
  for (let queueIndex = 0; queueIndex < numQueues; queueIndex += 1) {
    const currentQueue = allQueues[queueIndex]
    const nextQueue = allQueues[queueIndex + 1]
    if (currentQueue.size === 0) {
      continue
    }

    const currentWorkLoad = Array.from(currentQueue.values()).slice(0, maxWorkSize)
    const comboSelector = currentWorkLoad.join(',')
    const matchingElms = document.querySelectorAll(comboSelector)
    // Will hold selectors identified by _this_ queue pumping, that were
    // newly identified to be matching 1p content.  Will be sent to
    // the background script to do the un-hiding.
    const newlyIdentifiedFirstPartySelectors = new Set<string>()

    for (const aMatchingElm of matchingElms) {
      // Don't recheck elements / subtrees we already know are first party.
      // Once we know something is third party, we never need to evaluate it
      // again.
      if (alreadyKnownFirstPartySubtrees.has(aMatchingElm)) {
        continue
      }

      const elmSubtreeIsFirstParty = isSubTreeFirstParty(aMatchingElm)
      // If we find that a subtree is third party, then no need to change
      // anything, leave the selector as "hiding" and move on.
      // This element will likely be checked again on the next 'pump'
      // as long as another element from the selector does not match 1st party.
      if (elmSubtreeIsFirstParty === false) {
        continue
      }
      // Otherwise, we know that the given subtree was evaluated to be
      // first party, so we need to figure out which selector from the combo
      // selector did the matching.
      for (const selector of currentWorkLoad) {
        if (aMatchingElm.matches(selector) === false) {
          continue
        }

        // Similarly, if we already know a selector matches 1p content,
        // there is no need to notify the background script again, so
        // we don't need to consider further.
        if (alreadyUnhiddenSelectors.has(selector) === true) {
          continue
        }

        newlyIdentifiedFirstPartySelectors.add(selector)
        alreadyUnhiddenSelectors.add(selector)
      }
      alreadyKnownFirstPartySubtrees.add(aMatchingElm)
    }

    unhideSelectors(newlyIdentifiedFirstPartySelectors)

    for (const aUsedSelector of currentWorkLoad) {
      currentQueue.delete(aUsedSelector)
      // Don't requeue selectors we know identify first party content.
      const selectorMatchedFirstParty = newlyIdentifiedFirstPartySelectors.has(aUsedSelector)
      if (nextQueue && selectorMatchedFirstParty === false) {
        nextQueue.add(aUsedSelector)
      }
    }

    didPumpAnything = true
    // If we did something,  process the next queue, save it for next time.
    break
  }

  if (didPumpAnything) {
    queueIsSleeping = true
    window.setTimeout(() => {
      // Set this to false now even though there's a gap in time between now and
      // idle since all other calls to `pumpCosmeticFilterQueuesOnIdle` that occur during this time
      // will be ignored (and nothing else should be calling `pumpCosmeticFilterQueues` straight).
      queueIsSleeping = false
      // tslint:disable-next-line:no-use-before-declare
      pumpCosmeticFilterQueuesOnIdle()
    }, pumpIntervalMinMs)
  }
}

const pumpCosmeticFilterQueuesOnIdle = idleize(
  pumpCosmeticFilterQueues,
  pumpIntervalMaxMs
)

const startObserving = () => {
  // First queue up any classes and ids that exist before the mutation observer
  // starts running.
  const elmWithClassOrId = document.querySelectorAll('[class],[id]')
  for (const elm of elmWithClassOrId) {
    for (const aClassName of elm.classList.values()) {
      queriedClasses.add(aClassName)
    }
    const elmId = elm.getAttribute('id')
    if (elmId) {
      queriedIds.add(elmId)
    }
  }

  notYetQueriedClasses = Array.from(queriedClasses)
  notYetQueriedIds = Array.from(queriedIds)
  fetchNewClassIdRules()

  // Second, set up a mutation observer to handle any new ids or classes
  // that are added to the document.
  cosmeticObserver = new MutationObserver(handleMutations)
  let observerConfig = {
    subtree: true,
    childList: true,
    attributeFilter: ['id', 'class']
  }
  cosmeticObserver.observe(document.documentElement, observerConfig)
}

let _hasDelayOcurred: boolean = false
let _startCheckingId: number | undefined = undefined
const scheduleQueuePump = () => {
  // Three states possible here.  First, the delay has already occurred.  If so,
  // pass through to pumpCosmeticFilterQueues immediately.
  if (_hasDelayOcurred === true) {
    pumpCosmeticFilterQueuesOnIdle()
    return
  }
  // Second possibility is that we're already waiting for the delay to pass /
  // occur.  In this case, do nothing.
  if (_startCheckingId !== undefined) {
    return
  }
  // Third / final possibility, this is this the first time this has been
  // called, in which case set up a timmer and quit
  _startCheckingId = window.requestIdleCallback(function ({ didTimeout }) {
    _hasDelayOcurred = true
    startObserving()
    pumpCosmeticFilterQueuesOnIdle()
  }, { timeout: maxTimeMSBeforeStart })
}

chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
  const action = typeof msg === 'string' ? msg : msg.type
  switch (action) {
    case 'cosmeticFilteringBackgroundReady': {
      scheduleQueuePump()
      injectScriptlet(msg.scriptlet)
      break
    }
    case 'cosmeticFilterConsiderNewSelectors': {
      const { selectors } = msg
      let nextIndex = cosmeticStyleSheet.rules.length
      for (const selector of selectors) {
        if (allSelectorsToRules.has(selector)) {
          continue
        }
        // insertRule always adds to index 0,
        // so we always add to end of list manually.
        cosmeticStyleSheet.insertRule(
          `${selector}{display:none !important;}`,
          nextIndex
        )
        allSelectorsToRules.set(selector, nextIndex)
        nextIndex++
        firstRunQueue.add(selector)
      }
      // @ts-ignore
      if (!document.adoptedStyleSheets.includes(cosmeticStyleSheet)) {
        // @ts-ignore
        document.adoptedStyleSheets = [cosmeticStyleSheet]
      }
      scheduleQueuePump()
      break
    }
  }
})
