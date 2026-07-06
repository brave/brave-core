// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI
import UIKit

class DisplayURLLabel: UILabel {
  let pathPadding: CGFloat = 5.0
  var isCentered: Bool = false

  let clippingFade = GradientView(
    colors: [
      UIColor(braveSystemName: .containerBackground),
      UIColor(braveSystemName: .containerBackground).withAlphaComponent(0.0),
    ],
    positions: [0, 1],
    startPoint: .init(x: 0, y: 0.5),
    endPoint: .init(x: 1, y: 0.5)
  )

  override init(frame: CGRect) {
    super.init(frame: frame)
    addSubview(clippingFade)
  }

  private var textSize: CGSize = .zero
  var isLeftToRight: Bool = true {
    didSet {
      updateText()
      updateTextSize()
      updateClippingDirection()
      setNeedsLayout()
      setNeedsDisplay()
    }
  }

  override var font: UIFont! {
    didSet {
      updateText()
      updateTextSize()
    }
  }

  override var text: String? {
    didSet {
      clippingFade.isHidden = true
      if oldValue != text {
        updateText()
        updateTextSize()
        updateClippingDirection()
      }
      setNeedsDisplay()
    }
  }

  private func updateTextSize() {
    textSize = attributedText?.size() ?? .zero
    setNeedsLayout()
    setNeedsDisplay()
  }

  private func updateClippingDirection() {
    // Update clipping fade direction
    clippingFade.gradientLayer.startPoint = .init(x: isLeftToRight ? 1 : 0, y: 0.5)
    clippingFade.gradientLayer.endPoint = .init(x: isLeftToRight ? 0 : 1, y: 0.5)
  }

  private func updateText() {
    if let text = text {
      // Without attributed string, the label will always render RTL characters even if you force LTR layout.
      // This can introduce a security flaw! We must not flip the URL around based on RTL characters (Safari does not).
      let paragraphStyle = NSMutableParagraphStyle()
      paragraphStyle.lineBreakMode = .byClipping
      paragraphStyle.baseWritingDirection = .leftToRight
      paragraphStyle.alignment = isCentered ? .center : .natural

      self.attributedText = NSAttributedString(
        string: text,
        attributes: [
          .font: font ?? .preferredFont(forTextStyle: .body),
          .foregroundColor: textColor ?? UIColor(braveSystemName: .textPrimary),
          .paragraphStyle: paragraphStyle,
        ]
      )
    } else {
      self.attributedText = nil
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override var accessibilityTraits: UIAccessibilityTraits {
    get { [.staticText, .button] }
    set {}
  }

  override var canBecomeFirstResponder: Bool {
    return false
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    clippingFade.frame = .init(
      x: isLeftToRight ? bounds.width - 20 : 0,
      y: 0,
      width: 20,
      height: bounds.height
    )
  }

  // This override is done in case the eTLD+1 string overflows the width of textField.
  // In that case the textRect is adjusted to show right aligned and truncate left.
  // Since this textField changes with WebView domain change, performance implications are low.
  override func drawText(in rect: CGRect) {
    var rect = rect
    if textSize.width > bounds.width {
      let delta = (textSize.width - bounds.width)
      if !isLeftToRight {
        rect.origin.x -= delta
        rect.size.width += delta
      }
      bringSubviewToFront(clippingFade)
      clippingFade.isHidden = false
    } else {
      clippingFade.isHidden = true
    }
    super.drawText(in: rect)
  }
}

struct URLDisplayLabel: UIViewRepresentable {
  let formattedURL: String
  let isLeftToRight: Bool
  let textFont: UIFont
  let textColor: UIColor
  let gradientColors: [CGColor]

  func makeUIView(context: Context) -> DisplayURLLabel {
    let urlLabel = DisplayURLLabel(frame: .zero)
    urlLabel.isCentered = true
    urlLabel.font = textFont
    urlLabel.textColor = textColor
    urlLabel.clipsToBounds = true
    urlLabel.numberOfLines = 1
    urlLabel.clippingFade.gradientLayer.colors = gradientColors
    urlLabel.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
    return urlLabel
  }

  func updateUIView(_ uiView: DisplayURLLabel, context: Context) {
    uiView.text = formattedURL
    uiView.isLeftToRight = isLeftToRight
    uiView.textColor = textColor
    uiView.font = textFont
    uiView.clippingFade.gradientLayer.colors = gradientColors
  }
}
