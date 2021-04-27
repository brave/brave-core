/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
import {Router} from '../router.m.js';

(function() {
  'use strict';
  
/**
* @fileoverview
* 'settings-sync-subpage' is the settings page content
*/
Polymer({
  is: 'settings-p2p-keys-subpage',

  behaviors: [
    I18nBehavior,
    WebUIListenerBehavior
  ],

  properties: {
    /**
     * Array of sites to display in the widget.
     * @type {!Array<SiteException>}
     */
    keys: {
      type: Array,
      value() {
        return [];
      },
    },
    localNodeMethod: {
      type: Boolean,
      value: false,
      reflectToAttribute: true
    },
    localNodeLaunched: {
      type: Boolean,
      value: false
    },
    launchNodeButtonEnabled_: {
      type: Boolean,
      value: true,
    },
    localNodeLaunchError_: {
      type: Boolean,
      value: false,
    },
    showAddp2pKeyDialog_: {
      type: Boolean,
      value: false,
    }
  },

  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.BraveIPFSBrowserProxyImpl.getInstance();
    this.addWebUIListener('brave-ipfs-node-status-changed', (launched) => {
      this.onServiceLaunched(launched)
    })
  },
  
  onServiceLaunched: function(success) {
    this.launchNodeButtonEnabled_ = true;
    this.localNodeLaunched = success
    if (success) {
      this.localNodeLaunchError_ = false;
      this.updateKeys();
    } else {
      this.showAddp2pKeyDialog_ = false
    }
  },

  onStartNodeKeyTap_: function() {
    this.launchNodeButtonEnabled_ = false;
    this.browserProxy_.launchIPFSService().then((success) => {
      this.localNodeLaunchError_ = !success;
      this.onServiceLaunched(success)
    });
  },

  /*++++++
  * @override */
  ready: function() {
    this.onServiceLaunched(this.localNodeLaunched)
  },

  isDefaultKey_: function(name) {
    return name == 'self';
  },

  onAddKeyTap_: function(item) {
    this.showAddp2pKeyDialog_ = true
  },

  updateKeys: function() {
    this.browserProxy_.getIpnsKeysList().then(keys => {
      this.keys_ = JSON.parse(keys);
    });
  },

  onAddKeyDialogClosed_: function() {
    this.showAddp2pKeyDialog_ = false
    this.updateKeys();
  },

  onKeyDeleteTapped_: function(event) {
    let name_to_remove = event.model.item.name
    var message = this.i18n('ipfsDeleteKeyConfirmation', name_to_remove)
    if (!window.confirm(message))
      return
    this.browserProxy_.removeIpnsKey(name_to_remove).then(removed_name => {
      if (!removed_name)
        return;
      if (removed_name === name_to_remove) {
        this.updateKeys()
        return;
      }
    });
  }
});
})();
