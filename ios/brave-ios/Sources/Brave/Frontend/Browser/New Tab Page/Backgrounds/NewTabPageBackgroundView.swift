// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AVKit
import DesignSystem
import SnapKit
import UIKit

/// Non-interactive contents that appear behind the New Tab Page contents
class NewTabPageBackgroundView: UIView {
  /// The image wallpaper if the user has background images enabled
  let imageView = UIImageView().then {
    $0.contentMode = .scaleAspectFill
    $0.clipsToBounds = false
  }

  let playerLayer = AVPlayerLayer()

  func updateImageXOffset(by x: CGFloat) {
    bounds = .init(x: -x, y: 0, width: bounds.width, height: bounds.height)
  }

  func setupPlayerLayer(_ backgroundVideoPath: URL) {
    backgroundColor = parseColorFromFilename(
      filename: backgroundVideoPath.lastPathComponent
    )

    let resizeToFill = shouldResizeToFill(filename: backgroundVideoPath.lastPathComponent)
    if resizeToFill && UIDevice.isPhone {
      playerLayer.videoGravity = .resizeAspectFill
    } else {
      playerLayer.videoGravity = .resizeAspect
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    clipsToBounds = true
    backgroundColor = .init {
      if $0.userInterfaceStyle == .dark {
        return .secondaryBraveBackground
      }
      // We use a special color here unfortunately when there is no background because
      // favorite cells have white text
      return .init(rgb: 0x3b3e4f)
    }

    layer.addSublayer(playerLayer)

    addSubview(imageView)
    imageView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  private func parseColorFromFilename(filename: String) -> UIColor {
    var color: String?
    if let range = filename.range(of: "\\.RGB[a-fA-F0-9]+\\.", options: .regularExpression) {
      color = filename[range].replacingOccurrences(of: ".RGB", with: "")
        .replacingOccurrences(of: ".", with: "")
    }

    guard let color = color,
      color.count == 6
    else {
      return UIColor.black
    }

    var rgbValue: UInt64 = 0
    Scanner(string: color).scanHexInt64(&rgbValue)

    return UIColor(
      red: CGFloat((rgbValue & 0xFF0000) >> 16) / 255.0,
      green: CGFloat((rgbValue & 0x00FF00) >> 8) / 255.0,
      blue: CGFloat(rgbValue & 0x0000FF) / 255.0,
      alpha: CGFloat(1.0)
    )
  }

  private func shouldResizeToFill(filename: String) -> Bool {
    return filename.range(of: "\\.RTF\\.", options: .regularExpression) != nil
  }
}
