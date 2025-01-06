// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_button/cr_button.js'
import 'chrome://resources/cr_elements/cr_icon_button/cr_icon_button.js'
import 'chrome://resources/cr_elements/icons.html.js'
import 'chrome://resources/cr_elements/cr_shared_style.css.js'
import 'chrome://resources/cr_elements/cr_shared_vars.css.js'
import '../settings_shared.css.js'
import '../settings_vars.css.js'
import '../people_page/sync_controls.js'
import './brave_sync_configure.js'
import './brave_sync_setup.js'

import {
  StatusAction,
  SyncBrowserProxyImpl
} from '/shared/settings/people_page/sync_browser_proxy.js'

import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {assert} from 'chrome://resources/js/assert.js'
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import {RouteObserverMixin, Router} from '../router.js'

import {
  BraveSyncBrowserProxy,
  BraveSyncStatus
} from './brave_sync_browser_proxy.js'

import {getTemplate} from './brave_sync_subpage.html.js'

type PageStatus = 'configure' | 'setup' | 'spinner'

const SettingBraveSyncSubpageBase =
  I18nMixin(RouteObserverMixin(PolymerElement))

class SettingBraveSyncSubpage extends SettingBraveSyncSubpageBase {
  static get is() {
    return 'settings-brave-sync-subpage'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      pageStatus_: {
        type: String,
        value: 'configure',
      },

      /**
       * The current sync preferences, supplied by SyncBrowserProxy.
       * @type {SyncPrefs|undefined}
       */
      syncPrefs: {
        type: Object,
      },

      /** @type {BraveSyncStatus} */
      syncStatus: {
        type: Object,
      },

      syncCodeDialogType: {
        type: String,
        notify: true,
      },

      syncDisabledByAdmin_: {
        type: Boolean,
        value: false,
        computed: 'computeSyncDisabledByAdmin_(syncStatus.managed)',
      },

      hasLockedSafeStorage_ : {
        type: Boolean,
        value: false,
        computed: 'computeLockedSafeStorage_(syncStatus)',
      },

      hasUnlockedSafeStorageCannotDecryptSeed_ : {
        type: Boolean,
        value: false,
        computed: 'computeUnlockedSafeStorageCannotDecryptSeed_(syncStatus)',
      },

      syncSectionDisabled_: {
        type: Boolean,
        value: false,
        computed: 'computeSyncSectionDisabled_(' +
          'syncStatus.disabled, ' +
          'syncStatus.hasError, syncStatus.statusAction, ' +
          'syncPrefs.trustedVaultKeysRequired)',
      },
    }
  }

  private pageStatus_: PageStatus
  private syncStatus: BraveSyncStatus
  private syncCodeDialogType: 'qr' | 'words' | 'input' | 'choose' | null
  private syncDisabledByAdmin_: boolean
  private hasLockedSafeStorage_: boolean
  private hasUnlockedSafeStorageCannotDecryptSeed_: boolean
  private syncSectionDisabled_: boolean

  private browserProxy_ = SyncBrowserProxyImpl.getInstance()

  private braveSyncBrowserProxy_: BraveSyncBrowserProxy =
    BraveSyncBrowserProxy.getInstance()

  /**
   * The beforeunload callback is used to show the 'Leave site' dialog. This
   * makes sure that the user has the chance to go back and confirm the sync
   * opt-in before leaving.
   *
   * This property is non-null if the user is currently navigated on the sync
   * settings route.
   */
  private beforeunloadCallback_: ((e: Event) => void) | null = null

  /**
   * The unload callback is used to cancel the sync setup when the user hits
   * the browser back button after arriving on the page.
   * Note: Cases like closing the tab or reloading don't need to be handled,
   * because they are already caught in |PeopleHandler::~PeopleHandler|
   * from the C++ code.
   */
  private unloadCallback_: ((e: Event) => void) | null = null

  /**
   * Whether the user completed setup successfully.
   */
  private setupSuccessful_: boolean = false

  static get observers() {
    return [
      'updatePageStatus_(syncStatus.*)'
    ]
  }

  override currentRouteChanged() {
    const router = Router.getInstance()
    if (router.getCurrentRoute() === router.getRoutes().BRAVE_SYNC_SETUP) {
      this.onNavigateToPage_()
      return
    }

    if (router.getRoutes().BRAVE_SYNC_SETUP.
      contains(router.getCurrentRoute())) {
      return
    }

    this.onNavigateAwayFromPage_()
  }

  override connectedCallback() {
    super.connectedCallback()
    const router = Router.getInstance()
    if (router.getCurrentRoute() === router.getRoutes().BRAVE_SYNC_SETUP) {
      this.onNavigateToPage_()
    }
  }

  override disconnectedCallback() {
    const router = Router.getInstance()
    if (router.getRoutes().BRAVE_SYNC_SETUP.
      contains(router.getCurrentRoute())) {
      this.onNavigateAwayFromPage_()
    }

    if (this.beforeunloadCallback_) {
      window.removeEventListener('beforeunload', this.beforeunloadCallback_)
      this.beforeunloadCallback_ = null
    }
    if (this.unloadCallback_) {
      window.removeEventListener('unload', this.unloadCallback_)
      this.unloadCallback_ = null
    }

    super.disconnectedCallback()
  }

  private updatePageStatus_(_newValue: PageStatus, _oldValue: PageStatus) {
    const isFirstSetup = this.syncStatus && this.syncStatus.firstSetupInProgress
    this.pageStatus_ = isFirstSetup ? 'setup' : 'configure'
  }

  private computeSyncSectionDisabled_(): boolean {
    return this.syncStatus !== undefined &&
      (!!this.syncStatus.disabled ||
        !!this.syncStatus.hasSyncWordsDecryptionError ||
        (!!this.syncStatus.hasError &&
          this.syncStatus.statusAction !==
          StatusAction.ENTER_PASSPHRASE &&
          this.syncStatus.statusAction !==
          StatusAction.RETRIEVE_TRUSTED_VAULT_KEYS))
  }

  private computeSyncDisabledByAdmin_(): boolean {
    return !!this.syncStatus?.managed
  }

  private computeLockedSafeStorage_(): boolean {
    return this.syncStatus &&
           !!this.syncStatus.hasSyncWordsDecryptionError &&
           !this.syncStatus.isOsEncryptionAvailable
  }

  private computeUnlockedSafeStorageCannotDecryptSeed_(): boolean {
    return this.syncStatus &&
      !!this.syncStatus.hasSyncWordsDecryptionError &&
      !!this.syncStatus.isOsEncryptionAvailable
  }

  private isStatus_(expectedPageStatus: PageStatus): boolean {
    return expectedPageStatus === this.pageStatus_
  }

  private onNavigateToPage_() {
    const router = Router.getInstance()
    assert(router.getCurrentRoute() === router.getRoutes().BRAVE_SYNC_SETUP)
    if (this.beforeunloadCallback_) {
      return
    }

    // Triggers push of prefs to our handler
    this.browserProxy_.didNavigateToSyncPage()

    this.beforeunloadCallback_ = event => {
      // When the user tries to leave the sync setup, show the 'Leave site'
      // dialog.
      if (this.syncStatus && this.syncStatus.firstSetupInProgress) {
        event.preventDefault()
        event.returnValue = false

        chrome.metricsPrivate.recordUserAction(
          'Signin_Signin_AbortAdvancedSyncSettings')
      }
    }
    window.addEventListener('beforeunload', this.beforeunloadCallback_)

    this.unloadCallback_ = this.onNavigateAwayFromPage_.bind(this)
    window.addEventListener('unload', this.unloadCallback_)
  }

  private onNavigateAwayFromPage_() {
    if (!this.beforeunloadCallback_) {
      return
    }

    this.browserProxy_.didNavigateAwayFromSyncPage(!this.setupSuccessful_)

    // Reset state as this component could actually be kept around even though
    // it is hidden.
    this.setupSuccessful_ = false

    window.removeEventListener('beforeunload', this.beforeunloadCallback_)
    this.beforeunloadCallback_ = null

    if (this.unloadCallback_) {
      window.removeEventListener('unload', this.unloadCallback_)
      this.unloadCallback_ = null
    }
  }

  /**
   * Called when setup is complete and sync code is set
   */
  private onSetupSuccess_() {
    this.setupSuccessful_ = true
    // This navigation causes the firstSetupInProgress flag to be marked as
    // false via `didNavigateAwayFromSyncPage`.
    const router = Router.getInstance()
    if (router.getCurrentRoute() === router.getRoutes().BRAVE_SYNC_SETUP) {
      router.navigateTo(router.getRoutes().BRAVE_SYNC)
    }
  }

  /**
   * Called when we see that OS safe storage is not locked, but we can't decrypt
   * the passphrase. So we proposing user to clear data and re-join the chain.
   */
  async onRejoin_(_e: Event) {
    const messageText = this.i18n('braveSyncResetConfirmation')
    const shouldReset = confirm(messageText)
    if (!shouldReset) {
      return
    }
    this.pageStatus_ = 'spinner'
    await this.braveSyncBrowserProxy_.resetSyncChain()

    this.syncCodeDialogType = 'input'
  }
}

customElements.define(
  SettingBraveSyncSubpage.is, SettingBraveSyncSubpage)
