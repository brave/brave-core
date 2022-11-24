// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {WebUIListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js';
import {Router, RouteObserverMixin} from '../router.js';
import {BraveIPFSBrowserProxyImpl} from './brave_ipfs_browser_proxy.js';
import {PrefsMixin} from '../prefs/prefs_mixin.js';
import './p2p_keys_subpage.js';
import './ipfs_peers_subpage.js';
import './change_ipfs_gateway_dialog.js';
import {getTemplate} from './brave_ipfs_page.html.js'

/**
 * Keep it same as in IPFSResolveMethodTypes
 * in brave\components\ipfs\ipfs_constants.h */
const IPFS_RESOLVE_METHOD_TYPES = {
  IPFS_ASK: 0,
  IPFS_GATEWAY: 1,
  IPFS_LOCAL: 2,
  IPFS_DISABLED: 3
}

const SettingBraveIpfsPageElementBase = RouteObserverMixin(WebUIListenerMixin(PrefsMixin(PolymerElement)))

/**
 * 'settings-brave-default-extensions-page' is the settings page containing
 * brave's default extensions.
 */
class SettingsBraveIpfsPageElement extends SettingBraveIpfsPageElementBase {
  static get is() {
    return 'settings-brave-ipfs-page'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      ipfsEnabled_: Boolean,
      showChangeIPFSGatewayDialog_: Boolean,
      showChangeIPFSNFTGatewayDialog_: Boolean,
      isLocalNodeEnabled_: Boolean,
      isLocalNodeLaunched_: {
        type: Boolean,
        value: false,
      },
      showIPFSLearnMoreLink_: Boolean,
      mainBlockVisibility_: String
    }
  }

  /** @private {?settings.BraveIPFSBrowserProxy} */
  browserProxy_ = BraveIPFSBrowserProxyImpl.getInstance()

  ready() {
    super.ready()

    this.browserProxy_.getIPFSResolveMethodList().then(list => {
      this.ipfsResolveMethod_ = JSON.parse(list)
    });
    this.browserProxy_.getIPFSEnabled().then(enabled => {
      this.ipfsEnabled_ = enabled
    });

    this.addWebUIListener('brave-ipfs-node-status-changed', (launched) => {
      this.isLocalNodeLaunched_ = launched
    })
    this.browserProxy_.notifyIpfsNodeStatus();

    window.addEventListener('load', this.onLoad_.bind(this));
    if (document.readyState == 'complete') {
      this.onLoad_()
    }
  }

  currentRouteChanged() {
    let hidden = this.isPeersEditorRoute() || this.isKeysEditorRoute()
    this.mainBlockVisibility_ = hidden ? 'hidden' : ''
  }

  /** @private **/
  setupOptionsVisibility() {
    const resolve_method = this.getPref('brave.ipfs.resolve_method').value
    // Check if IPFS method is LOCAL_NODE
    this.isLocalNodeEnabled_ = (resolve_method == IPFS_RESOLVE_METHOD_TYPES.IPFS_LOCAL) &&
                                this.ipfsEnabled_

    this.showIPFSLearnMoreLink_ =
      (resolve_method == IPFS_RESOLVE_METHOD_TYPES.IPFS_ASK)
    this.$.ipfsStorageMax.value =
      this.getPref('brave.ipfs.storage_max').value
  }

  /** @private **/
  onLoad_() {
    this.setupOptionsVisibility()
    if (this.isKeysEditorRoute() && !this.isLocalNodeEnabled_) {
      const router = Router.getInstance()
      router.navigateTo(router.getRoutes().BRAVE_IPFS)
    }
  }

  /** @private **/
  onIpfsPeersEditorClick_() {
    const router = Router.getInstance()
    router.navigateTo(router.getRoutes().BRAVE_IPFS_PEERS)
  }

  /** @private **/
  onP2pKeysEditorClick_() {
    const router = Router.getInstance()
    router.navigateTo(router.getRoutes().BRAVE_IPFS_KEYS)
  }

  /** @private **/
  onChangeIpfsMethod_() {
    let local_node_used = this.isLocalNodeEnabled_
    this.setupOptionsVisibility()
    // Automatically start node if we changed method to local node
    if (!local_node_used && this.isLocalNodeEnabled_) {
      this.browserProxy_.launchIPFSService().then(success => {
        this.isLocalNodeLaunched_ = success
      });
    } else if (local_node_used && !this.isLocalNodeEnabled_) {
      this.browserProxy_.shutdownIPFSService().then(() => {})
    }
  }

  /** @private **/
  isKeysEditorRoute() {
    const router = Router.getInstance()
    return (router.getCurrentRoute() == router.getRoutes().BRAVE_IPFS_KEYS)
  }

  /** @private **/
  isPeersEditorRoute() {
    const router = Router.getInstance()
    return (router.getCurrentRoute() == router.getRoutes().BRAVE_IPFS_PEERS)
  }

  /** @private **/
  onChangeIpfsStorageMax_() {
    this.browserProxy_.setIPFSStorageMax(Number(this.$.ipfsStorageMax.value))
  }

  /** @private **/
  onIPFSCompanionEnabledChange_() {
    this.browserProxy_.setIPFSCompanionEnabled(this.$.ipfsCompanionEnabled.checked)
  }

  /** @private **/
  onChangeIPFSGatewayDialogTapped_() {
    this.showChangeIPFSGatewayDialog_ = true
  }

  /** @private **/
  onChangeIPFSGatewayDialogClosed_() {
    this.showChangeIPFSGatewayDialog_ = false
  }

  /** @private **/
  onChangeIPFSNFTGatewayDialogTapped_() {
    this.showChangeIPFSNFTGatewayDialog_ = true
  }

  /** @private **/
  onChangeIPFSNFTGatewayDialogClosed_() {
    this.showChangeIPFSNFTGatewayDialog_ = false
  }
}

customElements.define(
  SettingsBraveIpfsPageElement.is, SettingsBraveIpfsPageElement)
