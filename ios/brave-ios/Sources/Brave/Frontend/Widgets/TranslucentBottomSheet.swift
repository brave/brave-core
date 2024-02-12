// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared

/// A view controller that provides translucent background overlay for its contents and a close button in top right corner.
class TranslucentBottomSheet: UIViewController {
  private let animationDuration: TimeInterval = 0.25

  var closeHandler: (() -> Void)?

  let closeButton = UIButton().then {
    $0.setImage(UIImage(named: "close_translucent_popup", in: .module, compatibleWith: nil)!.template, for: .normal)
    $0.tintColor = .white
  }

  init() {
    super.init(nibName: nil, bundle: nil)
  }

  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) { fatalError() }

  override func viewDidLoad() {
    let blurEffectView = UIVisualEffectView(effect: UIBlurEffect(style: UIBlurEffect.Style.dark)).then {
      $0.frame = view.bounds
      $0.autoresizingMask = [.flexibleWidth, .flexibleHeight]
    }

    view.addSubview(blurEffectView)

    view.addSubview(closeButton)
    closeButton.addTarget(self, action: #selector(closeView), for: .touchUpInside)

    view.alpha = CGFloat.leastNormalMagnitude

    makeConstraints()
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)

    UIView.animate(withDuration: 0.25) {
      self.view.alpha = 1
    }
  }

  private func makeConstraints() {
    closeButton.snp.remakeConstraints {
      $0.top.equalToSuperview().inset(10)
      $0.trailing.equalTo(view.safeAreaLayoutGuide).inset(7)
      $0.size.equalTo(26)
    }
  }

  @objc private func closeView() {
    close()
  }

  func close(immediately: Bool = false) {
    let duration = immediately ? 0 : animationDuration
    UIView.animate(
      withDuration: duration,
      animations: {
        self.view.alpha = CGFloat.leastNormalMagnitude
      }
    ) { _ in
      self.closeHandler?()
      self.view.removeFromSuperview()
      self.removeFromParent()

    }
  }
}
