// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveRewards

/// A special QA settings menu that allows them to adjust debug rewards features
public class QASettingsViewController: UIViewController {
  
  public let rewards: BraveRewards
  
  public init(rewards: BraveRewards) {
    self.rewards = rewards
    super.init(nibName: nil, bundle: nil)
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  public override func viewDidLoad() {
    super.viewDidLoad()
    
    view.backgroundColor = .white
    
    title = "Rewards QA Settings"
    
    let scrollView = UIScrollView()
    
    let stackView = UIStackView().then {
      $0.axis = .vertical
      $0.spacing = 12
    }
    
    view.addSubview(scrollView)
    scrollView.addSubview(stackView)
    
    scrollView.snp.makeConstraints {
      $0.edges.equalTo(view)
    }
    
    scrollView.contentLayoutGuide.snp.makeConstraints {
      $0.width.equalTo(view)
    }
    
    stackView.snp.makeConstraints {
      $0.top.equalTo(scrollView.contentLayoutGuide.snp.top).offset(20)
      $0.leading.trailing.equalTo(view).inset(12)
      $0.bottom.equalTo(scrollView.contentLayoutGuide.snp.bottom).offset(-20)
    }
    
    stackView.addStackViewItems(
      .view(SwitchRow().then {
        $0.textLabel.text = "Is Debug"
        $0.toggleSwitch.isOn = BraveLedger.isDebug
        $0.valueChanged = { value in
          BraveLedger.isDebug = value
          BraveAds.isDebug = value
        }
      }),
      .view(EnvironmentRow().then {
        $0.segmentedControl.selectedSegmentIndex = BraveLedger.isProduction ? 1 : 0
        $0.valueChanged = { value in
          let isProd = value == 1
          if BraveLedger.isProduction != isProd {
            BraveLedger.isProduction = isProd
            BraveAds.isProduction = isProd
            self.rewards.reset()
          }
        }
      }),
      .view(SwitchRow().then {
        $0.textLabel.text = "Use Short Retries"
        $0.toggleSwitch.isOn = BraveLedger.useShortRetries
        $0.valueChanged = { value in
          BraveLedger.useShortRetries = value
        }
      }),
      .view(ValueRow().then {
        $0.textLabel.text = "Reconcile Time\n(0 = no override)"
        $0.textField.text = "\(BraveLedger.reconcileTime)"
        $0.textField.placeholder = "0"
        let textField = $0.textField
        $0.valueChanged = { value in
          guard let value = Int32(value) else {
            let alert = UIAlertController(title: "Invalid value", message: "Time has been reset to 0 (no override)", preferredStyle: .alert)
            alert.addAction(.init(title: "OK", style: .default, handler: nil))
            self.present(alert, animated: true)
            textField.text = "0"
            BraveLedger.reconcileTime = 0
            return
          }
          BraveLedger.reconcileTime = value
        }
      }),
      .customSpace(40),
      .view(UILabel().then {
        $0.text = "Changing the environment automatically resets Brave Rewards.\n\nIt is recommended you force-quit the app after Brave Rewards is reset"
        $0.numberOfLines = 0
        $0.textAlignment = .center
        $0.font = .systemFont(ofSize: 14)
      }),
      .customSpace(40),
      .view(UIButton(type: .roundedRect).then {
        $0.setTitle("Reset Rewards", for: .normal)
        $0.setTitleColor(.red, for: .normal)
        $0.layer.cornerRadius = 8
        $0.layer.borderWidth = 1.0 / UIScreen.main.scale
        $0.layer.borderColor = UIColor.lightGray.cgColor
        $0.snp.makeConstraints { $0.height.equalTo(44) }
        $0.addTarget(self, action: #selector(tappedReset), for: .touchUpInside)
      })
    )
  }
  
  @objc private func tappedReset() {
    rewards.reset()
  }
}

private class EnvironmentRow: UIStackView {
  var valueChanged: ((Int) -> Void)?
  
  private struct UX {
    static let textColor = Colors.grey200
  }
  
  let textLabel = UILabel().then {
    $0.text = "Environment"
    $0.font = .systemFont(ofSize: 14.0)
    $0.textColor = UX.textColor
    $0.numberOfLines = 0
  }
  
  let segmentedControl = UISegmentedControl(items: ["Staging", "Prod"])
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    addArrangedSubview(textLabel)
    addArrangedSubview(segmentedControl)
    
    segmentedControl.addTarget(self, action: #selector(selectedItemChanged), for: .valueChanged)
  }
  
  @objc func selectedItemChanged() {
    valueChanged?(segmentedControl.selectedSegmentIndex)
  }
  
  func textFieldShouldReturn(_ textField: UITextField) -> Bool {
    textField.resignFirstResponder()
    return true
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}

private class ValueRow: UIStackView, UITextFieldDelegate {
  
  var valueChanged: ((String) -> Void)?
  
  private struct UX {
    static let textColor = Colors.grey200
  }
  
  let textLabel = UILabel().then {
    $0.font = .systemFont(ofSize: 14.0)
    $0.textColor = UX.textColor
    $0.numberOfLines = 0
  }
  
  let textField = UITextField().then {
    $0.borderStyle = .roundedRect
    $0.autocorrectionType = .no
    $0.autocapitalizationType = .none
    $0.spellCheckingType = .no
    $0.returnKeyType = .done
    $0.textAlignment = .right
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    addArrangedSubview(textLabel)
    addArrangedSubview(textField)
    
    textField.addTarget(self, action: #selector(editingEnded), for: .editingDidEnd)
    textField.delegate = self
  }
  
  @objc func editingEnded() {
    valueChanged?(textField.text ?? "")
  }
  
  func textFieldShouldReturn(_ textField: UITextField) -> Bool {
    textField.resignFirstResponder()
    return true
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}

