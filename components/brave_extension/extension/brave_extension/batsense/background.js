import creative1Url from './demo-creatives/creative-1.html'

class DemoAdCatalog {
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

const adCatalog = new DemoAdCatalog()

// Respond to requests for creatives from tab content scripts
chrome.runtime.onMessage.addListener(
  function(request, sender, sendResponse) {
    if (request.type === 'ad-request') {
      const { sizes, isResponsive } = request
      sendResponse({
        adDetail: adCatalog.getAd(sizes, isResponsive)
      })
    }
  });