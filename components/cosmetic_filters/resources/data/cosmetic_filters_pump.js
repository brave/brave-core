/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

(function() {
  const { parseDomain, ParseResultType } = require('parse-domain')
  const pumpIntervalMinMs = 40
  const pumpIntervalMaxMs = 1000;
  const maxWorkSize = 60;
  let queueIsSleeping = false;
  // The cutoff for text ads.  If something has only text in it, it needs to have
  // this many, or more, characters.  Similarly, require it to have a non-trivial
  // number of words in it, to look like an actual text ad.
  const minAdTextChars = 30;
  const minAdTextWords = 5;

  /**
   * Provides a new function which can only be scheduled once at a time.
   *
   * @param onIdle function to run when the thread is less busy
   * @param timeout max time to wait. at or after this time the function will be run regardless of thread noise
  */
  const idleize = (onIdle, timeout) => {
    let idleId = undefined;
    return function WillRunOnIdle () {
      if (idleId !== undefined) {
        return;
      }
      idleId = window.requestIdleCallback(() => {
        idleId = undefined;
        onIdle();
      }, { timeout })
    }
  }

  const _parseDomainCache = Object.create(null);
  const getParsedDomain = (aDomain) => {
    const cacheResult = _parseDomainCache[aDomain];
    if (cacheResult !== undefined) {
      return cacheResult;
    }

    const newResult = parseDomain(aDomain);
    _parseDomainCache[aDomain] = newResult;
    console.log('aDomain == ' + aDomain);
    return newResult;
  }

  const isRelativeUrl = (url) => {
    return (
      !url.startsWith('//') &&
      !url.startsWith('http://') &&
      !url.startsWith('https://')
    )
  }

  const _parsedCurrentDomain = getParsedDomain(window.location.host)
  const isFirstPartyUrl = (url) => {
    if (isRelativeUrl(url)) {
      return true
    }

    const parsedTargetDomain = getParsedDomain(url)

    if (parsedTargetDomain.type !== _parsedCurrentDomain.type) {
      return false
    }

    if (parsedTargetDomain.type === ParseResultType.Listed) {
      const isSameEtldP1 = (_parsedCurrentDomain.icann.topLevelDomains === parsedTargetDomain.icann.topLevelDomains &&
        _parsedCurrentDomain.icann.domain === parsedTargetDomain.icann.domain)
      return isSameEtldP1
    }

    const looksLikePrivateOrigin =
      [ParseResultType.NotListed, ParseResultType.Ip, ParseResultType.Reserved].includes(parsedTargetDomain.type)
    if (looksLikePrivateOrigin) {
      return _parsedCurrentDomain.hostname === parsedTargetDomain.hostname
    }

    return false
  }

  const isHTMLElement = (node) => {
    return ('innerText' in node)
  }

  const asHTMLElement = (node) => {
    return isHTMLElement(node) ? node : null
  }

  const isTextAd = (text) => {
    const trimmedText = text.trim()
    if (trimmedText.length < minAdTextChars) {
      return false
    }
    if (trimmedText.split(' ').length < minAdTextWords) {
      return false
    }
    return true
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
    let queryResult;
    let isTopLevel;

    if (possibleQueryResult) {
      queryResult = possibleQueryResult;
      isTopLevel = false;
    } else {
      queryResult = {
        foundFirstPartyResource: false,
        foundThirdPartyResource: false,
        foundKnownThirdPartyAd: false
      };
      isTopLevel = true;
    }

    if (elm.getAttribute) {
      if (elm.hasAttribute('id')) {
        const elmId = elm.getAttribute('id');
        if (elmId.startsWith('google_ads_iframe_') ||
          elmId.startsWith('div-gpt-ad') ||
          elmId.startsWith('adfox_')) {
          queryResult.foundKnownThirdPartyAd = true;
          return false;
        }
      }

      if (elm.hasAttribute('src')) {
        const elmSrc = elm.getAttribute('src');
        const elmSrcIsFirstParty = isFirstPartyUrl(elmSrc);
        if (elmSrcIsFirstParty === true) {
          queryResult.foundFirstPartyResource = true;
          return true;
        }
        queryResult.foundThirdPartyResource = true;
      }

      if (elm.hasAttribute('style')) {
        const elmStyle = elm.getAttribute('style');
        if (elmStyle.includes('url(') ||
          elmStyle.includes('//')) {
          queryResult.foundThirdPartyResource = true;
        }
      }

      if (elm.hasAttribute('srcdoc')) {
        const elmSrcDoc = elm.getAttribute('srcdoc');
        if (elmSrcDoc.trim() === '') {
          queryResult.foundThirdPartyResource = true;
        }
      }
    }

    if (elm.firstChild) {
      isSubTreeFirstParty(elm.firstChild, queryResult)
      if (queryResult.foundKnownThirdPartyAd === true) {
        return false
      }
      if (queryResult.foundFirstPartyResource === true) {
        return true
      }
    }

    if (elm.nextSibling) {
      isSubTreeFirstParty(elm.nextSibling, queryResult)
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
    // Check for several things here:
    // 1. If its not an HTML node (i.e. its a text node), then its definitely
    //    not the root of first party ad (we'd have caught that it was a text
    //    add when checking the text node's parent).
    // 2. If it's a script node, then similarly, its definitely not the root
    //    of a first party ad on the page.
    // 3. Last, check the text content on the page.  If the node contains a
    //    non-trivial amount of text in it, then we _should_ treat it as a
    //    possible 1p ad.
    if (!htmlElement ||
      htmlElement.tagName.toLowerCase() === 'script' ||
      isTextAd(htmlElement.innerText) === false) {
      return false
    }

    return true
  }

  const unhideSelectors = (selectors) => {
    if (selectors.size === 0) {
      return
    }
    // Find selectors we have a rule index for
    const rulesToRemove = Array.from(selectors)
      .map(selector => window.allSelectorsToRules.get(selector))
      .filter(i => i !== undefined)
      .sort()
      .reverse()
    // Delete the rules
    let lastIdx = window.allSelectorsToRules.size - 1
    for (const ruleIdx of rulesToRemove) {
      // Safe to asset ruleIdx is a number because we've already filtered out
      // any `undefined` instances with the filter call above.
      window.cosmeticStyleSheet.deleteRule(ruleIdx)
    }
    // Re-sync the indexes
    // TODO: Sync is hard, just re-build by iterating through the StyleSheet rules.
    const ruleLookup = Array.from(window.allSelectorsToRules.entries())
    let countAtLastHighest = rulesToRemove.length
    for (let i = lastIdx; i > 0; i--) {
      const [selector, oldIdx] = ruleLookup[i]
      // Is this one we removed?
      if (rulesToRemove.includes(i)) {
        window.allSelectorsToRules.delete(selector)
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
      window.allSelectorsToRules.set(selector, oldIdx - countAtLastHighest)
    }
  }

  /**
  * Go through each of the 3 queues, only take maxWorkSize items from each one
  * 1. Take maxWorkSize selects from the first queue with any items
  * 2. Determine partyness of matched element:
  *   - If any are 3rd party, keep 'hide' rule and check again later in next queue.
  *   - If any are 1st party, remove 'hide' rule and never check selector again.
  * 3. If we're looking at the 3rd queue, don't requeue any selectors.
  */
  const pumpCosmeticFilterQueues = () => {
    if (queueIsSleeping === true) {
      return;
    }

    let didPumpAnything = false
    // For each "pump", walk through each queue until we find selectors
    // to evaluate. This means that nothing in queue N+1 will be evaluated
    // until queue N is completely empty.
    for (let queueIndex = 0; queueIndex < window.numQueues; queueIndex += 1) {
      const currentQueue = window.allQueues[queueIndex];
      const nextQueue = window.allQueues[queueIndex + 1];
      if (currentQueue.size === 0) {
        continue;
      }

      const currentWorkLoad = Array.from(currentQueue.values()).slice(0, maxWorkSize);
      const comboSelector = currentWorkLoad.join(',');
      const matchingElms = document.querySelectorAll(comboSelector);
      // Will hold selectors identified by _this_ queue pumping, that were
      // newly identified to be matching 1p content.  Will be sent to
      // the background script to do the un-hiding.
      const newlyIdentifiedFirstPartySelectors = new Set();

      for (const aMatchingElm of matchingElms) {
        // Don't recheck elements / subtrees we already know are first party.
        // Once we know something is third party, we never need to evaluate it
        // again.
        if (window.alreadyKnownFirstPartySubtrees.has(aMatchingElm)) {
          continue;
        }

        const elmSubtreeIsFirstParty = isSubTreeFirstParty(aMatchingElm);
        // If we find that a subtree is third party, then no need to change
        // anything, leave the selector as "hiding" and move on.
        // This element will likely be checked again on the next 'pump'
        // as long as another element from the selector does not match 1st party.
        if (elmSubtreeIsFirstParty === false) {
          continue;
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
          if (window.alreadyUnhiddenSelectors.has(selector) === true) {
            continue
          }

          newlyIdentifiedFirstPartySelectors.add(selector)
          window.alreadyUnhiddenSelectors.add(selector)
        }
        window.alreadyKnownFirstPartySubtrees.add(aMatchingElm)
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
  );

  pumpCosmeticFilterQueuesOnIdle();
})();
