// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// That script is executed from
// components/cosmetic_filters/content/renderer/cosmetic_filters_js_handler.cc
// several times:
// - for cosmetic filters work with CSS and stylesheet. That work itself
//   could call the script several times.

import { applyCompiledSelector, compileProceduralSelector } from './procedural_filters'

// Start looking for things to unhide before at most this long after
// the backend script is up and connected (eg backgroundReady = true),
// or sooner if the thread is idle.
const maxTimeMSBeforeStart = 2500

// The cutoff for text ads.  If something has only text in it, it needs to have
// this many, or more, characters.  Similarly, require it to have a non-trivial
// number of words in it, to look like an actual text ad.
const minAdTextChars = 30
const minAdTextWords = 5

const returnToMutationObserverIntervalMs = 10000

const selectorsPollingIntervalMs = 500
let selectorsPollingIntervalId: number | undefined

// The number of potentially new selectors that are processed during the last
// |scoreCalcIntervalMs|.
let currentMutationScore = 0
// The time frame used to calc |currentMutationScore|.
const scoreCalcIntervalMs = 1000
// The begin of the time frame to calc |currentMutationScore|.
let currentMutationStartTime = performance.now()

// The next allowed time to call FetchNewClassIdRules() if it's throttled.
let nextFetchNewClassIdRulesCall = 0
let fetchNewClassIdRulesTimeoutId: number | undefined

// Generate a random string between [a000000000, zzzzzzzzzz] (base 36)
const generateRandomAttr = () => {
  const min = Number.parseInt('a000000000', 36)
  const max = Number.parseInt('zzzzzzzzzz', 36)
  return Math.floor(Math.random() * (max - min) + min).toString(36)
}

const globalStyleAttr = generateRandomAttr()
const styleAttrMap = new Map<string, string>()

const queriedIds = new Set<string>()
const queriedClasses = new Set<string>()

const notYetQueriedElements: Array<(Element[] | NodeListOf<Element>)> = []

const classIdWithoutHtmlOrBody = '[id]:not(html):not(body),[class]:not(html):not(body)'

// Each of these get setup once the mutation observer starts running.
let notYetQueriedClasses: string[] = []
let notYetQueriedIds: string[] = []

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

CC.firstSelectorsPollingDelayMs = CC.firstSelectorsPollingDelayMs || undefined
CC.switchToSelectorsPollingThreshold =
  CC.switchToSelectorsPollingThreshold || undefined
CC.fetchNewClassIdRulesThrottlingMs =
  CC.fetchNewClassIdRulesThrottlingMs || undefined

/**
 * Provides a new function which can only be scheduled once at a time.
 *
 * @param onIdle function to run when the thread is less busy
 * @param timeout max time to wait. at or after this time the function will be run regardless of thread noise
 */
const idleize = (onIdle: Function, timeout: number) => {
  let idleId: number | undefined
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

// The fetchNewClassIdRules() can be called of each MutationObserver event.
// Under the hood it makes a lot of work: call to C++ => IPC to the browser
// process => request to the rust CS engine and back.
// So limit the number of calls to one per fetchNewClassIdRulesThrottlingMs.
const ShouldThrottleFetchNewClassIdsRules = (): boolean => {
  if (CC.fetchNewClassIdRulesThrottlingMs === undefined) {
    return false // the feature is disabled.
  }

  if (fetchNewClassIdRulesTimeoutId) {
    return true // The function has already scheduled and called later.
  }

  const now = performance.now()
  const msToWait = nextFetchNewClassIdRulesCall - now
  if (msToWait > 0) {
    // Schedule the call in |msToWait| ms and return.
    fetchNewClassIdRulesTimeoutId =
      window.setTimeout(
        () => {
          fetchNewClassIdRulesTimeoutId = undefined
          fetchNewClassIdRules()
        }
        , msToWait)
    return true
  }

  nextFetchNewClassIdRulesCall =
    now + CC.fetchNewClassIdRulesThrottlingMs
  return false
}

const queueElementIdAndClasses = (element: Element) => {
  const id = element.id
  if (id && !queriedIds.has(id)) {
    notYetQueriedIds.push(id)
    queriedIds.add(id)
  }
  for (const className of element.classList.values()) {
    if (className && !queriedClasses.has(className)) {
      notYetQueriedClasses.push(className)
      queriedClasses.add(className)
    }
  }
}

const fetchNewClassIdRules = () => {
  for (const elements of notYetQueriedElements) {
    for (const element of elements) {
      queueElementIdAndClasses(element)
    }
  }
  notYetQueriedElements.length = 0
  if ((!notYetQueriedClasses || notYetQueriedClasses.length === 0) &&
    (!notYetQueriedIds || notYetQueriedIds.length === 0)) {
    return
  }
  // Callback to c++ renderer process
  // @ts-expect-error
  cf_worker.hiddenClassIdSelectors(
      JSON.stringify({
        classes: notYetQueriedClasses, ids: notYetQueriedIds
      }))
  notYetQueriedClasses = []
  notYetQueriedIds = []
}

const useMutationObserver = () => {
  if (selectorsPollingIntervalId) {
    clearInterval(selectorsPollingIntervalId)
    selectorsPollingIntervalId = undefined
  }

  const observer = new MutationObserver(onMutations as MutationCallback)
  const observerConfig = {
    subtree: true,
    childList: true,
    attributeFilter: ['id', 'class']
  }
  observer.observe(document.documentElement, observerConfig)
}

const usePolling = (observer?: MutationObserver) => {
  if (observer) {
    observer.disconnect()
    notYetQueriedElements.length = 0
  }

  const futureTimeMs = window.Date.now() + returnToMutationObserverIntervalMs
  const queryAttrsFromDocumentBound = queryAttrsFromDocument.bind(undefined,
                                                                  futureTimeMs)

  selectorsPollingIntervalId = window.setInterval(queryAttrsFromDocumentBound,
                                                  selectorsPollingIntervalMs)
}

const queueAttrsFromMutations = (mutations: MutationRecord[]): number => {
  let mutationScore = 0
  for (const aMutation of mutations) {
    if (aMutation.type === 'attributes') {
      // Since we're filtering for attribute modifications, we can be certain
      // that the targets are always HTMLElements, and never TextNode.
      const changedElm = aMutation.target as Element
      switch (aMutation.attributeName) {
        case 'class':
          mutationScore += changedElm.classList.length
          for (const aClassName of changedElm.classList.values()) {
            if (!queriedClasses.has(aClassName)) {
              notYetQueriedClasses.push(aClassName)
              queriedClasses.add(aClassName)
            }
          }
          break

        case 'id':
          const mutatedId = changedElm.id
          mutationScore++
          if (!queriedIds.has(mutatedId)) {
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
        notYetQueriedElements.push([element])
        mutationScore += 1
        if (element.firstElementChild !== null) {
          const nodeList = element.querySelectorAll(classIdWithoutHtmlOrBody)
          notYetQueriedElements.push(nodeList)
          mutationScore += nodeList.length
        }
      }
    }
  }
  return mutationScore
}

const onMutations = (mutations: MutationRecord[], observer: MutationObserver) => {
  // Callback to c++ renderer process
  // @ts-expect-error
  const eventId: number | undefined = cf_worker.onHandleMutationsBegin?.()
  const mutationScore = queueAttrsFromMutations(mutations)

  // Check the conditions to switch to the alternative strategy
  // to get selectors.
  if (CC.switchToSelectorsPollingThreshold !== undefined) {
    const now = performance.now()

    if (now > currentMutationStartTime + scoreCalcIntervalMs) {
      // Start the next time frame.
      currentMutationStartTime = now
      currentMutationScore = 0
    }

    currentMutationScore += mutationScore
    if (currentMutationScore > CC.switchToSelectorsPollingThreshold) {
      usePolling(observer)
    }
  }

  if (!CC.generichide && !ShouldThrottleFetchNewClassIdsRules()) {
    fetchNewClassIdRules()
  }

  if (CC.hasProceduralActions) {
    const addedElements : Element[] = [];
    mutations.forEach(mutation =>
      mutation.addedNodes.length !== 0 && mutation.addedNodes.forEach(n =>
        n.nodeType === Node.ELEMENT_NODE && addedElements.push(n as Element)
      )
    )
    if (addedElements.length !== 0) {
      executeProceduralActions(addedElements);
    }
  }

  if (eventId) {
    // Callback to c++ renderer process
    // @ts-expect-error
    cf_worker.onHandleMutationsEnd(eventId)
  }
}

const isFirstPartyUrl = (url: string): boolean => {
  if (isRelativeUrl(url)) {
    return true
  }

  // Callback to c++ renderer process
  // @ts-expect-error
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
  if (!isHTMLElement(elm)) {
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
  foundFirstPartyResource: boolean
  foundThirdPartyResource: boolean
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
      if (elmSrcIsFirstParty) {
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
    if (queryResult.foundKnownThirdPartyAd) {
      return false
    }
    if (queryResult.foundFirstPartyResource) {
      return true
    }
  }

  if (elm.nextSibling) {
    isSubTreeFirstParty(elm.nextSibling as Element, queryResult)
    if (queryResult.foundKnownThirdPartyAd) {
      return false
    }
    if (queryResult.foundFirstPartyResource) {
      return true
    }
  }

  if (!isTopLevel) {
    return (!queryResult.foundThirdPartyResource)
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
  if (queueIsSleeping) {
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
      //
      // Also, if the subtree doesn't have a significant amount of text (e.g.,
      // it just says "Advertisement"), then no need to change anything; it
      // should stay hidden.
      if (!(elmSubtreeIsFirstParty || showsSignificantText(aMatchingElm))) {
        continue
      }

      // Otherwise, we know that the given subtree was evaluated to be
      // first party, so we need to figure out which selector from the combo
      // selector did the matching.
      for (const selector of currentWorkLoad) {
        if (!aMatchingElm.matches(selector)) {
          continue
        }

        // Similarly, if we already know a selector matches 1p content,
        // there is no need to notify the background script again, so
        // we don't need to consider further.
        if (CC.alreadyUnhiddenSelectors.has(selector)) {
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
      if (nextQueue && !selectorMatchedFirstParty) {
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
      pumpCosmeticFilterQueuesOnIdle()
    }, pumpIntervalMinMs)
  }
}

const pumpCosmeticFilterQueuesOnIdle = idleize(
  pumpCosmeticFilterQueues,
  pumpIntervalMaxMs
)

const queryAttrsFromDocument = (switchToMutationObserverAtTime?: number) => {
  // Callback to c++ renderer process
  // @ts-expect-error
  const eventId: number | undefined = cf_worker.onQuerySelectorsBegin?.()

  if (!CC.generichide) {
    const elmWithClassOrId = document.querySelectorAll(classIdWithoutHtmlOrBody)
    for (const elm of elmWithClassOrId) {
      queueElementIdAndClasses(elm)
    }

    fetchNewClassIdRules()
  }

  if (CC.hasProceduralActions) executeProceduralActions();

  if (eventId) {
    // Callback to c++ renderer process
    // @ts-expect-error
    cf_worker.onQuerySelectorsEnd(eventId)
  }

  if (switchToMutationObserverAtTime !== undefined &&
        window.Date.now() >= switchToMutationObserverAtTime) {
    useMutationObserver()
  }
}

const startObserving = () => {
  // First queue up any classes and ids that exist before the mutation observer
  // starts running.
  queryAttrsFromDocument()

  // Second, set up a mutation observer to handle any new ids or classes
  // that are added to the document.
  useMutationObserver()
}

const scheduleQueuePump = (hide1pContent: boolean, genericHide: boolean) => {
  // Three states possible here.  First, the delay has already occurred.  If so,
  // pass through to pumpCosmeticFilterQueues immediately.
  if (CC._hasDelayOcurred) {
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
  CC._startCheckingId = window.requestIdleCallback(_ => {
    CC._hasDelayOcurred = true
    if (!genericHide || CC.hasProceduralActions) {
      if (CC.firstSelectorsPollingDelayMs === undefined) {
        startObserving()
      } else {
        window.setTimeout(startObserving, CC.firstSelectorsPollingDelayMs)
      }
    }
    if (!hide1pContent) {
      pumpCosmeticFilterQueuesOnIdle()
    }
  }, { timeout: maxTimeMSBeforeStart })
}

const tryScheduleQueuePump = () => {
  if (!CC.observingHasStarted) {
    CC.observingHasStarted = true
    scheduleQueuePump(CC.hide1pContent, CC.generichide)
  } else {
    scheduleQueuePump(false, false)
  }
}

CC.tryScheduleQueuePump = CC.tryScheduleQueuePump || tryScheduleQueuePump

tryScheduleQueuePump()

const executeProceduralActions = (added?: Element[]) => {
  // If passed a list of added elements, do not query the entire document
  if (CC.proceduralActionFilters === undefined) {
    return
  }

  const getStyleAttr = (style: string): string => {
    let styleAttr = styleAttrMap.get(style)
    if (styleAttr === undefined) {
      styleAttr = generateRandomAttr()
      styleAttrMap.set(style, styleAttr)
      const css = `[${globalStyleAttr}][${styleAttr}]{${style}}`
      // @ts-expect-error
      cf_worker.injectStylesheet(css)
    }
    return styleAttr
  }

  const performAction = (element: any, action: any) => {
    if (action === undefined) {
      const attr = getStyleAttr('display: none !important')
      element.setAttribute(globalStyleAttr, '')
      element.setAttribute(attr, '')
    } else if (action.type === 'style') {
      const attr = getStyleAttr(action.arg)
      element.setAttribute(globalStyleAttr, '')
      element.setAttribute(attr, '')
    } else if (action.type === 'remove') {
      element.remove()
    } else if (action.type === 'remove-attr') {
      // We can remove attributes without checking if they exist
      element.removeAttribute(action.arg)
    } else if (action.type === 'remove-class') {
      // Check if the element has any classes to remove because
      // classList.remove(tokens...) always triggers another mutation
      // even if nothing was removed.
      if (element.classList.contains(action.arg)) {
        element.classList.remove(action.arg);
      }
    }
  }
  for (const { selector, action } of CC.proceduralActionFilters) {
    let matchingElements: Element[] | NodeListOf<any>;
    let startOperator: number;

    if (selector[0].type === 'css-selector' && added === undefined) {
      matchingElements = document.querySelectorAll(selector[0].arg);
      startOperator = 1;
    } else if (added === undefined) {
      matchingElements = document.querySelectorAll('*');
      startOperator = 0;
    } else {
      matchingElements = added;
      startOperator = 0;
    }

    if (startOperator === selector.length) {
      // First `css-selector` was already handled, and no more elements remain
      matchingElements.forEach(elem => performAction(elem, action))
    } else {
      try {
        const filter = compileProceduralSelector(selector.slice(startOperator));
        applyCompiledSelector(filter, matchingElements as HTMLElement[]).forEach(elem => performAction(elem, action))
      } catch (e) {
        console.error('Failed to apply filter ' + JSON.stringify(selector) + ' ' + JSON.stringify(action) + ': ');
        console.error(e.message);
        console.error(e.stack);
      }
    }
  }
};

if (CC.hasProceduralActions) executeProceduralActions();
