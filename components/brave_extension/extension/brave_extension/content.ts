const unique = require('unique-selector').default
//import { debounce } from '../../../common/debounce'

let target: EventTarget | null
let contentSiteFilters: any
let genericExceptions: Array<string> | undefined = undefined
let classIdBuffer: { classes: string[], ids: string[] } = { classes: [], ids: [] }

if (process.env.NODE_ENV === 'development') {
  console.info('development content script here')
}

function getCurrentURL () {
  return window.location.hostname
}

// when page loads, grab filter list and only activate if there are rules
chrome.storage.local.get('cosmeticFilterList', (storeData = {}) => {
  if (!storeData.cosmeticFilterList) {
    if (process.env.NODE_ENV === 'development') {
      console.info('applySiteFilters: no cosmetic filter store yet')
    }
    return
  }
  console.log('storeData.cosmeticFilterList', storeData.cosmeticFilterList)
  console.log('storeData.cosmeticFilterList[getCurrentURL()]', storeData.cosmeticFilterList[getCurrentURL()])
  // add length check here (can't read property slice of undefined)
  if (storeData.cosmeticFilterList[getCurrentURL()]) {
    contentSiteFilters = storeData.cosmeticFilterList[getCurrentURL()].slice()
    console.log('contentSiteFilters', contentSiteFilters)
    console.log('current site list in content script', contentSiteFilters)
  }
})

// let debouncedRemove = debounce((siteFilters: Array<object>) => {
//   console.log('REMOVING HERE')
//   removeAll(siteFilters)
// }, 1000 / 60)

// on load retrieve each website's filter list
chrome.storage.local.get('cosmeticFilterList', (storeData = {}) => { // fetch filter list
  // !storeData.cosmeticFilterList || storeData.cosmeticFilterList.length === 0 // if no rules, don't apply mutation observer

  if (!storeData.cosmeticFilterList) {
    console.log('storeData.cosmeticFilterList does not exist')
  } else if (Object.keys(storeData.cosmeticFilterList).length === 0) {
    console.log('storeData.cosmeticFilterList length === 0')
  } else {
    console.log('ON COMMITTED MUTATION OBSERVER BEING APPLIED:')
    // removeAll(contentSiteFilters)
    chrome.storage.local.get('cosmeticFilterList', (storeData = {}) => { // fetch filter list
      console.log('cosmeticFilterList.length:', Object.keys(storeData.cosmeticFilterList).length)
    })
  }
})

function applyDOMCosmeticFilterDebounce() {
  console.log('applyDOMCosmeticFilterDebounce call')
  let targetNode = document.documentElement
  let observer = new MutationObserver(mutations => {
    const nodeList: Element[] = [];
    for (const mutation of mutations) {
      for (let nodeIndex = 0; nodeIndex < mutation.addedNodes.length; nodeIndex++) {
        nodeList.push(mutation.addedNodes[nodeIndex] as Element);
      }
    }
    handleNewNodes(nodeList);
  })
  let observerConfig = {
    childList: true,
    subtree: true
    // characterData: true
  }
  observer.observe(targetNode, observerConfig)
}

/*const injectIncrementalStyles = (mutations: MutationRecord[]) => {
  // array of site filters, go through each one and check if idempotent/already applied

  siteFilters.map((mutations: any) => {
    console.log(filterData.filter)
    if (!filterData.isIdempotent || !filterData.applied) { // don't apply if filter is idempotent AND was already applied
      if (document.querySelector(filterData.filter)) { // attempt filter application
        document.querySelectorAll(filterData.filter).forEach(e => {
          console.log(filterData.filter, document.querySelectorAll(filterData.filter))
          e.remove()
          filterData.applied = true
        })
      }
      console.log(siteFilters)
    }
  })
}*/
/*
  let contentSiteFilters = [
    {
      'filter': 'filter1',
      'isIdempotent': true,
      'applied': false
    }, {
      'filter': 'filter2',
      'isIdempotent': false,
      'applied': false
    }, {
      'filter': 'filter3',
      'isIdempotent': false,
      'applied': false
    }
  ]
*/

const handleNewNodes = (newNodes: Element[]) => {
  const { classes, ids } = getClassesAndIds(newNodes);
  if (!(genericExceptions)) {
    classIdBuffer.classes.push(...classes)
    classIdBuffer.ids.push(...ids)
  } else {
    chrome.runtime.sendMessage({
      type: 'classIdStylesheet',
      classes,
      ids,
      exceptions: genericExceptions,
    })
  }
}

const getClassesAndIds = (function() {
  const queriedIds = new Set();
  const queriedClasses = new Set();
  const regexWhitespace = /\s/;

  const new_classes_and_ids = function(addedNodes: Element[]) {
    const ids = [];
    const classes = [];

    for (const node of addedNodes) {
      let v = node.id;
      if (typeof v === 'string' && v.length !== 0) {
        v = v.trim();
        if(!queriedIds.has(v) && v.length !== 0) {
          ids.push(v);
          queriedIds.add(v);
        }
      }
      let vv = node.className;
      if (typeof vv === 'string' && vv.length !== 0 && !regexWhitespace.test(vv)) {
        if(!queriedClasses.has(vv)) {
          classes.push(vv);
          queriedClasses.add(vv);
        }
      } else {
        let vvv = node.classList;
        if (vvv !== undefined) {
          let j = vvv.length;
          while (j--) {
            const v = vvv[j];
            if(queriedClasses.has(v) === false) {
              classes.push(v);
              queriedClasses.add(v);
            }
          }
        }
      }
    }
    return {classes, ids};
  };

  return new_classes_and_ids;
})();

let allNodes = Array.from(document.querySelectorAll('[id],[class]'))
handleNewNodes(allNodes)
applyDOMCosmeticFilterDebounce()

document.addEventListener('contextmenu', (event) => {
  // send host and store target
  // `target` needed for when background page handles `addBlockElement`
  target = event.target
  chrome.runtime.sendMessage({
    type: 'contextMenuOpened',
    baseURI: getCurrentURL()
  })
}, true)

chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
  const action = typeof msg === 'string' ? msg : msg.type
  switch (action) {
    case 'getTargetSelector': {
      sendResponse(unique(target))
    }
    case 'cosmeticFilterGenericExceptions': {
      genericExceptions = msg.exceptions
      if (classIdBuffer.classes.length !== 0 || classIdBuffer.ids.length !== 0) {
        chrome.runtime.sendMessage({
          type: 'classIdStylesheet',
          classes: classIdBuffer.classes,
          ids: classIdBuffer.ids,
          exceptions: genericExceptions,
        })
      }
      sendResponse(null)
    }
  }
})
