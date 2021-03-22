/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
 import './change_ipfs_gateway_dialog.js';
 import {PrefsBehavior} from '../prefs/prefs_behavior.m.js';

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
    PrefsBehavior
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
    isStorageMaxEnabled_: Boolean,
    showIPFSLearnMoreLink_: Boolean
  },

  /** @private {?settings.BraveIPFSBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.BraveIPFSBrowserProxyImpl.getInstance();
  },

  /** @override */
  ready: function() {
    this.onIPFSCompanionEnabledChange_ = this.onIPFSCompanionEnabledChange_.bind(this)
    this.onChangeIpfsStorageMax_ = this.onChangeIpfsStorageMax_.bind(this)
    this.onChangeIpfsMethod_ = this.onChangeIpfsMethod_.bind(this)

    this.browserProxy_.getIPFSResolveMethodList().then(list => {
      this.ipfsResolveMethod_ = JSON.parse(list)
    });
    this.browserProxy_.getIPFSEnabled().then(enabled => {
      this.ipfsEnabled_ = enabled
    });

    window.addEventListener('load', this.onLoad_.bind(this));
  },

  onLoad_: function() {
    this.isStorageMaxEnabled_ =
      this.getPref('brave.ipfs.resolve_method').value ==
        this.IPFSResolveMethodTypes.IPFS_LOCAL;
    // Check if IPFS method is ASK
    this.showIPFSLearnMoreLink_ =
      this.getPref('brave.ipfs.resolve_method').value ==
        this.IPFSResolveMethodTypes.IPFS_ASK;

    this.$.ipfsStorageMax.value =
      this.getPref('brave.ipfs.storage_max').value;
  },

  onChangeIpfsMethod_: function() {
    // Check if IPFS method is LOCAL_NODE
    const local_node_enabled =
      this.getPref('brave.ipfs.resolve_method').value ==
        this.IPFSResolveMethodTypes.IPFS_LOCAL;
    // Check if IPFS method is ASK
    this.showIPFSLearnMoreLink_ =
      this.getPref('brave.ipfs.resolve_method').value ==
        this.IPFSResolveMethodTypes.IPFS_ASK;

    this.isStorageMaxEnabled_ = local_node_enabled;
    if (local_node_enabled)
      this.browserProxy_.launchIPFSService();
      
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
