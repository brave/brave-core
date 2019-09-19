import targetAdSlotsBySelector from './slotTargeting/by-selector'
import startListeningCustomAdSlotReadyEvents from './creativeFetch/custom-event-receive'
import sendCustomEventForAdSlotReady from './creativeFetch/custom-event-send'
import runOnPageLoaded from './pageLifecycle/run-on-loaded'

runOnPageLoaded(function () {
  function getSizeForMarketWatchAdSlot (element) {
    const sizeData = element.getAttribute('data-size')
    if (sizeData) {
      const sizesByString = sizeData.split(',').map(sizeString => {
        const sizeData = sizeString.split('x')
        if (sizeData.length == 2) {
          return sizeData.map(s => Number(s))
        }
        return null
      }).filter(item => item)
      return sizesByString
    }
    return null
  }

  startListeningCustomAdSlotReadyEvents()
  targetAdSlotsBySelector('.ad', getSizeForMarketWatchAdSlot, sendCustomEventForAdSlotReady)
})
