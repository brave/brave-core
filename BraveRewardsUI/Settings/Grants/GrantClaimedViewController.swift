/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

private let claimGrantDateFormatter = DateFormatter().then {
  $0.dateStyle = .short
  $0.timeStyle = .none
}

class GrantClaimedViewController: UIViewController {
  
  enum GrantKind {
    case ugp(expirationDate: Date)
    case ads
    
    var isAdsGrant: Bool {
      switch self {
      case .ads: return true
      case .ugp: return false
      }
    }
  }
  
  let grantAmount: String
  let kind: GrantKind
  
  init(grantAmount: String, kind: GrantKind) {
    self.grantAmount = grantAmount
    self.kind = kind
    
    super.init(nibName: nil, bundle: nil)
    
    self.modalPresentationStyle = .currentContext
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  private var grantView: View {
    return view as! View // swiftlint:disable:this force_cast
  }
  
  override func loadView() {
    view = View(kind: self.kind)
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    grantView.infoView.freeTokenAmountLabel.text = grantAmount + " " + Strings.BAT
    if case .ugp(let expirationDate) = kind {
      grantView.infoView.expirationDateLabel.text = claimGrantDateFormatter.string(from: expirationDate)
    }
    grantView.okButton.addTarget(self, action: #selector(dismissController), for: .touchUpInside)
  }
  
  @objc private func dismissController() {
    dismiss(animated: true)
  }
}

extension GrantClaimedViewController {
  
  private class View: UIView {
    
    let infoView: InfoView
    
    let okButton = ActionButton(type: .system).then {
      $0.backgroundColor = BraveUX.braveOrange
      $0.layer.borderWidth = 0
      $0.setTitle(Strings.ok, for: .normal)
    }
    
    init(kind: GrantClaimedViewController.GrantKind) {
      infoView = InfoView(kind: kind)
      
      super.init(frame: .zero)
      
      backgroundColor = .white
      
      let isAdsGrant = kind.isAdsGrant
      
      let scrollView = UIScrollView().then {
        $0.alwaysBounceVertical = true
        $0.delaysContentTouches = false
      }
      
      let stackView = UIStackView().then {
        $0.axis = .vertical
        $0.alignment = .center
        $0.spacing = 4.0
      }
      
      let imageView = UIImageView(image: UIImage(frameworkResourceNamed: "bat-reward-graphic"))
      let titleLabel = UILabel().then {
        $0.numberOfLines = 0
        $0.appearanceTextColor = BraveUX.braveOrange
        $0.text = isAdsGrant ? Strings.adsGrantsClaimedTitle : Strings.grantsClaimedTitle
        $0.font = .systemFont(ofSize: 20.0)
        $0.textAlignment = .center
      }
      let subtitleLabel = UILabel().then {
        $0.numberOfLines = 0
        $0.appearanceTextColor = SettingsUX.subtitleTextColor
        $0.text = isAdsGrant ? Strings.adsGrantsClaimedSubtitle : Strings.grantsClaimedSubtitle
        $0.font = .systemFont(ofSize: 12.0)
        $0.textAlignment = .center
      }
      
      addSubview(scrollView)
      scrollView.addSubview(stackView)
      stackView.addArrangedSubview(imageView)
      stackView.setCustomSpacing(15.0, after: imageView)
      stackView.addArrangedSubview(titleLabel)
      stackView.addArrangedSubview(subtitleLabel)
      stackView.setCustomSpacing(25.0, after: subtitleLabel)
      stackView.addArrangedSubview(infoView)
      stackView.setCustomSpacing(25.0, after: infoView)
      stackView.addArrangedSubview(okButton)
      
      scrollView.snp.makeConstraints {
        $0.edges.equalToSuperview()
      }
      scrollView.contentLayoutGuide.snp.makeConstraints {
        $0.width.equalTo(self)
      }
      stackView.snp.makeConstraints {
        $0.edges.equalTo(scrollView.contentLayoutGuide).inset(30.0)
      }
      infoView.snp.makeConstraints {
        $0.leading.trailing.equalToSuperview()
      }
      okButton.snp.makeConstraints {
        $0.leading.trailing.equalToSuperview()
        $0.height.equalTo(38.0)
      }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
  }
}

extension GrantClaimedViewController {
  private class InfoView: UIView {
    private struct UX {
      static let infoAccentTextColor = Colors.magenta600
    }
    
    let freeTokenAmountLabel: UILabel
    let expirationDateLabel: UILabel
    
    let expirationDateTitleLabel = UILabel().then {
      $0.text = Strings.grantsClaimedExpirationDateTitle
      $0.appearanceTextColor = SettingsUX.subtitleTextColor
      $0.font = .systemFont(ofSize: 13.0)
    }
    
    let amountTitleLabel = UILabel().then {
      $0.appearanceTextColor = SettingsUX.subtitleTextColor
      $0.font = .systemFont(ofSize: 13.0)
    }
    
    init(kind: GrantClaimedViewController.GrantKind) {
      let infoLabelConfig: (UILabel) -> Void = {
        $0.appearanceTextColor = UX.infoAccentTextColor
        $0.font = .systemFont(ofSize: 14.0)
      }
      
      freeTokenAmountLabel = UILabel().then(infoLabelConfig)
      expirationDateLabel = UILabel().then(infoLabelConfig)
      
      super.init(frame: .zero)
      
      let isAdsGrant = kind.isAdsGrant
      
      amountTitleLabel.text = isAdsGrant ? Strings.adsGrantsClaimedAmountTitle : Strings.grantsClaimedAmountTitle
      
      backgroundColor = Colors.neutral100
      
      layer.cornerRadius = 6.0
      
      let stackView = UIStackView().then {
        $0.axis = .vertical
        $0.alignment = .center
        $0.spacing = 4.0
      }
      
      addSubview(stackView)
      stackView.addArrangedSubview(amountTitleLabel)
      stackView.addArrangedSubview(freeTokenAmountLabel)
      stackView.setCustomSpacing(8.0, after: freeTokenAmountLabel)
      
      if !isAdsGrant {
        stackView.addArrangedSubview(expirationDateTitleLabel)
        stackView.addArrangedSubview(expirationDateLabel)
      }
      
      stackView.snp.makeConstraints {
        $0.edges.equalToSuperview().inset(15.0)
      }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
  }
}
