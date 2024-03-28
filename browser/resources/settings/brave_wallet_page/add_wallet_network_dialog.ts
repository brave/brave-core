// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js';
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {I18nMixin, I18nMixinInterface} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {BaseMixin} from '../base_mixin.js'
import {loadTimeData} from '../i18n_setup.js'
import {NetworkInfo, BraveWalletBrowserProxy, BraveWalletBrowserProxyImpl} from './brave_wallet_browser_proxy.js';
import {CrInputElement} from 'chrome://resources/cr_elements/cr_input/cr_input.js'
import {getTemplate} from './add_wallet_network_dialog.html.js'
import '../ui/searchable_drop_down_cros.js';

const SettingsBraveAddWalletNetworkDialogElementBase =
  I18nMixin(BaseMixin(PolymerElement)) as {
    new(): PolymerElement & I18nMixinInterface
  }

export interface Url {
  value: string;
}

export interface ExtendedNetworkInfo extends NetworkInfo {
  searchString: string;
}

export type UrlType = 'rpc' | 'icon' | 'block';

declare global {
  interface Window {
    testing: any;
  }
}

export class SettingsBraveAddWalletNetworkDialogElement extends SettingsBraveAddWalletNetworkDialogElementBase {
  static get is() {
    return 'add-wallet-network-dialog'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      addNewAllowed: {
        type: Boolean,
        value: false
      },
      coin: {
        type: Number,
        value: 0
      },
      networks: {
        type: Array,
        value() {
          return [];
        },
      },
      selectedRpcUrl: {
        type: String,
        value: ''
      },
      rpcUrls: {
        type: Array,
        value() {
          return [{ value: '' }];
        }
      },
      iconUrls: {
        type: Array,
        value() {
          return [{ value: '' }];
        }
      },
      blockUrls: {
        type: Array,
        value() {
          return [{ value: '' }];
        }
      },
      isRpcPlusButtonDisabled_: {
        type: Boolean,
        value: true,
      },
      isIconPlusButtonDisabled_: {
        type: Boolean,
        value: true,
      },
      isBlockPlusButtonDisabled_: {
        type: Boolean,
        value: true,
      },
      isSubmitButtonEnabled_: {
        type: Boolean,
        value: false,
      },

      chainIdValue_: String,
      invalidChainIdMessage_: String,
      chainIdInvalid_: {
        type: Boolean,
        value: false,
      },

      chainNameValue_: String,
      chainNameInvalid_: {
        type: Boolean,
        value: false,
      },
      isCurrencyErrorHidden_: {
        type: Boolean,
        value: true,
      },
      isSubmissionErrorHidden_: {
        type: Boolean,
        value: true,
      },
      submissionErrorMessage_: {
        type: String,
        value: true
      },
      selected: {
        type: Object,
        value: {}
      },
      currencyNameValue_: String,
      currencySymbolValue_: String,
      currencyDecimalsValue_: Number,
      prepopulatedNetworks_: {
        type: Array,
        value: []
      },
      searchValue_: {
        type: String,
        value: '',
        observer: 'onSearchValueChanged_',
      },
      searchItems_: {
        type: Array,
        value: []
      },
    };
  }

  private addNewAllowed: boolean;
  private coin: number;
  private networks: NetworkInfo[];
  private selectedRpcUrl: string;
  private rpcUrls: Url[];
  private iconUrls: Url[];
  private blockUrls: Url[];
  private isRpcPlusButtonDisabled_: boolean;
  private isIconPlusButtonDisabled_: boolean;
  private isBlockPlusButtonDisabled_: boolean;
  private isSubmitButtonEnabled_: boolean;
  private chainIdValue_: string;
  private invalidChainIdMessage_: string;
  private chainIdInvalid_: boolean;
  private chainNameValue_: string;
  private chainNameInvalid_: boolean;
  private isCurrencyErrorHidden_: boolean;
  private isSubmissionErrorHidden_: boolean;
  private submissionErrorMessage_: string;
  private selected: NetworkInfo;
  private currencyNameValue_: string;
  private currencySymbolValue_: string;
  private currencyDecimalsValue_: number;
  private prepopulatedNetworks_: ExtendedNetworkInfo[];
  private searchValue_: string;
  private searchItems_: string[];

  browserProxy_: BraveWalletBrowserProxy = BraveWalletBrowserProxyImpl.getInstance();

  override ready() {
    super.ready()
    this.updatePrepopulatedNetworks()

    if (Object.keys(this.selected).length === 0)
      return
    this.chainIdValue_ = this.selected.chainId
    this.chainNameValue_ = this.selected.chainName
    this.currencyNameValue_ = this.selected.nativeCurrency.name
    this.currencySymbolValue_ = this.selected.nativeCurrency.symbol
    const decimals = this.selected.nativeCurrency.decimals
    if (decimals) {
      this.currencyDecimalsValue_ = decimals
    }
    this.rpcUrls = this.selected.rpcUrls.map(value => { return { value } })
    this.selectedRpcUrl = this.selected.rpcUrls[this.selected.activeRpcEndpointIndex] || this.selected.rpcUrls[0]
    if (this.selected.iconUrls.length)
      this.iconUrls = this.selected.iconUrls.map(value => { return { value } })
    if (this.selected.blockExplorerUrls.length)
      this.blockUrls = this.selected.blockExplorerUrls.map(value => { return { value } })
    this.isSubmitButtonEnabled_ = true
    this.updatePlusButtonState('rpc')
    this.updatePlusButtonState('icon')
    this.updatePlusButtonState('block')
  }

  override connectedCallback() {
    super.connectedCallback()
    if (loadTimeData.getBoolean('shouldExposeElementsForTesting')) {
      window.testing = window.testing || {}
      window.testing['addWalletNetworkDialog'] = this.shadowRoot
    }
  }

  override disconnectedCallback() {
    super.disconnectedCallback()
    if (loadTimeData.getBoolean('shouldExposeElementsForTesting')) {
      delete window.testing['addWalletNetworkDialog']
    }
  }

  updatePrepopulatedNetworks() {
    if (!this.addNewAllowed)
      return
    this.browserProxy_.getPrepopulatedNetworksList().then(payload => {
      if (!payload)
        return

      this.prepopulatedNetworks_ = []
      payload.forEach(item => {
        this.prepopulatedNetworks_.push(
          {...item, searchString: `${item.chainId}(${BigInt(item.chainId)}) ${item.chainName}`}
        )
      })
      this.searchItems_ = this.prepopulatedNetworks_.map(item => item.searchString)
    })
  }

  validateURL(value: string) {
    const url_ = value
    if (url_.trim() == '') {
      return false;
    }
    let url;
    try {
      url = new URL(url_.trim());
    } catch (e) {
      return false;
    }

    return url.protocol === "http:" || url.protocol === "https:"
  }

  chainIdChanged_() {
    const value = this.chainIdValue_
    this.chainIdInvalid_ = value.length === 0
    if (this.chainIdInvalid_) {
      this.invalidChainIdMessage_ = this.i18n('walletAddNetworkInvalidChainId')
    }
    this.updateSubmitButtonState_()
  }

  chainNameChanged_() {
    this.chainNameInvalid_ = this.chainNameValue_.trim() === ''
    this.updateSubmitButtonState_()
  }

  isInvalidInputForList_(value: string, list: string) {
    if (value.trim() === '') {
      return (list === 'icon' || list === 'block') ? false : true
    }
    return !this.validateURL(value)
  }

  getTailOfUrlsArray(list: UrlType) {
    if (list === 'rpc') {
      return this.rpcUrls.at(0)!.value
    } else if (list === 'icon') {
      return this.iconUrls.at(0)!.value
    } else /* (list === 'block') */ {
      return this.blockUrls.at(0)!.value
    }
  }

  updatePlusButtonState(list: UrlType) {
    const inputElementValue = this.getTailOfUrlsArray(list)
    const disabled = (!inputElementValue || inputElementValue.trim() === '')
    if (list === 'rpc') {
      this.isRpcPlusButtonDisabled_ = disabled
    } else if (list === 'icon') {
      this.isIconPlusButtonDisabled_ = disabled
    } else if (list === 'block') {
      this.isBlockPlusButtonDisabled_ = disabled
    }
  }

  updateSubmitButtonState_() {
    for (const inputElement of this.shadowRoot!.querySelectorAll('.mandatory')) {
      const input = inputElement as CrInputElement
      if (input && (input.invalid || !input.value || (input.value.trim && input.value.trim() === ''))) {
        this.isSubmitButtonEnabled_ = false
        return;
      }
    }
    for (const inputElement of this.shadowRoot!.querySelectorAll('.wallet-input')) {
      const input = inputElement as CrInputElement
      if (input && input.invalid) {
        this.isSubmitButtonEnabled_ = false
        return;
      }
    }
    if (this.chainIdValue_.length === 0) {
      this.isSubmitButtonEnabled_ = false
      return;
    }

    if (this.chainNameValue_ === '') {
      this.isSubmitButtonEnabled_ = false
      return;
    }
    if (!this.hasValidRPCUrls()) {
      this.isSubmitButtonEnabled_ = false
      return;
    }

    this.isSubmitButtonEnabled_ = true
  }

  nativeCurrencyChanged_() {
    this.isCurrencyErrorHidden_ = true
  }

  // Called for any change in the urls inputs
  // validates value of focused one and shows error if the value is invalid
  // calls update for Plus button state
  urlChangedImpl_(element: CrInputElement, list: UrlType) {
    element.invalid = this.isInvalidInputForList_(element.value, list)
    this.updateSubmitButtonState_()
    const empty = element.value.trim() === ''
    if (list == 'rpc' && element.invalid) {
      const text = empty && !this.hasValidRPCUrls() ? this.i18n('walletAddNetworkMandarotyFieldError')
        : this.i18n('walletAddNetworkInvalidURLInput')
      element.setAttribute('error-message', text)
    }
    if (!element.invalid || empty) {
      this.updatePlusButtonState(list)
    }
  }

  hasValidRPCUrls() {
    return this.rpcUrls.find(element => this.validateURL(element.value))
  }

  urlChangedIcons_(event: Event) {
    return this.urlChangedImpl_((event.target as CrInputElement), 'icon')
  }

  urlChangedRpc_(event: Event) {
    return this.urlChangedImpl_((event.target as CrInputElement), 'rpc')
  }

  urlChangedBlock_(event: Event) {
    return this.urlChangedImpl_((event.target as CrInputElement), 'block')
  }

  onAddRpcUrlTap_() {
    this.splice('rpcUrls', 0, 0, { value: '' });
    this.isRpcPlusButtonDisabled_ = true
  }

  onAddIconUrlTap_() {
    this.splice('iconUrls', 0, 0, { value: '' });
    this.isIconPlusButtonDisabled_ = true
  }

  onAddBlockUrlTap_() {
    this.splice('blockUrls', 0, 0, { value: '' });
    this.isBlockPlusButtonDisabled_ = true
  }

  transformListForSerializaion_(list: Url[]) {
    return list.reduce((filtered: string[] | null, item: Url) => {
      if (item && item.value.trim() !== '') {
        if (!filtered) {
          filtered = []
        }
        filtered.push(item.value)
      }
      return filtered
    }, null)
  }

  showCurrencyError() {
    this.isSubmissionErrorHidden_ = true
    this.isCurrencyErrorHidden_ = false
  }

  setSubmissionResult(success: boolean, errorMessage: string) {
    this.isCurrencyErrorHidden_ = this.isSubmissionErrorHidden_ = true
    if (!success) {
      this.isSubmissionErrorHidden_ = false
      this.submissionErrorMessage_ = errorMessage
    }
  }

  addNewNetwork(payload: NetworkInfo) {
    this.browserProxy_.addChain(payload)
      .then(([success, errorMessage]) => {
        this.setSubmissionResult(success, errorMessage)
        if (success) {
          this.dispatchEvent(new CustomEvent('close'));
          return
        }
      })
  }

  getHexNumber(value: string) {
    if (!isNaN(Number(value)))
      return '0x' + Number(value).toString(16)
    return value
  }

  onAddNetworkTap_() {
    let payload = Object({
      chainId: this.getHexNumber(this.chainIdValue_),
      chainName: this.chainNameValue_,
    })
    const nativeCurrency = Object({
      name: this.currencyNameValue_.trim(),
      symbol: this.currencySymbolValue_.trim(),
      decimals: this.currencyDecimalsValue_
    })
    if ((nativeCurrency.name || nativeCurrency.symbol || nativeCurrency.decimals)) {
      if (!nativeCurrency.name || !nativeCurrency.symbol || !nativeCurrency.decimals) {
        this.showCurrencyError()
        return;
      }
      payload.nativeCurrency = nativeCurrency;
    }
    payload.coin = this.coin
    payload.rpcUrls = this.transformListForSerializaion_(this.rpcUrls)
    payload.activeRpcEndpointIndex = payload.rpcUrls.findIndex((it: string) => it === this.selectedRpcUrl)
    if (payload.activeRpcEndpointIndex < 0)
      payload.activeRpcEndpointIndex = 0
    payload.blockExplorerUrls = this.transformListForSerializaion_(this.blockUrls)
    payload.iconUrls = this.transformListForSerializaion_(this.iconUrls)
    if (this.networks.find(element => { return element.chainId === payload.chainId })) {
      if (!window.confirm(this.i18n('walletAddNetworkDialogReplaceNetwork'))) {
        return
      }
      this.browserProxy_.removeChain(payload.chainId, this.coin).then((success) => {
        if (!success)
          return;
        this.addNewNetwork(payload)
      })
      return
    }
    this.addNewNetwork(payload)
  }

  onSearchValueChanged_(newValue: string) {
    if (!newValue)
      return
    const found = this.prepopulatedNetworks_.find(item => item.searchString === newValue)
    if (found) {
      this.chainIdValue_ = found.chainId
      this.invalidChainIdMessage_ = ''
      this.chainIdInvalid_ = false
      this.chainNameValue_ = found.chainName
      this.chainNameInvalid_ = false
      this.currencyNameValue_ = found.nativeCurrency.name
      this.currencySymbolValue_ = found.nativeCurrency.symbol
      const decimals = found.nativeCurrency.decimals
      if (decimals) {
        this.currencyDecimalsValue_ = decimals
      }
      this.rpcUrls = found.rpcUrls.map(value => { return { value } })
      this.selectedRpcUrl = found.rpcUrls[found.activeRpcEndpointIndex] || found.rpcUrls[0]
      if (found.iconUrls.length)
        this.iconUrls = found.iconUrls.map(value => { return { value } })
      if (found.blockExplorerUrls.length)
        this.blockUrls = found.blockExplorerUrls.map(value => { return { value } })
      this.isSubmitButtonEnabled_ = true
      this.updatePlusButtonState('rpc')
      this.updatePlusButtonState('icon')
      this.updatePlusButtonState('block')

      this.searchValue_ = ''
    }
  }
}

customElements.define(
  SettingsBraveAddWalletNetworkDialogElement.is, SettingsBraveAddWalletNetworkDialogElement)
