// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types

export class BraveIPFSBrowserProxyImpl {
  setIPFSCompanionEnabled (value) {
    chrome.send('setIPFSCompanionEnabled', [value])
  }

  setIPFSStorageMax (value) {
    chrome.send('setIPFSStorageMax', [value])
  }

  importIpnsKey (keyName) {
    chrome.send('importIpnsKey', [keyName])
  }

  exportIPNSKey (keyName) {
    chrome.send('exportIPNSKey', [keyName])
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

  getIPFSResolveMethodList () {
    return new Promise(resolve => {
      if (!chrome.ipfs) {
        resolve(false)
        return
      }
      chrome.ipfs.getResolveMethodList(resolve)
    })
  }

  removeIpfsPeer (id, address) {
    return new Promise(resolve => {
      if (!chrome.ipfs) {
        resolve(false)
        return
      }
      chrome.ipfs.removeIpfsPeer(id, address, resolve)
    })
  }

  getIpfsPeersList () {
    return new Promise(resolve => {
      if (!chrome.ipfs) {
        resolve(false)
        return
      }
      chrome.ipfs.getIpfsPeersList(resolve)
    })
  }

  addIpfsPeer (value) {
    return new Promise(resolve => {
      if (!chrome.ipfs) {
        resolve(false)
        return
      }
      chrome.ipfs.addIpfsPeer(value, resolve)
    })
  }

  getIpnsKeysList () {
    return new Promise(resolve => {
      if (!chrome.ipfs) {
        resolve(false)
        return
      }
      chrome.ipfs.getIpnsKeysList(resolve)
    })
  }

  rotateKey (name) {
    return new Promise(resolve => {
      if (!chrome.ipfs) {
        resolve(false)
        return
      }
      chrome.ipfs.rotateKey(name, resolve)
    })
  }

  addIpnsKey (name) {
    return new Promise(resolve => {
      if (!chrome.ipfs) {
        resolve(false)
        return
      }
      chrome.ipfs.addIpnsKey(name, resolve)
    })
  }

  removeIpnsKey (name) {
    return new Promise(resolve => {
      if (!chrome.ipfs) {
        resolve(false)
        return
      }
      chrome.ipfs.removeIpnsKey(name, resolve)
    })
  }

  getIPFSEnabled () {
    return new Promise(resolve => {
      if (!chrome.ipfs) {
        resolve(false)
        return
      }
      chrome.ipfs.getIPFSEnabled(resolve)
    })
  }

  validateGatewayUrl (url) {
    return new Promise(resolve => {
      if (!chrome.ipfs) {
        resolve(false)
        return
      }
      chrome.ipfs.validateGatewayUrl(url, resolve)
    })
  }

  static getInstance(): BraveIPFSBrowserProxyImpl {
    return instance || (instance = new BraveIPFSBrowserProxyImpl())
  }
}

let instance: BraveIPFSBrowserProxyImpl|null = null
