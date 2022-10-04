// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Convert to Polymer class and remove ts-nocheck

import 'chrome://resources/cr_elements/cr_link_row/cr_link_row.js'
import '../settings_page/settings_section.js'
import '../settings_shared.css.js'
import '../settings_vars.css.js'
import './add_ipfs_peer_dialog.js'

import {Polymer} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {I18nBehavior} from 'chrome://resources/cr_elements/i18n_behavior.js';
import {WebUIListenerBehavior} from 'chrome://resources/cr_elements/web_ui_listener_behavior.js';
import {BraveIPFSBrowserProxyImpl} from './brave_ipfs_browser_proxy.js';
import {getTemplate} from './ipfs_peers_subpage.html.js'

/**
* @fileoverview
* 'settings-sync-subpage' is the settings page content
*/
Polymer({
  is: 'settings-ipfs-peers-subpage',

  _template: getTemplate(),

  behaviors: [
    I18nBehavior,
    WebUIListenerBehavior
  ],

  properties: {
    /**
     * Array of sites to display in the widget.
     * @type {!Array<SiteException>}
     */
    peers: {
      type: Array,
      value() {
        return [];
      },
    },
    needToApply: {
      type: Boolean,
      value: false,
      reflectToAttribute: true
    },
    localNodeLaunched: {
      type: Boolean,
      value: false
    },
    localNodeLaunchError_: {
      type: Boolean,
      value: false,
    },
    showAddPeerDialog_: {
      type: Boolean,
      value: false,
    },
    nodeRestarting_: {
      type: Boolean,
      value: false,
    }
  },

  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = BraveIPFSBrowserProxyImpl.getInstance();
    this.addWebUIListener('brave-ipfs-node-status-changed', (launched) => {
      this.onServiceLaunched(launched)
    })
  },
  toggleUILayout: function(launched) {
    this.localNodeLaunched = launched
    if (launched) {
      this.localNodeLaunchError_ = false
    } else {
      this.showAddPeerDialog_ = false
    }
  },
  onServiceLaunched: function(success) {
    this.toggleUILayout(success)
    if (success) {
      this.updatePeers();
    }
  },
  onRestartNodeTap_: function() {
    this.nodeRestarting_ = true
    this.localNodeLaunchError_ = false
    this.browserProxy_.shutdownIPFSService().then(() => {
      this.browserProxy_.launchIPFSService().then(success => {
        this.isLocalNodeLaunched_ = success
        this.localNodeLaunchError_ = !success
        this.needToApply = !success
        this.nodeRestarting_ = false
      })
    })
  },
  notifyPeerslist: function() {
    const peersList =
    /** @type {IronListElement} */ (this.$$('#peersList'));
    if (peersList) {
      peersList.notifyResize();
    }
  },
  /*++++++
  * @override */
  ready: function() {
    this.onServiceLaunched(this.localNodeLaunched)
    this.updatePeers();
  },
  onAddPeerTap_: function(item) {
    this.showAddPeerDialog_ = true
  },
  updatePeers: function() {
    this.browserProxy_.getIpfsPeersList().then(peers => {
      if (!peers)
        return;
      let currentSize = this.peers.length
      this.peers = JSON.parse(peers);
      if (!this.needToApply && currentSize && this.localNodeLaunched) {
        this.needToApply = currentSize !== this.peers.length
      }
      this.notifyPeerslist();
    });
  },

  onAddPeerDialogClosed_: function(value) {
    this.showAddPeerDialog_ = false
    this.updatePeers();
  },

  onPeerDeleteTapped_: function(event) {
    let id_to_remove = event.model.item.name
    let address_to_remove = event.model.item.value
    var message = this.i18n('ipfsDeletePeerConfirmation', id_to_remove)
    if (!window.confirm(message))
      return
    this.browserProxy_.removeIpfsPeer(id_to_remove, address_to_remove).then(success => {
      if (success) {
        this.updatePeers()
        return;
      }
    });
  }
})
