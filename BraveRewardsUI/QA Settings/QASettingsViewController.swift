// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveRewards
import BraveShared
import Shared
import Static

private typealias EnvironmentOverride = Preferences.Rewards.EnvironmentOverride

/// A special QA settings menu that allows them to adjust debug rewards features
public class QASettingsViewController: TableViewController, UITextFieldDelegate {
  
  public let rewards: BraveRewards
  
  public init(rewards: BraveRewards) {
    self.rewards = rewards
    super.init(style: .grouped)
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  private let segmentedControl = UISegmentedControl(items: EnvironmentOverride.allCases.map { $0.name })
  
  @objc private func environmentChanged() {
    let value = segmentedControl.selectedSegmentIndex
    guard let _ = EnvironmentOverride(rawValue: value) else { return }
    Preferences.Rewards.environmentOverride.value = value
    self.rewards.reset()
    self.showResetRewardsAlert()
  }
  
  private let reconcileTimeTextField = UITextField().then {
    $0.borderStyle = .roundedRect
    $0.autocorrectionType = .no
    $0.autocapitalizationType = .none
    $0.spellCheckingType = .no
    $0.returnKeyType = .done
    $0.textAlignment = .right
    $0.text = "\(BraveLedger.reconcileTime)"
    $0.placeholder = "0"
  }
  
  @objc private func reconcileTimeEditingEnded() {
    guard let value = Int32(reconcileTimeTextField.text ?? "") else {
      let alert = UIAlertController(title: "Invalid value", message: "Time has been reset to 0 (no override)", preferredStyle: .alert)
      alert.addAction(.init(title: "OK", style: .default, handler: nil))
      self.present(alert, animated: true)
      reconcileTimeTextField.text = "0"
      BraveLedger.reconcileTime = 0
      return
    }
    BraveLedger.reconcileTime = value
  }
  
  public func textFieldShouldReturn(_ textField: UITextField) -> Bool {
    textField.resignFirstResponder()
    return true
  }
  
  public override func viewDidLoad() {
    super.viewDidLoad()
    
    view.backgroundColor = .white
    
    title = "Rewards QA Settings"
    
    let isDefaultEnvironmentProd = AppConstants.BuildChannel != .developer
    
    segmentedControl.selectedSegmentIndex = Preferences.Rewards.environmentOverride.value
    segmentedControl.addTarget(self, action: #selector(environmentChanged), for: .valueChanged)
    reconcileTimeTextField.addTarget(self, action: #selector(reconcileTimeEditingEnded), for: .editingDidEnd)
    reconcileTimeTextField.delegate = self
    reconcileTimeTextField.frame = CGRect(x: 0, y: 0, width: 50, height: 32)
    
    dataSource.sections = [
      Section(
        header: .title("Environment"),
        rows: [
          Row(text: "Default", detailText: isDefaultEnvironmentProd ? "Prod" : "Staging"),
          Row(text: "Override", accessory: .view(segmentedControl)),
        ],
        footer: .title("Changing the environment automatically resets Brave Rewards.\n\nThe app must be force-quit after rewards is reset")
      ),
      Section(
        rows: [
          Row(text: "Is Debug", accessory: .switchToggle(value: BraveLedger.isDebug, { value in
            BraveLedger.isDebug = value
            BraveAds.isDebug = value
          })),
          Row(text: "Use Short Retries", accessory: .switchToggle(value: BraveLedger.useShortRetries, { value in
            BraveLedger.useShortRetries = value
          })),
          Row(text: "Reconcile Time", detailText: "Number of minutes between reconciles. 0 = No Override", accessory: .view(reconcileTimeTextField), cellClass: MultilineSubtitleCell.self)
        ]
      ),
      Section(
        rows: [
          Row(text: "Reset Rewards", selection: {
            self.tappedReset()
          }, cellClass: ButtonCell.self)
        ]
      )
    ]
  }
  
  @objc private func tappedReset() {
    rewards.reset()
    showResetRewardsAlert()
  }
  
  private func showResetRewardsAlert() {
    let alert = UIAlertController(
      title: "Rewards Reset",
      message: "Brave must be restarted to ensure expected Rewards behavior",
      preferredStyle: .alert
    )
    alert.addAction(UIAlertAction(title: "Exit Now", style: .destructive, handler: { _ in
      fatalError()
    }))
    alert.addAction(UIAlertAction(title: "OK", style: .default, handler: nil))
    present(alert, animated: true)
  }
}

fileprivate class MultilineSubtitleCell: SubtitleCell {
  
  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: reuseIdentifier)
    textLabel?.numberOfLines = 0
    detailTextLabel?.numberOfLines = 0
  }
  
  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}
