
export default function listenForAdSlots() {
  // Get creatives from backend when an ad position becomes ready
  window.addEventListener('bat-ad-position-ready', (e) => {
    const { id: adId, sizes, isResponsive } = e.detail
    const adElement = document.querySelector(`[bat-ad-id="${adId}"]`)
    console.log('Content Script: GOT PRE FLIGHT FROM DOC', adId, sizes, isResponsive, e, adElement)
    if (adElement) {
      chrome.runtime.sendMessage(
        {
          type: 'ad-request',
          sizes,
          isResponsive
        },
        (response) => {
          const { adDetail } = response
          console.log('got response for ad', adDetail, adId)
          if (adDetail) {
            const dimensions = adDetail.size.join('x')
            adElement.setAttribute('creative-dimensions', dimensions)
            adElement.setAttribute('creative-url', adDetail.creativeUrl)
          }
        }
      )
    }
  })
}