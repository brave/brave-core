import RealAdDemoCatalog from './ad-catalogs/real-ads-demo'


const adCatalog = new RealAdDemoCatalog()

// Respond to requests for creatives from tab content scripts
chrome.runtime.onMessageExternal.addListener(
  function(request, sender, sendResponse) {
    console.log('got message', request.type, request)
    if (request.type === 'ad-request') {
      const { sizes, isResponsive } = request
      sendResponse({
        adDetail: adCatalog.getAd(sizes, isResponsive)
      })
    }
  });