const queriedIds = new Set()
const queriedClasses = new Set()
const regexWhitespace = /\s/

const getClassesAndIds = function (addedNodes: Element[]) {
  const ids = []
  const classes = []

  for (const node of addedNodes) {
    let nodeId = node.id
    if (nodeId && nodeId.length !== 0) {
      nodeId = nodeId.trim()
      if (!queriedIds.has(nodeId) && nodeId.length !== 0) {
        ids.push(nodeId)
        queriedIds.add(nodeId)
      }
    }
    let nodeClass = node.className
    if (nodeClass && nodeClass.length !== 0 && !regexWhitespace.test(nodeClass)) {
      if (!queriedClasses.has(nodeClass)) {
        classes.push(nodeClass)
        queriedClasses.add(nodeClass)
      }
    } else {
      let nodeClasses = node.classList
      if (nodeClasses) {
        let j = nodeClasses.length
        while (j--) {
          const nodeClassJ = nodeClasses[j]
          if (queriedClasses.has(nodeClassJ) === false) {
            classes.push(nodeClassJ)
            queriedClasses.add(nodeClassJ)
          }
        }
      }
    }
  }
  return { classes, ids }
}

const handleNewNodes = (newNodes: Element[]) => {
  const { classes, ids } = getClassesAndIds(newNodes)
  chrome.runtime.sendMessage({
    type: 'classIdStylesheet',
    classes,
    ids
  })
}

function applyCosmeticFilterMutationObserver () {
  let targetNode = document.documentElement
  let observer = new MutationObserver(mutations => {
    const nodeList: Element[] = []
    for (const mutation of mutations) {
      for (let nodeIndex = 0; nodeIndex < mutation.addedNodes.length; nodeIndex++) {
        nodeList.push(mutation.addedNodes[nodeIndex] as Element)
      }
    }
    handleNewNodes(nodeList)
  })
  let observerConfig = {
    childList: true,
    subtree: true
  }
  observer.observe(targetNode, observerConfig)
}

chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
  const action = typeof msg === 'string' ? msg : msg.type
  switch (action) {
    case 'cosmeticFilterGenericExceptions': {
      let allNodes = Array.from(document.querySelectorAll('[id],[class]'))
      handleNewNodes(allNodes)
      applyCosmeticFilterMutationObserver()

      sendResponse(null)
      break
    }
  }
})
