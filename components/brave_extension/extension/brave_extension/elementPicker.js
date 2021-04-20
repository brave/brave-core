"use strict"

// Namespace for creating SVG elements
const NSSVG = "http://www.w3.org/2000/svg"

const sendMessageActiveTab = (message, callback = null) => {
  chrome.tabs.query({ active: true, currentWindow: true }, (tabs) => {
    // If we cannot find active tab, this iframe is no longer valid.
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

  document.addEventListener('keydown', (event) => {
    if (event.key === 'Escape') {
      event.stopPropagation()
      event.preventDefault()
      sendMessageActiveTab({type: 'quitElementPicker'})
    }
  }, true)

  const svg = document.getElementById('picker-ui')

  svg.addEventListener('mousemove', (event) => {
    if (!hasSelectedTarget) {
      sendMessageActiveTab({
          type: 'elementPickerHoverCoordsChanged',
          coords: {
            x: event.clientX,
            y: event.clientY
          }
      })
    }
    event.stopPropagation()
  }, true)

  const rulesTextArea = document.querySelector('#rules-box > textarea')
  let textInputTimer = null
  rulesTextArea.addEventListener('input', (event) => {
    clearTimeout(textInputTimer)
    textInputTimer = setTimeout(() => {
      const selector = rulesTextArea.value.trim()
      if (selector.length > 0) {
        sendMessageActiveTab({
          type: 'elementPickerUserModifiedRule',
          selector: selector,
        })
      }
    }, 700)
  })
  rulesTextArea.addEventListener('focus', (event) => {
    hasSelectedTarget = true
    togglePopup(true)
  })
  const section = document.querySelector('section')
  const togglePopup = (show) => {
    if (show) {
      section.style.opacity = 1
    } else {
      section.style.opacity = 0.2
    }
  }

  const slider = document.getElementById('sliderSpecificity')

  const dispatchSelect = () => {
    sendMessageActiveTab({
      type: 'elementPickerUserSelectedTarget',
      specificity: parseInt(slider.value)
    }, (response) => {
      const { isValid, selector } = response
      if (isValid) {
        hasSelectedTarget = true
        togglePopup(true)
        // disable hovering new elements
        rulesTextArea.value = selector
      }
    })
  }

  slider.addEventListener('input', (event) => {
    dispatchSelect()
  })

  svg.addEventListener('click', (event) => {
    if (hasSelectedTarget) {
      // We are already previewing a target. We'll interpet another click
      // as the user wanting back control of the UI.
      hasSelectedTarget = false
      slider.value = '5'
      togglePopup(false)
      return
    }
    dispatchSelect()
  })

  const createButton = document.getElementById('btnCreate')
  createButton.addEventListener('click', (event) => {
    const selector = rulesTextArea.value.trim()
    if (selector.length > 0) {
      sendMessageActiveTab({
        type: 'elementPickerUserCreatedRule',
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
