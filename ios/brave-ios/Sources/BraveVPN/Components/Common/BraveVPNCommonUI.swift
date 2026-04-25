// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Foundation
import Shared
import UIKit

struct BraveVPNCommonUI {

  struct UX {
    static let purpleBackgroundColor = #colorLiteral(
      red: 0.0862745098,
      green: 0.06274509804,
      blue: 0.3960784314,
      alpha: 1
    )
  }

  static var navigationBarAppearance: UINavigationBarAppearance {
    let appearance = UINavigationBarAppearance()
    appearance.configureWithDefaultBackground()
    appearance.backgroundColor = BraveVPNCommonUI.UX.purpleBackgroundColor
    appearance.titleTextAttributes = [.foregroundColor: UIColor.white]
    appearance.largeTitleTextAttributes = [.foregroundColor: UIColor.white]
    return appearance
  }

  struct Views {
    static func poweredByView(
      textColor: UIColor,
      fontSize: CGFloat = 13,
      imageColor: UIColor
    ) -> UIStackView {
      UIStackView().then { stackView in
        stackView.distribution = .fillEqually
        stackView.spacing = 6

        let label = UILabel().then {
          $0.text = Strings.VPN.poweredBy
          $0.textAlignment = .center
          $0.textColor = textColor
          $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
          $0.textAlignment = .right
          $0.font = UIFont.systemFont(ofSize: fontSize, weight: .regular)
        }

        let image = UIImageView(image: UIImage(sharedNamed: "vpn_brand")!.template).then {
          $0.contentMode = .left
          $0.tintColor = imageColor
        }

        [label, image].forEach(stackView.addArrangedSubview(_:))
      }
    }

    static func checkmarkView(
      string: String,
      textColor: UIColor,
      font: UIFont,
      useShieldAsCheckmark: Bool
    ) -> UIStackView {
      UIStackView().then { stackView in
        stackView.alignment = useShieldAsCheckmark ? .center : .top
        stackView.spacing = 4
        let image =
          useShieldAsCheckmark
          ? UIImage(named: "vpn_checkmark", in: .module, compatibleWith: nil)!
          : UIImage(sharedNamed: "vpn_checkmark_popup")!

        let checkmarkImage = UIImageView(image: image).then {
          $0.contentMode = .scaleAspectFit
          $0.snp.makeConstraints { make in
            make.size.equalTo(24)
          }

        }
        stackView.addArrangedSubview(checkmarkImage)

        let verticalStackView = UIStackView().then {
          $0.axis = .vertical
          $0.alignment = .leading
        }

        verticalStackView.addArrangedSubview(UIView.spacer(.vertical, amount: 2))

        let label = ShrinkableLabel().then {
          $0.text = string
          $0.font = font
          $0.textColor = textColor
          $0.numberOfLines = 0
          $0.lineBreakMode = .byWordWrapping
        }
        verticalStackView.addArrangedSubview(label)

        stackView.addArrangedSubview(verticalStackView)
      }
    }

    class ShrinkableLabel: UILabel {
      override init(frame: CGRect) {
        super.init(frame: frame)

        minimumScaleFactor = 0.5
        adjustsFontSizeToFitWidth = true
      }

      @available(*, unavailable)
      required init(coder: NSCoder) { fatalError() }
    }
  }
}
