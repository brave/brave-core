const pageInjectionCode = function () {
  const batIconUrl = "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' version='1.1' viewBox='0 0 32 32' height='32px' width='32px' x='0px' y='0px'%3E%3Cpath fill-rule='evenodd' xml:space='preserve' fill='%23fff' d='M9.61 23.25h12.78L16 12 9.61 23.25z'%3E%3C/path%3E%3Cpath d='M3 26.8l7.67-4.52L16 13V4a.45.45 0 0 0-.38.28l-6.27 11-6.26 11a.48.48 0 0 0 0 .48' fill='%23ff4724' fill-rule='evenodd'%3E%3C/path%3E%3Cpath d='M16 4v9l5.29 9.31L29 26.8a.48.48 0 0 0-.05-.48l-6.26-11-6.27-11A.45.45 0 0 0 16 4' fill='%239e1f63' fill-rule='evenodd'%3E%3C/path%3E%3Cpath d='M29 26.8l-7.67-4.52H10.71L3 26.8a.47.47 0 0 0 .43.2h25.1a.47.47 0 0 0 .43-.2' fill='%23662d91' fill-rule='evenodd'%3E%3C/path%3E%3C/svg%3E"

  const templateInnerHTML = /*html*/`
    <style>
      :host {
        user-select: none;
        position: relative;
        margin: 0 auto;
        border: solid 1px white;
        background: #bbb;
        display: block;
        padding: 0 !important;
        font-family: sans-serif;
        font-size: 13px;
        color: white;
      }

      .controls {
        pointer-events: none;
        width: -webkit-fill-available;
        height: -webkit-fill-available;
        color: white;
        border-bottom: solid 1px white;
        border-left: solid 1px white;
        display: inline-flex;
        justify-content: flex-end;
        flex-wrap: wrap;
        align-items: flex-start;
        float: right;
      }

      .controls::after {
        display: block;
        height: 100%;
        content: '';
        width: 5px;
      }

      .controls__toggle {
        visibility: hidden;
        display: none;
      }

      .controls__header
      {
        z-index: 3;
        pointer-events: auto;
        cursor: help;
        backdrop-filter: blur(10px) brightness(.7);
        padding: 2px 8px;
        display: flex;
        align-items: center;
        white-space: nowrap;
        color: #51CF66;
        font-weight: bold;
      }

      .controls__header::before {
        width: 24px;
        height: 24px;
        margin-right: 4px;
        background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 32 32' height='32px' width='32px' x='0px' y='0px'%3E%3Cpath fill-rule='evenodd' xml:space='preserve' fill='%23fff' d='M9.61 23.25h12.78L16 12 9.61 23.25z'%3E%3C/path%3E%3Cpath d='M3 26.8l7.67-4.52L16 13V4a.45.45 0 0 0-.38.28l-6.27 11-6.26 11a.48.48 0 0 0 0 .48' fill='%23ff4724' fill-rule='evenodd'%3E%3C/path%3E%3Cpath d='M16 4v9l5.29 9.31L29 26.8a.48.48 0 0 0-.05-.48l-6.26-11-6.27-11A.45.45 0 0 0 16 4' fill='%239e1f63' fill-rule='evenodd'%3E%3C/path%3E%3Cpath d='M29 26.8l-7.67-4.52H10.71L3 26.8a.47.47 0 0 0 .43.2h25.1a.47.47 0 0 0 .43-.2' fill='%23662d91' fill-rule='evenodd'%3E%3C/path%3E%3C/svg%3E");
        background-size: contain;
        content: '';
        display: block;
        flex-shrink: 1;
      }

      .controls__header::after {
        margin-left: 4px;
        height: 20px;
        width: 20px;
        background-size: contain;
        color: white;
        content: '';
        display: block;
        flex-shrink: 1;
        background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 32 32' width='32px' height='32px' aria-hidden='true' focusable='false'%3E%3Cpath fill='white' d='M16 28C9.37258 28 4 22.62742 4 16S9.37258 4 16 4s12 5.37258 12 12-5.37258 12-12 12zm0-2c5.52285 0 10-4.47715 10-10S21.52285 6 16 6 6 10.47715 6 16s4.47715 10 10 10zm0-4c-.55228 0-1-.44772-1-1s.44772-1 1-1 1 .44772 1 1-.44772 1-1 1zm-1-11c0-.55228.44772-1 1-1s1 .44772 1 1v6.07107c0 .55228-.44772 1-1 1s-1-.44772-1-1V11z'%3E%3C/path%3E%3C/svg%3E");
        transform: rotate(180deg);
      }

      .controls__data
      {
        padding: 10px;
        opacity: 0;
        position: absolute;
        top: 0; bottom: 0;
        right: 0; left: 0;
        padding-top: 40px;
        backdrop-filter: blur(10px) brightness(.7);
        z-index: 2;
        transition: opacity .14s ease-in-out;
      }


      .controls__toggle:checked ~ .controls__data
      {
        opacity: 1;
        pointer-events: auto;
      }

      .controls__data p
      {
        max-width: 500px;
        margin: 0 auto 8px auto;
        line-height: 1.2;
      }

      .controls__data p:first-of-type
      {
        font-weight: bold;
      }

      .creative {
        border: 0;
        position: absolute;
        top: 0;
        bottom: 0;
        left: 0;
        right: 0;
        width: 100%;
        height: 100%;
        z-index: 1;
      }

      :host(:not([creative-url])) > * {
        display: none;
      }
    </style>
    <iframe class="creative"></iframe>
    <label class="controls">
      <input class="controls__toggle" type="checkbox" />
      <div class="controls__header">+0.05 BAT</div>
      <div class="controls__data">
        <p>
        This is a Brave Rewards advertisement. You earn tokens whenever you see one.
        </p>
        <p>
          This is served from your local Brave Browser app, and is only being shown because you have selected to show ads that are provided from Brave Rewards. Open Settings to turn these ads off.
        </p>
        <p>
          Brave Rewards protects your privacy by making it impossible for this advertiser to track your activity or gain any personal data from your web browser or the websites you visit.
        </p>
      </div>
    </label>
  `

  const wpAdTemplate = document.createElement('template')
  wpAdTemplate.innerHTML = `${templateInnerHTML}`

  // BATAd is generic - it should render an ad on any site
  // When provided with required data (target ad sizes),
  // it communicates with content script and backend to ask
  // for an ad.
  class BATAd extends HTMLElement {
    static get observedAttributes() { return ['responsive', 'id', 'creative-dimensions', 'creative-url']; }

    constructor() {
      super()
      this.attachShadow({mode: 'open'})
      this.shadowRoot.appendChild(wpAdTemplate.content.cloneNode(true))
    }

    get batAdId() {
      let val = this.getAttribute('bat-ad-id')
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
      }
    }

    connectedCallback() {
      requestAnimationFrame(() => {
        this.fetchCreative()
      })
    }

    fetchCreative() {
      if (!this.isConnected) {
        console.warn('BATAd: attempted to call fetchCreative when element not in DOM')
      }
      console.log('fetching creative', this, this.sizes, this.isResponsive)
      if (this.sizes && this.sizes.length) {
        console.log('performing pre-flight')
        this.setAttribute('bat-ad-id', '' + Math.random().toString(36).substr(2, 9))
        this.innerText = 'Fetching creative...'
        window.dispatchEvent(new CustomEvent('bat-ad-position-ready', {
          detail: {
            id: this.batAdId,
            isResponsive: this.isResponsive,
            sizes: this.sizes
          },
          bubbles: true
        }))
      }
    }
  }

  function targetAdSlotsBySelector(selector, fnGetAdSizes) {
    window.customElements.define('batsense-ad', BATAd)
    let slots = document.querySelectorAll(selector)
    for (const element of slots) {
      const slotSizes = fnGetAdSizes(element)
      console.log('BATSense: got sizes for potential ad slot', slotSizes, element)
      if (slotSizes && slotSizes.length) {
        // const alreadyHasShadow = element.shadowRoot != null
        // if (alreadyHasShadow) {
        //   console.warn('BATSense: potential slot already had shadow root', element)
        //   continue
        // }
        // const shadow = element.attachShadow({mode: 'closed'})
        const batAd = document.createElement('batsense-ad')
        element.appendChild(batAd)
        batAd.sizes = slotSizes
      }
    }
  }

  function getSizeForAdSlot (element) {
    const adOptionsString = element.getAttribute('data-ad-options')
    if (!adOptionsString) {
      return null
    }
    const adOptions = JSON.parse(adOptionsString)
    if (!adOptions) {
      return null
    }
    return adOptions.adSize
  }

  targetAdSlotsBySelector('[data-ad-options]', getSizeForAdSlot)
}

// When the page has loaded, inject intercept code to document for overtaking
// existing ad positions.
function inject () {
  const scriptEl = document.createElement('script')
  scriptEl.async = true
  scriptEl.textContent = '(' + pageInjectionCode.toString() + ')()'
  ;(document.body || document.documentElement).appendChild(scriptEl)
}

if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', inject)
} else {
  inject()
}

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