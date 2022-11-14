/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import 'chrome://resources/cr_elements/cr_link_row/cr_link_row.js'
import '../settings_page/settings_section.js'
import '../settings_shared.css.js'
import '../settings_vars.css.js'
import './rotate_p2p_key_dialog.js'
import './add_p2p_key_dialog.js'

import {DomRepeatEvent, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {I18nMixin, I18nMixinInterface} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {WebUIListenerMixin, WebUIListenerMixinInterface} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js';
import {BaseMixin} from '../base_mixin.js'
import {BraveIPFSBrowserProxyImpl} from './brave_ipfs_browser_proxy.js';
import {getTemplate} from './p2p_keys_subpage.html.js'
import {IronListElement} from 'chrome://resources/polymer/v3_0/iron-list/iron-list.js';
import {CrLazyRenderElement} from 'chrome://resources/cr_elements/cr_lazy_render/cr_lazy_render.js';
import {CrActionMenuElement} from 'chrome://resources/cr_elements/cr_action_menu/cr_action_menu.js';

/**
* @fileoverview
* 'settings-sync-subpage' is the settings page content
*/

const SettingsBraveP2pKeysSubpageElementBase = 
  I18nMixin(WebUIListenerMixin(BaseMixin(PolymerElement))) as {
    new(): PolymerElement & WebUIListenerMixinInterface & I18nMixinInterface
  }

export interface KeysListItem {
  name: string;
  value: string;
}

export interface SettingsBraveP2pKeysSubpageElement {
  $: {
    keysList: IronListElement,
    keyMenu: CrLazyRenderElement<CrActionMenuElement>,
    keyError: HTMLDivElement
  }
}

export class SettingsBraveP2pKeysSubpageElement extends SettingsBraveP2pKeysSubpageElementBase {
  static get is() {
    return 'settings-p2p-keys-subpage'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
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
    };
  }

  private keys: KeysListItem[];
  private localNodeMethod: boolean;
  private localNodeLaunched: boolean;
  private launchNodeButtonEnabled_: boolean;
  private localNodeLaunchError_: boolean;
  private actionKeysError_: boolean;
  private showAddp2pKeyDialog_: boolean;
  private showRotatep2pKeyDialog_: boolean;
  private actionItemName: string

  browserProxy_: BraveIPFSBrowserProxyImpl = BraveIPFSBrowserProxyImpl.getInstance();

  override ready() {
    super.ready()
    this.addWebUIListener('brave-ipfs-node-status-changed', (launched: boolean) => {
      this.onServiceLaunched(launched)
    })
    this.addWebUIListener('brave-ipfs-keys-loaded', () => {
      this.updateKeys()
    })
    this.addWebUIListener('brave-ipfs-key-exported', (key: string, success: boolean) => {
      this.actionKeysError_ = !success
      if (this.actionKeysError_) {
        const errorLabel = (this.$.keyError);
        errorLabel.textContent = this.i18n('ipfsKeyExportError', key)
        return;
      }
    })
    this.addWebUIListener('brave-ipfs-key-imported', (key: string, success: boolean) => {
      this.actionKeysError_ = !success
      if (this.actionKeysError_ ) {
        const errorLabel = (this.$.keyError);
        errorLabel.textContent = this.i18n('ipfsImporKeysError', key)
        return;
      }
      this.updateKeys();
    })
    window.addEventListener('load', this.onLoad_.bind(this));
    if (document.readyState == 'complete') {
      this.onLoad_()
    }
    this.browserProxy_.notifyIpfsNodeStatus();
  }

  onLoad_() {
    this.updateKeys();
  }

  notifyKeylist() {
    if (this.$.keysList) {
      this.$.keysList.notifyResize();
    }
  }

  toggleUILayout(launched: boolean) {
    this.launchNodeButtonEnabled_ = !launched;
    this.localNodeLaunched = launched
    this.actionKeysError_ = false
    if (launched) {
      this.localNodeLaunchError_ = false
    } else {
      this.showAddp2pKeyDialog_ = false
    }
  }

  onServiceLaunched(success: boolean) {
    this.toggleUILayout(success)
    this.localNodeMethod = success
    if (success) {
      this.updateKeys();
    }
  }

  onStartNodeKeyTap_() {
    this.launchNodeButtonEnabled_ = false;
    this.browserProxy_.launchIPFSService().then((success: boolean) => {
      this.localNodeLaunchError_ = !success;
      this.onServiceLaunched(success)
    });
  }

  isDefaultKey_(name: string) {
    return name == 'self';
  }

  getIconForKey(name: string) {
    return name == 'self' ? 'icon-button-self' : 'icon-button'
  }


  onAddKeyTap_() {
    this.showAddp2pKeyDialog_ = true
  }

  onRotateKeyDialogClosed_() {
    this.showRotatep2pKeyDialog_ = false
    this.updateKeys();
  }

  updateKeys() {
    this.browserProxy_.getIpnsKeysList().then((keys: string) => {
      if (!keys)
        return;
      this.keys = JSON.parse(keys);
      this.toggleUILayout(true)
      this.notifyKeylist();
    });
  }

  onAddKeyDialogClosed_() {
    this.showAddp2pKeyDialog_ = false
    this.actionItemName = ""
    this.updateKeys();
  }

  onKeyActionTapped_(event: DomRepeatEvent<KeysListItem>) {
    let name = event.model.item.name
    if (name != 'self')
      return;
    this.showRotatep2pKeyDialog_ = true
    return;
  }

  onKeyDeleteTapped_() {
    this.$.keyMenu.get().close();
    let name_to_remove = this.actionItemName
    var message = this.i18n('ipfsDeleteKeyConfirmation', name_to_remove)
    if (!window.confirm(message))
      return
    this.browserProxy_.removeIpnsKey(name_to_remove).then((removed_name: string) => {
      if (!removed_name)
        return;
      if (removed_name === name_to_remove) {
        this.updateKeys()
        return;
      }
    });
    this.actionItemName = ""
  }

  onExportTap_() {
    let name_to_export = this.actionItemName
    this.$.keyMenu.get().close();
    this.browserProxy_.exportIPNSKey(name_to_export);
    this.actionItemName = ""
  }

  onKeyMenuTapped_(event: Event) {
    this.actionItemName = (event.target as HTMLElement).getAttribute('itemName')!;
    const actionMenu = this.$.keyMenu.get();
    actionMenu.showAt(event.target as HTMLElement);
  }
}

customElements.define(
  SettingsBraveP2pKeysSubpageElement.is, SettingsBraveP2pKeysSubpageElement)
