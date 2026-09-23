// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import UIKit

private let iPadFactor: CGFloat = 1.06
private let iPhoneFactor: CGFloat = 0.88

public class DynamicFontHelper: NSObject {

  public static var defaultHelper: DynamicFontHelper {
    struct Singleton {
      static let instance = DynamicFontHelper()
    }
    return Singleton.instance
  }

  override init() {
    defaultStandardFontSize =
      UIFontDescriptor.preferredFontDescriptor(withTextStyle: .body).pointSize
    deviceFontSize =
      defaultStandardFontSize
      * (UIDevice.current.userInterfaceIdiom == .pad ? iPadFactor : iPhoneFactor)
    defaultMediumFontSize =
      UIFontDescriptor.preferredFontDescriptor(withTextStyle: .footnote).pointSize
    defaultSmallFontSize =
      UIFontDescriptor.preferredFontDescriptor(withTextStyle: .caption2).pointSize

    super.init()
  }

  /// Starts monitoring the ContentSizeCategory chantes
  public func startObserving() {
    NotificationCenter.default.addObserver(
      self,
      selector: #selector(contentSizeCategoryDidChange),
      name: UIContentSizeCategory.didChangeNotification,
      object: nil
    )
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
  }

  /// Device specific
  fileprivate var deviceFontSize: CGFloat

  var deviceFont: UIFont {
    return UIFont.systemFont(ofSize: deviceFontSize, weight: UIFont.Weight.medium)
  }
  var deviceFontLight: UIFont {
    return UIFont.systemFont(ofSize: deviceFontSize, weight: UIFont.Weight.light)
  }
  var deviceFontSmall: UIFont {
    return UIFont.systemFont(ofSize: deviceFontSize - 1, weight: UIFont.Weight.medium)
  }
  var deviceFontSmallLight: UIFont {
    return UIFont.systemFont(ofSize: deviceFontSize - 1, weight: UIFont.Weight.light)
  }
  var deviceFontSmallHistoryPanel: UIFont {
    return UIFont.systemFont(ofSize: deviceFontSize - 3, weight: UIFont.Weight.light)
  }
  var deviceFontHistoryPanel: UIFont {
    return UIFont.systemFont(ofSize: deviceFontSize)
  }
  var deviceFontSmallBold: UIFont {
    return UIFont.boldSystemFont(ofSize: deviceFontSize - 1)
  }
  var deviceFontLarge: UIFont {
    return UIFont.systemFont(ofSize: deviceFontSize + 3)
  }
  var deviceFontMedium: UIFont {
    return UIFont.systemFont(ofSize: deviceFontSize + 1)
  }
  var deviceFontLargeBold: UIFont {
    return UIFont.boldSystemFont(ofSize: deviceFontSize + 2)
  }
  var deviceFontMediumBold: UIFont {
    return UIFont.boldSystemFont(ofSize: deviceFontSize + 1)
  }
  var deviceFontExtraLargeBold: UIFont {
    return UIFont.boldSystemFont(ofSize: deviceFontSize + 4)
  }

  var mediumSizeRegularWeightAS: UIFont {
    let size = min(deviceFontSize, 18)
    return UIFont.systemFont(ofSize: size)
  }

  var largeSizeRegularWeightAS: UIFont {
    let size = min(deviceFontSize + 2, 20)
    return UIFont.systemFont(ofSize: size)
  }

  var mediumSizeHeavyWeightAS: UIFont {
    let size = min(deviceFontSize + 2, 18)
    return UIFont.systemFont(ofSize: size, weight: UIFont.Weight.heavy)
  }
  var smallSizeMediumWeightAS: UIFont {
    let size = min(defaultSmallFontSize, 14)
    return UIFont.systemFont(ofSize: size, weight: UIFont.Weight.medium)
  }

  var smallSizeBoldWeightAS: UIFont {
    let size = min(defaultSmallFontSize, 14)
    return UIFont.systemFont(ofSize: size, weight: UIFont.Weight.bold)
  }

  var mediumSizeBoldFontAS: UIFont {
    let size = min(deviceFontSize, 18)
    return UIFont.boldSystemFont(ofSize: size)
  }

  var smallSizeRegularWeightAS: UIFont {
    let size = min(defaultSmallFontSize, 14)
    return UIFont.systemFont(ofSize: size)
  }

  /// Small
  fileprivate var defaultSmallFontSize: CGFloat

  var defaultSmallFont: UIFont {
    return UIFont.systemFont(ofSize: defaultSmallFontSize, weight: UIFont.Weight.regular)
  }
  var defaultSmallFontBold: UIFont {
    return UIFont.boldSystemFont(ofSize: defaultSmallFontSize)
  }

  /// Medium
  fileprivate var defaultMediumFontSize: CGFloat

  var defaultMediumFont: UIFont {
    return UIFont.systemFont(ofSize: defaultMediumFontSize, weight: UIFont.Weight.regular)
  }
  var defaultMediumBoldFont: UIFont {
    return UIFont.boldSystemFont(ofSize: defaultMediumFontSize)
  }

  /// Standard
  fileprivate var defaultStandardFontSize: CGFloat

  var defaultStandardFont: UIFont {
    return UIFont.systemFont(ofSize: defaultStandardFontSize, weight: UIFont.Weight.regular)
  }
  var defaultStandardFontBold: UIFont {
    return UIFont.boldSystemFont(ofSize: defaultStandardFontSize)
  }

  /// Reader mode
  var readerStandardFontSize: CGFloat {
    return defaultStandardFontSize - 2
  }
  var readerBigFontSize: CGFloat {
    return defaultStandardFontSize + 5
  }

  /// Intro mode
  var introStandardFontSize: CGFloat {
    return min(defaultStandardFontSize - 1, 16)
  }
  var introBigFontSize: CGFloat {
    return min(defaultStandardFontSize + 1, 18)
  }

  func refreshFonts() {
    defaultStandardFontSize =
      UIFontDescriptor.preferredFontDescriptor(withTextStyle: .body).pointSize
    deviceFontSize =
      defaultStandardFontSize
      * (UIDevice.current.userInterfaceIdiom == .pad ? iPadFactor : iPhoneFactor)
    defaultMediumFontSize =
      UIFontDescriptor.preferredFontDescriptor(withTextStyle: .footnote).pointSize
    defaultSmallFontSize =
      UIFontDescriptor.preferredFontDescriptor(withTextStyle: .caption2).pointSize
  }

  @objc func contentSizeCategoryDidChange(_ notification: Notification) {
    refreshFonts()
    let notification = Notification(name: .dynamicFontChanged, object: nil)
    NotificationCenter.default.post(notification)
  }
}
