// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'
import { loadTimeData } from '//resources/js/load_time_data.js'

import type { PropertyValues } from '//resources/lit/v3_0/lit.rollup.js'
import { getHtml } from './cr_toolbar.html.js'
import { getCss } from './cr_toolbar.css.js'

import '//resources/brave/br_elements/br_toolbar/br_toolbar_search_field.js'

const customCurrentWebUINameMap: { [key: string]: string } = {
  extensions: 'settings',
  sync: 'settings',
}

// TODO(simonhong): Avoid any and use its type(BrToolbarSearchFieldElement).
// Can't import type from //resources/brave/br_elements/br_toolbar/br_toolbar_search_field.js.
export interface CrToolbarElement {
  $: {
    search: any
  }
}

export class CrToolbarElement extends CrLitElement {
  static get is() {
    return 'cr-toolbar'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      // Name to display in the toolbar, in titlecase.
      pageName: { type: String },

      // Prompt text to display in the search field.
      searchPrompt: { type: String },

      // Tooltip to display on the clear search button.
      clearLabel: { type: String },

      // Tooltip to display on the menu button.
      menuLabel: { type: String },

      // Promotional toolstip string, shown in narrow mode if showMenuPromo is
      // true.
      menuPromo: { type: String },

      // Value is proxied through to cr-toolbar-search-field. When true,
      // the search field will show a processing spinner.
      spinnerActive: { type: Boolean },

      // Controls whether the menu button is shown at the start of the menu.
      showMenu: { type: Boolean },

      // Whether to show menu promo tooltip.
      showMenuPromo: { type: Boolean },

      // Controls whether the search field is shown.
      showSearch: { type: Boolean },

      // True when the toolbar is displaying in narrow mode.
      narrow: { type: Boolean, reflect: true, notify: true },

      /**
       * The threshold at which the toolbar will change from normal to narrow
       * mode, in px.
       */
      narrowThreshold: { type: Number },

      closeMenuPromo: { type: String },

      /** @private */
      showingSearch_: {
        type: Boolean,
        reflect: true,
      },

      shouldShowRewardsButton_: {
        type: Boolean,
      },

      isBraveWalletAllowed_: {
        type: Boolean,
      },

      fontsLoadedClassName: {
        type: String
      },

      alwaysShowLogo: { type: Boolean, reflect: true },
      searchIconOverride: { type: String },
      searchInputAriaDescription: { type: String },
    }
  }

  pageName = ''
  searchPrompt = ''
  clearLabel = ''
  menuLabel = ''
  menuPromo = ''
  spinnerActive = false
  showMenu = false
  showMenuPromo = false
  showSearch = true
  narrow = false
  narrowThreshold = 900
  narrowQuery_: MediaQueryList | null = null
  closeMenuPromo = ''
  showingSearch = false
  showRewardsButton = true
  isBraveWalletAllowed_ = loadTimeData.getBoolean('brToolbarShowRewardsButton')

  alwaysShowLogo = false
  searchIconOverride?: string
  searchInputAriaDescription = ''

  // Localized strings:
  historyTitle = loadTimeData.getString('brToolbarHistoryTitle')
  settingsTitle = loadTimeData.getString('brToolbarSettingsTitle')
  bookmarksTitle = loadTimeData.getString('brToolbarBookmarksTitle')
  downloadsTitle = loadTimeData.getString('brToolbarDownloadsTitle')
  braveRewardsTitle = loadTimeData.getString('brToolbarRewardsTitle')
  walletsTitle = loadTimeData.getString('brToolbarWalletsTitle')

  // Settings from `loadTimeData`
  shouldShowRewardsButton_ = loadTimeData.getBoolean('brToolbarShowRewardsButton')

  // Non-observed properties
  fontsLoadedClassName = ''

  // Slotted content
  toolbarExtraSlot: HTMLSlotElement | null = null
  toolbarExtraElement: Element | null = null

  override willUpdate(changedProperties: PropertyValues<this>) {
    super.updated(changedProperties);

    if (changedProperties.has('narrowThreshold')) {
      this.narrowQuery_ = window.matchMedia(`(max-width: ${this.narrowThreshold}px)`);
      this.narrow = this.narrowQuery_.matches
      this.narrowQuery_.addEventListener('change', () => {
        this.narrow = this.narrowQuery_?.matches ?? false;
      });
    }
  }

  /** @return {!BrToolbarSearchFieldElement} */
  getSearchField() {
    return this.$.search;
  }

  protected onMenuClick_() {
    this.fire('cr-toolbar-menu-click')
  }

  focusMenuButton() {
    requestAnimationFrame(() => {
      // Wait for next animation frame in case dom-if has not applied yet and
      // added the menu button.
      const menuButton =
        this.shadowRoot!.querySelector<HTMLElement>('#menuButton');
      if (menuButton) {
        menuButton.focus();
      }
    });
  }

  /** @return {boolean} */
  isMenuFocused() {
    return !!this.shadowRoot!.activeElement &&
      this.shadowRoot!.activeElement.id === 'menuButton';
  }

  /**
   * @param {string} title
   * @param {boolean} showMenuPromo
   * @return {string} The title if the menu promo isn't showing, else "".
   */
  titleIfNotShowMenuPromo_(title: string, showMenuPromo: boolean) {
    return showMenuPromo ? '' : title;
  }

  getNavItemSelectedClassName(itemName: string) {
    // which navigation item is the current page?
    let currentWebUIName = document.location.hostname
    // override name from hostname, if applicable
    if (customCurrentWebUINameMap[currentWebUIName])
      currentWebUIName = customCurrentWebUINameMap[currentWebUIName]!

    if (itemName === currentWebUIName)
      return '-selected'
    // not selected
    return ''
  }

  notifyIfExtraSlotFilled() {
    const slotIsFilled = this.toolbarExtraSlot?.assignedNodes().length !== 0
    const classNameFilled = '-slot-filled'
    if (slotIsFilled) {
      this.toolbarExtraElement?.classList.add(classNameFilled)
    } else {
      this.toolbarExtraElement?.classList.remove(classNameFilled)
    }
  }

  initSlotFilledDetection() {
    // Style the 'extra items' slot only if it contains
    // content.
    const toolbarExtraElement = this.shadowRoot!.querySelector('.toolbar-extra')
    if (!toolbarExtraElement) {
      console.error('Could not find "toolbar-extra" element')
      return
    }
    const toolbarExtraSlot = toolbarExtraElement.querySelector('slot')
    if (!toolbarExtraSlot) {
      console.error('Could not find "toolbar-extra" slot')
      return
    }
    this.toolbarExtraElement = toolbarExtraElement
    this.toolbarExtraSlot = toolbarExtraSlot
    this.notifyIfExtraSlotFilled()
    toolbarExtraSlot.addEventListener('slotchange', () => {
      this.notifyIfExtraSlotFilled()
    })
  }

  override connectedCallback() {
    super.connectedCallback()
    this.initSlotFilledDetection()
    this.initFontLoadDetection()
  }

  async initFontLoadDetection() {
    // Font detection.
    // Wait for component to render with font style.
    await new Promise(resolve => window.requestAnimationFrame(resolve))
    const fontFaceString = '300 16px Poppins'
    if (document.fonts.check(fontFaceString)) {
      console.debug('fonts were loaded on init')
      this.setFontsAreLoaded()
      return
    }
    await document.fonts.ready
    if (!document.fonts.check(fontFaceString)) {
      console.debug('warning: fonts ready but string not set!')
    } else {
      console.debug('font was loaded after a wait')
    }
    this.setFontsAreLoaded()
  }

  setFontsAreLoaded() {
    this.fontsLoadedClassName = 'fonts-loaded'
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cr-toolbar': CrToolbarElement;
  }
}

customElements.define(
  CrToolbarElement.is, CrToolbarElement)


