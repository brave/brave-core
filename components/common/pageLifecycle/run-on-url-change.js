import injectToDocument from '../contentScript/inject-to-document'

const customEventName = 'brave-url-changed'

export default function runOnUrlChange(fn) {
  // content script observes
  window.addEventListener(customEventName, fn)
  // document script fires
  function fnPageInjectionCode ($customEventName) {
    console.log('replacing events and sending name', $customEventName)

    const prevPushState = window.history.pushState
    const prevReplaceState = window.history.replaceState

    window.history.pushState = function (...args) {
      console.log('pushState was called')
      requestAnimationFrame(() => window.dispatchEvent(new CustomEvent($customEventName)))
      return prevPushState.call(this, ...args)
    }

    window.history.replaceState = function (...args) {
      console.log('replaceState was called')
      requestAnimationFrame(() => window.dispatchEvent(new CustomEvent($customEventName)))
      return prevReplaceState.call(this, ...args)
    }
  }
  injectToDocument(fnPageInjectionCode, customEventName)
}
