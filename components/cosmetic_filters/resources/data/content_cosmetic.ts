// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

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

/**
 *
 *  Procedural Filters
 *
 */
const { applyCompiledSelector, compileProceduralSelector } = (function() {
  type CSSSelector = string
  type CSSInstruction = string
  type CSSValue = string

  type OperatorType = string
  type OperatorArg = CSSSelector | ProceduralSelector | string
  type OperatorResult = HTMLElement[]

  type UnboundStringFunc = (arg: string, element: HTMLElement) => OperatorResult
  type UnboundChildRuleOrStringFunc = (
    arg: string | ProceduralSelector,
    element: HTMLElement) => OperatorResult
  type UnboundOperatorFunc = UnboundStringFunc | UnboundChildRuleOrStringFunc
  type OperatorFunc = (element: HTMLElement) => OperatorResult

  /* post-processed for convenient usage in JS */
  interface CompiledProceduralOperator {
    type: OperatorType
    func: OperatorFunc
    args: OperatorArg[]
  }
  type CompiledProceduralSelector = CompiledProceduralOperator[]

  type NeedlePosition = number
  type TextMatchRule = (targetText: string, exact?: boolean) => boolean
  type KeyValueMatchRules = [
    keyMatchRule: TextMatchRule,
    valueMatchRule: TextMatchRule,
  ]

  const W = window

  const _asHTMLElement = (node: Node): HTMLElement | null => {
    return (node instanceof HTMLElement) ? node : null
  }

  const _compileRegEx = (regexText: string): RegExp => {
    const regexParts = regexText.split('/')
    const regexPattern = regexParts[1]
    const regexArgs = regexParts[2]
    const regex = new W.RegExp(regexPattern, regexArgs)
    return regex
  }

  // Check to see if the string `value` either
  // contains the the string `test` (if `test` does
  // not start with `/`) or if the string
  // value matches the regex `test`.
  // We assume if test isn't a string, its a regex object.
  //
  // Rules:
  //   - if `test` starts with `/` we treat it as a regex
  //     literal
  //   - if `text` is an empty string, we treat it as
  //     matching any case where value is only whitespace
  //   - otherwise, check to see if value contains the
  //     string `test`
  //
  // If `exact` is true, then the string case it tested
  // for an exact match (the regex case is not affected).
  const _testMatches = (test: string, value: string, exact: boolean = false): boolean => {
    if (test[0] === '/') {
      return value.match(_compileRegEx(test)) !== null
    }
    if (test === '') {
      return value.trim() === ''
    }
    if (exact) {
      return value === test
    }
    return value.includes(test)
  }

  const _extractKeyMatchRuleFromStr = (text: string): [TextMatchRule, number] => {
    const quotedTerminator = '"='
    const unquotedTerminator = '='
    const isQuotedCase = text[0] === '"'

    const [terminator, needlePosition] = isQuotedCase
      ? [quotedTerminator, 1]
      : [unquotedTerminator, 0]

    const indexOfTerminator = text.indexOf(terminator, needlePosition)
    if (indexOfTerminator === -1) {
      throw new Error(
        `Unable to parse key rule from ${text}. Key rule starts with `
        + `${text[0]}, but doesn't include '${terminator}'`)
    }

    const testCaseStr = text.slice(needlePosition, indexOfTerminator)
    const testCaseFunc = _testMatches.bind(undefined, testCaseStr)
    const finalNeedlePosition = indexOfTerminator + terminator.length
    return [testCaseFunc, finalNeedlePosition]
  }

  const _extractValueMatchRuleFromStr = (text: string,
                                         needlePosition = 0): TextMatchRule => {
    const isQuotedCase = text[needlePosition] === '"'
    let endIndex

    if (isQuotedCase) {
      if (text.at(-1) !== '"') {
        throw new Error(
          `Unable to parse value rule from ${text}. Value rule starts with `
          + '" but doesn\'t end with "')
      }
      needlePosition += 1
      endIndex = text.length - 1
    }
    else {
      endIndex = text.length
    }

    const testCaseStr = text.slice(needlePosition, endIndex)
    const testCaseFunc = _testMatches.bind(undefined, testCaseStr)
    return testCaseFunc
  }

  // Parse an argument like `"abc"="xyz"` into
  // a test for the key, and a test for the value.
  // This will return two functions then, that you
  // should use for checking the key and values
  // in your test case.
  //
  // const key = ..., value = ...
  // const [keyTestFunc, valueTestFunc] = _parseKeyValueMatchArg(arg)
  //
  // if (keyTestFunc(key))) {
  //   // key matches the test condition
  // }
  const _parseKeyValueMatchRules = (arg: string): KeyValueMatchRules => {
    const [keyMatchRule, needlePos] = _extractKeyMatchRuleFromStr(arg)
    const valueMatchRule = _extractValueMatchRuleFromStr(arg, needlePos)
    return [keyMatchRule, valueMatchRule]
  }

  const _parseCSSInstruction = (arg: string): [CSSInstruction, CSSValue] => {
    const rs = arg.split(':')
    if (rs.length !== 2) {
      throw Error(`Unexpected format for a CSS rule: ${arg}`)
    }
    return [rs[0].trim(), rs[1].trim()]
  }

  const _allOtherSiblings = (element: HTMLElement): HTMLElement[] => {
    if (!element.parentNode) {
      return []
    }
    const siblings = Array.from(element.parentNode.children)
    const otherHTMLElements = []
    for (const sib of siblings) {
      if (sib === element) {
        continue
      }
      const siblingHTMLElement = _asHTMLElement(sib)
      if (siblingHTMLElement !== null) {
        otherHTMLElements.push(siblingHTMLElement)
      }
    }
    return otherHTMLElements
  }

  const _nextSiblingElement = (element: HTMLElement): HTMLElement | null => {
    if (!element.parentNode) {
      return null
    }
    const siblings = W.Array.from(element.parentNode.children)
    const indexOfElm = siblings.indexOf(element)
    const nextSibling = siblings[indexOfElm + 1]
    if (nextSibling === undefined) {
      return null
    }
    return _asHTMLElement(nextSibling)
  }

  const _allChildren = (element: HTMLElement): HTMLElement[] => {
    return W.Array.from(element.children)
      .map(e => _asHTMLElement(e))
      .filter(e => e !== null) as HTMLElement[]
  }

  const _allChildrenRecursive = (element: HTMLElement): HTMLElement[] => {
    return W.Array.from(element.querySelectorAll(':scope *'))
      .map(e => _asHTMLElement(e))
      .filter(e => e !== null) as HTMLElement[]
  }

  // Implementation of ":css-selector" rule
  const operatorCssSelector = (selector: CSSSelector,
                               element: HTMLElement): OperatorResult => {
    const _stripOperator = (operator: string, selector: string) => {
      if (selector[0] !== operator) {
        throw new Error(
          `Expected to find ${operator} in initial position of "${selector}`)
      }
      return selector.replace(operator, '').trimStart()
    }

    const trimmedSelector = selector.trimStart()
    if (trimmedSelector.startsWith('+')) {
      const subOperator = _stripOperator('+', trimmedSelector)
      if (subOperator === null) {
        return []
      }
      const nextSibNode = _nextSiblingElement(element)
      if (nextSibNode === null) {
        return []
      }
      return nextSibNode.matches(subOperator) ? [nextSibNode] : []
    }
    else if (trimmedSelector.startsWith('~')) {
      const subOperator = _stripOperator('~', trimmedSelector)
      if (subOperator === null) {
        return []
      }
      const allSiblingNodes = _allOtherSiblings(element)
      return allSiblingNodes.filter(x => x.matches(subOperator))
    }
    else if (trimmedSelector.startsWith('>')) {
      const subOperator = _stripOperator('>', trimmedSelector)
      if (subOperator === null) {
        return []
      }
      const allChildNodes = _allChildren(element)
      return allChildNodes.filter(x => x.matches(subOperator))
    }
    else if (selector.startsWith(' ')) {
      return Array.from(element.querySelectorAll(':scope ' + trimmedSelector))
    }

    if (element.matches(selector)) {
      return [element]
    }
    else {
      return []
    }
  }

  const _hasPlainSelectorCase = (selector: CSSSelector,
                                 element: HTMLElement): OperatorResult => {
    return element.matches(selector) ? [element] : []
  }

  const _hasProceduralSelectorCase = (selector: ProceduralSelector,
                                      element: HTMLElement): OperatorResult => {
    const shouldBeGreedy = selector[0]?.type !== 'css-selector'
    const initElements = shouldBeGreedy
      ? _allChildrenRecursive(element)
      : [element]
    const matches = compileAndApplyProceduralSelector(selector, initElements)
    return matches.length === 0 ? [] : [element]
  }

  // Implementation of ":has" rule
  const operatorHas = (instruction: CSSSelector | ProceduralSelector,
                       element: HTMLElement): OperatorResult => {
    if (W.Array.isArray(instruction)) {
      return _hasProceduralSelectorCase(instruction, element)
    }
    else {
      return _hasPlainSelectorCase(instruction, element)
    }
  }

  // Implementation of ":has-text" rule
  const operatorHasText = (instruction: string,
                           element: HTMLElement): OperatorResult => {
    const text = element.innerText
    const valueTest = _extractValueMatchRuleFromStr(instruction)
    return valueTest(text) ? [element] : []
  }

  const _notPlainSelectorCase = (selector: CSSSelector,
                                 element: HTMLElement): OperatorResult => {
    return element.matches(selector) ? [] : [element]
  }

  const _notProceduralSelectorCase = (selector: ProceduralSelector,
                                      element: HTMLElement): OperatorResult => {
    const matches = compileAndApplyProceduralSelector(selector, [element])
    return matches.length === 0 ? [element] : []
  }

  // Implementation of ":not" rule
  const operatorNot = (instruction: CSSSelector | ProceduralSelector,
                       element: HTMLElement): OperatorResult => {
    if (Array.isArray(instruction)) {
      return _notProceduralSelectorCase(instruction, element)
    }
    else {
      return _notPlainSelectorCase(instruction, element)
    }
  }

  // Implementation of ":matches-property" rule
  const operatorMatchesProperty = (instruction: string,
                                   element: HTMLElement): OperatorResult => {
    const [keyTest, valueTest] = _parseKeyValueMatchRules(instruction)
    for (const [propName, propValue] of Object.entries(element)) {
      if (!keyTest(propName)) {
        continue
      }
      if (!valueTest(propValue)) {
        continue
      }
      return [element]
    }
    return []
  }

  // Implementation of ":min-text-length" rule
  const operatorMinTextLength = (instruction: string,
                                 element: HTMLElement): OperatorResult => {
    const minLength = +instruction
    if (minLength === W.NaN) {
      throw new Error(`min-text-length: Invalid arg, ${instruction}`)
    }
    return element.innerText.trim().length >= minLength ? [element] : []
  }

  // Implementation of ":matches-attr" rule
  const operatorMatchesAttr = (instruction: string,
                               element: HTMLElement): OperatorResult => {
    const [keyTest, valueTest] = _parseKeyValueMatchRules(instruction)
    for (const attrName of element.getAttributeNames()) {
      if (!keyTest(attrName)) {
        continue
      }
      const attrValue = element.getAttribute(attrName)
      if (attrValue === null || !valueTest(attrValue)) {
        continue
      }
      return [element]
    }
    return []
  }

  // Implementation of ":matches-css-*" rules
  const operatorMatchesCSS = (beforeOrAfter: string | null,
                              cssInstruction: string,
                              element: HTMLElement): OperatorResult => {
    const [cssKey, expectedVal] = _parseCSSInstruction(cssInstruction)
    const elmStyle = W.getComputedStyle(element, beforeOrAfter)
    const styleValue = elmStyle.getPropertyValue(cssKey)
    if (styleValue === undefined) {
      // We're querying for a style property that doesn't exist, which
      // trivially doesn't match then.
      return []
    }
    return expectedVal === styleValue ? [element] : []
  }

  // Implementation of ":matches-media" rule
  const operatorMatchesMedia = (instruction: string,
                                element: HTMLElement): OperatorResult => {
    return W.matchMedia(instruction).matches ? [element] : []
  }

  // Implementation of ":matches-path" rule
  const operatorMatchesPath = (instruction: string,
                               element: HTMLElement): OperatorResult => {
    const pathAndQuery = W.location.pathname + W.location.search
    const matchRule = _extractValueMatchRuleFromStr(instruction)
    return matchRule(pathAndQuery) ? [element] : []
  }

  const _upwardIntCase = (intNeedle: NeedlePosition,
                          element: HTMLElement): OperatorResult => {
    if (intNeedle < 1 || intNeedle >= 256) {
      throw new Error(`upward: invalid arg, ${intNeedle}`)
    }
    let currentElement: HTMLElement | ParentNode | null = element
    while (currentElement !== null && intNeedle > 0) {
      currentElement = currentElement.parentNode
      intNeedle -= 1
    }
    if (currentElement === null) {
      return []
    } else {
      const htmlElement = _asHTMLElement(currentElement)
      return (htmlElement === null) ? [] : [htmlElement]
    }
  }

  const _upwardProceduralSelectorCase = (selector: ProceduralSelector,
                                         element: HTMLElement): OperatorResult => {
    const childFilter = compileProceduralSelector(selector)
    let needle: ParentNode | HTMLElement | null = element
    while (needle !== null) {
      const currentElement = _asHTMLElement(needle)
      if (currentElement === null) {
        break
      }
      const matches = applyCompiledSelector(childFilter, [currentElement])
      if (matches.length !== 0) {
        return [currentElement]
      }
      needle = currentElement.parentNode
    }
    return []
  }

  const _upwardPlainSelectorCase = (selector: CSSSelector,
                                    element: HTMLElement): OperatorResult => {
    let needle: ParentNode | HTMLDocument | null = element
    while (needle !== null) {
      const currentElement = _asHTMLElement(needle)
      if (currentElement === null) {
        break
      }
      if (currentElement.matches(selector)) {
        return [currentElement]
      }
      needle = currentElement.parentNode
    }
    return []
  }

  // Implementation of ":upward" rule
  const operatorUpward = (instruction: string | ProceduralSelector,
                          element: HTMLElement): OperatorResult => {
    if (W.Number.isInteger(+instruction)) {
      return _upwardIntCase(+instruction, element)
    }
    else if (W.Array.isArray(instruction)) {
      return _upwardProceduralSelectorCase(instruction, element)
    }
    else {
      return _upwardPlainSelectorCase(instruction, element)
    }
  }

  // Implementation of ":xpath" rule
  const operatorXPath = (instruction: string,
                         element: HTMLElement): HTMLElement[] => {
    const result = W.document.evaluate(instruction, element, null,
                                       W.XPathResult.UNORDERED_NODE_ITERATOR_TYPE,
                                       null)
    const matches: HTMLElement[] = []
    let currentNode: Node | null
    while ((currentNode = result.iterateNext())) {
      const currentElement = _asHTMLElement(currentNode)
      if (currentElement !== null) {
        matches.push(currentElement)
      }
    }
    return matches
  }

  const ruleTypeToFuncMap: Record<OperatorType, UnboundOperatorFunc> = {
    'contains': operatorHasText,
    'css-selector': operatorCssSelector,
    'has': operatorHas,
    'has-text': operatorHasText,
    'matches-attr': operatorMatchesAttr,
    'matches-css': operatorMatchesCSS.bind(undefined, null),
    'matches-css-after': operatorMatchesCSS.bind(undefined, '::after'),
    'matches-css-before': operatorMatchesCSS.bind(undefined, '::before'),
    'matches-media': operatorMatchesMedia,
    'matches-path': operatorMatchesPath,
    'matches-property': operatorMatchesProperty,
    'min-text-length': operatorMinTextLength,
    'not': operatorNot,
    'upward': operatorUpward,
    'xpath': operatorXPath,
  }

  const compileProceduralSelector = (operators: ProceduralSelector): CompiledProceduralSelector => {
    const outputOperatorList = []
    for (const operator of operators) {
      const anOperatorFunc = ruleTypeToFuncMap[operator.type]
      const args = [operator.arg]
      if (anOperatorFunc === undefined) {
        throw new Error(`Not sure what to do with operator of type ${operator.type}`)
      }

      outputOperatorList.push({
        type: operator.type,
        func: anOperatorFunc.bind(undefined, ...args),
        args,
      })
    }

    return outputOperatorList
  }

  // List of operator types that will be either globally true or false
  // independent of the passed element. We use this list to optimize
  // applying each operator (i.e., we just check the first element, and then
  // accept or reject all elements in the consideration set accordingly).
  const fastPathOperatorTypes: OperatorType[] = [
    'matches-media',
    'matches-path',
  ]

  const applyCompiledSelector = (selector: CompiledProceduralSelector,
                                 initNodes?: HTMLElement[]): HTMLElement[] => {
    let nodesToConsider: HTMLElement[] = []
    let index = 0

    // A couple of special cases to consider.
    //
    // Case one: we're applying the procedural filter on a set of nodes (instead
    // of the entire document)  In this case, we already know which nodes to
    // consider, easy case.
    const firstOperator = selector[0]
    const firstOperatorType = firstOperator.type
    const firstArg = firstOperator.args[0]

    if (initNodes !== undefined) {
      nodesToConsider = W.Array.from(initNodes)
    }
    else if (firstOperatorType === 'css-selector') {
      const selector = firstArg as CSSSelector
      // Case two: we're considering the entire document, and the first operator
      // is a 'css-selector'. Here, we just special case using querySelectorAll
      // instead of starting with the full set of possible nodes.
      nodesToConsider = W.Array.from(W.document.querySelectorAll(selector))
      index += 1
    }
    else if (firstOperatorType === 'xpath') {
      const xpath = firstArg as string
      nodesToConsider = operatorXPath(xpath, W.document.documentElement)
      index += 1
    }
    else {
      // Case three: we gotta apply the first operator to the entire document.
      // Yuck but un-avoidable.
      const allNodes = W.Array.from(W.document.all)
      nodesToConsider = allNodes.filter(_asHTMLElement) as HTMLElement[]
    }

    const numOperators = selector.length
    for (index; nodesToConsider.length > 0 && index < numOperators; ++index) {
      const operator = selector[index]
      const operatorFunc = operator.func
      const operatorType = operator.type

      // Note that we special case the :matches-path case here, since if
      // if it passes for one element, then it will pass for all elements.
      if (fastPathOperatorTypes.includes(operatorType)) {
        const firstNode = nodesToConsider[0]
        if (operatorFunc(firstNode).length === 0) {
          nodesToConsider = []
        }
        // Note that unless we've taken the if-true branch above, then
        // the nodesToConsider array will still have all the elements
        // it started with.
        break
      }

      let newNodesToConsider: HTMLElement[] = []
      for (const aNode of nodesToConsider) {
        const result = operatorFunc(aNode)
        newNodesToConsider = newNodesToConsider.concat(result)
      }
      nodesToConsider = newNodesToConsider
    }

    return nodesToConsider
  }

  const compileAndApplyProceduralSelector = (selector: ProceduralSelector,
                                             initElements: HTMLElement[]): HTMLElement[] => {
    const compiled = compileProceduralSelector(selector)
    return applyCompiledSelector(compiled, initElements)
  }

  return {
    applyCompiledSelector,
    compileProceduralSelector,
  }
})();
