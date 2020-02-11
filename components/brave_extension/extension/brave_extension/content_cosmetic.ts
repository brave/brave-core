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

// Don't start looking for things to unblock until at least this long after
// the backend script is up and connected (eg backgroundReady = true)
const numMSBeforeStart = 2500
// Only let the mutation observer run for this long  After this point, the
// queues are frozen and the mutation observer will stop / disconnect.
const numMSCheckFor = 15000

const queriedIds = new Set<string>()
const queriedClasses = new Set<string>()

// Each of these get setup once the mutation observer starts running.
let notYetQueriedClasses: string[]
let notYetQueriedIds: string[]
let cosmeticObserver: MutationObserver | undefined = undefined

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
  if (!htmlElement || !htmlElement.innerText.trim().length) {
    return false
  }
  return true
}

const unhideSelectors = (selectors: string[]) => {
  if (selectors.length === 0) {
    return
  }

  chrome.runtime.sendMessage({
    type: 'showFirstPartySelectors',
    selectors
  })
}

const alreadyUnhiddenSelectors = new Set<string>()
const alreadyKnownFirstPartySubtrees = new WeakSet()
const allSelectorsSet = new Set<string>()
const firstRunQueue = new Set<string>()
const secondRunQueue = new Set<string>()
const finalRunQueue = new Set<string>()
const allQueues = [firstRunQueue, secondRunQueue, finalRunQueue]
const numQueues = allQueues.length
const pumpIntervalMs = 50
const maxWorkSize = 50
let queueIsSleeping = false

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
    const newlyIdentifiedFirstPartySelectors: string[] = []

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

        newlyIdentifiedFirstPartySelectors.push(selector)
        alreadyUnhiddenSelectors.add(selector)
      }
      alreadyKnownFirstPartySubtrees.add(aMatchingElm)
    }

    unhideSelectors(newlyIdentifiedFirstPartySelectors)

    for (const aUsedSelector of currentWorkLoad) {
      currentQueue.delete(aUsedSelector)
      // Don't requeue selectors we know identify first party content.
      const selectorMatchedFirstParty = newlyIdentifiedFirstPartySelectors.includes(aUsedSelector)
      if (nextQueue && selectorMatchedFirstParty === false) {
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

const stopObserving = () => {
  if (cosmeticObserver) {
    cosmeticObserver.disconnect()
  }
}

chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
  const action = typeof msg === 'string' ? msg : msg.type
  switch (action) {
    case 'cosmeticFilteringBackgroundReady': {
      setTimeout(startObserving, numMSBeforeStart)
      setTimeout(stopObserving, numMSBeforeStart + numMSCheckFor)
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