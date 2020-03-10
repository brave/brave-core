// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

class LedgerInitializationFailedView: UIView {
  
  private let gradientBackgroundView = GradientView.softBlueToClearGradientView()
  
  private let stackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 15
  }
  
  private let titleLabel = UILabel().then {
    $0.text = Strings.ledgerInitializationFailedTitle
    $0.font = .systemFont(ofSize: 18, weight: .semibold)
    $0.appearanceTextColor = .black
    $0.textAlignment = .center
    $0.numberOfLines = 0
  }
  
  private let bodyLabel = UILabel().then {
    $0.font = .systemFont(ofSize: 16)
    $0.appearanceTextColor = .gray
    $0.textAlignment = .center
    $0.numberOfLines = 0
  }
  
  private let okButton = FilledActionButton(type: .system).then {
    $0.appearanceBackgroundColor = .gray
    $0.titleLabel?.font = .systemFont(ofSize: 14.0, weight: .bold)
    $0.setTitle(Strings.ok, for: .normal)
  }
  
  private let dismissed: () -> Void
  
  init(failureMessage: String, dismissed: @escaping () -> Void) {
    self.dismissed = dismissed
    
    super.init(frame: .zero)
    
    addSubview(gradientBackgroundView)
    addSubview(stackView)
    stackView.addArrangedSubview(titleLabel)
    stackView.addArrangedSubview(bodyLabel)
    stackView.setCustomSpacing(50, after: bodyLabel)
    stackView.addArrangedSubview(okButton)
    
    bodyLabel.text = failureMessage
    okButton.addTarget(self, action: #selector(tappedOK), for: .touchUpInside)
    
    backgroundColor = .white
    
    gradientBackgroundView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }
    stackView.snp.makeConstraints {
      $0.top.greaterThanOrEqualTo(self).inset(32)
      $0.bottom.lessThanOrEqualTo(self).inset(32)
      $0.leading.trailing.equalTo(self).inset(32)
      $0.centerY.equalTo(self)
    }
    okButton.snp.makeConstraints {
      $0.height.equalTo(36)
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  @objc private func tappedOK() {
    dismissed()
  }
}
