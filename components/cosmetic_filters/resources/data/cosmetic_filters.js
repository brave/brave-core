/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

(function() {
  const queriedIds = new Set();
  const queriedClasses = new Set();
  var notYetQueriedClasses;
  var notYetQueriedIds;
  var cosmeticObserver;
  if (window.cosmeticStyleSheet === undefined) {
    window.cosmeticStyleSheet = new CSSStyleSheet();
  }
  if (window.allSelectorsToRules === undefined) {
    window.allSelectorsToRules = new Map();
  }
  // All new selectors go in `firstRunQueue`
  if (window.firstRunQueue === undefined) {
    window.firstRunQueue = new Set();
  }
  // Third party matches go in the second and third queues.
  if (window.secondRunQueue === undefined) {
    window.secondRunQueue = new Set();
  }
  // Once a selector gets in to this queue, it's only evaluated for 1p content one
  // more time.
  if (window.finalRunQueue === undefined) {
    window.finalRunQueue = new Set();
  }
  if (window.allQueues === undefined) {
    window.allQueues = [firstRunQueue, secondRunQueue, finalRunQueue];
  }
  if (window.numQueues === undefined) {
    window.numQueues = allQueues.length;
  }
  if (window.alreadyKnownFirstPartySubtrees === undefined) {
    window.alreadyKnownFirstPartySubtrees = new Set();
  }
  if (window.alreadyUnhiddenSelectors === undefined) {
    window.alreadyUnhiddenSelectors = new Set();
  }

  const fetchNewClassIdRules = function () {
    if ((!notYetQueriedClasses || notYetQueriedClasses.length === 0) &&
        (!notYetQueriedIds || notYetQueriedIds.length === 0)) {
      return;
    };
    // Callback to c++ renderer process
    cf_worker.hiddenClassIdSelectors(
        JSON.stringify({classes: notYetQueriedClasses, ids: notYetQueriedIds}));
    notYetQueriedClasses = [];
    notYetQueriedIds = [];
  };

  function isElement (node) {
    return (node.nodeType === 1);
  };

  function asElement (node) {
    return isElement(node) ? node : null;
  };

  const handleMutations = MutationCallback = function (mutations) {
    for (const aMutation of mutations) {
      if (aMutation.type === 'attributes') {
        const changedElm = aMutation.target;
        switch (aMutation.attributeName) {
          case 'class':
            for (const aClassName of changedElm.classList.values()) {
              if (queriedClasses.has(aClassName) === false) {
                notYetQueriedClasses.push(aClassName);
                queriedClasses.add(aClassName);
              };
            };
            break;
          case 'id':
            const mutatedId = changedElm.id;
            if (queriedIds.has(mutatedId) === false) {
              notYetQueriedIds.push(mutatedId);
              queriedIds.add(mutatedId);
            };
            break;
        };
      } else if (aMutation.addedNodes.length > 0) {
        for (const node of aMutation.addedNodes) {
          const element = asElement(node);
          if (!element) {
            continue;
          };
          const id = element.id;
          if (id && !queriedIds.has(id)) {
            notYetQueriedIds.push(id);
            queriedIds.add(id);
          };
          const classList = element.classList;
          if (classList) {
            for (const className of classList.values()) {
              if (className && !queriedClasses.has(className)) {
                notYetQueriedClasses.push(className);
                queriedClasses.add(className);
              };
            };
          };
        };
      };
    };
    fetchNewClassIdRules();
  };

  const startObserving = () => {
    // First queue up any classes and ids that exist before the mutation observer
    // starts running.
    const elmWithClassOrId = document.querySelectorAll('[class],[id]');
    for (const elm of elmWithClassOrId) {
      for (const aClassName of elm.classList.values()) {
        queriedClasses.add(aClassName);
      }
      const elmId = elm.getAttribute('id');
      if (elmId) {
        queriedIds.add(elmId);
      }
    };

    notYetQueriedClasses = Array.from(queriedClasses);
    notYetQueriedIds = Array.from(queriedIds);
    fetchNewClassIdRules();

    // Second, set up a mutation observer to handle any new ids or classes
    // that are added to the document.
    cosmeticObserver = new MutationObserver(handleMutations);
    let observerConfig = {
      subtree: true,
      childList: true,
      attributeFilter: ['id', 'class']
    };
    if (document.documentElement instanceof Node) {
      cosmeticObserver.observe(document.documentElement, observerConfig);
    }
  };
  startObserving();
})();
