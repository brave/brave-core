// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import os.log

public class WebcompatReporter {
  static let log = Logger(subsystem: Bundle.main.bundleIdentifier!, category: "WebcompatReporter")
  
  /// The raw values of the web-report.
  public struct Report {
    /// The URL of the broken site.
    /// - Note: This needs to be the cleaned up version with query params and fragments removed (as seen in the UI)
    let cleanedURL: URL
    /// Any user input details
    let additionalDetails: String?
    /// Any user input contact details that may be provided
    let contactInfo: String?
    /// A bool indicating if shields are enabled for that site
    let areShieldsEnabled: Bool
    /// The level of adblocking currently set for the page
    let adBlockLevel: ShieldLevel
    /// The level of fingerprinting protection currently set for this page
    let fingerprintProtectionLevel: ShieldLevel
    /// Titles of all enabled filter lists
    let adBlockListTitles: [String]
    /// If VPN is currently enabled
    let isVPNEnabled: Bool
    
    var domain: String? {
      return cleanedURL.normalizedHost() != nil ? cleanedURL.domainURL.absoluteString : cleanedURL.baseDomain
    }
    
    public init(
      cleanedURL: URL, additionalDetails: String? = nil, contactInfo: String? = nil,
      areShieldsEnabled: Bool, adBlockLevel: ShieldLevel, fingerprintProtectionLevel: ShieldLevel,
      adBlockListTitles: [String], isVPNEnabled: Bool
    ) {
      self.cleanedURL = cleanedURL
      self.additionalDetails = additionalDetails
      self.contactInfo = contactInfo
      self.areShieldsEnabled = areShieldsEnabled
      self.adBlockLevel = adBlockLevel
      self.fingerprintProtectionLevel = fingerprintProtectionLevel
      self.adBlockListTitles = adBlockListTitles
      self.isVPNEnabled = isVPNEnabled
    }
  }
  
  private struct Payload: Encodable {
    let report: Report
    let apiKey: String?
    let languageCode: String?
    
    enum CodingKeys: String, CodingKey {
      case url
      case domain
      case additionalDetails
      case contactInfo
      case apiKey = "api_key"
      
      case fpBlockSetting
      case adBlockSetting
      case adBlockLists
      case shieldsEnabled
      case languages
      case languageFarblingEnabled
      case braveVPNEnabled
    }
    
    public func encode(to encoder: Encoder) throws {
      // We want to ensure that the URL _can_ be normalized, since `domainURL` will return itself
      // (the full URL) if the URL can't be normalized. If the URL can't be normalized, send only
      // the base domain without scheme.
      guard let domain = report.domain else {
        throw EncodingError.invalidValue(CodingKeys.domain, EncodingError.Context(
          codingPath: encoder.codingPath, debugDescription: "Cannot extract `domain` from url"
        ))
      }
      
      guard let apiKey = apiKey else {
        throw EncodingError.invalidValue(CodingKeys.apiKey, EncodingError.Context(
          codingPath: encoder.codingPath, debugDescription: "Missing api_key"
        ))
      }
      
      var container: KeyedEncodingContainer<CodingKeys> = encoder.container(keyedBy: CodingKeys.self)
      try container.encode(domain, forKey: .domain)
      try container.encode(report.cleanedURL.absoluteString, forKey: .url)
      try container.encodeIfPresent(report.additionalDetails, forKey: .additionalDetails)
      try container.encodeIfPresent(report.contactInfo, forKey: .contactInfo)
      try container.encodeIfPresent(languageCode, forKey: .languages)
      try container.encode(true, forKey: .languageFarblingEnabled) // This is always enabled in iOS web-kit
      try container.encode(report.areShieldsEnabled, forKey: .shieldsEnabled)
      try container.encode(report.isVPNEnabled, forKey: .braveVPNEnabled)
      try container.encode(report.adBlockListTitles.joined(separator: ","), forKey: .adBlockLists)
      try container.encode(report.fingerprintProtectionLevel.reportLabel, forKey: .fpBlockSetting)
      try container.encode(report.adBlockLevel.reportLabel, forKey: .adBlockSetting)
      try container.encode(apiKey, forKey: .apiKey)
    }
  }
  
  private static var baseHost: String {
    if AppConstants.buildChannel == .debug {
      return "laptop-updates.bravesoftware.com"
    } else {
      return "laptop-updates.brave.com"
    }
  }

  private static let apiKeyPlistKey = "STATS_KEY"
  private static let version = "1"

  /// A custom user agent to send along with reports
  public static var userAgent: String?
  
  /// Get the user's language code
  private static var currentLanguageCode: String? {
    if #available(iOS 16, *) {
      return Locale.current.language.languageCode?.identifier
    } else {
      return Locale.current.languageCode
    }
  }

  /// Report a webcompat issue on a given website
  ///
  /// - Returns: A deferred boolean on whether or not it reported successfully (default queue: main)
  @discardableResult
  public static func send(report: Report) async -> Bool {
    let apiKey = (Bundle.main.infoDictionary?[apiKeyPlistKey] as? String)?.trimmingCharacters(in: .whitespacesAndNewlines)
    let payload = Payload(report: report, apiKey: apiKey, languageCode: currentLanguageCode)

    var components = URLComponents()
    components.scheme = "https"
    components.host = baseHost
    components.path = "/\(version)/webcompat"

    guard let endpoint = components.url else {
      Self.log.error("Failed to setup webcompat request")
      return false
    }

    do {
      let encoder = JSONEncoder()
      var request = URLRequest(url: endpoint)
      request.httpMethod = "POST"
      request.addValue("application/json", forHTTPHeaderField: "Content-Type")
      request.httpBody = try encoder.encode(payload)
      
      if let userAgent = userAgent {
        request.setValue(userAgent, forHTTPHeaderField: "User-Agent")
      }
      
      let session = URLSession(configuration: .ephemeral)
      let result = try await session.data(for: request)
      
      if let response = result.1 as? HTTPURLResponse {
        let success = response.statusCode >= 200 && response.statusCode < 300
        
        if !success {
          log.error("Failed to report webcompat issue: Status Code \(response.statusCode)")
        }
        
        return success
      } else {
        return false
      }
    } catch {
      Logger.module.error("Failed to setup webcompat request payload: \(error.localizedDescription)")
      return false
    }
  }
}

private extension ShieldLevel {
  /// The value that is sent to the webcompat report server
  var reportLabel: String {
    switch self {
    case .aggressive: return "aggressive"
    case .standard: return "standard"
    case .disabled: return "allow"
    }
  }
}
