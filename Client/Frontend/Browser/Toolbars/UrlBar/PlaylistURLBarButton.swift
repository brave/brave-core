// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import DesignSystem
import SnapKit

class PlaylistURLBarButton: UIButton {
  enum State {
    case addToPlaylist
    case addedToPlaylist
    case none
  }

  var buttonState: State = .none {
    didSet {
      switch buttonState {
      case .addToPlaylist:
        setImage(UIImage(braveSystemNamed: "brave.text.badge.plus"), for: .normal)
        tintColor = .white
        gradientView.isHidden = false
        backgroundView.isHidden = true
      case .addedToPlaylist:
        setImage(UIImage(braveSystemNamed: "brave.text.badge.checkmark"), for: .normal)
        tintColor = .braveSuccessLabel
        gradientView.isHidden = true
        backgroundView.isHidden = false
      case .none:
        setImage(nil, for: .normal)
        gradientView.isHidden = true
        backgroundView.isHidden = true
      }
      updateForTraitCollection()
    }
  }

  private let gradientView = BraveGradientView(gradient: .lightGradient02)
  private let backgroundView = UIView().then {
    $0.backgroundColor = .braveSeparator
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    imageView?.contentMode = .center
    insertSubview(gradientView, at: 0)
    insertSubview(backgroundView, belowSubview: gradientView)

    [backgroundView, gradientView].forEach {
      $0.do {
        $0.layer.cornerCurve = .continuous
        $0.layer.masksToBounds = true
        $0.isUserInteractionEnabled = false

        $0.snp.makeConstraints {
          $0.edges.equalToSuperview()
        }
      }
    }
    
    updateForTraitCollection()
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    updateForTraitCollection()
  }
  
  override func layoutSubviews() {
    super.layoutSubviews()
    
    let radius = backgroundView.bounds.height / 2.0
    backgroundView.layer.cornerRadius = radius
    gradientView.layer.cornerRadius = radius
  }
  
  private func updateForTraitCollection() {
    let sizeCategory = traitCollection.toolbarButtonContentSizeCategory
    let pointSize = UIFont.preferredFont(
      forTextStyle: .body,
      compatibleWith: .init(preferredContentSizeCategory: sizeCategory)
    ).pointSize
    setPreferredSymbolConfiguration(
      .init(pointSize: pointSize, weight: .regular, scale: .medium),
      forImageIn: .normal
    )
  }
}
