// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import DesignSystem
import Shared

/// A big UISwitch that has a fancy animated gradient when its turned on
///
/// Treat it the same way you'd use a UISwitch (addTarget for .valueChanged)
class ShieldsSwitch: UIControl {

  /// The static size of this switch
  private static let size = CGSize(width: 100, height: 60)

  /// Whether or not the switch is currently toggled on or off
  var isOn: Bool {
    get { _isOn }
    set { setOn(newValue, animated: false) }
  }

  private var _isOn: Bool = false

  func setOn(_ on: Bool, animated: Bool) {
    _isOn = on
    if animated {
      if gradientView.isHidden {
        gradientView.isHidden = false
        gradientView.alpha = 0.0
      }
      let animator = UIViewPropertyAnimator(duration: 0.4, curve: .linear) {
        self.gradientView.alpha = on ? 1.0 : 0.0
      }
      animator.addCompletion { [weak self] _ in
        guard let self = self else { return }
        if on {
          self.beginGradientAnimations()
        } else {
          self.gradientView.isHidden = true
          self.gradientView.alpha = 1.0
          self.endGradientAnimations()
        }
      }
      animator.startAnimation()
      animateThumbViewFrameUpdate()
    } else {
      thumbView.frame = self.thumbViewFrame
      gradientView.isHidden = !on
      if on {
        beginGradientAnimations()
      } else {
        endGradientAnimations()
      }
    }
  }

  /// The color of the background when the switch is off
  var offBackgroundColor = UIColor(white: 0.9, alpha: 1.0) {
    didSet {
      backgroundView.backgroundColor = offBackgroundColor
    }
  }

  private let gradientView = GradientView().then {
    $0.isUserInteractionEnabled = false
    $0.gradientLayer.type = .radial
    $0.gradientLayer.locations = [0, 1]
    $0.gradientLayer.startPoint = CGPoint(x: 1, y: 1)
    $0.gradientLayer.endPoint = .zero
    $0.gradientLayer.shadowOffset = .zero
    $0.gradientLayer.shadowRadius = 3
    $0.gradientLayer.shadowOpacity = 0.8
    $0.gradientLayer.borderColor = UIColor(white: 0.0, alpha: 0.2).cgColor
    $0.gradientLayer.borderWidth = 1.0 / UIScreen.main.scale
  }

  private let backgroundView = UIView().then {
    $0.backgroundColor = UIColor(white: 0.9, alpha: 1.0)
    $0.isUserInteractionEnabled = false
  }

  private let thumbView = UIView().then {
    $0.backgroundColor = .white
    $0.layer.shadowColor = UIColor.black.cgColor
    $0.layer.shadowOffset = CGSize(width: 0, height: 1)
    $0.layer.shadowRadius = 3
    $0.layer.shadowOpacity = 0.3
    $0.isUserInteractionEnabled = false
  }

  override init(frame: CGRect) {
    super.init(frame: CGRect(size: Self.size))

    isAccessibilityElement = true
    accessibilityTraits = [.button]
    accessibilityHint = Strings.Shields.toggleHint

    addSubview(backgroundView)
    addSubview(gradientView)
    addSubview(thumbView)

    gradientView.isHidden = true

    backgroundView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }
    gradientView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }

    if let step = steps.first {
      gradientView.gradientLayer.colors = step.gradientColors
      gradientView.gradientLayer.shadowColor = step.shadowColor
    }
  }

  override var accessibilityLabel: String? {
    get {
      "\(Strings.Shields.statusTitle): \(isOn ? Strings.Shields.statusValueUp : Strings.Shields.statusValueDown)"
    }
    set { assertionFailure() }  // swiftlint:disable:this unused_setter_value
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func action(for layer: CALayer, forKey event: String) -> CAAction? {
    if event == kCAOnOrderIn && isOn {
      // Resume animation
      beginGradientAnimations()
    }
    return super.action(for: layer, forKey: event)
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    gradientView.layer.shadowPath = UIBezierPath(roundedRect: bounds, cornerRadius: bounds.height / 2.0).cgPath
    backgroundView.layer.cornerRadius = bounds.size.height / 2.0
    gradientView.layer.cornerRadius = bounds.size.height / 2.0
    thumbView.layer.cornerRadius = thumbView.bounds.size.height / 2.0
  }

  override func accessibilityActivate() -> Bool {
    setOn(!isOn, animated: true)
    sendActions(for: .valueChanged)
    return true
  }

  private var thumbViewFrame: CGRect {
    let inset: CGFloat = 3
    let expandedWidthDelta: CGFloat = 10

    var frame: CGRect = .zero
    frame.origin.y = inset
    frame.size.height = bounds.height - (inset * 2)

    if isHighlighted {
      // Expand width
      frame.size.width = frame.size.height + expandedWidthDelta
    } else {
      frame.size.width = frame.size.height
    }
    if isOn {
      frame.origin.x = bounds.width - inset - frame.width
    } else {
      frame.origin.x = inset
    }
    return frame
  }

  private func animateThumbViewFrameUpdate() {
    let nextFrame = thumbViewFrame
    UIViewPropertyAnimator(duration: 0.3, dampingRatio: 0.85) {
      self.thumbView.frame = nextFrame
    }
    .startAnimation()
  }

  override var isHighlighted: Bool {
    didSet {
      animateThumbViewFrameUpdate()
    }
  }

  override var intrinsicContentSize: CGSize {
    return Self.size
  }

  // MARK: - Animation

  private struct AnimationStep {
    var gradientColors: [CGColor]
    var shadowColor: CGColor
    init(colors: [Int], shadow: Int) {
      gradientColors = colors.map { UIColor(rgb: $0).cgColor }
      shadowColor = UIColor(rgb: shadow).cgColor
    }
  }

  private let steps: [AnimationStep] = [
    .init(colors: [0xFFA73B, 0xFF7654], shadow: 0xFF7654),
    .init(colors: [0xFF7654, 0xFB542B], shadow: 0xFB542B),
    .init(colors: [0xFB542B, 0xF7241C], shadow: 0xF7241C),
    .init(colors: [0xF7241C, 0xFC4F82], shadow: 0xFC4F82),
    .init(colors: [0xFC4F82, 0xFFA73B], shadow: 0xFFA73B),
    .init(colors: [0xFFA73B, 0xFF7654], shadow: 0xFF7654),
  ]

  private func beginGradientAnimations() {
    let colorsKeyframe = CAKeyframeAnimation(keyPath: "colors")
    colorsKeyframe.calculationMode = .paced
    colorsKeyframe.values = steps.map { $0.gradientColors }

    let shadowKeyframe = CAKeyframeAnimation(keyPath: "shadowColor")
    shadowKeyframe.calculationMode = .paced
    shadowKeyframe.values = steps.map { $0.shadowColor }

    let group = CAAnimationGroup()
    group.animations = [colorsKeyframe, shadowKeyframe]
    group.duration = 0.75 * Double(steps.count)
    group.repeatCount = Float.infinity
    group.beginTime = CACurrentMediaTime()

    gradientView.gradientLayer.add(group, forKey: "animateBg")
  }

  private func endGradientAnimations() {
    gradientView.gradientLayer.removeAllAnimations()
    if let step = steps.first {
      gradientView.gradientLayer.colors = step.gradientColors
      gradientView.gradientLayer.shadowColor = step.shadowColor
    }
  }

  override func endTracking(_ touch: UITouch?, with event: UIEvent?) {
    if isTouchInside {
      setOn(!isOn, animated: true)
      sendActions(for: .valueChanged)
    }
    super.endTracking(touch, with: event)
  }
}
