// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared
import BraveCore

private let log = Logger.browserLogger

/// An helper class that manages the debouncing list and offers convenience methods
class DebouncingResourceDownloader {
  /// Object representing an item in the debouncing.json file found here:
  /// https://github.com/brave/adblock-lists/blob/master/brave-lists/debounce.json
  struct MatcherRule: Decodable {
    private enum CodingKeys: String, CodingKey {
      case include
      case exclude
      case action
      case param
    }

    enum Action: String {
      case base64
      case redirect
    }

    /// A set of patterns that are include in this rule
    let include: Set<String>
    /// A set of patterns that are excluded from this rule
    let exclude: Set<String>?
    /// The query param that contains the redirect `URL` to be extractted from
    let param: String
    /// The actions in a raw format. Need to be broken down to a set of `Action` entries
    private let action: String

    /// Actions in a strictly typed format and split up into an array
    /// - Note: Unrecognized actions will be filtered out
    var actions: Set<Action> {
      let actionStrings = action.split(separator: ",")
      return Set(actionStrings.compactMap({ Action(rawValue: String($0)) }))
    }

    /// Determines if this url is not excluded in the exclude list
    func isExcluded(url: URL) -> Bool {
      return exclude?.contains(where: { url.matches(pattern: $0) }) ?? false
    }

    /// Determines if this rule handles the given URL
    func isIncluded(url: URL) -> Bool {
      return include.contains(where: { url.matches(pattern: $0) })
    }

    /// Determines if this rule handles the given URL
    func handles(url: URL) -> Bool {
      // Is not in the excludes list (this should be shorter so we check it first)
      // and is in the include list
      return !isExcluded(url: url) && isIncluded(url: url)
    }
  }

  /// A class that, given a set of rules, helps in returning a redirect URL
  struct Matcher {
    typealias MatcherRuleEntry = (rule: MatcherRule, relevantPatterns: [String])
    private let etldToRule: [String: [MatcherRuleEntry]] // A
    private let queryToRule: [String: MatcherRule] // B
    private let otherRules: [MatcherRule] // C

    /// Initialize this matcher with the given rule set
    ///
    /// **Rules**
    ///
    /// 1. When parsing the rule set, place each rule in one of three buckets
    ///   A: all the URL patterns in the include and exclude fields are limited to a single eTLD+1
    ///   B: the rule applies to all URLs, regardless of origin
    ///   C: everything else (which I think is an empty set now)
    /// 2. Create a map of eTLD+1 -> bounce tracking rule
    /// 3. For each rule in bucket A, for each URL pattern, pull the eTLD+1 out of the URL pattern, and add it to the map from 2 (so, the eTLD+1 of the URL pattern points to the containing rule)
    /// 4. Create a map of query parameter name -> bounce tracking rule
    /// 5. For each rule in bucket B, for each query parameter, add it to the map from step 4
    init(rules: [MatcherRule]) {
      var etldToRule: [String: [MatcherRuleEntry]] = [:] // A
      var queryToRule: [String: MatcherRule] = [:] // B
      var otherRules: [MatcherRule] = [] // C

      for rule in rules {
        var etldToRelevantPatterns: [String: [String]] = [:]

        for pattern in rule.include {
          let urlPattern = URLPattern(validSchemes: .all)
          let parseResult = urlPattern.parse(pattern: pattern)

          switch parseResult {
          case .success:
            if urlPattern.isMatchingAllURLs {
              // We put this query param and rule to our A bucket
              // This seems to be an empty list for now
              queryToRule[rule.param] = rule
            } else if let etld1 = urlPattern.baseDomain {
              // We put this etld and rule to our B bucket
              var relevantPatterns = etldToRelevantPatterns[etld1] ?? []
              relevantPatterns.append(pattern)
              etldToRelevantPatterns[etld1] = relevantPatterns
            } else {
              // Everything else goes to our C bucket
              // For the time being this set only encompases patterns without a host
              otherRules.append(rule)
            }
          default:
            log.error("Cannot parse pattern `\(pattern)`. Got status `\(parseResult)`")
            continue
          }
        }

        // Add or append the entry for each etld
        for (etld1, relevantPatterns) in etldToRelevantPatterns {
          var entries = etldToRule[etld1] ?? []
          entries.append((rule, relevantPatterns))
          etldToRule[etld1] = entries
        }
      }

      self.etldToRule = etldToRule
      self.queryToRule = queryToRule
      self.otherRules = otherRules
    }

    /// Get redirect url recursively by continually applying the matcher rules to each url returned until we have no more redirects.
    /// Only handles `http` and `https` schemes.
    func redirectURLAll(from url: URL) -> URL? {
      var redirectingURL = url
      var result: URL?

      while let nextURL = redirectURLOnce(from: redirectingURL) {
        redirectingURL = nextURL
        result = nextURL
      }

      return result
    }

    /// Get a possible redirect url for the given URL and the given matchers. Will only get the next redirect url.
    /// Only handles `http` and `https` schemes.
    ///
    /// This code uses the patterns in the given matchers to determine if a redirect action is required.
    /// If it is, a redirect url will be returned provided we can extract it from the url
    ///
    /// **Rules**
    /// 1. For the given URL we *might* navigate too, pull out the `eTLD+1` and the query items
    /// 2. If the eTLD+1 is in the `etldToRule` map (from step 2 above), see if the full URL matches the rule, and return the rule
    /// (As an optimization, we only need to check the pattern the `eTLD+1` was extracted from and not the whole include list)
    /// 3. If any of the query params (the keys) in the map from #4 above are in URL we might navigate to, apply the corresponding rule
    /// 4. Apply all the rules from bucket C
    func redirectURLOnce(from url: URL) -> URL? {
      // Extract the redirect URL
      guard let components = URLComponents(url: url, resolvingAgainstBaseURL: false) else { return nil }
      guard let queryItems = components.queryItems else { return nil }
      guard let etld1 = url.baseDomain else { return nil }

      if let entries = etldToRule[etld1] {
        guard
          let entry = entries.first(where: { url.matches(any: $0.relevantPatterns) }),
          !entry.rule.isExcluded(url: url),
          let queryItem = queryItems.first(where: { $0.name == entry.rule.param })
        else {
          return nil
        }

        return redirectURL(queryItem: queryItem, actions: entry.rule.actions)
      } else if let pair = matchingQueryItemsAndActions(for: url, queryItems: queryItems) {
        return redirectURL(queryItem: pair.queryItem, actions: pair.actions)
      } else if let rule = otherRules.first(where: { $0.handles(url: url) }) {
        guard let queryItem = queryItems.first(where: { $0.name == rule.param }) else {
          return nil
        }

        return redirectURL(queryItem: queryItem, actions: rule.actions)
      } else {
        return nil
      }
    }

    /// Attempt to extract the query value and actions from the list of queryItems using the `queryToRule` (all urls) map.
    ///
    /// - Parameter queryItems: The query items to extract the value and actions from
    /// - Returns: A raw query item and actions to perform on that query item
    private func matchingQueryItemsAndActions(
      for url: URL,
      queryItems: [URLQueryItem]
    ) -> (queryItem: URLQueryItem, actions: Set<MatcherRule.Action>)? {
      for queryItem in queryItems {
        guard
          let rule = queryToRule[queryItem.name],
          !rule.isExcluded(url: url)
        else {
          continue
        }

        return (queryItem, rule.actions)
      }

      return nil
    }

    /// It attempts to extract a redirect `URL` from the given query item using the given actions
    ///
    /// It returns nil if it cannot find a valid URL within the query item
    private func redirectURL(queryItem: URLQueryItem, actions: Set<MatcherRule.Action>) -> URL? {
      guard let queryValue = queryItem.value else { return nil }

      // For now we only support redirecting so it makes sense to check this right away
      // No point in trying to decode anything if we don't have this action
      guard actions.contains(.redirect) else {
        return nil
      }

      if actions.contains(.base64) {
        // We need to base64 decode the url
        guard let data = Data(base64Encoded: queryValue),
              let decodedString = String(data: data, encoding: .utf8)
        else {
          return nil
        }

        return URL(string: decodedString)
      } else {
        return URL(string: queryValue)
      }
    }
  }

  /// The base s3 environment url that hosts the debouncing (and other) files.
  /// Cannot be used as-is and must be combined with a path
  private lazy var baseResourceURL: URL = {
    if AppConstants.buildChannel.isPublic {
      return URL(string: "https://adblock-data.s3.brave.com")!
    } else {
      return URL(string: "https://adblock-data-staging.s3.bravesoftware.com")!
    }
  }()

  /// The complete url that hosts the debounce rules. A combination of `baseResourceURL`
  /// and the path to the debounce list.
  private lazy var resourceURL: URL = {
    URL(string: "/ios/debounce.json", relativeTo: baseResourceURL)!
  }()

  static let shared = DebouncingResourceDownloader()
  private let networkManager: NetworkManager
  private let servicesKeyName = "SERVICES_KEY"
  private let servicesKeyHeaderValue = "BraveServiceKey"
  private let cacheFileName = "ios-debouce"
  private let cacheFolderName = "debounce-data"
  private var matcher: Matcher?

  /// A boolean indicating if this is a first time load of this downloader so we only load cached data once
  private var initialLoad = true
  /// Initialized with year 1970 to force adblock fetch at first launch.
  private var lastFetchDate = Date(timeIntervalSince1970: 0)
  /// How frequently to fetch the data
  private lazy var fetchInterval = AppConstants.buildChannel.isPublic ? 6.hours : 10.minutes

  /// Initialize this instance with a network manager
  init(networkManager: NetworkManager = NetworkManager()) {
    self.networkManager = networkManager
  }

  /// Setup this downloader with rule `JSON` data.
  func setup(withRulesJSON ruleData: Data) throws {
    // Decode the data and store it for later user
    let jsonDecoder = JSONDecoder()
    jsonDecoder.keyDecodingStrategy = .convertFromSnakeCase
    let rules = try jsonDecoder.decode([MatcherRule].self, from: ruleData)
    matcher = Matcher(rules: rules)
  }

  /// Downloads the required resources if they are not available. Loads any cached data if it already exists.
  func startLoading() {
    let now = Date()
    let resourceURL = self.resourceURL
    let cacheFileName = [self.cacheFileName, "json"].joined(separator: ".")
    let etagFileName = [cacheFileName, "etag"].joined(separator: ".")
    let cacheFolderName = self.cacheFolderName

    guard let cacheFolderURL = FileManager.default.getOrCreateFolder(name: cacheFolderName) else {
      log.error("Failed to get folder: \(cacheFolderName)")
      return
    }

    if initialLoad {
      initialLoad = false

      do {
        // Load data from disk if we have it
        if let cachedData = try self.dataFromDocument(inFolder: cacheFolderURL, fileName: cacheFileName) {
          try setup(withRulesJSON: cachedData)
        }
      } catch {
        log.error(error)
      }
    }

    if now.timeIntervalSince(lastFetchDate) >= fetchInterval {
      lastFetchDate = now

      let networkManager = self.networkManager
      let etag: String?
      let customHeaders: [String: String]

      if let servicesKeyValue = Bundle.main.getPlistString(for: self.servicesKeyName) {
        customHeaders = [self.servicesKeyHeaderValue: servicesKeyValue]
      } else {
        customHeaders = [:]
      }

      do {
        etag = try self.stringFromDocument(inFolder: cacheFolderURL, fileName: etagFileName)
      } catch {
        etag = nil
        log.error(error)
      }

      Task { [weak self] in
        guard let self = self else { return }

        let resource = try await networkManager.downloadResource(
          with: resourceURL,
          resourceType: .cached(etag: etag),
          checkLastServerSideModification: !AppConstants.buildChannel.isPublic,
          customHeaders: customHeaders
        )

        guard !resource.data.isEmpty else {
          return
        }

        do {
          // Save the data to file
          try self.writeDataToDisk(
            data: resource.data, inFolder: cacheFolderURL, fileName: cacheFileName
          )

          // Save the etag to file
          if let data = resource.etag?.data(using: .utf8) {
            try self.writeDataToDisk(
              data: data, inFolder: cacheFolderURL, fileName: etagFileName
            )
          }

          try setup(withRulesJSON: resource.data)
        } catch {
          log.error(error)
        }
      }
    }
  }

  /// Get a possible redirect url for the given URL. Does this
  ///
  /// This code uses the downloade `Matcher` to determine if a redirect action is required.
  /// If it is, a redirect url will be returned provided we can extract it from the url.
  func redirectURL(for url: URL) -> URL? {
    return matcher?.redirectURLAll(from: url)
  }

  /// Load data from disk  given by the folderName and fileName
  /// and found in the `applicationSupportDirectory` `SearchPathDirectory`.
  ///
  /// - Note: `fileName` must contain the full file name including the extension.
  private func dataFromDocument(inFolder folderURL: URL, fileName: String) throws -> Data? {
    let fileUrl = folderURL.appendingPathComponent(fileName)
    return FileManager.default.contents(atPath: fileUrl.path)
  }

  /// Load string from the document given by the `folderName` and `fileName`
  /// and found in the `applicationSupportDirectory` `SearchPathDirectory`.
  ///
  /// - Note: `fileName` must contain the full file name including the extension.
  private func stringFromDocument(inFolder folderURL: URL, fileName: String) throws -> String? {
    let fileUrl = folderURL.appendingPathComponent(fileName)
    guard let data = FileManager.default.contents(atPath: fileUrl.path) else { return nil }
    return String(data: data, encoding: .utf8)
  }

  /// Write data to disk to a file given by `folderName` and `fileName`
  /// into the `applicationSupportDirectory` `SearchPathDirectory`.
  ///
  /// - Note: `fileName` must contain the full file name including the extension.
  @discardableResult
  func writeDataToDisk(data: Data, inFolder folderURL: URL, fileName: String) throws -> URL {
    let fileUrl = folderURL.appendingPathComponent(fileName)
    try data.write(to: fileUrl, options: [.atomic])
    return fileUrl
  }
}

enum DirectoryError: Error {
  case cannotFindSearchPathDirectory
}

extension URL {
  func matches(pattern: String) -> Bool {
    let urlPattern = URLPattern(validSchemes: .all)
    let parseResult = urlPattern.parse(pattern: pattern)

    switch parseResult {
    case .success:
      return urlPattern.matchesURL(self)
    default:
      log.error("Cannot parse pattern `\(pattern)`. Got status `\(parseResult)`")
      return false
    }
  }

  func matches(any patterns: [String]) -> Bool {
    return patterns.contains(where: { self.matches(pattern: $0) })
  }
}

private extension URLPattern {
  var baseDomain: String? {
    let result = NSURL.domainAndRegistry(host: host)
    return result.isEmpty ? nil : result
  }
}
