import BATAd from '../bat-ad'
import runOnUrlChange from '../../../../../../common/pageLifecycle/run-on-url-change'

export default function targetAdSlotsBySelector(selector, fnGetAdSizes, onAdPositionReady) {
  const slotsSeen = new WeakMap()
  function findSlots() {
    const slots = document.querySelectorAll(selector)
    console.log(`BATSense: findSlots found ${slots.length} elements`)
    for (const element of slots) {
      if (slotsSeen.has(element)) {
        continue
      }
      const slotSizes = fnGetAdSizes(element)
      console.log('BATSense: got sizes for potential ad slot', slotSizes, element)
      if (slotSizes && slotSizes.length) {
        const batAd = new BATAd(element, onAdPositionReady).sizes = slotSizes
        slotsSeen.set(element, batAd)
      } else {
        slotsSeen.set(element, null)
      }
    }
  }
  findSlots()
  runOnUrlChange(findSlots)
}