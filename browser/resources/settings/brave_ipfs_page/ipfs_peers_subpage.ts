// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_link_row/cr_link_row.js'
import '../settings_page/settings_section.js'
import '../settings_shared.css.js'
import '../settings_vars.css.js'
import './add_ipfs_peer_dialog.js'

import {DomRepeatEvent, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {I18nMixin, I18nMixinInterface} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {WebUIListenerMixin, WebUIListenerMixinInterface} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js';
import {BraveIPFSBrowserProxyImpl} from './brave_ipfs_browser_proxy.js';
import {BaseMixin} from '../base_mixin.js'
import {getTemplate} from './ipfs_peers_subpage.html.js'
import {IronListElement} from 'chrome://resources/polymer/v3_0/iron-list/iron-list.js';

/**
* @fileoverview
* 'settings-sync-subpage' is the settings page content
*/

const SettingsBraveIpfsPeersSubpageElementBase =
  I18nMixin(WebUIListenerMixin(BaseMixin(PolymerElement))) as {
    new(): PolymerElement & WebUIListenerMixinInterface & I18nMixinInterface
  }

export interface PeersListItem {
  name: string;
  value: string;
}

export interface SettingsBraveIpfsPeersSubpageElement {
  $: {
    peersList: IronListElement,
  }
}

export class SettingsBraveIpfsPeersSubpageElement extends SettingsBraveIpfsPeersSubpageElementBase {
  static get is() {
    return 'settings-ipfs-peers-subpage'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
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
    };
  }

  private peers: PeersListItem[];
  private needToApply: boolean;
  private localNodeLaunched: boolean;
  private localNodeLaunchError_: boolean;
  private showAddPeerDialog_: boolean;
  private nodeRestarting_: boolean;

  browserProxy_: BraveIPFSBrowserProxyImpl = BraveIPFSBrowserProxyImpl.getInstance();

  override ready() {
    super.ready()
    this.addWebUIListener('brave-ipfs-node-status-changed', (launched: boolean) => {
      this.onServiceLaunched(launched)
    })
    this.onServiceLaunched(this.localNodeLaunched)
    this.updatePeers();
  }

  private toggleUILayout(launched: boolean) {
    this.localNodeLaunched = launched
    if (launched) {
      this.localNodeLaunchError_ = false
    } else {
      this.showAddPeerDialog_ = false
    }
  }

  private onServiceLaunched(success: boolean) {
    this.toggleUILayout(success)
    if (success) {
      this.updatePeers();
    }
  }

  private onRestartNodeTap() {
    this.nodeRestarting_ = true
    this.localNodeLaunchError_ = false
    this.browserProxy_.shutdownIPFSService().then(() => {
      this.browserProxy_.launchIPFSService().then(success => {
        this.localNodeLaunched = success
        this.localNodeLaunchError_ = !success
        this.needToApply = !success
        this.nodeRestarting_ = false
      })
    })
  }

  private notifyPeerslist() {
    if (this.$.peersList) {
      this.$.peersList.notifyResize();
    }
  }

  private onAddPeerTap_() {
    this.showAddPeerDialog_ = true
  }

  private updatePeers() {
    this.browserProxy_.getIpfsPeersList().then((peers: string) => {
      if (!peers)
        return;
      let currentSize = this.peers.length
      this.peers = JSON.parse(peers);
      if (!this.needToApply && currentSize && this.localNodeLaunched) {
        this.needToApply = currentSize !== this.peers.length
      }
      this.notifyPeerslist();
    });
  }

  private onAddPeerDialogClosed_() {
    this.showAddPeerDialog_ = false
    this.updatePeers();
  }

  private onPeerDeleteTapped_(event: DomRepeatEvent<PeersListItem>) {
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
}

customElements.define(
  SettingsBraveIpfsPeersSubpageElement.is, SettingsBraveIpfsPeersSubpageElement)
