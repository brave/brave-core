import injectToDocument from '../util/inject-to-document'

export default function waitForWindowVar(varName, onValue) {

  const customEventName = `bat-ad-value-found-${varName}`

  function onCustomEvent (e) {
    const { varValue } = e.detail
    if (varValue) {
      window.removeEventListener(customEventName, onCustomEvent)
      onValue(varValue)
    }
  }

  window.addEventListener(customEventName, onCustomEvent)

  function fnPageInjectionCode ($varName, $customEventName) {
    function valueFound (varValue) {
      window.dispatchEvent(new CustomEvent($customEventName, {
        detail: {
          varValue
        },
        bubbles: true
      }))
    }

    if (window[$varName]) {
      valueFound(window[$varName])
      return
    }

    let _value
    Object.defineProperty(window, $varName, {
      configurable: true,
      set: function (value) {
        _value = value
        valueFound(_value)
      },
      get: function () {
        return _value
      }
    })
  }

  injectToDocument(fnPageInjectionCode, varName, customEventName)
}
