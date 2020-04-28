import createTemplate from './createTemplate'

let adTemplate

// BATAd is generic - it should render an ad on any site
// When provided with required data (target ad sizes),
// it communicates with content script and backend to ask
// for an ad.
export default class BATAd {
  static get observedAttributes() { return ['responsive', 'id', 'creative-dimensions', 'creative-url'] }

  constructor(adContainer, onAdPositionReady) {
    this.element = adContainer
    if (!adTemplate) adTemplate = createTemplate()
    this.shadowRoot = this.element.attachShadow({mode: 'open'})
    this.shadowRoot.appendChild(adTemplate.content.cloneNode(true))
    this.elementObserver = new MutationObserver(this.elementMutatedCallback.bind(this))
    this.elementObserver.observe(this.element, { attributes: true })
    this.onAdPositionReady = onAdPositionReady
    this.fetchCreative()
  }

  get batAdId() {
    let val = this.element.getAttribute('bat-ad-id')
    return val
  }

  get sizes() {
    return this.sizes_
  }

  set sizes(value) {
    console.log('BATAd: received sizes for element')
    this.sizes_ = value
    this.fetchCreative()
  }

  get isResponsive() {
    return this.isResponsive_
  }

  set isResponsive(value) {
    console.log('Batad isR change')
    this.isResponsive_ = Boolean(value)
    this.fetchCreative()
  }

  get style() {
    return this.element.style
  }

  elementMutatedCallback (mutationsList) {
    for(const mutation of mutationsList) {
      if (mutation.type === 'attributes' && BATAd.observedAttributes.includes(mutation.attributeName)) {
        console.log('BATAd: The ' + mutation.attributeName + ' attribute was modified.')
        this.attributeChangedCallback(
          mutation.attributeName,
          mutation.oldValue,
          this.element.getAttribute(mutation.attributeName)
        )
      }
    }
  }

  attributeChangedCallback(name, oldValue, newValue) {
    if (name === 'responsive') {
      this.fetchCreative()
    }
    if (name === 'creative-dimensions' && newValue && newValue !== oldValue) {
      const dimensions = newValue.split('x')
      if (dimensions.length !== 2) {
        console.error('BATAd invalid dimension syntax', dimensions)
        return
      }
      this.style.width = `${dimensions[0]}px`
      this.style.height = `${dimensions[1]}px`
    }
    if (name === 'creative-url') {
      this.shadowRoot.querySelector('.creative').setAttribute('src', newValue)
      this.ensureVisible()
    }
  }

  ensureVisible() {
    // Override cosmetic filters
    window.requestAnimationFrame(() => {
      let element = this.element
      while (element) {
        if (element.computedStyleMap().get('display').value === 'none') {
          element.style.setProperty('display', 'block', 'important')
        }
        element = element.parentElement
      }
    })
  }

  fetchCreative() {
    if (!this.isConnected) {
      console.warn('BATAd: attempted to call fetchCreative when element not in DOM')
    }
    console.log('fetching creative', this.element, this.sizes, this.isResponsive)
    if (this.sizes && this.sizes.length) {
      console.log('performing pre-flight')
      this.element.setAttribute('bat-ad-id', '' + Math.random().toString(36).substr(2, 9))
      this.element.innerText = 'Fetching creative...'
      this.onAdPositionReady(this)
    }
  }
}