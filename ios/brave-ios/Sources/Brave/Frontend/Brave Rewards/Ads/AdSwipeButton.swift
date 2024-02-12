// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

class AdSwipeButton: UIControl {

  enum ContentType {
    case text(String, textColor: UIColor)
    case image(UIImage)
  }

  private let highlightView = UIView().then {
    $0.backgroundColor = UIColor(white: 1.0, alpha: 0.3)
    $0.alpha = 0.0
    $0.isUserInteractionEnabled = false
  }

  private let clippedView = UIView().then {
    $0.isUserInteractionEnabled = false
  }

  init(contentType: ContentType) {
    super.init(frame: .zero)

    addSubview(clippedView)
    clippedView.addSubview(highlightView)
    highlightView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }

    switch contentType {
    case .image(let image):
      let imageView = UIImageView(image: image)
      imageView.contentMode = .center
      clippedView.addSubview(imageView)
      imageView.snp.makeConstraints {
        $0.center.equalTo(self)
      }
    case .text(let text, let textColor):
      let label = UILabel()
      label.text = text
      label.textColor = textColor
      label.textAlignment = .center
      label.font = .systemFont(ofSize: 14.0, weight: .semibold)
      clippedView.addSubview(label)
      label.snp.makeConstraints {
        $0.center.equalTo(self)
      }
    }

    layer.borderColor = UIColor.black.withAlphaComponent(0.15).cgColor
    layer.borderWidth = 1.0 / UIScreen.main.scale
    layer.cornerRadius = 10
    layer.cornerCurve = .continuous
    layer.shadowColor = UIColor.black.cgColor
    layer.shadowOpacity = 0.25
    layer.shadowOffset = CGSize(width: 0, height: 1)
    layer.shadowRadius = 2

    clippedView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }
    clippedView.layer.cornerRadius = layer.cornerRadius
    clippedView.layer.cornerCurve = layer.cornerCurve
    clippedView.clipsToBounds = true
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override var isHighlighted: Bool {
    didSet {
      UIViewPropertyAnimator(duration: 0.1, curve: .linear) { [self] in
        highlightView.alpha = isHighlighted ? 1.0 : 0.0
      }
      .startAnimation()
    }
  }
}
