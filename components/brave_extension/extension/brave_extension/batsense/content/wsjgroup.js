import targetAdSlotsBySelector from './slotTargeting/by-selector'
import fetchAdCreatives from './creativeFetch/same-context'
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

  targetAdSlotsBySelector('[data-ad-options]', getSizeForAdSlot, fetchAdCreatives)
})
