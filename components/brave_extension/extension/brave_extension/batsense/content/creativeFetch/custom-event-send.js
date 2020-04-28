
export default function sendCustomEvent (batAd) {
  window.dispatchEvent(new CustomEvent('bat-ad-position-ready', {
    detail: {
      id: batAd.batAdId,
      isResponsive: batAd.isResponsive,
      sizes: batAd.sizes
    },
    bubbles: true
  }))
}