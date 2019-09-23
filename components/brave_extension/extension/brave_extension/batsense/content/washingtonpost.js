import BATAd from './bat-ad'
import fetchAdCreatives from './creativeFetch/same-context'
import waitForWindowVar from '../../../../../common/pageLifecycle/wait-for-window-var'
import runOnPageLoaded from '../../../../../common/pageLifecycle/run-on-loaded'


// WPAd is specific to washingtonpost.com (WP)
// It takes over WP-specific ad elements, finds their target size options
// and relays to BATAd
class WPAd extends BATAd {
  constructor(adContainer, onAdPositionReady) {
    super(adContainer, onAdPositionReady)
  }

  setInventory(inventory) {
    if (!inventory.size || !inventory.size.length) {
      console.error('invalid inventory for wpad', inventory, this)
    }
    const dimensionSizes = []
    let isFluid = false
    for (const size of inventory.size) {
      if (Array.isArray(size) && size.length == 2 && size.every(s => Number.isInteger(s))) {
        dimensionSizes.push(size)
      } else if (typeof size === 'string' && size === 'fluid') {
        isFluid = true
      }
    }
    console.log('calling sizes', { dimensionSizes, isFluid })
    this.sizes = dimensionSizes
    this.isResponsive = isFluid
  }

  attributeChangedCallback(name, oldValue, newValue) {
    super.attributeChangedCallback(name, oldValue, newValue)
    if (name === 'id' && newValue !== oldValue) {
      console.log('WPAd: id changed', oldValue, newValue)
      this.setMatchingInventory()
    }
  }

  setMatchingInventory() {
    const adKey = this.element.id.replace('slug_', '')
    console.log('setMatchingInventory', adKey, this)
    if (!adKey) {
      console.error('no id found!', this.element.id, this)
      return
    }
    if (!window.wpAd) {
      return
    }
    // Choose a matching size for ad
    let inventoryMatch
    for (const inventoryKey in window.wpAd.inventory) {
      let hasMatch = (inventoryKey.includes('*'))
        ? (adKey.startsWith(inventoryKey.replace('*', '')))
        : (inventoryKey == adKey)
      if (hasMatch) {
        inventoryMatch = wpAd.inventory[inventoryKey]
        break
      }
    }

    if (inventoryMatch) {
      console.log(`BATSense: Found matching inventory for ad element`, adKey, inventoryMatch)
      this.setInventory(inventoryMatch)
    } else {
      console.warn('BATSense: No inventory match found for ad element', adKey)
    }
  }
}

runOnPageLoaded(function () {
  function ChooseAdSizesForWP() {
    const adElements = document.querySelectorAll('wp-ad')
    // We can't parse ad size data until we have window.wpAds, so store
    // refs to WPAd elements until that is defined.
    for (const element of adElements) {
      new WPAd(element, fetchAdCreatives).setMatchingInventory()
    }
  }

  waitForWindowVar('wpAd', (val) => {
    window.wpAd = val
    console.log('BATSense: window.wpAd received by content script')
    ChooseAdSizesForWP()
  })
})
