// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AVKit
import DesignSystem
import SnapKit
import UIKit
import WebKit

/// Non-interactive contents that appear behind the New Tab Page contents
class NewTabPageBackgroundView: UIView, WKNavigationDelegate {
  /// The image wallpaper if the user has background images enabled
  let imageView = UIImageView().then {
    $0.contentMode = .scaleAspectFill
    $0.clipsToBounds = false
  }

  let playerLayer = AVPlayerLayer()
  
  let webView: WKWebView
//  let webViewLayer = CALayer()

  func updateImageXOffset(by x: CGFloat) {
    bounds = .init(x: -x, y: 0, width: bounds.width, height: bounds.height)
  }
  
  func setupWebViewLayer(_ htmlPath: URL) {
//    webViewLayer.contents = webView.layer
//
//    layer.addSublayer(webViewLayer)

    webView.load(URLRequest(url: htmlPath))
    webView.isUserInteractionEnabled = true
    webView.navigationDelegate = self

    addSubview(webView)
    webView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

//    addSubview(webView)
//    webView.snp.makeConstraints {
//      $0.edges.equalToSuperview()
//    }
  }

  func webView(_ webView: WKWebView, decidePolicyFor navigationAction: WKNavigationAction, decisionHandler: @escaping (WKNavigationActionPolicy) -> Void) {
      if let url = navigationAction.request.url {
        if url == URL(string: "brave://CTA") {
          onTapToCTA()
        }
      }

      decisionHandler(.cancel)
  }
  
  func onTapToCTA() {
  }

  func setupPlayerLayer(_ backgroundVideoPath: URL, player: AVPlayer?) {
    playerLayer.player = player
    layer.addSublayer(playerLayer)

    backgroundColor = parseBackgroundColorFromFilename(
      filename: backgroundVideoPath.lastPathComponent
    )

    if shouldResizePlayerToFill(filename: backgroundVideoPath.lastPathComponent) {
      playerLayer.videoGravity = .resizeAspectFill
    } else {
      playerLayer.videoGravity = .resizeAspect
    }
  }

  func resetPlayerLayer() {
    playerLayer.player = nil
    playerLayer.removeFromSuperlayer()
  }

  override init(frame: CGRect) {
    webView = WKWebView(frame: frame)
    super.init(frame: frame)
    
    //webViewLayer.l

    clipsToBounds = true
    backgroundColor = .init {
      if $0.userInterfaceStyle == .dark {
        return .secondaryBraveBackground
      }
      // We use a special color here unfortunately when there is no background because
      // favorite cells have white text
      return .init(rgb: 0x3b3e4f)
    }

    addSubview(imageView)
    imageView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  private func parseBackgroundColorFromFilename(filename: String) -> UIColor {
    var colorHex: String?
    if let range = filename.range(of: "\\.RGB[a-fA-F0-9]+\\.", options: .regularExpression) {
      colorHex = filename[range].replacingOccurrences(of: ".RGB", with: "")
        .replacingOccurrences(of: ".", with: "")
    }

    guard let colorHex, colorHex.count == 6 else {
      return UIColor.black
    }

    return UIColor(colorString: colorHex)
  }

  private func shouldResizePlayerToFill(filename: String) -> Bool {
    return filename.contains(".RTF.")
  }
}
