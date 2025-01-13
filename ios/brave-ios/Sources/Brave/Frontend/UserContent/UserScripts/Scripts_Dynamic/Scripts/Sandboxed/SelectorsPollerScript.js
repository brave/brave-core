// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

window.__firefox__.execute(function($) {
  const args = $<args>;
  const proceduralFilters = $<procedural_filters>;
  const messageHandler = '$<message_handler>';
  const partinessMessageHandler = '$<partiness_message_handler>';
  const {
    applyCompiledSelector, compileProceduralSelector
  } = (function() {
    $<procedural_filters_script>
  })();

  /**
   * Send ids and classes to iOS and await new hide selectors
   * @param {Array} ids The ids found on this page
   * @param {Array} classes The classes found on this page
   * @returns A Promise that resolves new hide selectors
   */
  const sendSelectors = $((ids, classes) => {
    return $.postNativeMessage(messageHandler, {
      "securityToken": SECURITY_TOKEN,
      "data": {
        windowOrigin: $.windowOrigin,
        ids: ids,
        classes: classes
      }
    })
  })

  /**
   * Send new urls found on the page and return their partiness
   * @param {Array} urls The urls found on this page
   * @returns A Promise resolving to a dictionary base urls and their first party status
   */
  const getPartiness = $((urls) => {
    return $.postNativeMessage(partinessMessageHandler, {
      "securityToken": SECURITY_TOKEN,
      "data": {
        windowOrigin: $.windowOrigin,
        urls: urls,
      }
    })
  })

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
  // Elements that are not yet queried for selectors
  const notYetQueriedElements = []
  // The query to perform when extracting classes and ids
  const classIdWithoutHtmlOrBody = '[id]:not(html):not(body),[class]:not(html):not(body)'

  // Generate a random string between [a000000000, zzzzzzzzzz] (base 36)
  const generateRandomAttr = () => {
    const min = Number.parseInt('a000000000', 36)
    const max = Number.parseInt('zzzzzzzzzz', 36)
    return Math.floor(Math.random() * (max - min) + min).toString(36)
  }
  const globalStyleAttr = generateRandomAttr()
  const styleAttrMap = new Map()

  const CC = {
    allSelectors: new Set(),
    pendingSelectors: { ids: new Set(), classes: new Set() },
    alwaysHiddenSelectors: new Set(),
    hiddenSelectors: new Set(),
    unhiddenSelectors: new Set(),
    allStyleRules: [],
    runQueues: [
      // All new selectors go in this first run queue
      new Set(),
      // Third party matches go in the second and third queues.
      new Set(),
      // This is the final run queue.
      // It's only evaluated for 1p content one more time.
      new Set()
    ],
    // URLS
    pendingOrigins: new Set(),
    // A map of origin strings and their isFirstParty results
    urlFirstParty: new Map(),
    alreadyKnownFirstPartySubtrees: new WeakSet(),
    // All the procedural rules that exist and need to be processed
    // when the script is loaded and a new element is added
    proceduralActionFilters: undefined,
    // Tells us if procedural filtering is available
    hasProceduralActions: false,
  }

  /**
   * Send any new urls to the iOS subrutine so we can determine its partyness (1st or 3rd party)
   * @returns A Promise that returns if new party information was returned or no
   */
  const sendPendingOriginsIfNeeded = async () => {
    if (CC.pendingOrigins.size === 0) {
      return false
    }

    const origins = Array.from(CC.pendingOrigins)
    CC.pendingOrigins = new Set()
    const results = await getPartiness(origins)

    for (const origin of origins) {
      const isFirstParty = results[origin]
      if (isFirstParty !== undefined) {
        CC.urlFirstParty[origin] = isFirstParty
      } else {
        console.error(`Missing partiness for ${origin}`)
      }
    }

    return true
  }

  /**
   * Send any pending id and class selectors to iOS so we can determine hide selectors.
   */
  const sendPendingSelectorsIfNeeded = async () => {
    for (const elements of notYetQueriedElements) {
      for (const element of elements) {
        extractNewSelectors(element)
      }
    }
    notYetQueriedElements.length = 0

    if (CC.pendingSelectors.ids.size === 0 && CC.pendingSelectors.classes.size === 0) {
      return
    }

    const ids = Array.from(CC.pendingSelectors.ids)
    const classes = Array.from(CC.pendingSelectors.classes)
    CC.pendingSelectors.ids = new Set()
    CC.pendingSelectors.classes = new Set()

    let hasChanges = false
    const results = await sendSelectors(ids, classes)
    if (results.standardSelectors && results.standardSelectors.length > 0) {
      if (processHideSelectors(results.standardSelectors, !args.hideFirstPartyContent)) {
        hasChanges = true
      }
    }

    if (results.aggressiveSelectors && results.aggressiveSelectors.length > 0) {
      if (processHideSelectors(results.aggressiveSelectors, false)) {
        hasChanges = true
      }
    }

    if (!hasChanges) { return }
    setRulesOnStylesheetThrottled()

    if (!args.hideFirstPartyContent) {
      pumpCosmeticFilterQueuesOnIdle()
    }
  }

  /**
   * The timer for sending selectors to the browser
   */
  let sendPendingSelectorsTimerId

  /**
   * Send any pending id and class selectors to iOS so we can determine hide selectors.
   * Do this throttled so we don't send selectors too often.
   */
  const sendPendingSelectorsThrottled = () => {
    if (!args.fetchNewClassIdRulesThrottlingMs) {
      sendPendingSelectorsIfNeeded()
      return
    }

    // Ensure we are not already waiting on a timer
    if (sendPendingSelectorsTimerId) {
      // Each time this is called cancel the timer and allow a new one to start
      window.clearTimeout(sendPendingSelectorsTimerId)
    }

    sendPendingSelectorsTimerId = window.setTimeout(() => {
      sendPendingSelectorsIfNeeded()
      delete sendPendingSelectorsTimerId
    }, args.fetchNewClassIdRulesThrottlingMs)
  }

  /**
   * Extract any new id selector from the element
   * @param {object} element The element to extract from
   * @returns True or false indicating if anything was extracted
   */
  const extractIDSelectorIfNeeded = (element) => {
    const id = element.getAttribute('id')
    if (!id) { return false }
    if (typeof id !== 'string' && !(id instanceof String)) {
      return false
    }
    const selector = `#${id}`
    if (!CC.allSelectors.has(selector)) {
      CC.allSelectors.add(selector)
      CC.pendingSelectors.ids.add(id)
      return true
    } else {
      return false
    }
  }

  /**
   * Extract any new class selectors from the element
   * @param {object} element The element to extract from
   * @returns True or false indicating if anything was extracted
   */
  const extractClassSelectorsIfNeeded = (element) => {
    let hasNewSelectors = false

    for (const className of element.classList) {
      if (!className) { continue }
      const selector = `.${className}`
      if (!CC.allSelectors.has(selector)) {
        CC.pendingSelectors.classes.add(className)
        CC.allSelectors.add(selector)
        hasNewSelectors = true
      }
    }

    return hasNewSelectors
  }

  /**
   * Extract any selectors that are new (not yet seen) on the given element
   * @param {object} element The element to extract from
   * @returns True or false depending on if selectors were extracted
   */
  const extractNewSelectors = (element) => {
    if (element.hasAttribute === undefined) {
      return false
    }

    let hasNewSelectors = false
    if (element.hasAttribute('id')) {
      hasNewSelectors = extractIDSelectorIfNeeded(element)
    }

    if (extractClassSelectorsIfNeeded(element)) {
      hasNewSelectors = true
    }

    return hasNewSelectors
  }

  /**
   * Extract new urls form the given element
   * and returns a boolean indicating if the elemenent had a url
   * @param {object} element The element to extract from
   * @returns True or false indicating if the element had a url even if nothing was extracted.
   */
  const extractOriginIfNeeded = (element) => {
    // If we hide first party content we don't care to check urls for partiness.
    // Otherwise we need to have a src attribute.
    if (args.hideFirstPartyContent || element.hasAttribute === undefined || !element.hasAttribute('src')) {
      return false
    }

    const src = element.getAttribute('src')
    isFirstPartyURL(src)
    return true
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

  const isHTMLElement = (node) => {
    return ('innerText' in node)
  }

  const onMutations = (mutations, observer) => {
    const mutationScore = queueSelectorsFromMutations(mutations)

    if (mutationScore > 0) {
      sendPendingSelectorsThrottled()
    }

    // Check the conditions to switch to the alternative strategy
    // to get selectors.
    if (args.switchToSelectorsPollingThreshold !== undefined) {
      const now = performance.now()
      if (now > currentMutationStartTime + scoreCalcIntervalMs) {
        // Start the next time frame.
        currentMutationStartTime = now
        currentMutationScore = 0
      }
      currentMutationScore += mutationScore
      if (currentMutationScore > args.switchToSelectorsPollingThreshold) {
        usePolling(observer)
      }
    }

    if (CC.hasProceduralActions) {
      const addedElements = [];
      mutations.forEach(mutation =>
        mutation.addedNodes.length !== 0 && mutation.addedNodes.forEach(n =>
          n.nodeType === Node.ELEMENT_NODE && addedElements.push(n)
        )
      )
      if (addedElements.length !== 0) {
        executeProceduralActions(addedElements);
      }
    }
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
      observer.disconnect()
      notYetQueriedElements.length = 0
    }

    const futureTimeMs = window.Date.now() + returnToMutationObserverIntervalMs
    const queryAttrsFromDocumentBound = querySelectorsFromDocument.bind(undefined, futureTimeMs)
    selectorsPollingIntervalId = window.setInterval(queryAttrsFromDocumentBound, selectorsPollingIntervalMs)
  }

  const queueSelectorsFromMutations = (mutations) => {
    let mutationScore = 0
    for (const mutation of mutations) {
      const changedElm = mutation.target

      switch (mutation.type) {
        case 'attributes':
          // Since we're filtering for attribute modifications, we can be certain
          // that the targets are always HTMLElements, and never TextNode.
          switch (mutation.attributeName) {
            case 'class':
              mutationScore += changedElm.classList.length
              extractClassSelectorsIfNeeded(changedElm)
              break
            case 'id':
              mutationScore++
              extractIDSelectorIfNeeded(changedElm)
              break
          }
          break
        case 'childList':
          for (const node of mutation.addedNodes) {
            if (!isElement(node)) { continue }
            notYetQueriedElements.push([node])
            mutationScore += 1
            if (node.firstElementChild !== null) {
              const nodeList = node.querySelectorAll(classIdWithoutHtmlOrBody)
              notYetQueriedElements.push(nodeList)
              mutationScore += nodeList.length
            }
          }
      }
    }

    return mutationScore
  }

  /**
   * Tries to extract origin from the given URL string
   * @param {string} urlString The string to extract the origin from
   * @returns Origin string if found otherwise undefined
   */
  const extractOriginFromURLString = (urlString) => {
    try {
      const url = new URL(urlString, window.location.toString())
      return url.origin
    } catch (error) {
      console.error(error)
      return undefined
    }
  }

  /**
   * Determine the partiness (1st or 3rd party) of a given string.
   * The string should be an absolute or relative url.
   * @param {string} urlString The string to determine the partiness of
   * @returns True if the url is first party
   */
  const isFirstPartyURL = (urlString) => {
    if (isRelativeUrl(urlString)) {
      return true
    }

    const origin = extractOriginFromURLString(urlString)

    if (origin !== undefined) {
      const isFirstParty = CC.urlFirstParty[origin]

      if (isFirstParty === undefined) {
        CC.pendingOrigins.add(origin)
      }

      return isFirstParty
    } else {
      console.error(`Could not get origin from ${urlString}`)
      return false
    }
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
  const subTreePartyInfo = (elm, queryResult) => {
    queryResult = queryResult || {
      foundFirstPartyResource: false,
      foundThirdPartyResource: false,
      foundKnownThirdPartyAd: false,
      pendingSrcAttributes: []
    }

    if (elm.getAttribute) {
      if (elm.hasAttribute('id')) {
        const elmId = elm.getAttribute('id')
        if (elmId.startsWith('google_ads_iframe_') ||
          elmId.startsWith('div-gpt-ad') ||
          elmId.startsWith('adfox_')) {
          queryResult.foundKnownThirdPartyAd = true
          // Stop searching if we found a known 3rd party ad
          return queryResult
        }
      }
      if (elm.hasAttribute('src')) {
        const elmSrc = elm.getAttribute('src')
        const elmSrcIsFirstParty = isFirstPartyURL(elmSrc)
        if (elmSrcIsFirstParty === undefined) {
          queryResult.pendingSrcAttributes.push(elmSrc)
        } else if (elmSrcIsFirstParty) {
          queryResult.foundFirstPartyResource = true
          // Stop searching if we found a 1st party resource
          return queryResult
        } else {
          queryResult.foundThirdPartyResource = true
        }
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

    const subElms = []
    if (elm.firstChild) {
      subElms.push(elm.firstChild)
    }
    if (elm.nextSibling) {
      subElms.push(elm.nextSibling)
    }

    for (const subElm of subElms) {
      subTreePartyInfo(subElm, queryResult)

      if (queryResult.foundKnownThirdPartyAd) {
        // We stop if we found a known 3rd party ad.
        return queryResult
      } else if (queryResult.foundFirstPartyResource) {
        // We stop if we found a first party resource.
        return queryResult
      }
    }

    return queryResult
  }

  const shouldUnhideElement = (element, pendingSrcAttributes) => {
    const queryResults = subTreePartyInfo(element)

    if (queryResults.foundKnownThirdPartyAd) {
      return false
    } else if (queryResults.foundFirstPartyResource) {
      return true
    } else if (showsSignificantText(element)) {
      // If the subtree HAS a significant amount of text (e.g., it doesn't just says "Advertisement")
      // it should be unhidden.
      return true
    } else if (queryResults.foundThirdPartyResource || queryResults.pendingSrcAttributes.size > 0) {
      if (pendingSrcAttributes !== undefined) {
        queryResults.pendingSrcAttributes.forEach((src) => {
          pendingSrcAttributes.push(src)
        })
      }

      return false
    }
    return false
  }

  const shouldUnhideElementAsync = async (element) => {
    const pendingSrcAttributes = []
    const shouldUnhide = shouldUnhideElement(element, pendingSrcAttributes)

    if (shouldUnhide) {
      return true
    } else if (pendingSrcAttributes.length > 0) {
      // If we are missing some url party information
      // Fetch it and check the urls again
      await sendPendingOriginsIfNeeded()

      // Find the first 1p src.
      // This is enough to be a 1st party element
      for (const src of pendingSrcAttributes) {
        if (isFirstPartyURL(src)) {
          return true
        }
      }
    } else {
      return false
    }
  }

  /**
   * Unhide the given selectors.
   * (i.e. Remove them from CC.hiddenSelectors and move them to CC.unhiddenSelectors)
   * This will not recreate the stylesheet
   * @param {Set} selectors The selectors to unhide
   */
  const unhideSelectors = (selectors) => {
    if (selectors.size === 0) { return }

    Array.from(selectors).forEach((selector) => {
      if (CC.unhiddenSelectors.has(selector)) { return }
      CC.hiddenSelectors.delete(selector)
      CC.unhiddenSelectors.add(selector)

      // Remove these selectors from the run queues
      for (let index = 0; index < CC.runQueues; index++) {
        CC.runQueues[index].delete(selector)
      }
    })
  }

  const pumpIntervalMinMs = 40
  const pumpIntervalMaxMs = 100
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
  const pumpCosmeticFilterQueuesOnIdle = idleize(async () => {
    if (queueIsSleeping) { return }
    let didPumpAnything = false
    // For each "pump", walk through each queue until we find selectors
    // to evaluate. This means that nothing in queue N+1 will be evaluated
    // until queue N is completely empty.
    for (let queueIndex = 0; queueIndex < CC.runQueues.length; queueIndex += 1) {
      const currentQueue = CC.runQueues[queueIndex]
      const nextQueue = CC.runQueues[queueIndex + 1]
      if (currentQueue.size === 0) { continue }

      const currentWorkLoad = Array.from(currentQueue.values()).slice(0, maxWorkSize)
      const comboSelector = currentWorkLoad.join(',')
      const matchingElms = document.querySelectorAll(comboSelector)

      for (const aMatchingElm of matchingElms) {
        // Don't recheck elements / subtrees we already know are first party.
        // Once we know something is third party, we never need to evaluate it
        // again.
        if (CC.alreadyKnownFirstPartySubtrees.has(aMatchingElm)) {
          continue
        }

        const shouldUnhide = await shouldUnhideElementAsync(aMatchingElm)

        if (!shouldUnhide) {
          // If we don't unhide this element, we skip it
          continue
        }

        // Otherwise, we know that the given subtree should be unhidden.
        // So we need to figure out which selector from the combo selector did the matching.
        for (const selector of currentWorkLoad) {
          if (!aMatchingElm.matches(selector)) {
            continue
          }

          // Unhide this selector if we need to
          if (CC.hiddenSelectors.has(selector) || !CC.unhiddenSelectors.has(selector)) {
            CC.unhiddenSelectors.add(selector)
            CC.hiddenSelectors.delete(selector)
          }
        }

        CC.alreadyKnownFirstPartySubtrees.add(aMatchingElm)
      }

      for (const selector of currentWorkLoad) {
        currentQueue.delete(selector)

        // Don't requeue selectors we know identify first party content.
        if (nextQueue && !CC.unhiddenSelectors.has(selector)) {
          nextQueue.add(selector)
        }
      }

      didPumpAnything = true
      // If we did something, process the next queue, save it for next time.
      break
    }

    if (!didPumpAnything) { return }
    queueIsSleeping = true

    await sendPendingOriginsIfNeeded()
    setRulesOnStylesheetThrottled()

    window.setTimeout(() => {
      // Set this to false now even though there's a gap in time between now and
      // idle since all other calls to `pumpCosmeticFilterQueuesOnIdle` that occur during this time
      // will be ignored (and nothing else should be calling `pumpCosmeticFilterQueues` straight).
      queueIsSleeping = false
      pumpCosmeticFilterQueuesOnIdle()
    }, pumpIntervalMinMs)
  }, pumpIntervalMaxMs)

  /**
   * Extract any selectors from the document
   * @param {object} element The element to extract the selectors from
   */
  const querySelectorsFromElement = (element) => {
    extractNewSelectors(element)
    const elmWithClassOrId = element.querySelectorAll(classIdWithoutHtmlOrBody)

    elmWithClassOrId.forEach((node) => {
      extractNewSelectors(node)
    })
  }

  /**
   * Extract any selectors from the document
   * @param {number} switchToMutationObserverAtTime A timestamp indicating when we shoudl switch back to mutation observer
   */
  const querySelectorsFromDocument = (switchToMutationObserverAtTime) => {
    querySelectorsFromElement(document)

    // Send any found selectors to the browser
    sendPendingSelectorsThrottled()

    if (switchToMutationObserverAtTime !== undefined &&
      window.Date.now() >= switchToMutationObserverAtTime) {
      useMutationObserver()
    }
  }

  /**
   * First it will query the document for selectors (ids and classes)
   * Then it will begin the selectors mutation observer
   */
  const startPollingSelectors = async () => {
    // First queue up any classes and ids that exist before the mutation observer
    // starts running.
    querySelectorsFromElement(document)

    // Send any found selectors to the browser and await the results
    await sendPendingSelectorsIfNeeded()

    // Second, set up a mutation observer to handle any new ids or classes
    // that are added to the document.
    useMutationObserver()
  }

  /**
   * Will start the selector polling and pumping depending on the provided booleans
   * @param {boolean} hide1pContent If this is false, it will start the pump mechanism
   * @param {boolean} genericHide If this is false, it will start the polling mechanism
   */
  const scheduleQueuePump = (hide1pContent, genericHide) => {
    if (!genericHide) {
      if (args.firstSelectorsPollingDelayMs === undefined) {
        startPollingSelectors()
      } else {
        window.setTimeout(startPollingSelectors, args.firstSelectorsPollingDelayMs)
      }
    }

    if (!hide1pContent) {
      pumpCosmeticFilterQueuesOnIdle()
    }
  }

  /**
   * Return all the selectors that are hidden for this element.
   * @param {object} element The element to search for hidden selectors on
   * @returns A Set containing all the hidden selectors this element uses
   */
  const hiddenSelectorsForElement = (element) => {
    if (element.hasAttribute === undefined) {
      return []
    }

    return Array.from(CC.hiddenSelectors).filter((selector) => {
      try {
        return element.matches(selector)
      } catch (error) {
        // Remove the culprit from everywhere so it doesn't cause errors
        CC.hiddenSelectors.delete(selector)
        CC.unhiddenSelectors.add(selector)

        for (let queueIndex = 0; queueIndex < CC.runQueues.length; queueIndex += 1) {
          CC.runQueues[queueIndex]
          CC.runQueues[queueIndex].delete(selector)
        }
        return false
      }
    })
  }

  /**
   * Unhide any selectors matching this  if it is a 1st party element
   * @param {object} element The node to attempt to unhide
   * @returns An array of unhid selectors if element is 1st party otherwise undefined.
   */
  const unhideSelectorsMatchingElementIf1P = (element) => {
    const selectors = hiddenSelectorsForElement(element)
    if (selectors.length === 0) { return }
    const shouldUnhide = shouldUnhideElement(element)

    if (!shouldUnhide) { return }
    unhideSelectors(selectors)
    return selectors
  }

  /**
   * This method will attempt to unhide the node or any of its parent nodes recursively
   * and return any unhidden selectors.
   * The recursion stops when it hits the document.body
   * @param {object} node The node to attempt to undhide recursively
   */
  const unhideSelectorsMatchingElementAndItsParents = (node) => {
    const unhiddenSelectors = unhideSelectorsMatchingElementIf1P(node) || []

    if (node.parentElement && node.parentElement !== document.body) {
      const newSelectors = unhideSelectorsMatchingElementAndItsParents(node.parentElement)

      for (const selector of newSelectors) {
        unhiddenSelectors.push(selector)
      }
    }

    return unhiddenSelectors
  }

  /**
   * This method will unhide the node an all parent nodes if they are needed.
   * This will move up each parent for the each node until it reaches the document body
   * @param {Array} nodes Array of WeakRef nodes
   * @returns A list of unhidden selectors
   */
  const unhideSelectorsMatchingElementsAndTheirParents = (nodes) => {
    const selectorsUnHidden = new Set()

    for (const nodeRef of nodes) {
      const node = nodeRef.deref()
      if (node === undefined) { return }
      const newSelectors = unhideSelectorsMatchingElementAndItsParents(node)

      for (const selector in newSelectors) {
        selectorsUnHidden.add(selector)
      }
    }

    return selectorsUnHidden.size > 0
  }

  /**
   * Handle the results of the urls mutation observer
   * It will:
   * 1. Extract any urls from the mutations and add them to pendingURLs
   * 2. Send them to iOS for 1st party analysis
   * 3. Unhide any elements (or their parents) that are 1st party
   * @param {*} mutations
   * @param {MutationObserver} observer
   */
  const onURLMutations = async (mutations, observer) => {
    const elementsWithURLs = []
    mutations.forEach((mutation) => {
      if (mutation.type === 'attributes') {
        // Since we're filtering for attribute modifications, we can be certain
        // that the targets are always HTMLElements, and never TextNode.
        const changedElm = mutation.target
        switch (mutation.attributeName) {
          case 'src':
            if (extractOriginIfNeeded(changedElm)) {
              elementsWithURLs.push(new WeakRef(changedElm))
            }
            break
        }
      } else if (mutation.addedNodes.length > 0) {
        for (const node of mutation.addedNodes) {
          if (!isElement(node)) { continue }
          if (extractOriginIfNeeded(node)) {
            elementsWithURLs.push(new WeakRef(node))
          }
        }
      }
    })

    // Send any new urls to iOS and await the results
    const changes = await sendPendingOriginsIfNeeded()
    if (!changes) { return }

    // If we have some new values, we want to unhide any new selectors
    unhideSelectorsMatchingElementIf1P(elementsWithURLs)
    setRulesOnStylesheetThrottled()
  }

  /**
   * Start a mutation observer that finds new urls.
   * This should only be used if 1st party ads are not hidden
   */
  const startURLMutationObserver = () => {
    if (selectorsPollingIntervalId) {
      clearInterval(selectorsPollingIntervalId)
      selectorsPollingIntervalId = undefined
    }

    const observer = new MutationObserver(onURLMutations)

    const observerConfig = {
      subtree: true,
      childList: true,
      attributeFilter: ['src']
    }

    observer.observe(document.body, observerConfig)
  }

  /**
   * Query the given element for urls and add them to the pending Set
   * @param {object} element The element to query on
   * @returns an array of elements that have urls
   */
  const queryURLOriginsInElement = (element) => {
    const possibleAdChildNodes = []
    const elmWithClassOrId = element.querySelectorAll('[src]')

    elmWithClassOrId.forEach((node) => {
      if (extractOriginIfNeeded(node)) {
        possibleAdChildNodes.push(new WeakRef(node))
      }
    })

    return possibleAdChildNodes
  }

  /**
   * Adds given selectors to hiddenSelectors unless they are in the unhiddenSelectors set
   * @param {*} selectors The selectors to add
   */
  const processHideSelectors = (selectors, canUnhide1PElements) => {
    let hasChanges = false

    selectors.forEach(selector => {
      if ((typeof selector === 'string') && !CC.unhiddenSelectors.has(selector)) {
        if (canUnhide1PElements) {
          if (CC.hiddenSelectors.has(selector)) { return }
          CC.hiddenSelectors.add(selector)
          CC.runQueues[0].add(selector)
          hasChanges = true
        } else {
          if (CC.alwaysHiddenSelectors.has(selector)) { return }
          CC.alwaysHiddenSelectors.add(selector)
          hasChanges = true
        }
      }
    })

    return hasChanges
  }

  /// Moves the stylesheet to the bottom of the page
  const moveStyle = () => {
    const styleElm = CC.cosmeticStyleSheet
    const targetElm = document.body
    styleElm.parentElement.removeChild(styleElm)
    targetElm.appendChild(styleElm)
  }

  /**
   * Create a stylesheet and append it to the bottom of the body element.
   * The stylesheet is stored on `cosmeticStyleSheet` for future reference.
   */
  const createStylesheet = () => {
    const targetElm = document.body
    const styleElm = document.createElement('style')
    styleElm.setAttribute('type', 'text/css')
    targetElm.appendChild(styleElm)
    CC.cosmeticStyleSheet = styleElm

    // Start a timer that moves the stylesheet down
    window.setInterval(() => {
      if (styleElm.nextElementSibling === null || styleElm.parentElement !== targetElm) {
        return
      }
      // `darkreader` (from our night mode) fights with us to be last element
      // in `document.body`. Repeatedly moving our stylesheet can cause
      // unwanted animations to repeat every time we move the stylesheet
      const nextElementClass = styleElm.getAttribute("class");
      if (nextElementClass.includes("darkreader")) {
        return;
      }
      moveStyle()
    }, 1000)
  }

  /**
   * Rewrite the stylesheet body with the hidden rules and style rules
   */
  const setRulesOnStylesheet = () => {
    const hideRules = Array.from(CC.hiddenSelectors).map(selector => {
      return selector + '{display:none !important;}'
    })

    const alwaysHideRules = Array.from(CC.alwaysHiddenSelectors).map(selector => {
      return selector + '{display:none !important;}'
    })

    const allRules = alwaysHideRules.concat(hideRules.concat(CC.allStyleRules))
    const ruleText = allRules.filter(rule => {
      return rule !== undefined && !rule.startsWith(':')
    }).join('')

    CC.cosmeticStyleSheet.innerText = ruleText
  }

  /**
   * The timer id for throttling setRulesOnStylesheet
   */
  let setRulesTimerId

  /**
   * This method only allows a single setRulesOnStylesheet to be applied.
   * This is an optimaization so we don't constantly re-apply rules
   * for each small change that may happen simultaneously.
   */
  const setRulesOnStylesheetThrottled = () => {
    if (setRulesTimerId) {
      // Each time this is called cancell the timer and allow a new one to start
      window.clearTimeout(setRulesTimerId)
    }

    setRulesTimerId = window.setTimeout(() => {
      setRulesOnStylesheet()
      delete setRulesTimerId
    }, 200)
  }

  /**
   * Start polling the page for content and start the queue pump (if needed)
   * If no polling is needed, simply create the stylesheet.
   */
  const startPolling = async () => {
    // Ensure the body is not editable
    if (document.body.contentEditable === "true") {
      // If we have this attribute, we don't want to use CF
      return
    }
    // 1. First create the stylesheet
    createStylesheet()

    if (!args.hideFirstPartyContent) {
      // 2. Collect any origins from the document body and fetch their 1p statuses
      const nodesWithExtractedURLs = queryURLOriginsInElement(document.body)
      await sendPendingOriginsIfNeeded()
      // 3. Unhide any elements (or their parents) we know have urls
      unhideSelectorsMatchingElementsAndTheirParents(nodesWithExtractedURLs)
    }

    // 4. Perform procedural actions before setting stylesheet
    if (CC.hasProceduralActions) {
      executeProceduralActions()
    }

    // 5. Set the rules on the stylesheet. So far we don't have any rules set
    // We could do this earlier but it causes elements to hide and unhide
    // It's best to wait to this period so we have
    setRulesOnStylesheet()
    // 6. Start our queue pump if we need to
    scheduleQueuePump(args.hideFirstPartyContent, args.genericHide)

    if (!args.hideFirstPartyContent) {
      // 7. Start listening to new urls
      startURLMutationObserver()
    }
  }

  /// Process any :style, :remove, :remove-attr, or :remove-class selectors
  const executeProceduralActions = (added) => {
    // If passed a list of added elements, do not query the entire document
    if (CC.proceduralActionFilters === undefined) {
      return
    }

    // Returns attribute from the `styleAttrMap`, or
    // creates new attribute and adds to `styleAttrMap`
    const getStyleAttr = (style) => {
      let styleAttr = styleAttrMap.get(style)
      if (styleAttr === undefined) {
        styleAttr = generateRandomAttr()
        styleAttrMap.set(style, styleAttr);
        const css = `[${globalStyleAttr}][${styleAttr}]{${style}}`
        CC.allStyleRules.push(css)
      }
      return styleAttr
    }

    const performAction = (element, action) => {
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
      let matchingElements = [];
      let startOperator = 0;

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
        matchingElements.forEach(elem => {
          performAction(elem, action);
        });
      } else {
        try {
          const filter = compileProceduralSelector(selector.slice(startOperator));
          applyCompiledSelector(filter, matchingElements).forEach(elem => {
            performAction(elem, action);
          });
        } catch (e) {
          console.error('Failed to apply filter ' + JSON.stringify(selector) + ' ' + JSON.stringify(action) + ': ');
          console.error(e.message);
          console.error(e.stack);
        }
      }
    }
  };

  const waitForBody = () => {
    if (document.body) {
      // we can start right away
      startPolling()
      return
    }

    // Wait until document body is ready
    const timerId = window.setInterval(() => {
      if (!document.body) {
        // we need to wait longer.
        return
      }

      // Body is ready, kill this interval and create the stylesheet
      window.clearInterval(timerId)
      startPolling()
    }, 500)
  }

  // Load some static hide rules if they are defined
  if (args.standardSelectors) {
    processHideSelectors(args.standardSelectors, !args.hideFirstPartyContent)
  }

  if (args.aggressiveSelectors) {
    processHideSelectors(args.aggressiveSelectors, false)
  }

  if (proceduralFilters && proceduralFilters.length > 0) {
    CC.proceduralActionFilters = proceduralFilters
    CC.hasProceduralActions = true
  }

  window.setTimeout(waitForBody, maxTimeMSBeforeStart)
});
