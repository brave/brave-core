import BATAd from '../bat-ad'

export default function targetAdSlotsBySelector(selector, fnGetAdSizes, onAdPositionReady) {
  const slots = document.querySelectorAll(selector)
  for (const element of slots) {
    const slotSizes = fnGetAdSizes(element)
    console.log('BATSense: got sizes for potential ad slot', slotSizes, element)
    if (slotSizes && slotSizes.length) {
      new BATAd(element, onAdPositionReady).sizes = slotSizes
    }
  }
}