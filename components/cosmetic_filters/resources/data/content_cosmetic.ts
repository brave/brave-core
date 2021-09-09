// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// That script is executed from
// components/cosmetic_filters/content/renderer/cosmetic_filters_js_handler.cc
// several times:
// - for cosmetic filters work with CSS and stylesheet. That work itself
//   could call the script several times.

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

window.content_cosmetic = window.content_cosmetic || {}
const CC = window.content_cosmetic

CC.cosmeticStyleSheet = CC.cosmeticStyleSheet || new CSSStyleSheet()
CC.allSelectorsToRules = CC.allSelectorsToRules || new Map<string, number>()
CC.observingHasStarted = CC.observingHasStarted || false
// All new selectors go in `firstRunQueue`
CC.firstRunQueue = CC.firstRunQueue || new Set<string>()
// Third party matches go in the second and third queues.
CC.secondRunQueue = CC.secondRunQueue || new Set<string>()
// Once a selector gets in to this queue, it's only evaluated for 1p content one
// more time.
CC.finalRunQueue = CC.finalRunQueue || new Set<string>()
CC.allQueues = CC.allQueues || [
  CC.firstRunQueue, CC.secondRunQueue, CC.finalRunQueue]
CC.numQueues = CC.numQueues || CC.allQueues.length
CC.alreadyUnhiddenSelectors = CC.alreadyUnhiddenSelectors || new Set<string>()
CC.alreadyKnownFirstPartySubtrees =
  CC.alreadyKnownFirstPartySubtrees || new WeakSet()
CC._hasDelayOcurred = CC._hasDelayOcurred || false
CC._startCheckingId = CC._startCheckingId || undefined

/**
 * Provides a new function which can only be scheduled once at a time.
 *
 * @param onIdle function to run when the thread is less busy
 * @param timeout max time to wait. at or after this time the function will be run regardless of thread noise
 */
const idleize = (onIdle: Function, timeout: number) => {
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

const isRelativeUrl = (url: string): boolean => {
  return (
    !url.startsWith('//') &&
    !url.startsWith('http://') &&
    !url.startsWith('https://')
  )
}

const isElement = (node: Node): boolean => {
  return (node.nodeType === 1)
}

const asElement = (node: Node): Element | null => {
  return isElement(node) ? node as Element : null
}

const isHTMLElement = (node: Node): boolean => {
  return ('innerText' in node)
}

const asHTMLElement = (node: Node): HTMLElement | null => {
  return isHTMLElement(node) ? node as HTMLElement : null
}

const fetchNewClassIdRules = () => {
  if ((!notYetQueriedClasses || notYetQueriedClasses.length === 0) &&
    (!notYetQueriedIds || notYetQueriedIds.length === 0)) {
    return
  }
  // Callback to c++ renderer process
  // @ts-ignore
  cf_worker.hiddenClassIdSelectors(
      JSON.stringify({
        classes: notYetQueriedClasses, ids: notYetQueriedIds
      }))
  notYetQueriedClasses = []
  notYetQueriedIds = []
}

const handleMutations: MutationCallback = (mutations: MutationRecord[]) => {
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

const isFirstPartyUrl = (url: string): boolean => {
  if (isRelativeUrl(url)) {
    return true
  }

  // Callback to c++ renderer process
  // @ts-ignore
  return cf_worker.isFirstPartyUrl(url)
}

const stripChildTagsFromText = (elm: HTMLElement, tagName: string, text: string): string => {
  const childElms = Array.from(elm.getElementsByTagName(tagName)) as HTMLElement[]
  let localText = text
  for (const anElm of childElms) {
    localText = localText.replaceAll(anElm.innerText, '')
  }
  return localText
}

/**
 * Used to just call innerText on the root of the subtree, but in some cases
 * this will surprisingly include the text content of script nodes
 * (possibly of nodes that haven't been executed yet?).
 *
 * So instead  * we call innerText on the root, and remove the contents of any
 * script or style nodes.
 *
 * @see https://github.com/brave/brave-browser/issues/9955
 */
const showsSignificantText = (elm: Element): boolean => {
  if (isHTMLElement(elm) === false) {
    return false
  }

  const htmlElm = elm as HTMLElement
  const tagsTextToIgnore = ['script', 'style']

  let currentText = htmlElm.innerText
  for (const aTagName of tagsTextToIgnore) {
    currentText = stripChildTagsFromText(htmlElm, aTagName, currentText)
  }

  const trimmedText = currentText.trim()
  if (trimmedText.length < minAdTextChars) {
    return false
  }

  let wordCount = 0
  for (const aWord of trimmedText.split(' ')) {
    if (aWord.trim().length === 0) {
      continue
    }
    wordCount += 1
  }

  return wordCount >= minAdTextWords
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
  return true
}

const unhideSelectors = (selectors: Set<string>) => {
  if (selectors.size === 0) {
    return
  }
  // Find selectors we have a rule index for
  const rulesToRemove = Array.from(selectors)
    .map(selector => CC.allSelectorsToRules.get(selector))
    .filter(i => i !== undefined)
    .sort()
    .reverse()
  // Delete the rules
  let lastIdx: number = CC.allSelectorsToRules.size - 1
  for (const ruleIdx of rulesToRemove) {
    // Safe to asset ruleIdx is a number because we've already filtered out
    // any `undefined` instances with the filter call above.
    CC.cosmeticStyleSheet.deleteRule(ruleIdx as number)
  }
  // Re-sync the indexes
  // TODO: Sync is hard, just re-build by iterating through the StyleSheet rules.
  const ruleLookup = Array.from(CC.allSelectorsToRules.entries())
  let countAtLastHighest = rulesToRemove.length
  for (let i = lastIdx; i > 0; i--) {
    const [selector, oldIdx] = ruleLookup[i]
    // Is this one we removed?
    if (rulesToRemove.includes(i)) {
      CC.allSelectorsToRules.delete(selector)
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
    CC.allSelectorsToRules.set(selector, oldIdx - countAtLastHighest)
  }
}

const pumpIntervalMinMs = 40
const pumpIntervalMaxMs = 1000
const maxWorkSize = 60
let queueIsSleeping = false

/**
 * Go through each of the 3 queues, only take 50 items from each one
 * 1. Take 50 selectors from the first queue with any items
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
  for (let queueIndex = 0; queueIndex < CC.numQueues; queueIndex += 1) {
    const currentQueue = CC.allQueues[queueIndex]
    const nextQueue = CC.allQueues[queueIndex + 1]
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
      if (CC.alreadyKnownFirstPartySubtrees.has(aMatchingElm)) {
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

      // If the subtree doesn't have a significant amount of text (e.g., it
      // just says "Advertisement"), then no need to change anything; it should
      // stay hidden.
      if (showsSignificantText(aMatchingElm) === false) {
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
        if (CC.alreadyUnhiddenSelectors.has(selector) === true) {
          continue
        }

        newlyIdentifiedFirstPartySelectors.add(selector)
        CC.alreadyUnhiddenSelectors.add(selector)
      }
      CC.alreadyKnownFirstPartySubtrees.add(aMatchingElm)
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

const scheduleQueuePump = (hide1pContent: boolean, genericHide: boolean) => {
  // Three states possible here.  First, the delay has already occurred.  If so,
  // pass through to pumpCosmeticFilterQueues immediately.
  if (CC._hasDelayOcurred === true) {
    pumpCosmeticFilterQueuesOnIdle()
    return
  }
  // Second possibility is that we're already waiting for the delay to pass /
  // occur.  In this case, do nothing.
  if (CC._startCheckingId !== undefined) {
    return
  }
  // Third / final possibility, this is this the first time this has been
  // called, in which case set up a timer and quit
  CC._startCheckingId = window.requestIdleCallback(function ({ didTimeout }) {
    CC._hasDelayOcurred = true
    if (!genericHide) {
      startObserving()
    }
    if (!hide1pContent) {
      pumpCosmeticFilterQueuesOnIdle()
    }
  }, { timeout: maxTimeMSBeforeStart })
}

if (!CC.observingHasStarted) {
  CC.observingHasStarted = true
  scheduleQueuePump(CC.hide1pContent, CC.generichide)
} else {
  scheduleQueuePump(false, false)
}
