// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI

/// Container for all of the purchase information
class SKUPurchaseDetailsView: UIView {
  private struct UX {
    static let backgroundColor = UIColor(hex: 0xF1F1F2)// Colors.blurple800
    static let headerHeight: CGFloat = 60
    static let titleColor = UIColor(hex: 0x392dd1)
    static let bodyColor = Colors.grey700
  }
  
  let dismissButton = DismissButton()
  let grabberView = GrabberView(style: .dark)
  let scrollView = UIScrollView().then {
    $0.alwaysBounceVertical = true
    $0.showsVerticalScrollIndicator = false
    $0.contentInset = UIEdgeInsets(top: UX.headerHeight, left: 0.0, bottom: 0.0, right: 0.0)
  }
  
  let bodyStackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 28
    $0.layoutMargins = UIEdgeInsets(top: 25, left: 25, bottom: 25, right: 25)
    $0.isLayoutMarginsRelativeArrangement = true
  }
  
  let headerView = UIView().then {
    $0.backgroundColor = UX.backgroundColor
    $0.layer.shadowOpacity = 0.1
    $0.layer.shadowOffset = CGSize(width: 0, height: 1)
    $0.layer.shadowRadius = 0
    $0.isUserInteractionEnabled = false
  }
  
  let titleLabel = UILabel().then {
    $0.textAlignment = .center
    $0.text = Strings.SKUPurchaseTitle
    $0.font = .systemFont(ofSize: 18, weight: .semibold)
    $0.appearanceTextColor = Colors.grey800
  }
  
  let itemDetailTitleLabel = UILabel().then {
    $0.text = Strings.SKUPurchaseItemSelected
    $0.font = .systemFont(ofSize: 15.0, weight: .semibold)
    $0.appearanceTextColor = UX.titleColor
  }
  
  let itemDetailValueLabel = UILabel().then {
    $0.font = .systemFont(ofSize: 15.0)
    $0.appearanceTextColor = UX.bodyColor
    $0.numberOfLines = 0
  }
  
  let orderTotalLabel = UILabel().then {
    $0.text = Strings.SKUPurchaseOrderTotal
    $0.font = .systemFont(ofSize: 15.0, weight: .semibold)
    $0.appearanceTextColor = UX.titleColor
  }
  
  let orderAmountLabels = BATUSDPairView(
    batAmountConfig: {
      $0.font = .systemFont(ofSize: 22.0, weight: .semibold)
      $0.appearanceTextColor = Colors.grey900
    },
    batKindConfig: {
      $0.text = Strings.BAT
      $0.font = .systemFont(ofSize: 18.0)
      $0.appearanceTextColor = Colors.grey900
    },
    usdConfig: {
      $0.font = .systemFont(ofSize: 16.0)
      $0.appearanceTextColor = Colors.grey700
    }
  )
  
  let balanceView = SKUPurchaseWalletBalanceView()
  
  let insufficientBalanceView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 16.0
    $0.alignment = .center
    $0.isHidden = true
    $0.addStackViewItems(
      .view(UIImageView(image: UIImage(frameworkResourceNamed: "icn-frowning-face").alwaysTemplate).then {
        $0.tintColor = UIColor(hex: 0xfc798f)
      }),
      .view(UILabel().then {
        $0.text = String(format: Strings.SKUPurchaseInsufficientFunds, Strings.BAT)
        $0.textAlignment = .center
        $0.numberOfLines = 0
        $0.font = .systemFont(ofSize: 14.0)
        $0.appearanceTextColor = Colors.grey900
      })
    )
  }
  
  let processingView = SKUPurchaseProcessingView().then {
    $0.isHidden = true
  }
  let completedView = SKUPurchaseCompletedView().then {
    $0.isHidden = true
  }
  
  var contentView: UIView? {
    didSet {
      oldValue?.removeFromSuperview()
      if let view = contentView {
        scrollView.addSubview(view)
        view.snp.makeConstraints {
          $0.top.bottom.equalTo(self.scrollView.contentLayoutGuide)
          $0.leading.trailing.equalTo(self)
        }
      }
    }
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    backgroundColor = UX.backgroundColor
    
    clipsToBounds = true
    layer.cornerRadius = 8.0
    layer.maskedCorners = [.layerMinXMinYCorner, .layerMaxXMinYCorner]
    
    addSubview(scrollView)
    addSubview(headerView)
    headerView.addSubview(titleLabel)
    addSubview(grabberView)
    addSubview(dismissButton)
    scrollView.addSubview(bodyStackView)
    scrollView.addSubview(processingView)
    scrollView.addSubview(completedView)
    bodyStackView.addStackViewItems(
      .view(UIStackView().then {
        $0.axis = .vertical
        $0.alignment = .leading
        $0.spacing = 16
        $0.addStackViewItems(
          .view(itemDetailTitleLabel),
          .customSpace(4),
          .view(itemDetailValueLabel),
          .view(orderTotalLabel),
          .customSpace(4),
          .view(orderAmountLabels)
        )
      }),
      .view(balanceView),
      .view(insufficientBalanceView)
    )
    
    scrollView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }
    scrollView.contentLayoutGuide.snp.makeConstraints {
      $0.width.equalTo(self)
    }
    headerView.snp.makeConstraints {
      $0.top.leading.trailing.equalTo(self)
      $0.height.equalTo(UX.headerHeight)
    }
    titleLabel.snp.makeConstraints {
      $0.bottom.equalToSuperview().inset(6)
      $0.leading.trailing.equalToSuperview().inset(20)
    }
    dismissButton.snp.makeConstraints {
      $0.top.trailing.equalToSuperview().inset(8.0)
    }
    grabberView.snp.makeConstraints {
      $0.centerX.equalTo(self)
      $0.top.equalTo(self).offset(5.0)
    }
    bodyStackView.snp.makeConstraints {
      $0.top.bottom.equalTo(self.scrollView.contentLayoutGuide)
      $0.leading.trailing.equalTo(self)
    }
    processingView.snp.makeConstraints {
      $0.height.equalTo(300)
      $0.top.equalTo(self.scrollView)
      $0.leading.trailing.equalTo(self)
    }
    completedView.snp.makeConstraints {
      $0.height.equalTo(300)
      $0.top.equalTo(self.scrollView)
      $0.leading.trailing.equalTo(self)
    }
    
    updateForTraits()
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    updateForTraits()
  }
  
  func updateForTraits() {
    grabberView.isHidden = traitCollection.horizontalSizeClass == .regular
  }
}

class SKUPurchaseWalletBalanceView: UIView {
  
  let amountLabels = BATUSDPairView(
    batAmountConfig: {
      $0.font = .systemFont(ofSize: 18.0, weight: .medium)
      $0.appearanceTextColor = Colors.grey900
    },
    batKindConfig: {
      $0.text = Strings.BAT
      $0.font = .systemFont(ofSize: 16.0)
      $0.appearanceTextColor = Colors.grey900
    },
    usdConfig: {
      $0.font = .systemFont(ofSize: 14.0)
      $0.appearanceTextColor = Colors.grey700
    }
  )
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    layer.borderColor = UIColor.black.withAlphaComponent(0.2).cgColor
    layer.borderWidth = 1.0 / UIScreen.main.scale
    layer.cornerRadius = 8
    
    let stackView = UIStackView()
    stackView.axis = .vertical
    stackView.spacing = 6
    stackView.alignment = .leading
    stackView.addStackViewItems(
      .view(UILabel().then {
        $0.text = Strings.walletHeaderTitle
        $0.font = .systemFont(ofSize: 15.0, weight: .semibold)
        $0.appearanceTextColor = Colors.grey900
      }),
      .view(UIStackView().then {
        $0.spacing = 10
        $0.alignment = .center
        $0.addStackViewItems(
          .view(UIImageView(image: RewardsPanelController.batLogoImage).then {
            $0.setContentHuggingPriority(.required, for: .horizontal)
            $0.setContentHuggingPriority(.required, for: .vertical)
          }),
          .view(amountLabels)
        )
      })
    )
    addSubview(stackView)
    stackView.snp.makeConstraints {
      $0.edges.equalTo(self).inset(UIEdgeInsets(top: 12, left: 16, bottom: 12, right: 16))
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
