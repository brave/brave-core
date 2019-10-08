// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveRewards
import pop

/// The main ads view. Mimics a system notification in that it shows an icon, "app name" (always will be "Brave Rewards"), title and body.
class AdContentButton: UIControl {
  let titleLabel = UILabel().then {
    $0.textColor = .black
    $0.font = .systemFont(ofSize: 15.0, weight: .semibold)
    $0.numberOfLines = 2
  }
  let bodyLabel = UILabel().then {
    $0.textColor = UIColor(white: 0.0, alpha: 0.5)
    $0.font = .systemFont(ofSize: 15.0)
    $0.numberOfLines = 3
  }
  private let appNameLabel = UILabel().then {
    $0.textColor = .black
    $0.font = .systemFont(ofSize: 14.0, weight: .medium)
    $0.text = Strings.AdNotificationTitle
  }
  
  private let backgroundView: UIVisualEffectView = {
    let backgroundView: UIVisualEffectView
    if #available(iOS 13.0, *) {
      backgroundView = UIVisualEffectView(effect: UIBlurEffect(style: .systemThinMaterial))
      } else {
      backgroundView = UIVisualEffectView(effect: UIBlurEffect(style: .extraLight))
    }
    backgroundView.isUserInteractionEnabled = false
    backgroundView.contentView.backgroundColor = UIColor.white.withAlphaComponent(0.7)
    backgroundView.layer.cornerRadius = 10
    backgroundView.layer.masksToBounds = true
    return backgroundView
  }()
  
  private var isDarkMode: Bool {
    return traitCollection.userInterfaceStyle == .dark
  }
  
  override public init(frame: CGRect) {
    super.init(frame: frame)
    
    let headerStackView = UIStackView().then {
      $0.spacing = 8.0
    }
    
    let stackView = UIStackView().then {
      $0.axis = .vertical
      $0.spacing = 2.0
      $0.alignment = .leading
      $0.isUserInteractionEnabled = false
    }
    
    addSubview(backgroundView)
    addSubview(stackView)
    stackView.addArrangedSubview(headerStackView)
    stackView.setCustomSpacing(8, after: headerStackView)
    stackView.addArrangedSubview(titleLabel)
    stackView.addArrangedSubview(bodyLabel)
   
    let iconImageView = UIImageView(image: UIImage(frameworkResourceNamed: "bat-small"))
    
    headerStackView.addArrangedSubview(iconImageView)
    headerStackView.addArrangedSubview(appNameLabel)
    
    backgroundView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }
    
    stackView.snp.makeConstraints {
      $0.edges.equalTo(self).inset(UIEdgeInsets(top: 8, left: 12, bottom: 8, right: 12))
    }
    
    backgroundColor = .clear
    
    layer.borderColor = UIColor.black.withAlphaComponent(0.15).cgColor
    layer.borderWidth = 1.0 / UIScreen.main.scale
    layer.cornerRadius = 10
    layer.shadowColor = UIColor.black.cgColor
    layer.shadowOpacity = 0.25
    layer.shadowOffset = CGSize(width: 0, height: 1)
    layer.shadowRadius = 2
    
    applyTheme()
  }
  
  public override func layoutSubviews() {
    super.layoutSubviews()
    
    layer.shadowPath = UIBezierPath(roundedRect: bounds, cornerRadius: layer.cornerRadius).cgPath
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  public override var isHighlighted: Bool {
    didSet {
      let scale: CGFloat = self.isHighlighted ? 1.025 : 1
      self.layer.springAnimate(property: kPOPLayerScaleXY, key: "isHighlighted") { animation, _ in
        animation.springSpeed = 16
        animation.springBounciness = 16
        animation.toValue = CGSize(width: scale, height: scale)
      }
    }
  }
  
  func applyTheme() {
    appNameLabel.textColor = isDarkMode ? .white : .black
    titleLabel.textColor = isDarkMode ? .white : .black
    bodyLabel.textColor = UIColor(white: isDarkMode ? 1.0 : 0.0, alpha: 0.5)
    backgroundView.contentView.backgroundColor = isDarkMode ? UIColor.black.withAlphaComponent(0.3) : UIColor.white.withAlphaComponent(0.7)
  }
  
  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    applyTheme()
  }
}
