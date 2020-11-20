(function() {
  const queriedIds = new Set();
  const queriedClasses = new Set();
  var notYetQueriedClasses;
  var notYetQueriedIds;
  var cosmeticObserver;
  if (window.cosmeticStyleSheet === undefined) {
    window.cosmeticStyleSheet = new CSSStyleSheet();
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
    cosmeticObserver = new MutationObserver(handleMutations);
    let observerConfig = {
      subtree: true,
      childList: true,
      attributeFilter: ['id', 'class']
    };
    cosmeticObserver.observe(document.documentElement, observerConfig);
  };
  startObserving();
})();
