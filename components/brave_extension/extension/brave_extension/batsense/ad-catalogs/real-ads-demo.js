import audiDsky1Url from '../demo-creatives/audi-dsky-1.jpg'
import audiLboard1Url from '../demo-creatives/audi-lboard-1.jpg'

const categories = {
  automotive: 'Automotive'
}

const realAds = [
  {
    size: [300, 600],
    path: audiDsky1Url,
    category: categories.automotive
  },
  {
    size: [728, 90],
    path: audiLboard1Url,
    category: categories.automotive
  }
]

export default class RealAdDemoCatalog {
  getAd (sizes, isResponsive) {
    // What fixed numeric sizes does this slot support?
    const sizesTyped = sizes
      .filter(size => Array.isArray(size) && size.length === 2)
      .map(sizeArray => [Number(sizeArray[0]), Number(sizeArray[1])])
    // Get the first ad that has a size within the list of slot sizes
    const chosenAd = realAds.find(ad => sizesTyped.some(size => size[0] === ad.size[0] && size[1] === ad.size[1]))
    if (!chosenAd) {
      return null
    }
    return {
      size: chosenAd.size,
      creativeUrl: chrome.runtime.getURL(chosenAd.path),
      category: chosenAd.category
    }
  }
}
