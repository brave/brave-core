// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import BraveWallet
import Preferences
import BraveCore
import Data

extension BrowserViewController: Web3IPFSScriptHandlerDelegate {
  func web3IPFSDecisionHandler(_ proceed: Bool, originalURL: URL) {
    if proceed {
      if let resolvedUrl = braveCore.ipfsAPI.resolveGatewayUrl(for: originalURL) {
        finishEditingAndSubmit(resolvedUrl)
      } else {
        finishEditingAndSubmit(originalURL)
      }
      Preferences.Wallet.resolveIPFSResources.value = Preferences.Wallet.Web3IPFSOption.enabled.rawValue
    } else {
      if let errorPageHelper = tabManager.selectedTab?.getContentScript(name: ErrorPageHelper.scriptName) as? ErrorPageHelper, let webView = tabManager.selectedTab?.webView {
        errorPageHelper.loadPage(IPFSErrorPageHandler.disabledError, forUrl: originalURL, inWebView: webView)
      }

      Preferences.Wallet.resolveIPFSResources.value = Preferences.Wallet.Web3IPFSOption.disabled.rawValue
    }
  }
}
