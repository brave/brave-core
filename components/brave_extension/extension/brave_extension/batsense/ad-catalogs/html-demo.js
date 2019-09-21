import creative1Url from './demo-creatives/creative-1.html'

export default class HTMLDemoAdCatalog {
  getAd (sizes, isResponsive) {
    // TODO: More creatives
    // TODO: destination Url for each creative
    // TODO: min / max size for each creative
    // TODO: return null if no match
    console.log('asking for ad', sizes)
    if (!sizes.some(size => size.length === 2 && Number(size[0]) > 200 && Number(size[1]) > 70)) {
      console.log('too small')
      return null
    }
    console.log('ok size', sizes[sizes.length - 1])
    return {
      creativeUrl: chrome.runtime.getURL(creative1Url),
      size: sizes[sizes.length - 1]
    }
  }
}