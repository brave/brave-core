/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

private let claimGrantDateFormatter = DateFormatter().then {
  $0.dateStyle = .short
  $0.timeStyle = .none
}

class GrantClaimedViewController: UIViewController {
  
  let grantAmount: String
  let expirationDate: Date
  
  init(grantAmount: String, expirationDate: Date) {
    self.grantAmount = grantAmount
    self.expirationDate = expirationDate
    
    super.init(nibName: nil, bundle: nil)
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  private var grantView: View {
    return view as! View // swiftlint:disable:this force_cast
  }
  
  override func loadView() {
    view = View()
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    grantView.infoView.freeTokenAmountLabel.text = grantAmount
    grantView.infoView.expirationDateLabel.text = claimGrantDateFormatter.string(from: expirationDate)
    grantView.okButton.addTarget(self, action: #selector(dismissController), for: .touchUpInside)
  }
  
  @objc private func dismissController() {
    dismiss(animated: true)
  }
}

extension GrantClaimedViewController {
  
  private class View: UIView {
    
    let infoView = InfoView()
    
    let okButton = ActionButton(type: .system).then {
      $0.backgroundColor = BraveUX.braveOrange
      $0.layer.borderWidth = 0
      $0.setTitle(Strings.OK, for: .normal)
    }
    
    override init(frame: CGRect) {
      super.init(frame: frame)
      
      backgroundColor = .white
      
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
        $0.text = Strings.GrantsClaimedTitle
        $0.font = .systemFont(ofSize: 20.0)
        $0.textAlignment = .center
      }
      let subtitleLabel = UILabel().then {
        $0.numberOfLines = 0
        $0.appearanceTextColor = SettingsUX.subtitleTextColor
        $0.text = Strings.GrantsClaimedSubtitle
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
      static let infoAccentTextColor = Colors.magenta300
    }
    
    let freeTokenAmountLabel: UILabel
    let expirationDateLabel: UILabel
    
    override init(frame: CGRect) {
      let infoLabelConfig: (UILabel) -> Void = {
        $0.appearanceTextColor = UX.infoAccentTextColor
        $0.font = .systemFont(ofSize: 14.0)
      }
      
      freeTokenAmountLabel = UILabel().then(infoLabelConfig)
      expirationDateLabel = UILabel().then(infoLabelConfig)
      
      super.init(frame: frame)
      
      backgroundColor = Colors.neutral800
      
      layer.cornerRadius = 6.0
      
      let stackView = UIStackView().then {
        $0.axis = .vertical
        $0.alignment = .center
        $0.spacing = 4.0
      }
      let amountTitleLabel = UILabel().then {
        $0.text = Strings.GrantsClaimedAmountTitle
        $0.appearanceTextColor = SettingsUX.subtitleTextColor
        $0.font = .systemFont(ofSize: 13.0)
      }
      let expirdationDateTitleLabel = UILabel().then {
        $0.text = Strings.GrantsClaimedExpirationDateTitle
        $0.appearanceTextColor = SettingsUX.subtitleTextColor
        $0.font = .systemFont(ofSize: 13.0)
      }
      
      addSubview(stackView)
      stackView.addArrangedSubview(amountTitleLabel)
      stackView.addArrangedSubview(freeTokenAmountLabel)
      stackView.setCustomSpacing(8.0, after: freeTokenAmountLabel)
      stackView.addArrangedSubview(expirdationDateTitleLabel)
      stackView.addArrangedSubview(expirationDateLabel)
      
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
