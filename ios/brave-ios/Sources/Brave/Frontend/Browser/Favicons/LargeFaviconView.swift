// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Data
import Favicon
import Foundation
import UIKit

struct FaviconUX {
  static let faviconBorderColor = UIColor(white: 0, alpha: 0.2)
  static let faviconBorderWidth = 1.0 / UIScreen.main.scale
}

struct FaviconConfiguration {
  var borderColor: UIColor
  var borderWidth: CGFloat
  var cornerRadius: CGFloat = 8
  var displayBackground: Bool = true

  static let defaultConfig = FaviconConfiguration(
    borderColor: FaviconUX.faviconBorderColor,
    borderWidth: FaviconUX.faviconBorderWidth
  )
}

/// Displays a large favicon given some favorite
class LargeFaviconView: UIView {
  private let isBackgroundDisplayed: Bool
  init(
    config: FaviconConfiguration? = nil
  ) {
    isBackgroundDisplayed = config?.displayBackground ?? true
    super.init(frame: .zero)

    layer.cornerRadius = 8
    layer.cornerCurve = .continuous
    if let config {
      layer.borderColor = config.borderColor.cgColor
      layer.borderWidth = config.borderWidth
      layer.cornerRadius = config.cornerRadius
    }
    clipsToBounds = true
    layoutMargins = .zero

    addSubview(backgroundView)
    addSubview(monogramFallbackLabel)
    addSubview(imageView)

    backgroundView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    imageView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    monogramFallbackLabel.snp.makeConstraints {
      $0.center.equalTo(self)
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  func loadFavicon(
    siteURL: URL,
    isPrivateBrowsing: Bool,
    monogramFallbackCharacter: Character? = nil
  ) {
    faviconTask?.cancel()
    faviconTask = Task { @MainActor in
      if let favicon = await FaviconFetcher.getIconFromCache(for: siteURL) {
        try Task.checkCancellation()

        self.imageView.image = favicon.image ?? Favicon.defaultImage
        self.backgroundColor = favicon.backgroundColor
        self.imageView.contentMode = .scaleAspectFit

        if let image = favicon.image {
          self.backgroundView.isHidden =
            !isBackgroundDisplayed || !favicon.isMonogramImage && !image.hasTransparentEdges
        } else {
          self.backgroundView.isHidden =
            !isBackgroundDisplayed || !favicon.hasTransparentBackground && !favicon.isMonogramImage
        }
        return
      }

      let isPersistent = !isPrivateBrowsing
      do {
        let favicon = try await FaviconFetcher.loadIcon(
          url: siteURL,
          persistent: isPersistent
        )

        try Task.checkCancellation()

        self.imageView.image = favicon.image
        self.backgroundColor = favicon.backgroundColor
        self.imageView.contentMode = .scaleAspectFit

        if let image = favicon.image {
          self.backgroundView.isHidden =
            !isBackgroundDisplayed || !favicon.isMonogramImage && !image.hasTransparentEdges
        } else {
          self.backgroundView.isHidden =
            !isBackgroundDisplayed || !favicon.hasTransparentBackground && !favicon.isMonogramImage
        }
      } catch {
        self.imageView.image = Favicon.defaultImage
        self.backgroundColor = nil
        self.imageView.contentMode = .scaleAspectFit
        self.backgroundView.isHidden = !isBackgroundDisplayed
      }
    }
  }

  func cancelLoading() {
    faviconTask?.cancel()
    faviconTask = nil
    imageView.image = nil
    imageView.contentMode = .scaleAspectFit
    backgroundColor = .clear
    layoutMargins = .zero
    backgroundView.isHidden = !isBackgroundDisplayed
  }

  private var faviconTask: Task<Void, Error>?

  private let imageView = UIImageView().then {
    $0.contentMode = .scaleAspectFit
  }

  private let monogramFallbackLabel = UILabel().then {
    $0.textColor = .white
    $0.isHidden = true
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    if bounds.height > 0 {
      monogramFallbackLabel.font = .systemFont(ofSize: bounds.height / 2)
    }
  }

  private let backgroundView = UIVisualEffectView(effect: UIBlurEffect(style: .regular)).then {
    $0.isHidden = true
  }
}
