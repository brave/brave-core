import AdCatalog from './ad-catalogs/brave-rewards-ads-catalog'

const adCatalog = new AdCatalog()

// Respond to requests for creatives from tab content scripts
chrome.runtime.onMessageExternal.addListener(
  async function (request, sender, sendResponse) {
    console.log('got message', request.type, request)
    switch (request.type) {
      case 'ad-request':
        const { url, sizes, isResponsive } = request
        const ads = await adCatalog.getAd(url, sizes, isResponsive)
        console.log('got ads for', request, ads)
        sendResponse({
          ads
        })
        break
      case 'ads-trigger-view':
        chrome.braveRewards.triggerPublisherAdViewed(request.ad)
        break
      case 'ads-trigger-interaction':
        chrome.braveRewards.triggerPublisherAdInteracted(request.ad)
        break
    }
  }
)
