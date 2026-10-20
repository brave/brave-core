// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Strings

extension ExternalLinkMode: @retroactive Identifiable {
  public static var allOptions: [ExternalLinkMode] {
    [.current, .ask, .regular, .privateBrowsing, .quickView].filter(\.isAvailable)
  }

  public var isAvailable: Bool {
    self != .quickView || FeatureList.kQuickViewEnabled.enabled
  }

  public var id: Int { rawValue }

  public var localizedTitle: String {
    switch self {
    case .current: return Strings.ExternalLinks.currentOption
    case .ask: return Strings.ExternalLinks.askOption
    case .regular: return Strings.ExternalLinks.regularModeOption
    case .privateBrowsing: return Strings.ExternalLinks.privateModeOption
    case .quickView: return Strings.ExternalLinks.quickViewOption
    @unknown default: return Strings.ExternalLinks.currentOption
    }
  }

  public var localizedDescription: String {
    switch self {
    case .current: return Strings.ExternalLinks.currentOptionDescription
    case .ask: return Strings.ExternalLinks.askOptionDescription
    case .regular: return Strings.ExternalLinks.regularModeOptionDescription
    case .privateBrowsing: return Strings.ExternalLinks.privateModeOptionDescription
    case .quickView: return Strings.ExternalLinks.quickViewOptionDescription
    @unknown default: return Strings.ExternalLinks.currentOptionDescription
    }
  }
}

/// A choice made for a specific link opened from another app.
public enum ExternalLinkChoice: CaseIterable, Identifiable {
  case regularMode
  case privateMode
  case quickView

  public init?(_ mode: ExternalLinkMode) {
    guard mode.isAvailable else { return nil }
    switch mode {
    case .regular: self = .regularMode
    case .privateBrowsing: self = .privateMode
    case .quickView: self = .quickView
    case .current, .ask: return nil
    @unknown default: return nil
    }
  }

  public static var allAvailable: [ExternalLinkChoice] {
    allCases.filter(\.isAvailable)
  }

  public var id: Self { self }

  public var isAvailable: Bool {
    self != .quickView || FeatureList.kQuickViewEnabled.enabled
  }

  var braveSystemImage: String {
    switch self {
    case .regularMode: return "leo.window.tab"
    case .privateMode: return "leo.product.private-window"
    case .quickView: return "leo.browser.quick-view"
    }
  }

  var localizedPromptDescription: String {
    switch self {
    case .regularMode: return Strings.ExternalLinks.promptRegularModeDescription
    case .privateMode: return Strings.ExternalLinks.promptPrivateModeDescription
    case .quickView: return Strings.ExternalLinks.promptQuickViewDescription
    }
  }

  public var contentSettingMode: ExternalLinkMode {
    switch self {
    case .regularMode: return .regular
    case .privateMode: return .privateBrowsing
    case .quickView: return .quickView
    }
  }
}
