// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import DesignSystem
import Strings
import UIKit
@_spi(ChromiumWebViewAccess) import Web
import os.log

extension TabDataValues {
  private struct ElementPickerTabHelperKey: TabDataKey {
    static var defaultValue: ElementPickerTabHelper?
  }
  public var elementPickerTabHelper: ElementPickerTabHelper? {
    get { self[ElementPickerTabHelperKey.self] }
    set { self[ElementPickerTabHelperKey.self] = newValue }
  }
}

@MainActor
public class ElementPickerTabHelper: TabObserver {

  private weak var tab: (any TabState)?

  /// Called when the picker's Manage Filters button is tapped.
  public var onManageCustomFilters: (() -> Void)?

  public init(tab: some TabState) {
    self.tab = tab
    tab.addObserver(self)
  }

  /// Shows the element picker in this tab. Calling this while the picker is
  /// already showing un-minimizes it instead of starting over.
  public func showPicker() {
    guard let tab else { return }
    BraveWebView.from(tab: tab)?.showElementPicker()
  }

  // MARK: - TabObserver

  public func tabDidCreateWebView(_ tab: some TabState) {
    BraveWebView.from(tab: tab)?.setElementPickerTabHelperBridge(self)
  }

  public func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }
}

extension ElementPickerTabHelper: @MainActor ElementPickerTabHelperBridge {

  public func addCosmeticFilter(_ selector: String, for url: URL) {
    guard let host = url.host else { return }
    Task {
      let storage = CustomFilterListStorage.shared
      do {
        let existing = try await storage.loadCustomRules() ?? ""
        let prefix = existing.isEmpty || existing.hasSuffix("\n") ? "" : "\n"
        try await storage.save(customRules: "\(existing)\(prefix)\(host)##\(selector)\n")
      } catch {
        Logger.module.error("Failed to save element picker filter: \(error.localizedDescription)")
      }
    }
    Logger.module.debug("Element picker requested adding custom filters")
  }

  public func manageCustomFilters() {
    onManageCustomFilters?()
  }

  public func elementPickerThemeInfo(
    completion: @escaping (Bool, Int32) -> Void
  ) {
    let traitCollection = tab?.view.traitCollection ?? .current
    let isDarkMode = traitCollection.userInterfaceStyle == .dark
    let backgroundColor = UIColor(braveSystemName: .containerBackground)
      .resolvedColor(with: traitCollection)
    completion(isDarkMode, backgroundColor.packedRGB)
  }

  public func elementPickerLocalizedTexts(
    completion: @escaping (ElementPickerLocalizedTexts?) -> Void
  ) {
    completion(
      ElementPickerLocalizedTexts(
        createDisabledText: Strings.Shields.elementPickerCreateDisabledTitle,
        createEnabledText: Strings.Shields.elementPickerCreateEnabledTitle,
        manageText: Strings.Shields.elementPickerManageTitle,
        showRulesText: Strings.Shields.elementPickerShowRulesTitle,
        hideRulesText: Strings.Shields.elementPickerHideRulesTitle,
        quitText: Strings.Shields.elementPickerQuitTitle
      )
    )
  }
}

extension UIColor {
  /// The colour as a `0xRRGGBB` integer, the format the element picker's
  /// JavaScript unpacks.
  fileprivate var packedRGB: Int32 {
    var red: CGFloat = 0
    var green: CGFloat = 0
    var blue: CGFloat = 0
    var alpha: CGFloat = 0
    getRed(&red, green: &green, blue: &blue, alpha: &alpha)
    let component = { (value: CGFloat) in Int32(round(max(0, min(1, value)) * 255)) }
    return component(red) << 16 | component(green) << 8 | component(blue)
  }
}
