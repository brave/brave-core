import slotFilteringAll from '../slotFiltering/all'
import slotFilteringBest from '../slotFiltering/one-best-on-page'

const slotFilterStrategy = slotFilteringBest

let accumulatorTimeoutId
let accumulations = []
export default function adSlotReady (batAd) {
  // accumulate so we can use a picking strategy
  accumulations.push(batAd)
  if (!accumulatorTimeoutId) {
    accumulatorTimeoutId = setTimeout(accumulationDone, 1000)
  }
}

function filterUnsuitableSizes (batAds) {
  return batAds.filter(batAd => batAd.sizes.some(size => {
    if (size.length !== 2)
      return false
    const sizeTyped = [Number(size[0]), Number(size[1])]
    return (sizeTyped[0] > 200 && sizeTyped[1] > 70) ||
      (sizeTyped[1] > 200 && sizeTyped[0] > 70)
  }))
}

function accumulationDone () {
  accumulatorTimeoutId = null
  let batAds = accumulations
  accumulations = []
  console.log(`BATSense: accumulator done, got ${batAds.length} slots`)
  batAds = filterUnsuitableSizes(batAds)
  console.log(`BATSense: got ${batAds.length} slots with useable size`)
  batAds = slotFilterStrategy(batAds)
  console.log(`BATSense: filter strategy done, got ${batAds.length} slots`, batAds)
  for (const batAd of batAds) {
    fetchCreativeFromBackend(batAd)
    .then(adDetail => {
      const adElement = batAd.element
      const adId = batAd.batAdId
      console.log('BATSense got response for ad', adId, adDetail)
      if (adDetail) {
        const dimensions = adDetail.size.join('x')
        adElement.setAttribute('creative-dimensions', dimensions)
        adElement.setAttribute('creative-url', adDetail.creativeUrl)
      }
    })
  }
}

function fetchCreativeFromBackend (batAd) {
  return new Promise((resolve, reject)  => {
    chrome.runtime.sendMessage(
      {
        type: 'ad-request',
        sizes: batAd.sizes,
        isResponsive: batAd.isResponsive
      },
      (response) => {
        const { adDetail } = response
        resolve(adDetail)
      }
    )
  })
}
