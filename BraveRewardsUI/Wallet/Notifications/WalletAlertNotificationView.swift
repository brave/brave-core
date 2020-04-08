/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

struct WalletAlertNotification {
  struct Category {
    let icon: UIImage
    let backgroundImage: UIImage
    
    static let success = Category(
      icon: UIImage(frameworkResourceNamed: "icn-validated"),
      backgroundImage: UIImage(frameworkResourceNamed: "notification_header_normal")
    )
    static let warning = Category(
      icon: UIImage(frameworkResourceNamed: "icn-warning"),
      backgroundImage: UIImage(frameworkResourceNamed: "notification_header_warning")
    )
    static let error = Category(
      icon: UIImage(frameworkResourceNamed: "icn-error"),
      backgroundImage: UIImage(frameworkResourceNamed: "notification_header_error")
    )
  }
  let category: Category
  let title: String?
  let body: String
}

class WalletAlertNotificationView: WalletNotificationView {
  
  let notification: WalletAlertNotification
  
  init(notification: WalletAlertNotification) {
    self.notification = notification
    super.init(frame: .zero)
    
    stackView.spacing = 20.0
    stackView.alignment = .center
    
    backgroundView.image = notification.category.backgroundImage
    iconImageView.image = notification.category.icon
    
    let label = UILabel().then {
      $0.numberOfLines = 0
      $0.appearanceTextColor = nil
      $0.attributedText = bodyAttributedString()
//      $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    }
    
    stackView.addArrangedSubview(label)
  }
  
  /// Forms the body string: "{bolded title} {body}"
  private func bodyAttributedString() -> NSAttributedString {
    let string = NSMutableAttributedString()
    if let title = notification.title {
      string.append(NSAttributedString(
        string: title,
        attributes: [
          .font: UIFont.systemFont(ofSize: 15.0, weight: .semibold),
          .foregroundColor: Colors.grey800,
        ]
      ))
      string.append(NSAttributedString(
        string: " ",
        attributes: [ .font: UIFont.systemFont(ofSize: 14.0) ]
      ))
    }
    string.append(NSAttributedString(
      string: notification.body,
      attributes: [
        .font: UIFont.systemFont(ofSize: 15.0),
        .foregroundColor: Colors.grey800,
      ]
    ))
    return string
  }
}
