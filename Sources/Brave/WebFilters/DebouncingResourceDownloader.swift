// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared
import BraveCore
import os.log

/// An helper class that manages the debouncing list and offers convenience methods
public class DebouncingResourceDownloader {
  /// An object representing errors returned from `MatcherRule`
  enum RuleError: Error {
    /// An error returned during parsing when an action is not supported by iOS.
    /// This could be a valid scenario if we decide to add new actions in the future
    case unsupportedAction(String)
    /// An error returned during parsing when an action is not supported by iOS
    /// This could be a valid scenario if we decide to add new prefs in the future
    case unsupportedPreference(String)
  }
  
  /// Object representing an item in the debouncing.json file found here:
  /// https://github.com/brave/adblock-lists/blob/master/brave-lists/debounce.json
  struct MatcherRule: Decodable {
    private enum CodingKeys: String, CodingKey {
      case include, exclude, action, param, pref, prependScheme
    }

    /// Defines actions that should be performed on the matching url
    /// - Note: Some rules can be paired while others cannot.
    enum Action: String {
      /// Requres the extracted query param to be base64 decoded
      /// The extracted url is provided as a query param and the param name is defined in the `param` field
      case base64
      /// Defines that the url should be redirected to the extracted url
      /// The extracted url is provided as a query param and the param name is defined in the `param` field
      /// unless `regex-path` is used
      case redirect
      /// Defines that the url should be extracted by a regex pattern.
      /// The regex is available in the `param` field.
      case regexPath = "regex-path"
    }
    
    enum Preference: String {
      case deAmpEnabled = "brave.de_amp.enabled"
    }

    /// A set of patterns that are include in this rule
    let include: Set<String>
    /// A set of patterns that are excluded from this rule
    let exclude: Set<String>?
    /// The query param that contains the redirect `URL` to be extractted from
    let param: String
    /// The scheme to prepend if the scheme is not available
    let prependScheme: String?
    /// The actions to be performed on the URL
    let actions: Set<Action>
    /// A list of preferences to be checked before debouncing
    let preferences: Set<Preference>

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
    
    public init(from decoder: Decoder) throws {
      let container = try decoder.container(keyedBy: CodingKeys.self)
      self.include = try container.decode(Set<String>.self, forKey: .include)
      self.exclude = try container.decode(Set<String>.self, forKey: .exclude)
      self.param = try container.decode(String.self, forKey: .param)
      self.prependScheme = try container.decodeIfPresent(String.self, forKey: .prependScheme)
      
      let pref = try container.decodeIfPresent(String.self, forKey: .pref)
      let action = try container.decode(String.self, forKey: .action)
      self.actions = try Self.makeActions(fromString: action)
      self.preferences = try Self.makePreferences(fromString: pref)
    }
    
    /// Actions in a strictly typed format and split up into an array
    /// - Note: Unrecognized actions will throw an error
    private static func makeActions(fromString string: String) throws -> Set<Action> {
      let actionStrings = string.split(separator: ",")
      
      return Set(try actionStrings.map({
        if let action = Action(rawValue: String($0)) {
          return action
        } else {
          throw RuleError.unsupportedAction(String($0))
        }
      }))
    }
    
    /// Preferences in a strictly typed format and split up into an array
    /// - Note: Unrecognized preferences will throw an error
    private static func makePreferences(fromString string: String?) throws -> Set<Preference> {
      guard let prefStrings = string?.split(separator: ",") else {
        return []
      }
      
      return Set(try prefStrings.map({
        if let action = Preference(rawValue: String($0)) {
          return action
        } else {
          throw RuleError.unsupportedPreference(String($0))
        }
      }))
    }
  }
  
  /// Defines redirect actions that should be performed on the matching url.
  /// These rules contain additional information needed to successfully extract the redirect URL.
  ///
  /// - Note: Unlike `MatcherRule.Action`, only one action can be applied on a single url
  public enum RedirectAction {
    /// An action that requires the url to be extracted by applying a regex pattern on the path
    case regexParam(regex: String)
    /// An action that requires the url to be extracted from a query param given by its name
    case queryParam(name: String)
  }
  
  /// An object representing a complete and sanitized redirect rule with valid preferences and actions.
  public struct RedirectRule {
    let action: RedirectAction
    let shouldBase64Decode: Bool
    let preferences: Set<MatcherRule.Preference>
    let prependScheme: String?
    
    /// Extracts a redirect url from the given url. This function appled all necessary actions (MatcherRule.Action)
    /// and returns a value that is ready to go and can be applied.
    ///
    /// - Note: You still need to check the preferences however.
    public func extractRedirectURL(from url: URL) throws -> URL? {
      guard let value = try extractDecodedValue(from: url) else { return nil }
      
      if let prependScheme = prependScheme {
        return makeRedirectURL(string: value, prependScheme: prependScheme)
      } else {
        return makeRedirectURL(string: value)
      }
    }
    
    /// Extracts and decodes a matching value from the given url.
    ///
    /// - Note: This value is not yet ready to be transformed into a URL as further actions,
    /// such as prepending a scheme, must be performed.
    private func extractDecodedValue(from url: URL) throws -> String? {
      guard let value = try extractRawValue(from: url) else { return nil }
      
      if shouldBase64Decode {
        // We need to base64 decode the url
        guard let data = Data(base64Encoded: value) else {
          return nil
        }

        return String(data: data, encoding: .utf8)
      } else {
        return value
      }
    }
    
    /// Attempt to extract a raw value without any decoding.
    /// - Note: This value is not yet ready to be transformed into a URL as the value may still need to be decoded.
    private func extractRawValue(from url: URL) throws -> String? {
      guard let components = URLComponents(url: url, resolvingAgainstBaseURL: false) else { return nil }
      
      switch action {
      case .regexParam(let pattern):
        // This is a requirement that the pattern is less than 200 characters long
        guard pattern.count < 200 else { return nil }
        
        let path = components.path
        let regex = try NSRegularExpression(pattern: pattern)
        
        // This is a requirement that there is only 1 capture group
        guard regex.numberOfCaptureGroups == 1 else { return nil }
        
        let range = NSRange(location: 0, length: path.count)
        let matches = regex.matches(in: path, range: range)
        guard matches.count == 1, let match = matches.first else { return nil }
        let matchRange = match.range(at: 1)
        
        if let swiftRange = Range(matchRange, in: path) {
          let result = String(path[swiftRange])
          return result
        } else {
          return nil
        }
      case .queryParam(let param):
        guard let queryItems = components.queryItems else { return nil }
        return queryItems.first(where: { $0.name == param })?.value
      }
    }
    
    /// Checks if the given string represents a valid URL we can redirect to.
    ///
    /// For a URL to be valid it must:
    /// 1. Have a valid scheme and have a host
    /// 2. Is an `IP` address or have a valid `eTLD+1`
    ///
    /// Example:
    /// - `https://basicattentiontoken.com` returns `true` because tthe host is in the registry and has a scheme
    /// - `basicattentiontoken.com` returns `false` because the scheme is missing
    /// - `https://xyz` returns `false` because it has no eTLD+1
    /// - `xyz` returns  `false` because scheme is missing, has no eTLD+1
    private func isValidRedirectURL(string: String) -> Bool {
      guard let url = URL(string: string), url.schemeIsValid, let host = url.host else { return false }
      
      // Check that we have an IP address or 2 host components seperated by a `.` (eTLD+1)
      return NSURL.isHostIPAddress(host: host) || !NSURL.domainAndRegistry(host: host).isEmpty
    }
    
    /// Tries to construct a valid URL string by checking `isValidRedirectURL`
    /// and ensuring that there is a valid URL scheme.
    func makeRedirectURL(string: String) -> URL? {
      guard isValidRedirectURL(string: string) else { return nil }
      return URL(string: string)
    }

    /// Attempts to make a redirect url by pre-pending the scheme.
    ///
    /// - Note: If the raw value alread has a scheme, this method will return nil.
    func makeRedirectURL(string: String, prependScheme: String) -> URL? {
      guard !isValidRedirectURL(string: string) else {
        // When we need to apply the scheme, we should not have a valid url before applying the scheme.
        return nil
      }
      
      if let url = makeRedirectURL(string: [prependScheme, string].joined(separator: "://")) {
        return url
      } else {
        return nil
      }
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
    init(rules: [Result<MatcherRule, Error>]) {
      var etldToRule: [String: [MatcherRuleEntry]] = [:] // A
      var queryToRule: [String: MatcherRule] = [:] // B
      var otherRules: [MatcherRule] = [] // C

      for result in rules {
        do {
          let rule = try result.get()
          var etldToRelevantPatterns: [String: [String]] = [:]

          for pattern in rule.include {
            let urlPattern = URLPattern(validSchemes: .all)
            let parseResult = urlPattern.parse(pattern: pattern)
            let actions = rule.actions

            switch parseResult {
            case .success:
              if urlPattern.isMatchingAllURLs && !actions.contains(.regexPath) {
                // We put this query param and rule to our A bucket
                // This seems to be an empty list for now
                queryToRule[rule.param] = rule
              } else if let etld1 = urlPattern.baseDomain, !etld1.isEmpty {
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
              Logger.module.error("Cannot parse pattern `\(pattern)`. Got status `\(parseResult.rawValue)`")
              continue
            }
          }
          
          // Add or append the entry for each etld
          for (etld1, relevantPatterns) in etldToRelevantPatterns {
            var entries = etldToRule[etld1] ?? []
            entries.append((rule, relevantPatterns))
            etldToRule[etld1] = entries
          }
        } catch RuleError.unsupportedAction(let rawRule) {
          // We may add new actions in the future. If thats the case, this will throw but we don't want
          // an error presented or anything to happen because iOS is not ready to handle this new rule
          // But since it's impossible to discriminate a programmer mistake from a new rule added
          // we at least log this error.
          Logger.module.error("Unsupported rule action used for debouncing: `\(rawRule)`")
        } catch RuleError.unsupportedPreference(let rawRule) {
          // We may add new prefs in the future. If thats the case, this will throw but we don't want
          // an error presented or anything to happen because iOS is not ready to handle this new rule
          // But since it's impossible to discriminate a programmer mistake from a new rule added
          // we at least log this error.
          Logger.module.error("Unsupported rule preference used for debouncing: `\(rawRule)`")
        } catch {
          Logger.module.error("\(error.localizedDescription)")
        }
      }

      self.etldToRule = etldToRule
      self.queryToRule = queryToRule
      self.otherRules = otherRules
    }
    
    /// Get redirect url recursively by continually applying the matcher rules to each url returned until we have no more redirects.
    ///
    /// The following conditions must be met to redirect:
    /// 1. Must have a `MatcherRule` that satisifes this url
    /// 2. Extracted URL  not be the same domain as the base URL
    /// 3. Extracted URL must be a valid URL (Have valid scheme, have an `eTLD+1` or be an IP address etc.. )
    /// 4. Extracted URL must have a `http` or `https` scheme
    func redirectChain(from url: URL) -> [(url: URL, rule: RedirectRule)] {
      var redirectingURL = url
      var result: [(url: URL, rule: RedirectRule)] = []

      do {
        while let nextElement = try redirectURLOnce(from: redirectingURL) {
          redirectingURL = nextElement.url
          result.append(nextElement)
        }
      } catch {
        Logger.module.error("\(error.localizedDescription)")
      }

      return result
    }

    /// Get a possible redirect URL and rule for the given URL.
    /// This only returns the next redirect entry and does not process the returned urls recursively.
    ///
    /// The following conditions must be met to redirect:
    /// 1. Must have a `MatcherRule` that satisifes this url
    /// 2. Extracted URL must not have the same origin as that of the base URL
    /// 3. Extracted URL must not have the same `eTLD+1` as that of the base URL (#6146)
    /// 4. Extracted URL must be a valid URL (Have valid scheme, have an `eTLD+1` or be an IP address etc.. )
    /// 5. Extracted URL must have a `http` or `https` scheme
    private func redirectURLOnce(from url: URL) throws -> (url: URL, rule: RedirectRule)? {
      guard let rule = matchingRedirectRule(for: url) else {
        return nil
      }
      
      guard let extractedURL = try rule.extractRedirectURL(from: url),
            url.origin != extractedURL.origin,
            url.baseDomain != extractedURL.baseDomain,
            extractedURL.scheme == "http" || extractedURL.scheme == "https" else {
        return nil
      }
      
      return (extractedURL, rule)
    }
    
    /// Attempts to find a valid  query param  and a set of actions for the given URL
    /// - Throws: May throw `DebouncingResourceDownloader.RuleError`
    private func matchingRedirectRule(for url: URL) -> RedirectRule? {
      guard let rule = matchingRule(for: url) else {
        return nil
      }
      
      let actions = rule.actions
      let preferences = rule.preferences
      
      if actions.contains(.regexPath) {
        return RedirectRule(
          action: RedirectAction.regexParam(regex: rule.param),
          shouldBase64Decode: actions.contains(.base64),
          preferences: preferences,
          prependScheme: rule.prependScheme
        )
      } else {
        return RedirectRule(
          action: RedirectAction.queryParam(name: rule.param),
          shouldBase64Decode: actions.contains(.base64),
          preferences: preferences,
          prependScheme: rule.prependScheme
        )
      }
    }
    
    /// Attempts to find a valid  rule for the given URL
    ///
    /// **Rules**
    /// 1. For the given URL we *might* navigate too, pull out the `eTLD+1` and the query items
    /// 2. If the eTLD+1 is in the `etldToRule` map (from step 2 above), see if the full URL matches the rule, and return the rule
    /// (As an optimization, we only need to check the pattern the `eTLD+1` was extracted from and not the whole include list)
    /// 3. If any of the query params (the keys) in the map from #4 above are in URL we might navigate to, apply the corresponding rule
    /// 4. Apply all the rules from bucket C
    private func matchingRule(for url: URL) -> MatcherRule? {
      if let etld1 = url.baseDomain, let entries = etldToRule[etld1] {
        guard
          let entry = entries.first(where: { url.matches(any: $0.relevantPatterns) }),
          !entry.rule.isExcluded(url: url)
        else {
          return nil
        }

        return entry.rule
      } else {
        return matchingCachedQueryParamRule(for: url)
          ?? otherRules.first(where: { $0.handles(url: url) })
      }
    }

    /// Attempt to extract the query value and actions from the list of queryItems using the `queryToRule` (all urls) map.
    ///
    /// - Parameter url: The url to extract the query params from
    /// - Returns: A rule for the given url
    private func matchingCachedQueryParamRule(for url: URL) -> MatcherRule? {
      // Extract the redirect URL
      guard let components = URLComponents(url: url, resolvingAgainstBaseURL: false) else { return nil }
      guard let queryItems = components.queryItems else { return nil }
      
      for queryItem in queryItems {
        if let rule = queryToRule[queryItem.name], !rule.isExcluded(url: url) {
          return rule
        }
      }

      return nil
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

  public static let shared = DebouncingResourceDownloader()
  private(set) var matcher: Matcher?

  /// Initialize this instance with a network manager
  init() {}

  /// Setup this downloader with rule `JSON` data.
  ///
  /// - Note: Decoded values that have unknown types are filtered out
  func setup(withRulesJSON ruleData: Data) throws {
    // Decode the data and store it for later user
    let jsonDecoder = JSONDecoder()
    jsonDecoder.keyDecodingStrategy = .convertFromSnakeCase
    let rules = try jsonDecoder.decode([Result<MatcherRule, Error>].self, from: ruleData)
    matcher = Matcher(rules: rules)
  }
  
  /// Get a possible redirect url for the given URL. Does this
  ///
  /// This code uses the downloade `Matcher` to determine if a redirect action is required.
  /// If it is, a redirect url will be returned provided we can extract it from the url.
  func redirectChain(for url: URL) -> [(url: URL, rule: RedirectRule)] {
    return matcher?.redirectChain(from: url) ?? []
  }
}

extension URL {
  func matches(pattern: String) -> Bool {
    let urlPattern = URLPattern(validSchemes: .all)
    let parseResult = urlPattern.parse(pattern: pattern)

    switch parseResult {
    case .success:
      return urlPattern.matchesURL(self)
    default:
      Logger.module.error("Cannot parse pattern `\(pattern)`. Got status `\(parseResult.rawValue)`")
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

extension Result: Decodable where Success: Decodable, Failure == Error {
  public init(from decoder: Decoder) throws {
    do {
      let decodable = try Success(from: decoder)
      self = .success(decodable)
    } catch {
      self = .failure(error)
    }
  }
}
