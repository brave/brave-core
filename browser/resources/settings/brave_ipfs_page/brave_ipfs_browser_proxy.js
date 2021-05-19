/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {addSingletonGetter} from 'chrome://resources/js/cr.m.js';

/** @interface */
export class BraveIPFSBrowserProxy {
  /**
   * @param {boolean} value name.
   */
  setIPFSCompanionEnabled (value) {}
  getIPFSResolveMethodList () {}
  getIPFSEnabled () {}
  setIPFSStorageMax (value) {}
  importIpnsKey () {}
}

/**
 * @implements {settings.BraveIPFSBrowserProxy}
 */
export class BraveIPFSBrowserProxyImpl {
  setIPFSCompanionEnabled (value) {
    chrome.send('setIPFSCompanionEnabled', [value])
  }

  setIPFSStorageMax (value) {
    chrome.send('setIPFSStorageMax', [value])
  }

  importIpnsKey (value) {
    chrome.send('importIpnsKey', [value])
  }
  notifyIpfsNodeStatus () {
    chrome.send('notifyIpfsNodeStatus', [])
  }
  launchIPFSService () {
    return new Promise(resolve => {
      if (!chrome.ipfs) {
        resolve(false)
        return
      }
      chrome.ipfs.launch(resolve)
    })
  }

  shutdownIPFSService () {
    return new Promise(resolve => {
      if (!chrome.ipfs) {
        resolve(false)
        return
      }
      chrome.ipfs.shutdown(resolve)
    })
  }

  /** @override */
  getIPFSResolveMethodList () {
    return new Promise(resolve => {
      if (!chrome.ipfs) {
        resolve(false)
        return
      }
      chrome.ipfs.getResolveMethodList(resolve)
    })
  }
  
  /** @override */
  removeIpfsPeer (id, address) {
    return new Promise(resolve => {
      if (!chrome.ipfs) {
        resolve(false)
        return
      }
      chrome.ipfs.removeIpfsPeer(id, address, resolve)
    })
  }
  /** @override */
  getIpfsPeersList () {
    return new Promise(resolve => {
      if (!chrome.ipfs) {
        resolve(false)
        return
      }
      chrome.ipfs.getIpfsPeersList(resolve)
    })
  }

  /** @override */
  addIpfsPeer (value) {
    return new Promise(resolve => {
      if (!chrome.ipfs) {
        resolve(false)
        return
      }
      chrome.ipfs.addIpfsPeer(value, resolve)
    })
  }
  
  /** @override */
  getIpnsKeysList () {
    return new Promise(resolve => {
      if (!chrome.ipfs) {
        resolve(false)
        return
      }
      chrome.ipfs.getIpnsKeysList(resolve)
    })
  }

  /** @override */
  addIpnsKey (name) {
    return new Promise(resolve => {
      if (!chrome.ipfs) {
        resolve(false)
        return
      }
      chrome.ipfs.addIpnsKey(name, resolve)
    })
  }

  /** @override */
  removeIpnsKey (name) {
    return new Promise(resolve => {
      if (!chrome.ipfs) {
        resolve(false)
        return
      }
      chrome.ipfs.removeIpnsKey(name, resolve)
    })
  }

  /** @override */
  getIPFSEnabled () {
    return new Promise(resolve => {
      if (!chrome.ipfs) {
        resolve(false)
        return
      }
      chrome.ipfs.getIPFSEnabled(resolve)
    })
  }
}

cr.addSingletonGetter(BraveIPFSBrowserProxyImpl)
