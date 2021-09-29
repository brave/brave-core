/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
import {Router} from '../router.js';
import {BraveIPFSBrowserProxyImpl} from './brave_ipfs_browser_proxy.m.js';
import {assert} from 'chrome://resources/js/assert.m.js';

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
    actionKeysError_: {
      type: Boolean,
      value: false,
    },
    showAddp2pKeyDialog_: {
      type: Boolean,
      value: false,
    },
    showRotatep2pKeyDialog_: {
      type: Boolean,
      value: false,
    }
  },

  browserProxy_: null,
  actionItemName : String,
  /** @override */
  created: function() {
    this.browserProxy_ = BraveIPFSBrowserProxyImpl.getInstance();
    this.addWebUIListener('brave-ipfs-node-status-changed', (launched) => {
      this.onServiceLaunched(launched)
    })
    this.addWebUIListener('brave-ipfs-keys-loaded', (success) => {
      this.updateKeys()
    })
    this.addWebUIListener('brave-ipfs-key-exported', (key, success) => {
      this.actionKeysError_ = !success
      if (this.actionKeysError_ ) {
        const errorLabel = (this.$$('#key-error'));
        errorLabel.textContent = this.i18n('ipfsKeyExportError', key)
        return;
      }
    })
    this.addWebUIListener('brave-ipfs-key-imported', (key, value, success) => {
      this.actionKeysError_ = !success
      if (this.actionKeysError_ ) {
        const errorLabel = (this.$$('#key-error'));
        errorLabel.textContent = this.i18n('ipfsImporKeysError', key)
        return;
      }
      this.updateKeys();
    })
    window.addEventListener('load', this.onLoad_.bind(this));
  },
  onLoad_: function() {
    this.updateKeys();
  },
  notifyKeylist: function() {
    const keysList =
    /** @type {IronListElement} */ (this.$$('#keysList'));
    if (keysList) {
      keysList.notifyResize();
    }
  },
  toggleUILayout: function(launched) {
    this.launchNodeButtonEnabled_ = !launched;
    this.localNodeLaunched = launched
    this.actionKeysError_ = false
    if (launched) {
      this.localNodeLaunchError_ = false
    } else {
      this.showAddp2pKeyDialog_ = false
    }
  },

  onServiceLaunched: function(success) {
    this.toggleUILayout(success)
    if (success) {
      this.updateKeys();
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
    this.browserProxy_.notifyIpfsNodeStatus();
  },

  isDefaultKey_: function(name) {
    return name == 'self';
  },

  getIconForKey: function(name) {
    return name == 'self' ? 'icon-button-self' : 'icon-button'
  },
  

  onAddKeyTap_: function(item) {
    this.showAddp2pKeyDialog_ = true
  },

  onRotateKeyDialogClosed_: function() {
    this.showRotatep2pKeyDialog_ = false
    this.updateKeys();
  },

  updateKeys: function() {
    this.browserProxy_.getIpnsKeysList().then(keys => {
      if (!keys)
        return;
      this.keys_ = JSON.parse(keys);
      this.toggleUILayout(true)
      this.notifyKeylist();
    });
  },

  onAddKeyDialogClosed_: function() {
    this.showAddp2pKeyDialog_ = false
    this.actionItemName = ""
    this.updateKeys();
  },

  onKeyActionTapped_: function(event) {
    let name = event.model.item.name
    if (name != 'self')
      return;
    this.showRotatep2pKeyDialog_ = true
    return;
  },

  onKeyDeleteTapped_: function(event) {
    this.$$('cr-action-menu').close();
    let name_to_remove = this.actionItemName
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
    this.actionItemName = ""
  },
  
  onExportTap_: function(event) {
    let name_to_export = this.actionItemName
    this.$$('cr-action-menu').close();
    this.browserProxy_.exportIPNSKey(name_to_export);
    this.actionItemName = ""
  },
  
  onKeyMenuTapped_: function(event) {
    this.actionItemName = event.model.item.name
    const actionMenu =
        /** @type {!CrActionMenuElement} */ (this.$$('#key-menu').get());
    actionMenu.showAt(event.target);
  }
});
})();
