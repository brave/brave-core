import targetAdSlotsBySelector from './slotTargeting/by-selector'
import fetchAdCreatives from './creativeFetch/same-context'
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

  targetAdSlotsBySelector('.ad', getSizeForMarketWatchAdSlot, fetchAdCreatives)
})
