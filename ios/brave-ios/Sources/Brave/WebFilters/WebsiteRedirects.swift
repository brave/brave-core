// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import Preferences

struct WebsiteRedirects {
  private struct Rule {
    /// This is the host we want to redirect users to. All other parts of the url remain unchanged
    let hostToRedirectTo: String
    /// What hosts should be redirected. Due to compat reasons not every host may be easily replaced.
    let eligibleHosts: Set<String>
    /// Any additional predicates on whether or not the URL should redirect (requires all to be satified)
    ///
    /// It can be assumed that any URL passed into these predicates will be a eligible host
    var additionalPredicates: [(URL) -> Bool] = []
    /// What hosts should not be redirected. It's either due to web compat reasons or to let user explicitely type a url to not override it.
    /// Reddit is good example, regular reddit.com and new.reddit.com point to the same new user interface.
    /// So we redirect all regular reddit.com link, but the user may explicitely go to new.reddit.com without having to disable the reddit redirect toggle.
    let excludedHosts: Set<String>
  }
  
  private static let reddit = Rule(
    hostToRedirectTo: "old.reddit.com",
    eligibleHosts: ["reddit.com", "www.reddit.com", "np.reddit.com", "amp.reddit.com", "i.reddit.com"],
    additionalPredicates: [ { url in
        !url.path.hasPrefix("/media")
      }
    ],
    excludedHosts: ["new.reddit.com"])
  
  private static let npr = Rule(
    hostToRedirectTo: "text.npr.org",
    eligibleHosts: ["www.npr.org", "npr.org"],
    excludedHosts: ["account.npr.org"])
  
  private static var enabledRules: [Rule] {
    var rules = [Rule]()
    
    if Preferences.WebsiteRedirects.reddit.value {
      rules.append(reddit)
    }
    
    if Preferences.WebsiteRedirects.npr.value {
      rules.append(npr)
    }
    
    return rules
  }
  
  /// Decides whether a website the user is on should bre redirected to another website.
  /// Returns nil if no redirection should happen.
  static func redirect(for url: URL) -> URL? {
    guard let host = url.host else { return nil }
    
    let foundMatch = enabledRules
      .filter { !$0.excludedHosts.contains(host) && host != $0.hostToRedirectTo }
      .first(where: { $0.eligibleHosts.contains(host) })
    
    guard let redirect = foundMatch,
          redirect.additionalPredicates.allSatisfy({ $0(url) }),
          var components = URLComponents(url: url, resolvingAgainstBaseURL: false) else { return nil }
    
    // For privacy reasons we do not redirect websites if username or password are present.
    if components.user != nil || components.password != nil { return nil }
    
    components.host = redirect.hostToRedirectTo
    return components.url
  }
}
