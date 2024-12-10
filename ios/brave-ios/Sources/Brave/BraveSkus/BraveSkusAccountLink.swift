// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AIChat
import BraveStore
import Foundation
import Preferences
import WebKit
import os.log

extension BraveStoreProduct {
  /// The key to use when storing the receipt in WebKit's LocalStorage
  var localStorageKey: String {
    switch self {
    case .vpnMonthly, .vpnYearly: return "braveVpn.receipt"
    case .leoMonthly, .leoYearly: return "braveLeo.receipt"
    }
  }
}

/// A class for linking Product Purchases with a Brave Account
class BraveSkusAccountLink {
  /// An enum representing the Skus Environment
  private enum Environment: String, CaseIterable {
    /// The skus development environment
    case development

    /// The skus staging environment
    case staging

    /// The skus production environment
    case production

    /// The host/domain for the environment
    var host: String {
      switch self {
      case .development: return "account.brave.software"
      case .staging: return "account.bravesoftware.com"
      case .production: return "account.brave.com"
      }
    }
  }

  @MainActor
  static func injectLocalStorage(webView: WKWebView) async {
    if let vpnSubscriptionProductId = Preferences.VPN.subscriptionProductId.value,
      let product = BraveStoreProduct(rawValue: vpnSubscriptionProductId)
    {
      await BraveSkusAccountLink.injectLocalStorage(webView: webView, product: product)
    }

    if let aiChatSubscriptionProductId = Preferences.AIChat.subscriptionProductId.value,
      let product = BraveStoreProduct(rawValue: aiChatSubscriptionProductId)
    {
      await BraveSkusAccountLink.injectLocalStorage(webView: webView, product: product)
    }
  }

  /// Injects Skus product order receipt information into WebKit's LocalStorage for use by the `Environment.host` page
  /// - Parameter webView: The web-view whose LocalStorage to inject the product order information
  /// - Parameter product: The product whose receipt information to inject
  @MainActor
  @discardableResult private static func injectLocalStorage(
    webView: WKWebView,
    product: BraveStoreProduct
  ) async -> Bool {
    // The WebView has no URL so do nothing
    guard let url = webView.url else {
      return false
    }

    // The URL must have a scheme and host
    guard let components = URLComponents(url: url, resolvingAgainstBaseURL: false),
      let scheme = components.scheme,
      let host = components.host
    else {
      return false
    }

    // Validate the scheme and host is the correct page to inject sensitive purchase information
    // Purchase information must only be injected into a secure page and must only be injected into
    // one of the Brave account pages as determined by `Environment.host`
    if scheme != "https" || !Environment.allCases.map({ $0.host }).contains(host) {
      return false
    }

    do {
      // Retrieve the LocalStorage Key and Receipt to inject
      let storageKey = product.localStorageKey
      let receipt = try BraveSkusSDK.receipt(for: product)

      // Inject the receipt into LocalStorage
      try await webView.evaluateSafeJavaScriptThrowing(
        functionName: "localStorage.setItem",
        args: [storageKey, receipt],
        contentWorld: .defaultClient
      )

      // Brave-Leo requires Order-ID to be injected into LocalStorage.
      if let orderId = Preferences.AIChat.subscriptionOrderId.value {
        try await webView.evaluateSafeJavaScriptThrowing(
          functionName: "localStorage.setItem",
          args: ["braveLeo.orderId", orderId],
          contentWorld: .defaultClient
        )
      }

      return true
    } catch {
      Logger.module.error(
        "[BraveSkusAccountLink] - Error Injecting SkusSDK receipt into LocalStorage: \(error)"
      )
    }

    return false
  }
}
