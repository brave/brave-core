// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AIChat
import BraveCore
import BraveStore
import BraveVPN
import Foundation
import Preferences
import Shared
import os.log

extension SkusSkusService {
  @MainActor
  public func refreshVPNCredentials() async {
    if let domain = Preferences.VPN.skusCredentialDomain.value {
      // Always refresh credentials and trust the credentials from Brave-Core rather than cached credentials
      let summary = await credentialSummary(domain: domain).message
      if !summary.isEmpty, summary != "{}" {
        await updatePreferences(for: domain, summaryData: Data(summary.utf8))
      }
    }
  }

  @MainActor
  func updatePreferences(for domain: String, summaryData: Data) async {
    do {
      // Once we switch VPN over to the new Skus v2 APIs, the we can switch this to SkusCredentials
      // from the SDK. Not sure if VPN has CredentialSummary.order.id so leaving as is for now.
      let credentialSummary = try CredentialSummary.from(data: summaryData)
      let credentialType = CredentialType.from(domain: domain)
      switch credentialSummary.state {
      case .valid:
        switch credentialType {
        case .vpn:
          Logger.module.debug("[SkusManager] - Preparing VPN Credentials")
          let credential = await prepareCredentialsPresentation(domain: domain, path: "*").message
          if !credential.isEmpty,
            let vpnCredential = BraveSkusWebHelper.fetchVPNCredential(credential, domain: domain)
          {
            Preferences.VPN.skusCredential.value = credential
            BraveVPN.setCustomVPNCredential(vpnCredential)
          }

          Preferences.VPN.skusCredentialDomain.value = domain
          Preferences.VPN.expirationDate.value = credentialSummary.expiresAt
          Preferences.VPN.subscriptionProductId.value = credentialSummary.product?.rawValue
        case .leo:
          if Preferences.AIChat.subscriptionOrderId.value == nil {
            Preferences.AIChat.subscriptionOrderId.value = credentialSummary.orderId
          }

          if Preferences.AIChat.subscriptionOrderId.value != nil {
            Logger.module.debug("[SkusManager] - Preparing Leo Credentials")
            _ = await prepareCredentialsPresentation(domain: domain, path: "*")

            Preferences.AIChat.subscriptionExpirationDate.value = credentialSummary.expiresAt
          }
        case .origin:
          if Preferences.BraveOrigin.purchaseOrderId.value == nil {
            Preferences.BraveOrigin.purchaseOrderId.value = credentialSummary.orderId
          }

          if Preferences.BraveOrigin.purchaseOrderId.value != nil {
            Logger.module.debug("[SkusManager] - Preparing Origin Credentials")
            _ = await prepareCredentialsPresentation(domain: domain, path: "*")
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
        if credentialType == .vpn {
          BraveVPN.markSkusSessionExpired()
        }
      }
    } catch {
      Logger.module.error("[SkusManager] - \(error)")
    }

  }
}

private enum CredentialType {
  case unknown
  case vpn
  case leo
  case origin

  static func from(domain: String) -> CredentialType {
    switch domain {
    case "vpn.brave.software", "vpn.bravesoftware.com", "vpn.brave.com": return .vpn
    case "leo.brave.software", "leo.bravesoftware.com", "leo.brave.com": return .leo
    case "origin.brave.software", "origin.bravesoftware.com", "origin.brave.com": return .origin
    default:
      return .unknown
    }
  }
}

private struct CredentialSummary {
  let expiresAt: Date?
  let active: Bool
  let remainingCredentialCount: Int
  let product: BraveStoreProduct?
  let orderId: String?

  static func from(data: Data) throws -> CredentialSummary {
    let summary = try jsonDecoder.decode(SkusCredentialSummary.self, from: data)
    let orderId = summary.order.id
    let expiresAt = summary.order.expiresAt
    let product = summary.order.items.compactMap({ BraveStoreProduct(rawValue: $0.sku) }).first
    return CredentialSummary(
      expiresAt: expiresAt,
      active: summary.active,
      remainingCredentialCount: Int(summary.remainingCredentialCount),
      product: product,
      orderId: orderId
    )
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
