// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Data
import Shared
import SnapKit
import UIKit

class SyncDeviceTypeButton: UIControl {

  var imageView: UIImageView = UIImageView()
  var label: UILabel = UILabel()
  var type: SyncDeviceType!

  // Color for the opposite state of `pressed`
  private var pressedReversedColor = UIColor.braveBlurpleTint
  var pressed: Bool = false {
    didSet {
      if pressed == oldValue {
        // Needed with usage of `pressedReversedColor`
        return
      }

      let newColor = pressedReversedColor
      pressedReversedColor = label.textColor
      label.textColor = newColor

      UIViewPropertyAnimator(duration: 0.3, dampingRatio: 0.8) { [pressed] in
        let scale = pressed ? 0.9 : 1.0
        self.transform = CGAffineTransform(scaleX: scale, y: scale)
      }
      .startAnimation()
    }
  }

  convenience init(image: String, title: String, type: SyncDeviceType) {
    self.init(frame: CGRect.zero)

    backgroundColor = .braveBackground
    clipsToBounds = false
    layer.cornerRadius = 12
    layer.cornerCurve = .continuous
    layer.shadowColor = UIColor(white: 0.2, alpha: 1.0).cgColor
    layer.shadowRadius = 3
    layer.shadowOpacity = 0.1
    layer.shadowOffset = CGSize(width: 0, height: 1)

    imageView.image = UIImage(named: image, in: .module, compatibleWith: nil)
    imageView.contentMode = .center
    imageView.tintColor = .braveLabel
    addSubview(imageView)

    label.text = title
    label.font = UIFont.systemFont(ofSize: 17.0, weight: UIFont.Weight.bold)
    label.textAlignment = .center
    label.textColor = .braveLabel
    addSubview(label)

    self.type = type

    imageView.snp.makeConstraints { (make) in
      make.centerX.equalTo(self)
      make.centerY.equalTo(self).offset(-20)
    }

    label.snp.makeConstraints { (make) in
      make.top.equalTo(imageView.snp.bottom).offset(20)
      make.centerX.equalTo(self)
      make.width.equalTo(self)
    }

    // Prevents bug where user can tap on two device types at once.
    isExclusiveTouch = true
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func beginTracking(_ touch: UITouch, with event: UIEvent?) -> Bool {
    pressed = true
    return true
  }

  override func endTracking(_ touch: UITouch?, with event: UIEvent?) {
    pressed = false
  }

  override func cancelTracking(with event: UIEvent?) {
    pressed = false
  }
}

class SyncSelectDeviceTypeViewController: SyncViewController {
  var syncInitHandler: ((String, SyncDeviceType) -> Void)?

  let loadingView = UIView()
  let chooseDeviceLabel = UILabel().then {
    $0.text = Strings.Sync.chooseDeviceHeader
    $0.textAlignment = .center
    $0.textColor = .braveLabel
    $0.numberOfLines = 0
    $0.font = UIFont.systemFont(ofSize: 15, weight: UIFont.Weight.regular)
  }

  let mainStackView = UIStackView().then {
    $0.axis = .vertical
    $0.distribution = .fill
    $0.spacing = 16
  }

  let mobileButton = SyncDeviceTypeButton(
    image: "sync-mobile",
    title: Strings.Sync.tabletOrMobileDevice,
    type: .mobile
  )
  let computerButton = SyncDeviceTypeButton(
    image: "sync-computer",
    title: Strings.Sync.computerDevice,
    type: .computer
  )

  override func viewDidLoad() {
    super.viewDidLoad()

    title = Strings.Sync.chooseDevice

    let chooseDeviceStackView = UIStackView(arrangedSubviews: [
      UIView.spacer(.horizontal, amount: 24),
      chooseDeviceLabel,
      UIView.spacer(.horizontal, amount: 24),
    ])

    chooseDeviceStackView.setContentCompressionResistancePriority(
      UILayoutPriority.defaultHigh,
      for: NSLayoutConstraint.Axis.vertical
    )

    let devicesStackView = UIStackView()
    devicesStackView.axis = .vertical
    devicesStackView.distribution = .fillEqually
    devicesStackView.spacing = 16

    mainStackView.addArrangedSubview(chooseDeviceStackView)
    mainStackView.addArrangedSubview(devicesStackView)
    view.addSubview(mainStackView)

    mainStackView.snp.makeConstraints { make in
      make.top.equalTo(self.view.safeArea.top).offset(16)
      make.left.right.equalTo(self.view).inset(16)
      make.bottom.equalTo(self.view.safeArea.bottom).inset(16)
    }

    devicesStackView.addArrangedSubview(mobileButton)
    devicesStackView.addArrangedSubview(computerButton)

    mobileButton.addTarget(self, action: #selector(addDevice), for: .touchUpInside)
    computerButton.addTarget(self, action: #selector(addDevice), for: .touchUpInside)

    // Loading View

    // This should be general, and abstracted

    let spinner = UIActivityIndicatorView(style: .large)
    spinner.startAnimating()
    loadingView.backgroundColor = UIColor(white: 0.5, alpha: 0.5)
    loadingView.isHidden = true
    loadingView.addSubview(spinner)
    view.addSubview(loadingView)

    spinner.snp.makeConstraints { (make) in
      make.center.equalTo(spinner.superview!)
    }

    loadingView.snp.makeConstraints { (make) in
      make.edges.equalTo(loadingView.superview!)
    }
  }

  @objc func addDevice(sender: SyncDeviceTypeButton) {
    doIfConnected {
      self.syncInitHandler?(sender.label.text ?? "", sender.type)
    }
  }
}

extension SyncSelectDeviceTypeViewController: NavigationPrevention {
  func enableNavigationPrevention() {
    navigationItem.hidesBackButton = true
    loadingView.isHidden = false
  }

  func disableNavigationPrevention() {
    navigationItem.hidesBackButton = false
    loadingView.isHidden = true
  }
}
