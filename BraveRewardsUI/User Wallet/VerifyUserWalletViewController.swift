// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveShared
import SnapKit
import BraveUI

class VerifyUserWalletViewController: UIViewController {
  
  private let verifyWalletTapped: () -> Void
  
  init(_ verifyWalletTapped: @escaping () -> Void) {
    self.verifyWalletTapped = verifyWalletTapped
    
    super.init(nibName: nil, bundle: nil)
    
    modalPresentationStyle = .overFullScreen
    modalPresentationCapturesStatusBarAppearance = true
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override var preferredStatusBarStyle: UIStatusBarStyle {
    return .lightContent
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    let view = self.view as! View // swiftlint:disable:this force_cast
    view.closeButton.addTarget(self, action: #selector(tappedClose), for: .touchUpInside)
    view.verifyWalletButton.addTarget(self, action: #selector(tappedVerifyWallet), for: .touchUpInside)
  }
  
  override func loadView() {
    view = View()
  }
  
  @objc private func tappedClose() {
    dismiss(animated: true, completion: nil)
  }
  
  @objc private func tappedVerifyWallet() {
    verifyWalletTapped()
  }
}

private extension VerifyUserWalletViewController {
  
  class View: UIView {
    private let scrollView = UIScrollView().then {
      $0.alwaysBounceVertical = true
    }
    
    let closeButton = DismissButton(defaultBackgroundColor: UIColor.white.withAlphaComponent(0.2))
    
    private let gradientView = GradientView.purpleRewardsGradientView()
    
    private let stackView = UIStackView().then {
      $0.axis = .vertical
      $0.spacing = 4
      $0.alignment = .center
    }
    
    let verifyWalletButton = FilledActionButton(type: .system).then {
      $0.contentEdgeInsets = UIEdgeInsets(top: 12, left: 40, bottom: 12, right: 40)
      $0.setTitle(Strings.userWalletOnboardingVerifyButtonTitle, for: .normal)
      $0.titleLabel?.font = .systemFont(ofSize: 14, weight: .bold)
      $0.appearanceBackgroundColor = BraveUX.braveOrange
      $0.setTitleColor(.white, for: .normal)
    }
    
    override init(frame: CGRect) {
      super.init(frame: frame)
      
      let benefitsStackView = UIStackView().then {
        $0.axis = .vertical
        $0.spacing = 16
        $0.isLayoutMarginsRelativeArrangement = true
        $0.layoutMargins = UIEdgeInsets(top: 0, left: 8, bottom: 0, right: 8)
      }
      benefitsStackView.addStackViewItems(
        .view(BenefitItemView(text: Strings.userWalletOnboardingBenefitsOne)),
        .view(BenefitItemView(text: Strings.userWalletOnboardingBenefitsTwo)),
        .view(BenefitItemView(text: Strings.userWalletOnboardingBenefitsThree))
      )
      
      addSubview(gradientView)
      addSubview(scrollView)
      addSubview(closeButton)
      scrollView.addSubview(stackView)
      
      stackView.addStackViewItems(
        .view(UIImageView(image: UIImage(frameworkResourceNamed: "verify-wallet-graphic"))),
        .customSpace(24),
        .view(UILabel().then {
          $0.text = Strings.userWalletOnboardingTitle
          $0.font = .systemFont(ofSize: 22, weight: .semibold)
          $0.appearanceTextColor = .white
          $0.textAlignment = .center
          $0.numberOfLines = 0
        }),
        .customSpace(38),
        .view(UILabel().then {
          $0.text = Strings.userWalletOnboardingBenefitsTitle
          $0.font = .systemFont(ofSize: 17, weight: .semibold)
          $0.appearanceTextColor = .white
          $0.textAlignment = .center
          $0.numberOfLines = 0
        }),
        .customSpace(18),
        .view(benefitsStackView),
        .customSpace(34),
        .view(verifyWalletButton),
        .customSpace(24),
        .view(PoweredByUpholdView()),
        .customSpace(16),
        .view(UILabel().then {
          $0.text = Strings.userWalletOnboardingUpholdDisclosure
          $0.font = .systemFont(ofSize: 12)
          $0.appearanceTextColor = .white
          $0.textAlignment = .left
          $0.numberOfLines = 0
        })
      )
      
      gradientView.snp.makeConstraints {
        $0.edges.equalTo(self)
      }
      scrollView.snp.makeConstraints {
        $0.edges.equalTo(self)
      }
      scrollView.contentLayoutGuide.snp.makeConstraints {
        $0.width.equalTo(self)
      }
      stackView.snp.makeConstraints {
        $0.edges.equalTo(self.scrollView.contentLayoutGuide).inset(UIEdgeInsets(top: 90, left: 25, bottom: 40, right: 25))
      }
      closeButton.snp.makeConstraints {
        $0.top.trailing.equalTo(self.safeAreaLayoutGuide).inset(16)
      }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
  }
  
  class BenefitItemView: UIStackView {
    init(text: String) {
      super.init(frame: .zero)
      
      spacing = 12
      alignment = .top
      
      addStackViewItems(
        .view(UIImageView(image: UIImage(frameworkResourceNamed: "verify-benefit")).then {
          $0.snp.makeConstraints {
            $0.width.equalTo(24)
          }
        }),
        .view(UILabel().then {
          $0.setContentCompressionResistancePriority(.required, for: .horizontal)
          $0.text = text
          $0.font = .systemFont(ofSize: 15)
          $0.numberOfLines = 0
          $0.appearanceTextColor = .white
        })
      )
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
  }
  
  class PoweredByUpholdView: UIStackView {
    override init(frame: CGRect) {
      super.init(frame: frame)
      spacing = 5
      alignment = .center
      
      addStackViewItems(
        .view(UILabel().then {
          $0.appearanceTextColor = nil
          $0.attributedText = {
            let string = NSMutableAttributedString(string: Strings.userWalletOnboardingPoweredByUphold, attributes: [.font: UIFont.systemFont(ofSize: 12), .foregroundColor: UIColor.white])
            if let upholdRange = string.string.range(of: Strings.userWalletOnboardingPoweredByUpholdBoldedWord) {
              let nsRange = NSRange(upholdRange, in: string.string)
              string.addAttribute(.font, value: UIFont.systemFont(ofSize: 12, weight: .bold), range: nsRange)
            }
            return string
          }()
        }),
        .view(UIImageView(image: UIImage(frameworkResourceNamed: "uphold").alwaysTemplate).then {
          $0.tintColor = BraveUX.upholdGreen
        })
      )
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
  }
}
