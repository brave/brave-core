// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { CrLitElement, css } from '//resources/lit/v3_0/lit.rollup.js';
import { getHtml } from './br_toolbar.html.js';
import { loadTimeData } from '//resources/js/load_time_data.js';

import type { PropertyValues } from '//resources/lit/v3_0/lit.rollup.js';

// TODO: This is a hack while I haven't converted to the search field over yet
import('//resources/brave/br_elements' + '/br_toolbar/br_toolbar_search_field.js' as any)

const customCurrentWebUINameMap: { [key: string]: string } = {
  extensions: 'settings',
  sync: 'settings',
}

export interface CrToolbarElement {
  $: {
    search: HTMLElement
  }
}

export class CrToolbarElement extends CrLitElement {
  static get is() {
    return 'cr-toolbar'
  }

  static override get styles() {
    return css`
  #menuButtonContainer {
    position: absolute;
    left: 0;
    top: 0;
    bottom: 0;
    display: flex;
    align-items: center;
    justify-content: center;
  }
  
  #menuButton {
    --cr-icon-button-fill-color: currentColor;
  }

  .br-toolbar {
    --toolbar-background: var(--leo-gradient-toolbar-background);
    background: var(--leo-gradient-toolbar-background);
    color: var(--leo-color-white);
    height: 56px;
    position: relative;
  }

  .nav-items {
    align-items: stretch;
    display: flex;
    justify-content: center;
    margin: 0;
    padding: 6px 0;
    gap: 6px;
  }

  .nav-items-list-item {
    display: flex;
  }

  .nav-item {
    align-items: center;
    color: var(--leo-color-white);
    cursor: pointer;
    display: flex;
    /* update br_toolbar.js font-load detection
    if font-weight or name changes */
    font: var(--leo-font-components-navbutton) !important;
    opacity: 0;
    overflow: hidden;
    padding: 0 var(--leo-spacing-xl);
    text-decoration: none;
    transition: background 100ms ease-out;
    height: 44px;
    border-radius: var(--leo-radius-l);
  }

  .fonts-loaded .nav-item {
    opacity: 1;
  }
  .nav-item:hover {
    background: rgba(0, 0, 0, 0.1);
  }
  .nav-item:focus {
    background: rgba(0, 0, 0, 0.2);
  }
  .nav-item:active {
    background: rgba(0, 0, 0, 0.4);
  }
  .nav-item.-selected {
    background: rgba(0, 0, 0, 0.4);
    cursor: default;
  }

  .nav-item-icon {
    align-items: center;
    color: inherit;
    display: flex;
  }

  .nav-item-icon path {
    fill: currentColor;
  }

  .nav-item-text {
    display: block;
    margin: 0 0 0 8px;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
  @media only screen and (max-width: 1200px) {
    .nav-item-text {
      display: none;
    }
  }

  .toolbar-extra {
    position: absolute;
    right: 0;
    top: 100%;
    z-index: 1;
  }

  .toolbar-extra.-slot-filled {
    color: rgb(34, 34, 34);
    padding: 5px;
  }
  @media (prefers-color-scheme: dark) {
    .toolbar-extra.-slot-filled {
      color: rgb(221, 225, 226);
    }
  }
`
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
      showMenuPromo: {
        type: Boolean,
      },

      // Controls whether the search field is shown.
      showSearch: { type: Boolean },

      // True when the toolbar is displaying in narrow mode.
      narrow: {
        type: Boolean,
        reflectToAttribute: true,
        readonly: true,
        notify: true,
      },

      /**
       * The threshold at which the toolbar will change from normal to narrow
       * mode, in px.
       */
      narrowThreshold: {
        type: Number,
      },

      closeMenuPromo: { type: String },

      noSearch: {
        observer: 'noSearchChanged_',
        type: Boolean,
      },

      /** @private */
      showingSearch_: {
        type: Boolean,
        reflectToAttribute: true,
      },

      shouldShowRewardsButton_: {
        type: Boolean,
      },

      isBraveWalletAllowed_: {
        type: Boolean,
      },

      fontsLoadedClassName: {
        type: String
      }
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
  noSearch = false
  showingSearch = false
  showRewardsButton = true
  isBraveWalletAllowed_ = loadTimeData.getBoolean('brToolbarShowRewardsButton')

  // Localized strings:
  // These will be loaded from `loadTimeData`
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
  toolbarExtraSlot: HTMLSlotElement
  toolbarExtraElement: Element

  override willUpdate(changedProperties: PropertyValues<this>) {
    super.willUpdate(changedProperties);
    if (changedProperties.has('narrowThreshold')) {
      this.narrowQuery_ = window.matchMedia(`(max-width: ${this.narrowThreshold}px)`);
      this.narrow = this.narrowQuery_.matches
      this.narrowQuery_.addEventListener('change', () => {
        this.narrow = this.narrowQuery_?.matches ?? false;
      });
    }
  }

  /** @return {!CrToolbarSearchFieldElement} */
  getSearchField() {
    return this.$.search;
  }

  /** @private */
  onMenuClick_() {
    this.dispatchEvent(new CustomEvent(
      'cr-toolbar-menu-click', { bubbles: true, composed: true }))
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

  /** @private */
  noSearchChanged_() {
    this.updateSearchDisplayed_()
  }

  /** @private */
  updateSearchDisplayed_() {
    this.$.search.hidden = this.noSearch
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
      currentWebUIName = customCurrentWebUINameMap[currentWebUIName]

    if (itemName === currentWebUIName)
      return '-selected'
    // not selected
    return ''
  }

  notifyIfExtraSlotFilled() {
    const slotIsFilled = this.toolbarExtraSlot.assignedNodes().length !== 0
    const classNameFilled = '-slot-filled'
    if (slotIsFilled) {
      this.toolbarExtraElement.classList.add(classNameFilled)
    } else {
      this.toolbarExtraElement.classList.remove(classNameFilled)
    }
  }

  initSlotFilledDetection() {
    // Style the 'extra items' slot only if it contains
    // content.
    const toolbarExtraElement = this.querySelector('.toolbar-extra')
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

/* declare global {
    interface HTMLElementTagNameMap {
        'cr-toolbar': CrToolbarElement;
    }
} */

customElements.define(
  CrToolbarElement.is, CrToolbarElement)


