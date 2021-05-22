/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
import {PrefsBehavior} from '../prefs/prefs_behavior.js';
import {Router, RouteObserverBehavior} from '../router.js';
import {BraveIPFSBrowserProxyImpl} from './brave_ipfs_browser_proxy.m.js';

(function() {
'use strict';

/**
 * 'settings-brave-default-extensions-page' is the settings page containing
 * brave's default extensions.
 */
Polymer({
  is: 'settings-brave-ipfs-page',

  behaviors: [
    WebUIListenerBehavior,
    PrefsBehavior,
    RouteObserverBehavior
  ],

  /**
   * Keep it same as in IPFSResolveMethodTypes
   * in brave\components\ipfs\ipfs_constants.h */
  IPFSResolveMethodTypes: {
    IPFS_ASK: 0,
    IPFS_GATEWAY: 1,
    IPFS_LOCAL: 2,
    IPFS_DISABLED: 3
  },

  properties: {
    ipfsEnabled_: Boolean,
    showChangeIPFSGatewayDialog_: Boolean,
    isLocalNodeEnabled_: Boolean,
    isLocalNodeLaunched_: {
      type: Boolean,
      value: false,
    },
    showIPFSLearnMoreLink_: Boolean,
    mainBlockVisibility_: String
  },

  /** @private {?settings.BraveIPFSBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = BraveIPFSBrowserProxyImpl.getInstance();
  },

  /** @override */
  ready: function() {
    this.onIPFSCompanionEnabledChange_ = this.onIPFSCompanionEnabledChange_.bind(this)
    this.onChangeIpfsStorageMax_ = this.onChangeIpfsStorageMax_.bind(this)
    this.onChangeIpfsMethod_ = this.onChangeIpfsMethod_.bind(this)
    this.onP2pKeysEditorClick_ = this.onP2pKeysEditorClick_.bind(this)
    this.onIpfsPeersEditorClick_ = this.onIpfsPeersEditorClick_.bind(this)

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
  },

  setupOptionsVisibility: function() {
    const resolve_method = this.getPref('brave.ipfs.resolve_method').value;
    // Check if IPFS method is LOCAL_NODE
    this.isLocalNodeEnabled_ = (resolve_method == this.IPFSResolveMethodTypes.IPFS_LOCAL) &&
                                this.ipfsEnabled_;
    this.showIPFSLearnMoreLink_ =
      (resolve_method == this.IPFSResolveMethodTypes.IPFS_ASK);
    this.$.ipfsStorageMax.value =
      this.getPref('brave.ipfs.storage_max').value;
  },

  onLoad_: function() {
    this.setupOptionsVisibility();
    if (this.isKeysEditorRoute() && !this.isLocalNodeEnabled_) {
      const router = Router.getInstance();
      router.navigateTo(router.getRoutes().BRAVE_IPFS);
    }
  },
  onIpfsPeersEditorClick_: function() {
    const router = Router.getInstance();
    router.navigateTo(router.getRoutes().BRAVE_IPFS_PEERS);
  },
  onP2pKeysEditorClick_: function() {
    const router = Router.getInstance();
    router.navigateTo(router.getRoutes().BRAVE_IPFS_KEYS);
  },
  onChangeIpfsMethod_: function() {
    let local_node_used = this.isLocalNodeEnabled_
    this.setupOptionsVisibility();
    // Automatically start node if we changed method to local node
    if (!local_node_used && this.isLocalNodeEnabled_) {
      this.browserProxy_.launchIPFSService().then(success => {
        this.isLocalNodeLaunched_ = success
      });
    } else if (local_node_used && !this.isLocalNodeEnabled_) {
      this.browserProxy_.shutdownIPFSService().then(() => {})
    }
  },

  isKeysEditorRoute: function () {
    const router = Router.getInstance();
    return (router.getCurrentRoute() == router.getRoutes().BRAVE_IPFS_KEYS);
  },
  isPeersEditorRoute: function () {
    const router = Router.getInstance();
    return (router.getCurrentRoute() == router.getRoutes().BRAVE_IPFS_PEERS);
  },

  /** @protected */
  currentRouteChanged: function() {
    let hidden = this.isPeersEditorRoute() || this.isKeysEditorRoute()
    this.mainBlockVisibility_ = hidden ? 'hidden' : ''
  },

  onChangeIpfsStorageMax_: function() {
    this.browserProxy_.setIPFSStorageMax(Number(this.$.ipfsStorageMax.value));
  },

  onIPFSCompanionEnabledChange_: function() {
    this.browserProxy_.setIPFSCompanionEnabled(this.$.ipfsCompanionEnabled.checked);
  },

  onChangeIPFSGatewayDialogTapped_: function() {
    this.showChangeIPFSGatewayDialog_ = true;
  },

  onChangeIPFSGatewayDialogClosed_: function() {
    this.showChangeIPFSGatewayDialog_ = false;
  },

});
})();
