// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import CodableHelpers
import Foundation
import OSLog
import UIKit

/// Fetches thumbnails from oEmbed providers
actor OEmbedThumbnailFetcher {
  static let shared = OEmbedThumbnailFetcher()

  private var providers: [Provider] = []
  private var hasInitializedProviders: Bool = false
  private let logger = Logger(
    subsystem: Bundle.main.bundleIdentifier ?? "com.brave.ios",
    category: "OEmbedThumbnailFetcher"
  )

  /// Loads the providers file
  private func loadProviders() async {
    defer { hasInitializedProviders = true }
    if hasInitializedProviders {
      return
    }
    guard let path = Bundle.module.path(forResource: "oembed_providers", ofType: "json") else {
      return
    }
    do {
      let data = try Data(contentsOf: URL(filePath: path))
      let decoder = JSONDecoder()
      providers = try decoder.decode([FailableDecodable<Provider>].self, from: data).compactMap {
        $0.wrappedValue
      }
    } catch {
      logger.error("Failed to load oembed providers: \(error.localizedDescription)")
    }
  }

  private func endpoint(for url: URL) async -> Provider.Endpoint? {
    await loadProviders()

    let domain = url.baseDomain
    guard let provider = providers.first(where: { $0.providerURL?.baseDomain == domain }) else {
      return nil
    }

    for endpoint in provider.endpoints {
      guard let schemes = endpoint.schemes else { continue }
      // Each scheme will be a URL that may contain wildcard (`*`) entries in it to match against
      // many different pages on the site that may vend said embedded content.
      //
      // We can use NSPredicate's `LIKE` keyword here to match wildcards properly, but unfortunately
      // we need to drop down to `NSArray` to use the `filtered(using:)` method to apply the
      // predicate.
      let predicate = NSCompoundPredicate(
        type: .or,
        subpredicates: schemes.map {
          NSPredicate(format: "self LIKE %@", $0)
        }
      )
      if !([url.absoluteString] as NSArray).filtered(using: predicate).isEmpty {
        return endpoint
      }
    }
    return nil
  }

  /// Fetch a oembed thumbnail for a given URL, or return `nil` if the provider isn't supported
  /// or an error occured
  func oembedThumbnail(for url: URL) async -> UIImage? {
    guard let endpoint = await endpoint(for: url),
      let providerURL = endpoint.url,
      var components = URLComponents(url: providerURL, resolvingAgainstBaseURL: false)
    else {
      return nil
    }
    components.scheme = "https"  // Enforce strict https-only oembed calls
    components.queryItems = [
      .init(name: "url", value: url.absoluteString),
      .init(name: "format", value: "json"),
    ]
    guard let requestURL = components.url else { return nil }
    let request = URLRequest(url: requestURL)
    do {
      let (data, _) = try await URLSession.shared.data(for: request)
      let response = try JSONDecoder().decode(ProviderResponse.self, from: data)
      guard let thumbnailURL = response.thumbnailURL else {
        return nil
      }
      let (thumbnailData, _) = try await URLSession.shared.data(from: thumbnailURL)
      return UIImage(data: thumbnailData)
    } catch {
      return nil
    }
  }

  private struct Provider: Decodable {
    struct Endpoint: Decodable {
      var schemes: [String]?
      var urlString: String
      var url: URL? {
        URL(string: urlString.replacingOccurrences(of: "{format}", with: "json"))
      }

      enum CodingKeys: String, CodingKey {
        case schemes
        case urlString = "url"
      }
    }
    @URLString var providerURL: URL?
    var endpoints: [Endpoint]

    enum CodingKeys: String, CodingKey {
      case providerURL = "provider_url"
      case endpoints
    }
  }

  private struct ProviderResponse: Decodable {
    @URLString var thumbnailURL: URL?

    enum CodingKeys: String, CodingKey {
      case thumbnailURL = "thumbnail_url"
    }
  }
}
