// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import os.log

/// An interface representing a match rule.
///
/// This protocol allows us to use slightly different rule objects depending on the service.
protocol MatcherRuleProtocol: Decodable {
  /// URL patterns that are included from this rule
  var include: Set<String> { get }
  /// URL patterns that are excluded from this rule
  var exclude: Set<String>? { get }
  /// The param that we can match this rule by in case we have a match-all pattern
  var matchAllParam: String? { get }
}

extension MatcherRuleProtocol {
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
struct URLMatcher<Rule: MatcherRuleProtocol> {
  typealias MatcherRuleEntry = (rule: Rule, relevantPatterns: [String])
  private let etldToRule: [String: [MatcherRuleEntry]] // A
  private let queryToRule: [String: Rule] // B
  private let otherRules: [Rule] // C
  
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
  init(rules: [Result<Rule, Error>]) {
    var etldToRule: [String: [MatcherRuleEntry]] = [:] // A
    var queryToRule: [String: Rule] = [:] // B
    var otherRules: [Rule] = [] // C

    for result in rules {
      do {
        let rule = try result.get()
        var etldToRelevantPatterns: [String: [String]] = [:]

        for pattern in rule.include {
          let urlPattern = URLPattern(validSchemes: .all)
          let parseResult = urlPattern.parse(pattern: pattern)

          switch parseResult {
          case .success:
            if let param = rule.matchAllParam, urlPattern.isMatchingAllURLs {
              // We put this query param and rule to our A bucket
              // This seems to be an empty list for now
              queryToRule[param] = rule
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
            ContentBlockerManager.log.error("Cannot parse pattern `\(pattern)`. Got status `\(parseResult.rawValue)`")
            continue
          }
        }
        
        // Add or append the entry for each etld
        for (etld1, relevantPatterns) in etldToRelevantPatterns {
          var entries = etldToRule[etld1] ?? []
          entries.append((rule, relevantPatterns))
          etldToRule[etld1] = entries
        }
      } catch {
        ContentBlockerManager.log.error("\(error.localizedDescription)")
      }
    }

    self.etldToRule = etldToRule
    self.queryToRule = queryToRule
    self.otherRules = otherRules
  }
  
  /// Attempts to find a valid rule for the given URL
  ///
  /// **Rules**
  /// 1. For the given URL we *might* navigate too, pull out the `eTLD+1` and the query items
  /// 2. If the eTLD+1 is in the `etldToRule` map (from step 2 above), see if the full URL matches the rule, and return the rule
  /// (As an optimization, we only need to check the pattern the `eTLD+1` was extracted from and not the whole include list)
  /// 3. If any of the query params (the keys) in the map from #4 above are in URL we might navigate to, apply the corresponding rule
  /// 4. Apply all the rules from bucket C
  func matchingRule(for url: URL) -> Rule? {
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
  private func matchingCachedQueryParamRule(for url: URL) -> Rule? {
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

extension URL {
  func matches(pattern: String) -> Bool {
    let urlPattern = URLPattern(validSchemes: .all)
    let parseResult = urlPattern.parse(pattern: pattern)

    switch parseResult {
    case .success:
      return urlPattern.matchesURL(self)
    default:
      ContentBlockerManager.log.error("Cannot parse pattern `\(pattern)`. Got status `\(parseResult.rawValue)`")
      return false
    }
  }

  func matches(any patterns: [String]) -> Bool {
    return patterns.contains(where: { self.matches(pattern: $0) })
  }
}

extension URLPattern {
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
