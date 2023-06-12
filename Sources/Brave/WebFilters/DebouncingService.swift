// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// An helper class that manages the debouncing list and offers convenience methods
public class DebouncingService {
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
  struct MatcherRule: Decodable, MatcherRuleProtocol {
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
    
    /// The param that we can match this rule by in case we have a match-all pattern
    var matchAllParam: String? {
      // We shouldn't use the param for matching if we have a regexPath action
      // Because our match-all is not really a match all. We need to consider the `regexPath`
      guard !actions.contains(.regexPath) else { return nil }
      return param
    }
    
    public init(from decoder: Decoder) throws {
      let container = try decoder.container(keyedBy: CodingKeys.self)
      self.include = try container.decode(Set<String>.self, forKey: .include)
      self.exclude = try container.decodeIfPresent(Set<String>.self, forKey: .exclude)
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
  
  public static let shared = DebouncingService()
  private(set) var matcher: URLMatcher<MatcherRule>?

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
    matcher = URLMatcher(rules: rules)
  }
  
  /// Get redirect url recursively by continually applying the matcher rules to each url returned until we have no more redirects.
  ///
  /// The following conditions must be met to redirect:
  /// 1. Must have a `MatcherRule` that satisifes this url
  /// 2. Extracted URL  not be the same domain as the base URL
  /// 3. Extracted URL must be a valid URL (Have valid scheme, have an `eTLD+1` or be an IP address etc.. )
  /// 4. Extracted URL must have a `http` or `https` scheme
  func redirectChain(for url: URL) -> [(url: URL, rule: RedirectRule)] {
    var redirectingURL = url
    var result: [(url: URL, rule: RedirectRule)] = []

    do {
      while let nextElement = try redirectURLOnce(from: redirectingURL) {
        redirectingURL = nextElement.url
        result.append(nextElement)
      }
    } catch {
      ContentBlockerManager.log.error("\(error.localizedDescription)")
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
    guard let rule = matcher?.matchingRule(for: url) else {
      return nil
    }
    
    let preferences = rule.preferences
    let actions = rule.actions
    
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
}
