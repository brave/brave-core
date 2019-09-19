import targetAdSlotsBySelector from './slotTargeting/by-selector'
import startListeningCustomAdSlotReadyEvents from './creativeFetch/custom-event-receive'
import sendCustomEventForAdSlotReady from './creativeFetch/custom-event-send'
import runOnPageLoaded from './pageLifecycle/run-on-loaded'

runOnPageLoaded(function () {
  function getSizeForAdSlot (element) {
    const adOptionsString = element.getAttribute('data-ad-options')
    if (!adOptionsString) {
      return null
    }
    const adOptions = JSON.parse(adOptionsString)
    if (!adOptions) {
      return null
    }
    return adOptions.adSize
  }

  startListeningCustomAdSlotReadyEvents()
  targetAdSlotsBySelector('[data-ad-options]', getSizeForAdSlot, sendCustomEventForAdSlotReady)
})
