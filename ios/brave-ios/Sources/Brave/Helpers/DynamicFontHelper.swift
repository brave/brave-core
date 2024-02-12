/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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
    defaultStandardFontSize = UIFontDescriptor.preferredFontDescriptor(withTextStyle: .body).pointSize  // 14pt -> 17pt -> 23pt
    deviceFontSize = defaultStandardFontSize * (UIDevice.current.userInterfaceIdiom == .pad ? iPadFactor : iPhoneFactor)
    defaultMediumFontSize = UIFontDescriptor.preferredFontDescriptor(withTextStyle: .footnote).pointSize  // 12pt -> 13pt -> 19pt
    defaultSmallFontSize = UIFontDescriptor.preferredFontDescriptor(withTextStyle: .caption2).pointSize  // 11pt -> 11pt -> 17pt

    super.init()
  }

  /**
     * Starts monitoring the ContentSizeCategory chantes
     */
  public func startObserving() {
    NotificationCenter.default.addObserver(self, selector: #selector(contentSizeCategoryDidChange), name: UIContentSizeCategory.didChangeNotification, object: nil)
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
  }

  /**
     * Device specific
     */
  fileprivate var deviceFontSize: CGFloat

  var DeviceFont: UIFont {
    return UIFont.systemFont(ofSize: deviceFontSize, weight: UIFont.Weight.medium)
  }
  var DeviceFontLight: UIFont {
    return UIFont.systemFont(ofSize: deviceFontSize, weight: UIFont.Weight.light)
  }
  var DeviceFontSmall: UIFont {
    return UIFont.systemFont(ofSize: deviceFontSize - 1, weight: UIFont.Weight.medium)
  }
  var DeviceFontSmallLight: UIFont {
    return UIFont.systemFont(ofSize: deviceFontSize - 1, weight: UIFont.Weight.light)
  }
  var DeviceFontSmallHistoryPanel: UIFont {
    return UIFont.systemFont(ofSize: deviceFontSize - 3, weight: UIFont.Weight.light)
  }
  var DeviceFontHistoryPanel: UIFont {
    return UIFont.systemFont(ofSize: deviceFontSize)
  }
  var DeviceFontSmallBold: UIFont {
    return UIFont.boldSystemFont(ofSize: deviceFontSize - 1)
  }
  var DeviceFontLarge: UIFont {
    return UIFont.systemFont(ofSize: deviceFontSize + 3)
  }
  var DeviceFontMedium: UIFont {
    return UIFont.systemFont(ofSize: deviceFontSize + 1)
  }
  var DeviceFontLargeBold: UIFont {
    return UIFont.boldSystemFont(ofSize: deviceFontSize + 2)
  }
  var DeviceFontMediumBold: UIFont {
    return UIFont.boldSystemFont(ofSize: deviceFontSize + 1)
  }
  var DeviceFontExtraLargeBold: UIFont {
    return UIFont.boldSystemFont(ofSize: deviceFontSize + 4)
  }

  /*
     Activity Stream supports dynamic fonts up to a certain point. Large fonts dont work.
     Max out the supported font size.
     Small = 14, medium = 18, larger = 20
     */

  var MediumSizeRegularWeightAS: UIFont {
    let size = min(deviceFontSize, 18)
    return UIFont.systemFont(ofSize: size)
  }

  var LargeSizeRegularWeightAS: UIFont {
    let size = min(deviceFontSize + 2, 20)
    return UIFont.systemFont(ofSize: size)
  }

  var MediumSizeHeavyWeightAS: UIFont {
    let size = min(deviceFontSize + 2, 18)
    return UIFont.systemFont(ofSize: size, weight: UIFont.Weight.heavy)
  }
  var SmallSizeMediumWeightAS: UIFont {
    let size = min(defaultSmallFontSize, 14)
    return UIFont.systemFont(ofSize: size, weight: UIFont.Weight.medium)
  }
  
  var SmallSizeBoldWeightAS: UIFont {
    let size = min(defaultSmallFontSize, 14)
    return UIFont.systemFont(ofSize: size, weight: UIFont.Weight.bold)
  }

  var MediumSizeBoldFontAS: UIFont {
    let size = min(deviceFontSize, 18)
    return UIFont.boldSystemFont(ofSize: size)
  }

  var SmallSizeRegularWeightAS: UIFont {
    let size = min(defaultSmallFontSize, 14)
    return UIFont.systemFont(ofSize: size)
  }

  /**
     * Small
     */
  fileprivate var defaultSmallFontSize: CGFloat

  var DefaultSmallFont: UIFont {
    return UIFont.systemFont(ofSize: defaultSmallFontSize, weight: UIFont.Weight.regular)
  }
  var DefaultSmallFontBold: UIFont {
    return UIFont.boldSystemFont(ofSize: defaultSmallFontSize)
  }

  /**
     * Medium
     */
  fileprivate var defaultMediumFontSize: CGFloat

  var DefaultMediumFont: UIFont {
    return UIFont.systemFont(ofSize: defaultMediumFontSize, weight: UIFont.Weight.regular)
  }
  var DefaultMediumBoldFont: UIFont {
    return UIFont.boldSystemFont(ofSize: defaultMediumFontSize)
  }

  /**
     * Standard
     */
  fileprivate var defaultStandardFontSize: CGFloat

  var DefaultStandardFont: UIFont {
    return UIFont.systemFont(ofSize: defaultStandardFontSize, weight: UIFont.Weight.regular)
  }
  var DefaultStandardFontBold: UIFont {
    return UIFont.boldSystemFont(ofSize: defaultStandardFontSize)
  }

  /**
     * Reader mode
     */
  var ReaderStandardFontSize: CGFloat {
    return defaultStandardFontSize - 2
  }
  var ReaderBigFontSize: CGFloat {
    return defaultStandardFontSize + 5
  }

  /**
     * Intro mode
     */
  var IntroStandardFontSize: CGFloat {
    return min(defaultStandardFontSize - 1, 16)
  }
  var IntroBigFontSize: CGFloat {
    return min(defaultStandardFontSize + 1, 18)
  }

  func refreshFonts() {
    defaultStandardFontSize = UIFontDescriptor.preferredFontDescriptor(withTextStyle: .body).pointSize
    deviceFontSize = defaultStandardFontSize * (UIDevice.current.userInterfaceIdiom == .pad ? iPadFactor : iPhoneFactor)
    defaultMediumFontSize = UIFontDescriptor.preferredFontDescriptor(withTextStyle: .footnote).pointSize
    defaultSmallFontSize = UIFontDescriptor.preferredFontDescriptor(withTextStyle: .caption2).pointSize
  }

  @objc func contentSizeCategoryDidChange(_ notification: Notification) {
    refreshFonts()
    let notification = Notification(name: .dynamicFontChanged, object: nil)
    NotificationCenter.default.post(notification)
  }
}
