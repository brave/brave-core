// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

window.__firefox__.execute(function($) {
  const args = $<args>
  const messageHandler = '$<message_handler>';
  
  const sendMessage = $((ids, classes) => {
    return $.postNativeMessage(messageHandler, {
      "securityToken": SECURITY_TOKEN,
      "data": {
        sourceURL: window.location.href,
        ids: ids,
        classes: classes
      }
    });
  });
  
  // Start looking for things to unhide before at most this long after
  // the backend script is up and connected (eg backgroundReady = true),
  // or sooner if the thread is idle.
  const maxTimeMSBeforeStart = 1000
  // The cutoff for text ads.  If something has only text in it, it needs to have
  // this many, or more, characters.  Similarly, require it to have a non-trivial
  // number of words in it, to look like an actual text ad.
  const minAdTextChars = 30
  const minAdTextWords = 5
  const returnToMutationObserverIntervalMs = 10000
  const selectorsPollingIntervalMs = 500
  let selectorsPollingIntervalId
  // The number of potentially new selectors that are processed during the last
  // |scoreCalcIntervalMs|.
  let currentMutationScore = 0
  // The time frame used to calc |currentMutationScore|.
  const scoreCalcIntervalMs = 1000
  // The begin of the time frame to calc |currentMutationScore|.
  let currentMutationStartTime = performance.now()
  // The next allowed time to call FetchNewClassIdRules() if it's throttled.
  let nextFetchNewClassIdRulesCall = 0
  let fetchNewClassIdRulesTimeoutId
  const queriedIds = new Set()
  const queriedClasses = new Set()
  // Each of these get setup once the mutation observer starts running.
  let notYetQueriedClasses = []
  let notYetQueriedIds = []
  
  const CC = {}
  CC.allHideSelectorsToRules = new Map()
  CC.allRules = []
  CC.observingHasStarted = false
  // All new selectors go in `firstRunQueue`
  CC.firstRunQueue = new Set()
  // Third party matches go in the second and third queues.
  CC.secondRunQueue = new Set()
  // Once a selector gets in to this queue, it's only evaluated for 1p content one
  // more time.
  CC.finalRunQueue = new Set()
  CC.allQueues = [
    CC.firstRunQueue, CC.secondRunQueue, CC.finalRunQueue
  ]
  CC.numQueues = CC.allQueues.length
  CC.alreadyUnhiddenSelectors = new Set()
  CC.alreadyKnownFirstPartySubtrees = new WeakSet()
  CC._hasDelayOcurred = false
  CC._startCheckingId = undefined
  CC.firstSelectorsPollingDelayMs = undefined
  CC.switchToSelectorsPollingThreshold = undefined
  CC.fetchNewClassIdRulesThrottlingMs = undefined
  
  // The passed configuration to this script
  CC.hide1pContent = args.hideFirstPartyContent
  CC.generichide = args.genericHide
  
  if (args.firstSelectorsPollingDelayMs) {
    CC.firstSelectorsPollingDelayMs = args.firstSelectorsPollingDelayMs
  }
  
  if (args.switchToSelectorsPollingThreshold) {
    CC.switchToSelectorsPollingThreshold = args.switchToSelectorsPollingThreshold
  }
  
  if (args.fetchNewClassIdRulesThrottlingMs) {
    CC.fetchNewClassIdRulesThrottlingMs = args.fetchNewClassIdRulesThrottlingMs
  }
  
  /**
   * Provides a new function which can only be scheduled once at a time.
   *
   * @param onIdle function to run when the thread is less busy
   * @param timeout max time to wait. at or after this time the function will be run regardless of thread noise
   */
  const idleize = (onIdle, timeout) => {
    let idleId
    return function WillRunOnIdle () {
      if (idleId !== undefined) {
        return
      }
      idleId = window.setTimeout(() => {
        idleId = undefined
        onIdle()
      }, timeout)
    }
  }

  const isRelativeUrl = (url) => {
    return (!url.startsWith('//') &&
      !url.startsWith('http://') &&
      !url.startsWith('https://'))
  }

  const isElement = (node) => {
    return (node.nodeType === 1)
  }

  const asElement = (node) => {
    return isElement(node) ? node : null
  }

  const isHTMLElement = (node) => {
    return ('innerText' in node)
  }

  // The fetchNewClassIdRules() can be called of each MutationObserver event.
  // Under the hood it makes a lot of work: call to C++ => IPC to the browser
  // process => request to the rust CS engine and back.
  // So limit the number of calls to one per fetchNewClassIdRulesThrottlingMs.
  const ShouldThrottleFetchNewClassIdsRules = () => {
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
        window.setTimeout(function () {
          fetchNewClassIdRulesTimeoutId = undefined
          fetchNewClassIdRules()
        }, msToWait)
      return true
    }
    nextFetchNewClassIdRulesCall =
      now + CC.fetchNewClassIdRulesThrottlingMs
    return false
  }
  
  /// Takes selectors and adds them to the style sheet
  const processHideSelectors = (selectors) => {
    selectors.forEach(selector => {
      if ((typeof selector === 'string') && (CC.hide1pContent || !CC.allHideSelectorsToRules.has(selector))) {
        const rule = selector + '{display:none !important;}'
        CC.allRules.push(rule)
        
        if (!CC.hide1pContent) {
          CC.allHideSelectorsToRules.set(selector, CC.allRules.length - 1)
          CC.firstRunQueue.add(selector)
        }
      }
    })
  }
  
  /// Takes selectors and adds them to the style sheet
  const processStyleSelectors = (styleSelectors) => {
    styleSelectors.forEach(entry => {
      let rule = entry.selector + '{' + entry.rules.join(';') + ';}'
      CC.allRules.push(rule)
    })
  }
  
  /// Moves the stylesheet to the bottom of the page
  const moveStyle = () => {
    const styleElm = CC.cosmeticStyleSheet
    const targetElm = document.body
    styleElm.parentElement.removeChild(styleElm)
    targetElm.appendChild(styleElm)
  }
  
  const setRulesOnStylesheet = () => {
    const ruleText = CC.allRules.filter(rule => {
      return rule !== undefined && !rule.startsWith(':')
    }).join('')
    CC.cosmeticStyleSheet.innerText = ruleText
  }

  const fetchNewClassIdRules = () => {
    if ((!notYetQueriedClasses || notYetQueriedClasses.length === 0) &&
      (!notYetQueriedIds || notYetQueriedIds.length === 0)) {
      return
    }
    
    sendMessage(notYetQueriedIds, notYetQueriedClasses).then(selectors => {
      if (!selectors) { return }
      processHideSelectors(selectors)
      setRulesOnStylesheet()
    })
    
    notYetQueriedClasses = []
    notYetQueriedIds = []
  }

  const useMutationObserver = () => {
    if (selectorsPollingIntervalId) {
      clearInterval(selectorsPollingIntervalId)
      selectorsPollingIntervalId = undefined
    }
    
    const observer = new MutationObserver(onMutations)
    
    const observerConfig = {
      subtree: true,
      childList: true,
      attributeFilter: ['id', 'class']
    }
    
    observer.observe(document.documentElement, observerConfig)
  }

  const usePolling = (observer) => {
    if (observer) {
      const mutations = observer.takeRecords()
      observer.disconnect()
      
      if (mutations) {
        queueAttrsFromMutations(mutations)
      }
    }
    
    const futureTimeMs = window.Date.now() + returnToMutationObserverIntervalMs
    const queryAttrsFromDocumentBound = queryAttrsFromDocument.bind(undefined, futureTimeMs)
    selectorsPollingIntervalId = window.setInterval(queryAttrsFromDocumentBound,
                                                       selectorsPollingIntervalMs)
  }

  const queueAttrsFromMutations = (mutations) => {
    let mutationScore = 0
    for (let _i = 0, mutations1 = mutations; _i < mutations1.length; _i++) {
      const aMutation = mutations1[_i]
      if (aMutation.type === 'attributes') {
        // Since we're filtering for attribute modifications, we can be certain
        // that the targets are always HTMLElements, and never TextNode.
        const changedElm = aMutation.target
        switch (aMutation.attributeName) {
          case 'class':
            mutationScore += changedElm.classList.length
            for (let _b = 0, _c = changedElm.classList; _b < _c.length; _b++) {
              const aClassName = _c[_b]
              if (!queriedClasses.has(aClassName)) {
                notYetQueriedClasses.push(aClassName)
                queriedClasses.add(aClassName)
              }
            }
            break
          case 'id':
            mutationScore++
            if (!queriedIds.has(changedElm.id)) {
              notYetQueriedIds.push(changedElm.id)
              queriedIds.add(changedElm.id)
            }
            break
        }
      } else if (aMutation.addedNodes.length > 0) {
        for (let _d = 0, _e = aMutation.addedNodes; _d < _e.length; _d++) {
          const node = _e[_d]
          const element = asElement(node)
          if (!element) {
            continue
          }
          mutationScore++
          const id = element.id
          if (id && !queriedIds.has(id)) {
            notYetQueriedIds.push(id)
            queriedIds.add(id)
          }
          const classList = element.classList
          if (classList) {
            mutationScore += classList.length
            for (let _f = 0, _g = classList; _f < _g.length; _f++) {
              const className = _g[_f]
              if (className && !queriedClasses.has(className)) {
                notYetQueriedClasses.push(className)
                queriedClasses.add(className)
              }
            }
          }
        }
      }
    }
    
    return mutationScore
  }
  
  const onMutations = (mutations, observer) => {
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
    if (!ShouldThrottleFetchNewClassIdsRules()) {
      fetchNewClassIdRules()
    }
  }

  const isFirstPartyUrl = (url) => {
    if (isRelativeUrl(url)) {
      return true
    }
    
    // TODO: @JS Handle first party for more control over cosmetic filtering agressivness
    // For now we return false. The original source code was getting this from C++ but we cannot
    // Do this easily in iOS so we will have to get creative here if we ever implement this.
    // Perhaps we can do a close enough js implementation.
    return false
  }

  const stripChildTagsFromText = (elm, tagName, text) => {
    const childElms = Array.from(elm.getElementsByTagName(tagName))
    let localText = text
    for (let _i = 0, childElms1 = childElms; _i < childElms1.length; _i++) {
      const anElm = childElms1[_i]
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
  const showsSignificantText = (elm) => {
    if (!isHTMLElement(elm)) {
      return false
    }
    const htmlElm = elm
    const tagsTextToIgnore = ['script', 'style']
    let currentText = htmlElm.innerText
    for (let _i = 0, tagsTextToIgnore1 = tagsTextToIgnore; _i < tagsTextToIgnore1.length; _i++) {
      const aTagName = tagsTextToIgnore1[_i]
      currentText = stripChildTagsFromText(htmlElm, aTagName, currentText)
    }
    const trimmedText = currentText.trim()
    if (trimmedText.length < minAdTextChars) {
      return false
    }
    let wordCount = 0
    for (let _a = 0, _b = trimmedText.split(' '); _a < _b.length; _a++) {
      const aWord = _b[_a]
      if (aWord.trim().length === 0) {
        continue
      }
      wordCount += 1
    }
    return wordCount >= minAdTextWords
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
  const isSubTreeFirstParty = (elm, possibleQueryResult) => {
    let queryResult
    let isTopLevel
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
        const elmId = elm.getAttribute('id')
        if (elmId.startsWith('google_ads_iframe_') ||
          elmId.startsWith('div-gpt-ad') ||
          elmId.startsWith('adfox_')) {
          queryResult.foundKnownThirdPartyAd = true
          return false
        }
      }
      if (elm.hasAttribute('src')) {
        const elmSrc = elm.getAttribute('src')
        const elmSrcIsFirstParty = isFirstPartyUrl(elmSrc)
        if (elmSrcIsFirstParty) {
          queryResult.foundFirstPartyResource = true
          return true
        }
        queryResult.foundThirdPartyResource = true
      }
      if (elm.hasAttribute('style')) {
        const elmStyle = elm.getAttribute('style')
        if (elmStyle.includes('url(') ||
          elmStyle.includes('//')) {
          queryResult.foundThirdPartyResource = true
        }
      }
      if (elm.hasAttribute('srcdoc')) {
        const elmSrcDoc = elm.getAttribute('srcdoc')
        if (elmSrcDoc.trim() === '') {
          queryResult.foundThirdPartyResource = true
        }
      }
    }
    if (elm.firstChild) {
      isSubTreeFirstParty(elm.firstChild, queryResult)
      if (queryResult.foundKnownThirdPartyAd) {
        return false
      }
      if (queryResult.foundFirstPartyResource) {
        return true
      }
    }
    if (elm.nextSibling) {
      isSubTreeFirstParty(elm.nextSibling, queryResult)
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

  const unhideSelectors = (selectors) => {
    if (selectors.size === 0) {
      return
    }
    
    Array.from(selectors).forEach((selector) => {
      const index = CC.allHideSelectorsToRules[selector]
      
      if (index !== undefined) {
        CC.allHideSelectorsToRules.delete(selector)
        delete CC.allRules[index]
      }
    })
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
    if (queueIsSleeping) { return }
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
      const newlyIdentifiedFirstPartySelectors = new Set()
      for (let _i = 0, matchingElms1 = matchingElms; _i < matchingElms1.length; _i++) {
        const aMatchingElm = matchingElms1[_i]
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
        if (!elmSubtreeIsFirstParty) {
          continue
        }
        // If the subtree doesn't have a significant amount of text (e.g., it
        // just says "Advertisement"), then no need to change anything; it should
        // stay hidden.
        if (!showsSignificantText(aMatchingElm)) {
          continue
        }
        // Otherwise, we know that the given subtree was evaluated to be
        // first party, so we need to figure out which selector from the combo
        // selector did the matching.
        for (let _a = 0, currentWorkLoad1 = currentWorkLoad; _a < currentWorkLoad1.length; _a++) {
          const selector = currentWorkLoad1[_a]
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
      for (let _b = 0, currentWorkLoad2 = currentWorkLoad; _b < currentWorkLoad2.length; _b++) {
        const aUsedSelector = currentWorkLoad2[_b]
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

  const pumpCosmeticFilterQueuesOnIdle = idleize(pumpCosmeticFilterQueues, pumpIntervalMaxMs)

  const queryAttrsFromDocument = (switchToMutationObserverAtTime) => {
    const elmWithClassOrId = document.querySelectorAll('[class],[id]')

    for (let _i = 0, elmWithClassOrId1 = elmWithClassOrId; _i < elmWithClassOrId1.length; _i++) {
      const elm = elmWithClassOrId1[_i]
      for (let _b = 0, _c = elm.classList; _b < _c.length; _b++) {
        const aClassName = _c[_b]

        if (aClassName && !queriedClasses.has(aClassName)) {
          notYetQueriedClasses.push(aClassName)
          queriedClasses.add(aClassName)
        }
      }
      const elmId = elm.getAttribute('id')
      if (elmId && !queriedIds.has(elmId)) {
        notYetQueriedIds.push(elmId)
        queriedIds.add(elmId)
      }
    }
    fetchNewClassIdRules()
    
    if (switchToMutationObserverAtTime !== undefined
        && window.Date.now() >= switchToMutationObserverAtTime) {
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

  const scheduleQueuePump = (hide1pContent, genericHide) => {
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
    CC._startCheckingId = setTimeout(_ => {
      CC._hasDelayOcurred = true
      if (!genericHide) {
        if (CC.firstSelectorsPollingDelayMs === undefined) {
          startObserving()
        } else {
          window.setTimeout(startObserving, CC.firstSelectorsPollingDelayMs)
        }
      }
      if (!hide1pContent) {
        pumpCosmeticFilterQueuesOnIdle()
      }
    }, maxTimeMSBeforeStart)
  }

  const tryScheduleQueuePump = () => {
    if (!CC.observingHasStarted) {
      CC.observingHasStarted = true
      scheduleQueuePump(CC.hide1pContent, CC.generichide)
    } else {
      scheduleQueuePump(false, false)
    }
  }
  
  // Load some static hide rules if they are defined
  if (args.hideSelectors) {
    processHideSelectors(args.hideSelectors)
  }
  
  // Load some static style selectors if they are defined
  if (args.styleSelectors) {
    processStyleSelectors(args.styleSelectors)
  }
  
  // Wait until document body is ready
  const timerId = window.setInterval(() => {
    if (!document.body) {
      // we need to wait longer.
      return
    }
    
    // Body is ready, kill this interval and create the stylesheet
    window.clearInterval(timerId)
    const targetElm = document.body
    const styleElm = document.createElement('style')
    styleElm.setAttribute('type', 'text/css')
    targetElm.appendChild(styleElm)
    CC.cosmeticStyleSheet = styleElm
    setRulesOnStylesheet()
    
    // Start listening to new selectors
    tryScheduleQueuePump()
    
    // Start a timer that moves the stylesheet down
    setInterval(() => {
      if (styleElm.nextElementSibling === null && styleElm.parentElement === targetElm) {
        return
      }
      moveStyle()
    }, 1000)
  }, 500)
});
