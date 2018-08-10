const unique = require('unique-selector').default

function getCurrentURL () {
  return window.location.hostname
}

document.addEventListener('contextmenu', (event) => {
  let selector = unique(event.target) // this has to be done here, events can't be passed through the messaging API
  let baseURI = getCurrentURL()
  console.log(selector, baseURI)
  chrome.runtime.sendMessage({
    selector: selector,
    baseURI: baseURI
  })
}, true)
