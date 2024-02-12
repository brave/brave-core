// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveShared
import Data
import UIKit
import Favicon

struct FaviconUX {
  static let faviconBorderColor = UIColor(white: 0, alpha: 0.2)
  static let faviconBorderWidth = 1.0 / UIScreen.main.scale
}

/// Displays a large favicon given some favorite
class LargeFaviconView: UIView {
  func loadFavicon(siteURL: URL, isPrivateBrowsing: Bool, monogramFallbackCharacter: Character? = nil) {
    faviconTask?.cancel()
    if let favicon = FaviconFetcher.getIconFromCache(for: siteURL) {
      faviconTask = nil
      
      self.imageView.image = favicon.image ?? Favicon.defaultImage
      self.backgroundColor = favicon.backgroundColor
      self.imageView.contentMode = .scaleAspectFit
      
      if let image = favicon.image {
        self.backgroundView.isHidden = !favicon.isMonogramImage && !image.hasTransparentEdges
      } else {
        self.backgroundView.isHidden = !favicon.hasTransparentBackground && !favicon.isMonogramImage
      }
      return
    }
    
    faviconTask = Task { @MainActor in
      let isPersistent = !isPrivateBrowsing
      do {
        let favicon = try await FaviconFetcher.loadIcon(url: siteURL,
                                                        kind: .largeIcon,
                                                        persistent: isPersistent)
        
        self.imageView.image = favicon.image
        self.backgroundColor = favicon.backgroundColor
        self.imageView.contentMode = .scaleAspectFit
        
        if let image = favicon.image {
          self.backgroundView.isHidden = !favicon.isMonogramImage && !image.hasTransparentEdges
        } else {
          self.backgroundView.isHidden = !favicon.hasTransparentBackground && !favicon.isMonogramImage
        }
      } catch {
        self.imageView.image = Favicon.defaultImage
        self.backgroundColor = nil
        self.imageView.contentMode = .scaleAspectFit
        self.backgroundView.isHidden = false
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
    backgroundView.isHidden = false
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

  override init(frame: CGRect) {
    super.init(frame: frame)

    layer.cornerRadius = 8
    layer.cornerCurve = .continuous

    clipsToBounds = true
    layer.borderColor = FaviconUX.faviconBorderColor.cgColor
    layer.borderWidth = FaviconUX.faviconBorderWidth

    layoutMargins = .zero

    addSubview(backgroundView)
    addSubview(monogramFallbackLabel)
    addSubview(imageView)

    backgroundView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    imageView.snp.makeConstraints {
      $0.center.equalTo(self)
      $0.leading.top.greaterThanOrEqualTo(layoutMarginsGuide)
      $0.trailing.bottom.lessThanOrEqualTo(layoutMarginsGuide)
    }
    monogramFallbackLabel.snp.makeConstraints {
      $0.center.equalTo(self)
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
