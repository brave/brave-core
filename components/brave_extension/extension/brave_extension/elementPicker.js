// Namespace for creating SVG elements
const NSSVG = "http://www.w3.org/2000/svg"

const sendMessageActiveTab = (message, callback = null) => {
  chrome.tabs.query({ active: true, currentWindow: true }, (tabs) => {
    if (tabs.length > 0) {
      if (callback !== null) {
        chrome.tabs.sendMessage(tabs[0].id, message, callback)
      } else {
        chrome.tabs.sendMessage(tabs[0].id, message)
      }
    }
  })
}

window.onload = () => {
  let hasSelectedTarget = false
  const section = document.querySelector('section')
  const togglePopup = (show) => {
    if (show) {
      section.style.display = 'block'
    } else {
      section.style.display = 'none'
    }
  }

  document.addEventListener('keydown', (event) => {
    if (event.key === 'Escape') {
      event.stopPropagation()
      event.preventDefault()
      sendMessageActiveTab({type: 'quitElementPicker'})
    }
  }, true);

  const svg = document.getElementById('picker-ui')

  svg.addEventListener('mousemove', (event) => {
    if (!hasSelectedTarget) {
      sendMessageActiveTab({
          type: 'targetingCoordsChanged',
          coords: {
            x: event.clientX,
            y: event.clientY
          }
      })
    }
    event.stopPropagation()
  }, true)

  svg.addEventListener('click', (event) => {
    if (hasSelectedTarget) {
      // We are already previewing a target. We'll interpet another click
      // as the user wanting back control of the UI.
      hasSelectedTarget = false
      togglePopup(false)
      return
    }
    sendMessageActiveTab({type: 'userSelectedElement'}, (response) => {
      const { isValid, selector } = response
      if (isValid) {
        hasSelectedTarget = true
        togglePopup(true)
        // disable hovering new elements
        const rulesBox = document.getElementById('rules-box')
        rulesBox.innerText = selector
      }
    })
  });

  const createButton = document.getElementById('btnCreate')
  createButton.addEventListener('click', (event) => {
    const rulesBox = document.getElementById('rules-box')
    const selector = rulesBox.innerText.trim()
    if (selector.length > 0) {
      sendMessageActiveTab({
        type: 'userSelectedRule',
        selector: selector,
      })
    }
  })

  const quitButton = document.getElementById('btnQuit')
  quitButton.addEventListener('click', (event) => {
    sendMessageActiveTab({type: 'quitElementPicker'})
  })
}

chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
  const action = typeof msg === 'string' ? msg : msg.type
  switch (action) {
    case 'highlightElements': {
      // Delete old element targeting rectangles and their corresponding masks
      const oldMask = document.getElementsByClassName('mask')
      while (oldMask.length > 0) { oldMask[0].remove() }

      const svg = document.getElementById('picker-ui')
      const svgMask = document.getElementById('highlight-mask')
      for (const rect of msg.coords) {
        // Add the mask to the SVG definition so the dark background is removed
        const mask = document.createElementNS(NSSVG, 'rect')
        mask.classList.add('mask')
        mask.setAttribute('x', rect.x)
        mask.setAttribute('y', rect.y)
        mask.setAttribute('width', rect.width)
        mask.setAttribute('height', rect.height)
        svgMask.appendChild(mask)

        // Use the same element, but add the target class which turns the
        // target rectangle orange
        const braveTargetingArea = mask.cloneNode()
        braveTargetingArea.classList.add('target')
        svg.appendChild(braveTargetingArea)
      }
      break
    }
  }
})
