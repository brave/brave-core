// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AIChat
import BraveCore
import BraveVPN
import Foundation
import Preferences
import Shared
import os.log

public class BraveSkusManager {
  private let sku: SkusSkusService

  public init?(isPrivateMode: Bool) {
    guard let skusService = Skus.SkusServiceFactory.get(privateMode: isPrivateMode) else {
      assert(
        isPrivateMode,
        "[SkusManager] - SkusServiceFactory failed to intialize in regular mode, something is wrong."
      )
      return nil
    }

    self.sku = skusService
  }

  @MainActor
  public func refreshVPNCredentials() async {
    if let domain = Preferences.VPN.skusCredentialDomain.value {
      // Always refresh credentials and trust the credentials from Brave-Core rather than cached credentials
      _ = await credentialSummary(for: domain)
    }
  }

  // MARK: - Handling SKU methods.

  @MainActor
  func refreshOrder(for orderId: String, domain: String) async -> Any? {
    Logger.module.debug("[SkusManager] - RefreshOrder")
    let order = await sku.refreshOrder(domain: domain, orderId: orderId)
    guard !order.isEmpty,
      let data = order.data(using: .utf8),
      let json = try? JSONSerialization.jsonObject(with: data, options: .fragmentsAllowed)
    else {
      Logger.module.debug("[SkusManager] - Failed to Serialize Order")
      return nil
    }

    return json
  }

  @MainActor
  func fetchOrderCredentials(for orderId: String, domain: String) async -> String {
    Logger.module.debug("[SkusManager] - FetchOrderCredentials")
    return await sku.fetchOrderCredentials(domain: domain, orderId: orderId)
  }

  @MainActor
  func prepareCredentialsPresentation(for domain: String, path: String) async -> String? {
    Logger.module.debug("[SkusManager] - PrepareCredentialsPresentation")

    let credentialType = CredentialType.from(domain: domain)
    let credential = await sku.prepareCredentialsPresentation(domain: domain, path: path)
    if !credential.isEmpty {
      switch credentialType {
      case .vpn:
        if let vpnCredential = BraveSkusWebHelper.fetchVPNCredential(credential, domain: domain) {
          Preferences.VPN.skusCredential.value = credential
          Preferences.VPN.skusCredentialDomain.value = domain
          Preferences.VPN.expirationDate.value = vpnCredential.expirationDate

          BraveVPN.setCustomVPNCredential(vpnCredential)
        }
      case .leo:
        if let cookie = CredentialCookie.from(credential: credential, domain: domain) {
          Preferences.AIChat.subscriptionExpirationDate.value = cookie.expirationDate
        }
        break
      case .unknown:
        Logger.module.debug("[SkusManager] - Unknown Credentials")
        break
      }
    }

    return credential
  }

  @MainActor
  func credentialSummary(for domain: String) async -> Any? {
    let summary = await sku.credentialSummary(domain: domain)
    guard !summary.isEmpty,
      let data = summary.data(using: .utf8),
      let json = try? JSONSerialization.jsonObject(with: data, options: .fragmentsAllowed)
    else {
      Logger.module.debug("[SkusManager] - Failed to Serialize Credential Summary")
      return nil
    }

    guard summary != "{}" else {
      return json
    }

    do {
      // Once we switch VPN over to the new Skus v2 APIs, the we can switch this to SkusCredentials from the SDK
      // Not sure if VPN has CredentialSummary.order.id so leaving as is for now.
      let credentialSummary = try CredentialSummary.from(data: data)
      switch credentialSummary.state {
      case .valid:
        let credentialType = CredentialType.from(domain: domain)
        switch credentialType {
        case .vpn:
          Logger.module.debug("[SkusManager] - Preparing VPN Credentials")
          _ = await prepareCredentialsPresentation(for: domain, path: "*")
        case .leo:
          if Preferences.AIChat.subscriptionOrderId.value != nil {
            Logger.module.debug("[SkusManager] - Preparing Leo Credentials")
            _ = await prepareCredentialsPresentation(for: domain, path: "*")
          }
        case .unknown:
          Logger.module.debug("[SkusManager] - Unknown Credentials")
          break
        }
      case .invalid:
        if !credentialSummary.active {
          Logger.module.debug("[SkusManager] - The credential summary is not active")
        }

        if credentialSummary.remainingCredentialCount <= 0 {
          Logger.module.debug(
            "[SkusManager] - The credential summary does not have any remaining credentials"
          )
        }
      case .sessionExpired:
        Logger.module.debug("[SkusManager] - This credential session has expired")
        Self.keepShowingSessionExpiredState = true
      }
    } catch {
      Logger.module.error("[SkusManager] - \(error)")
    }

    return json
  }

  // MARK: - Session Expired state
  /// An in-memory flag that will show a "session expired" prompt to the user in the current browsing session.
  public static var keepShowingSessionExpiredState = false

  public static func sessionExpiredStateAlert(
    loginCallback: @escaping (UIAlertAction) -> Void
  ) -> UIAlertController {
    let alert = UIAlertController(
      title: Strings.VPN.sessionExpiredTitle,
      message: Strings.VPN.sessionExpiredDescription,
      preferredStyle: .alert
    )

    let loginButton = UIAlertAction(
      title: Strings.VPN.sessionExpiredLoginButton,
      style: .default,
      handler: loginCallback
    )
    let dismissButton = UIAlertAction(
      title: Strings.VPN.sessionExpiredDismissButton,
      style: .cancel
    )
    alert.addAction(loginButton)
    alert.addAction(dismissButton)

    return alert
  }
}

private enum CredentialType {
  case unknown
  case vpn
  case leo

  static func from(domain: String) -> CredentialType {
    switch domain {
    case "vpn.brave.software", "vpn.bravesoftware.com", "vpn.brave.com": return .vpn
    case "leo.brave.software", "leo.bravesoftware.com", "leo.brave.com": return .leo
    default:
      return .unknown
    }
  }
}

private struct CredentialSummary: Codable {
  let expiresAt: Date?
  let active: Bool
  let remainingCredentialCount: Int

  static func from(data: Data) throws -> CredentialSummary {
    return try jsonDecoder.decode(Self.self, from: data)
  }

  enum State {
    case valid
    case invalid
    case sessionExpired
  }

  var state: State {
    if active && remainingCredentialCount > 0 { return .valid }
    if active && remainingCredentialCount == 0 { return .sessionExpired }
    return .invalid
  }

  private static var jsonDecoder: JSONDecoder {
    let formatter = ISO8601DateFormatter()
    formatter.formatOptions = [
      .withYear,
      .withMonth,
      .withDay,
      .withTime,
      .withDashSeparatorInDate,
      .withColonSeparatorInTime,
    ]

    let decoder = JSONDecoder()
    decoder.keyDecodingStrategy = .convertFromSnakeCase
    decoder.dateDecodingStrategy = .custom({ decoder in
      let container = try decoder.singleValueContainer()
      let dateString = try container.decode(String.self)

      guard let date = formatter.date(from: try container.decode(String.self)) else {
        throw DecodingError.dataCorruptedError(
          in: container,
          debugDescription: "Cannot decode date string \(dateString)"
        )
      }

      return date
    })
    return decoder
  }
}

private struct CredentialCookie {
  let expirationDate: Date
  let cookie: String

  static func from(credential: String, domain: String) -> CredentialCookie? {
    guard let credential = credential.unescape(),
      let cookieDomain = URL(string: "https://\(domain)")
    else {
      return nil
    }

    guard
      let cookie = HTTPCookie.cookies(
        withResponseHeaderFields: ["Set-Cookie": credential],
        for: cookieDomain
      ).first
    else {
      return nil
    }

    guard let expirationDate = cookie.expiresDate else {
      return nil
    }

    return CredentialCookie(expirationDate: expirationDate, cookie: cookie.value)
  }
}
