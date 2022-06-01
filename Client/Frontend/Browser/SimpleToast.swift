/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import UIKit

struct SimpleToastUX {
  static let toastHeight = bottomToolbarHeight
  static let toastAnimationDuration = 0.5
  static let toastDefaultColor = UIColor.braveInfoBorder
  static let toastFont = UIFont.systemFont(ofSize: 15)
  static let toastDismissAfter = DispatchTimeInterval.milliseconds(4500)  // 4.5 seconds.
  static let toastDelayBefore = DispatchTimeInterval.milliseconds(0)  // 0 seconds
  static let bottomToolbarHeight = CGFloat(45)
}

struct SimpleToast {
  func showAlertWithText(_ text: String, bottomContainer: UIView) {
    let toast = self.createView()
    toast.text = text
    bottomContainer.addSubview(toast)
    toast.snp.makeConstraints { (make) in
      make.width.equalTo(bottomContainer)
      make.left.equalTo(bottomContainer)
      make.height.equalTo(SimpleToastUX.toastHeight)
      make.bottom.equalTo(bottomContainer)
    }
    animate(toast)
  }

  fileprivate func createView() -> UILabel {
    let toast = UILabel()
    toast.textColor = .white
    toast.backgroundColor = SimpleToastUX.toastDefaultColor
    toast.font = SimpleToastUX.toastFont
    toast.textAlignment = .center
    return toast
  }

  fileprivate func dismiss(_ toast: UIView) {
    UIView.animate(
      withDuration: SimpleToastUX.toastAnimationDuration,
      animations: {
        var frame = toast.frame
        frame.origin.y = frame.origin.y + SimpleToastUX.toastHeight
        frame.size.height = 0
        toast.frame = frame
      },
      completion: { finished in
        toast.removeFromSuperview()
      }
    )
  }

  fileprivate func animate(_ toast: UIView) {
    UIView.animate(
      withDuration: SimpleToastUX.toastAnimationDuration,
      animations: {
        var frame = toast.frame
        frame.origin.y = frame.origin.y - SimpleToastUX.toastHeight
        frame.size.height = SimpleToastUX.toastHeight
        toast.frame = frame
      },
      completion: { finished in
        let dispatchTime = DispatchTime.now() + SimpleToastUX.toastDismissAfter

        DispatchQueue.main.asyncAfter(
          deadline: dispatchTime,
          execute: {
            self.dismiss(toast)
          })
      }
    )
  }
}
