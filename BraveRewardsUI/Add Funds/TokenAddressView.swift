/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import CoreImage
import BraveShared

class TokenAddressView: UIView {
  
  enum TokenKind {
    case bitcoin
    case ethereum
    case basicAttentionToken
    case litecoin
    
    var name: String {
      switch self {
      case .bitcoin:
        return "Bitcoin (BTC)"
      case .ethereum:
        return "Ethereum (ETH)"
      case .basicAttentionToken:
        return "Basic Attention Token (BAT)"
      case .litecoin:
        return "Litecoin (LTC)"
      }
    }
    var image: UIImage {
      switch self {
      case .bitcoin:
        return UIImage(frameworkResourceNamed: "token-btc")
      case .basicAttentionToken:
        return UIImage(frameworkResourceNamed: "token-bat")
      case .ethereum:
        return UIImage(frameworkResourceNamed: "token-eth")
      case .litecoin:
        return UIImage(frameworkResourceNamed: "token-ltc")
      }
    }
    var codePrefix: String {
      switch self {
      case .bitcoin:
        return "bitcoin"
      case .ethereum, .basicAttentionToken:
        return "ethereum"
      case .litecoin:
        return "litecoin"
      }
    }
  }
  
  func setQRCode(image: UIImage?) {
    qrCodeView.qrCodeButton.isHidden = image != nil
    qrCodeView.imageView.image = image
  }
  
  var viewQRCodeButtonTapped: ((TokenAddressView) -> Void)?
  
  let addressTextView = UITextView().then {
    $0.isScrollEnabled = false
    $0.isEditable = false
    $0.layer.borderColor = Colors.grey600.cgColor
    $0.layer.borderWidth = 1.0 / UIScreen.main.scale
    $0.textContainerInset = UIEdgeInsets(top: 8, left: 4, bottom: 8, right: 4)
    $0.layer.cornerRadius = 4.0
    $0.appearanceTextColor = Colors.grey100
    $0.font = UIFont(name: "Menlo-Regular", size: 13.0)
  }
  
  let tokenKind: TokenKind
  
  private let qrCodeView = QRCodeView()
  
  init(tokenKind: TokenKind) {
    self.tokenKind = tokenKind
    
    super.init(frame: .zero)
    
    let containerStackView = UIStackView().then {
      $0.axis = .vertical
      $0.spacing = 10.0
    }
    
    qrCodeView.qrCodeButton.addTarget(self, action: #selector(tappedQRCodeButton), for: .touchUpInside)
    
    addSubview(containerStackView)
    containerStackView.addStackViewItems(
      .view(UIStackView().then {
        $0.spacing = 20.0
        $0.alignment = .center
        $0.addStackViewItems(
          .view(UIImageView(image: tokenKind.image).then {
            $0.contentMode = .scaleAspectFit
            $0.setContentHuggingPriority(.required, for: .horizontal)
            $0.snp.makeConstraints {
              $0.width.height.equalTo(38.0)
            }
          }),
          .view(UILabel().then {
            $0.text = tokenKind.name
            $0.font = .systemFont(ofSize: 14.0)
            $0.numberOfLines = 0
          })
        )
      }),
      .customSpace(15.0),
      .view(UILabel().then {
        $0.text = Strings.AddFundsTokenWalletAddress
        $0.font = .systemFont(ofSize: 12.0, weight: .medium)
        $0.appearanceTextColor = Colors.grey100
      }),
      .customSpace(4.0),
      .view(addressTextView),
      .view(qrCodeView)
    )
    
    containerStackView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  @objc private func tappedQRCodeButton() {
    viewQRCodeButtonTapped?(self)
  }
}

extension TokenAddressView {
  final class QRCodeView: UIView {
    let imageView = UIImageView().then {
      $0.backgroundColor = UIColor(white: 0.9, alpha: 1.0)
      $0.contentMode = .scaleAspectFit
      $0.layer.magnificationFilter = .nearest
    }
    let qrCodeButton = ActionButton(type: .system).then {
      $0.setTitle(Strings.AddFundsShowQRCode, for: .normal)
      $0.contentEdgeInsets = UIEdgeInsets(top: 0, left: 15.0, bottom: 0, right: 15.0)
      $0.backgroundColor = Colors.blurple400
      $0.layer.borderWidth = 0
    }
    override init(frame: CGRect) {
      super.init(frame: frame)
      
      addSubview(imageView)
      addSubview(qrCodeButton)
      
      imageView.snp.makeConstraints {
        $0.centerX.equalTo(self)
        $0.width.height.equalTo(90)
        $0.top.bottom.equalTo(self)
      }
      qrCodeButton.snp.makeConstraints {
        $0.center.equalTo(self)
        $0.height.equalTo(38.0)
      }
    }
    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
  }
}
