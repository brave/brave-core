// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import Shared

public protocol ProviderDisplayString {
  var provider: String { get }
  /// A properly capitalized string to use when displaying the provider.
  ///
  /// Example: `youtube` becomes `YouTube`, `github` becomes `GitHub`, etc.
  var providerDisplayString: String { get }
}

extension ProviderDisplayString {
  public var providerDisplayString: String {
    let providers = [
      "github": "GitHub",
      "reddit": "Reddit",
      "twitter": "Twitter",
      "twitch": "Twitch",
      "vimeo": "Vimeo",
      "youtube": "YouTube",
    ]
    return providers[provider.lowercased()] ?? provider
  }
}

extension BraveCore.BraveRewards.PublisherInfo: ProviderDisplayString {}

extension BraveCore.BraveRewards.PublisherInfo {
  /// The display name to show when showing Publisher names (example: "X on GitHub", "Y on
  /// YouTube", or "reddit.com" for publishers that dont have individual content providers)
  var displayName: String {
    if provider.isEmpty || name.isEmpty {
      return id
    }
    return "\(name) \(String(format: Strings.Rewards.onProviderText, providerDisplayString))"
  }

  /// The attributed display name to show when showing Publisher names where the publishers name
  /// is bolded. (i.e. "**X** on GitHub")
  func attributedDisplayName(fontSize: CGFloat) -> NSAttributedString {
    if provider.isEmpty || name.isEmpty {
      return NSAttributedString(string: id, attributes: [.font: UIFont.systemFont(ofSize: fontSize)])
    }
    let string = NSMutableAttributedString(
      string: "\(name) \(String(format: Strings.Rewards.onProviderText, providerDisplayString))",
      attributes: [.font: UIFont.systemFont(ofSize: fontSize)]
    )
    let range = NSRange(name.startIndex..<name.endIndex, in: name)
    if range.length > 0 {
      string.addAttribute(.font, value: UIFont.systemFont(ofSize: fontSize, weight: .semibold), range: range)
    }
    return string
  }
}
