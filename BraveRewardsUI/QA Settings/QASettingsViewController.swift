// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveRewards
import BraveShared
import Shared
import Static
import CoreServices

private typealias EnvironmentOverride = Preferences.Rewards.EnvironmentOverride

/// A special QA settings menu that allows them to adjust debug rewards features
public class QASettingsViewController: TableViewController {
  
  public let rewards: BraveRewards
  
  public init(rewards: BraveRewards) {
    self.rewards = rewards
    super.init(style: .grouped)
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  private let segmentedControl = UISegmentedControl(items: EnvironmentOverride.sortedCases.map { $0.name })
  
  @objc private func environmentChanged() {
    let value = segmentedControl.selectedSegmentIndex
    guard value < EnvironmentOverride.sortedCases.count else { return }
    let overrideForIndex = EnvironmentOverride.sortedCases[value]
    Preferences.Rewards.environmentOverride.value = overrideForIndex.rawValue
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
    $0.keyboardType = .numberPad
    $0.text = "\(BraveLedger.reconcileInterval)"
    $0.placeholder = "0"
  }
  
  private let customUserAgentTextField = UITextField().then {
    $0.borderStyle = .roundedRect
    $0.autocorrectionType = .no
    $0.autocapitalizationType = .none
    $0.spellCheckingType = .no
    $0.returnKeyType = .done
    $0.textAlignment = .right
  }
  
  @objc private func reconcileTimeEditingEnded() {
    guard let value = Int32(reconcileTimeTextField.text ?? "") else {
      let alert = UIAlertController(title: "Invalid value", message: "Time has been reset to 0 (no override)", preferredStyle: .alert)
      alert.addAction(.init(title: "OK", style: .default, handler: nil))
      self.present(alert, animated: true)
      reconcileTimeTextField.text = "0"
      BraveLedger.reconcileInterval = 0
      return
    }
    BraveLedger.reconcileInterval = value
  }
  
  private let adsDismissalTextField = UITextField().then {
    $0.borderStyle = .roundedRect
    $0.autocorrectionType = .no
    $0.autocapitalizationType = .none
    $0.spellCheckingType = .no
    $0.keyboardType = .numberPad
    $0.returnKeyType = .done
    $0.textAlignment = .right
    $0.placeholder = "0"
  }
  
  @objc private func adsDismissalEditingEnded() {
    let value = Int(adsDismissalTextField.text ?? "") ?? 0
    Preferences.Rewards.adsDurationOverride.value = value > 0 ? value : nil
  }
  
  @objc private func customUserAgentEditingEnded() {
    rewards.ledger.customUserAgent = customUserAgentTextField.text
  }
  
  private var numpadDismissalToolbar: UIToolbar {
    return UIToolbar().then {
      $0.items = [
        UIBarButtonItem(barButtonSystemItem: .flexibleSpace, target: nil, action: nil),
        UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(textFieldDismissed))
      ]
      $0.frame = CGRect(width: self.view.bounds.width, height: 44)
    }
  }
  
  @objc private func textFieldDismissed() {
    view.endEditing(true)
  }
  
  public override func viewDidLoad() {
    super.viewDidLoad()
    
    view.backgroundColor = .white
    
    title = "Rewards QA Settings"
    
    let isDefaultEnvironmentProd = AppConstants.buildChannel != .debug
    
    let override: EnvironmentOverride = EnvironmentOverride(rawValue: Preferences.Rewards.environmentOverride.value) ?? .none
    segmentedControl.selectedSegmentIndex = EnvironmentOverride.sortedCases.firstIndex(of: override) ?? 0
    segmentedControl.addTarget(self, action: #selector(environmentChanged), for: .valueChanged)
    reconcileTimeTextField.addTarget(self, action: #selector(reconcileTimeEditingEnded), for: .editingDidEnd)
    reconcileTimeTextField.frame = CGRect(x: 0, y: 0, width: 50, height: 32)
    reconcileTimeTextField.inputAccessoryView = numpadDismissalToolbar
    
    adsDismissalTextField.addTarget(self, action: #selector(adsDismissalEditingEnded), for: .editingDidEnd)
    adsDismissalTextField.frame = CGRect(x: 0, y: 0, width: 50, height: 32)
    adsDismissalTextField.inputAccessoryView = numpadDismissalToolbar
    adsDismissalTextField.text = "\(Preferences.Rewards.adsDurationOverride.value ?? 0)"
    
    customUserAgentTextField.addTarget(self, action: #selector(customUserAgentEditingEnded), for: .editingDidEnd)
    customUserAgentTextField.delegate = self
    customUserAgentTextField.frame = CGRect(x: 0, y: 0, width: 125, height: 32)
    customUserAgentTextField.inputAccessoryView = numpadDismissalToolbar
    customUserAgentTextField.text = rewards.ledger.customUserAgent
    
    KeyboardHelper.defaultHelper.addDelegate(self)
    
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
        header: .title("Ledger"),
        rows: [
          Row(text: "Is Debug", accessory: .switchToggle(value: BraveLedger.isDebug, { value in
            BraveLedger.isDebug = value
            BraveAds.isDebug = value
          })),
          Row(text: "Use Short Retries", accessory: .switchToggle(value: BraveLedger.useShortRetries, { value in
            BraveLedger.useShortRetries = value
          })),
          Row(text: "Reconcile Time", detailText: "Number of minutes between reconciles. 0 = No Override", accessory: .view(reconcileTimeTextField), cellClass: MultilineSubtitleCell.self),
          Row(text: "Custom User Agent", detailText: "Non-persistant. Empty = default", accessory: .view(customUserAgentTextField), cellClass: MultilineSubtitleCell.self)
        ]
      ),
      Section(
        header: .title("Wallet"),
        rows: [
          Row(text: "Recovery Key", detailText: obfuscatedPassphrase ?? "—", selection: {
            if let passphrase = self.rewards.ledger.walletPassphrase {
              let sheet = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
              sheet.popoverPresentationController?.sourceView = self.view
              sheet.popoverPresentationController?.sourceRect = self.view.bounds
              sheet.addAction(UIAlertAction(title: "Copy Wallet Passphrase", style: .default, handler: { _ in
                UIPasteboard.general.string = passphrase
              }))
              sheet.addAction(UIAlertAction(title: "Cancel", style: .cancel, handler: nil))
              self.present(sheet, animated: true, completion: nil)
            }
          }),
          Row(text: "Restore Wallet", selection: {
            self.tappedRestoreWallet()
          }, cellClass: ButtonCell.self)
        ]
      ),
      Section(
        header: .title("Ads"),
        rows: [
          Row(text: "Dismissal Timer", detailText: "Number of seconds before an ad is automatically dismissed. 0 = Default", accessory: .view(adsDismissalTextField), cellClass: MultilineSubtitleCell.self)
        ]
      ),
      Section(
        header: .title("Attestation Data"),
        rows: [
          Row(text: "Device Check Debugger", selection: {
            guard let paymentId = self.rewards.ledger.paymentId, !paymentId.isEmpty else {
              self.displayAlert(message: "Enable Rewards First")
              return
            }
            
            let debugController = QAAttestationDebugViewController(paymentId: paymentId)
            self.navigationController?.pushViewController(debugController, animated: true)
            
          }, cellClass: ButtonCell.self)
        ]
      ),
      Section(
        header: .title("Database"),
        rows: [
          Row(text: "Import Rewards Database", selection: {
            self.tappedImportRewardsDatabase()
          }, cellClass: ButtonCell.self),
          Row(text: "Export Rewards Database", selection: {
            self.tappedShareRewardsDatabase()
          }, cellClass: ButtonCell.self),
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
  
  private func displayAlert(title: String? = nil, message: String) {
    DispatchQueue.main.async {
      let alert = UIAlertController(title: title, message: message, preferredStyle: .alert)
      alert.addAction(UIAlertAction(title: "OK", style: .cancel, handler: nil))
      self.present(alert, animated: true, completion: nil)
    }
  }
  
  private func tappedImportRewardsDatabase() {
    let docPicker = UIDocumentPickerViewController(documentTypes: [String(kUTTypeData), String(kUTTypeDatabase)], in: .import)
    if #available(iOS 13.0, *) {
      docPicker.shouldShowFileExtensions = true
    }
    docPicker.delegate = self
    self.present(docPicker, animated: true)
  }
  
  private func tappedShareRewardsDatabase() {
    guard let appSupportPath = NSSearchPathForDirectoriesInDomains(.applicationSupportDirectory, .userDomainMask, true).first else { return }
    let dbPath = (appSupportPath as NSString).appendingPathComponent("ledger/Rewards.db")
    let activity = UIActivityViewController(activityItems: [URL(fileURLWithPath: dbPath)], applicationActivities: nil)
    if UIDevice.current.userInterfaceIdiom == .pad {
      activity.popoverPresentationController?.sourceView = view
    }
    self.present(activity, animated: true)
  }
  
  @objc private func tappedRestoreWallet() {
    let environment: String = {
      switch BraveLedger.environment {
      case .production: return "production"
      case .staging: return "staging"
      case .development: return "development"
      default: return "(unknown)"
      }
    }()
    let alert = UIAlertController(title: "Restore Wallet", message: "Enter the recovery key for the wallet (must be a \(environment) wallet). Your existing wallet will be overwritten.", preferredStyle: .alert)
    alert.addTextField { textField in
      textField.returnKeyType = .done
    }
    alert.addAction(UIAlertAction(title: "Cancel", style: .cancel, handler: nil))
    alert.addAction(UIAlertAction(title: "Restore", style: .default, handler: { action in
      guard let passphrase = alert.textFields?.first?.text else { return }
      self.rewards.ledger.recoverWallet(usingPassphrase: passphrase) { (error) in
        let message = error?.localizedDescription ?? "Restore Complete. Brave must be restarted to ensure expected Rewards behavior"
        let completionAlert = UIAlertController(title: "Restore Wallet", message: message, preferredStyle: .alert)
        if error == nil {
          DeviceCheckClient.resetDeviceEnrollment()
          completionAlert.addAction(UIAlertAction(title: "Exit Now", style: .destructive, handler: { _ in
            fatalError()
          }))
        }
        completionAlert.addAction(UIAlertAction(title: "OK", style: .default, handler: nil))
        alert.dismiss(animated: true) {
          self.present(completionAlert, animated: true)
        }
      }
    }))
    present(alert, animated: true)
  }
  
  private var obfuscatedPassphrase: String? {
    if let passphrase = self.rewards.ledger.walletPassphrase {
      let words = passphrase.split(separator: " ")
      var obfuscated = String(words.first ?? "")
      for i in 1..<min(words.count - 1, 4) {
        let word = String(words[i])
        obfuscated.append(" \((0..<word.count).map { _ in "•" }.joined())")
      }
      return obfuscated
    }
    return nil
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

extension QASettingsViewController: KeyboardHelperDelegate {
  public func keyboardHelper(_ keyboardHelper: KeyboardHelper, keyboardWillShowWithState state: KeyboardState) {
    self.tableView.contentInset = UIEdgeInsets(top: 0, left: 0, bottom: state.intersectionHeightForView(view), right: 0)
  }
  public func keyboardHelper(_ keyboardHelper: KeyboardHelper, keyboardWillHideWithState state: KeyboardState) {
    self.tableView.contentInset = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 0)
  }
  public func keyboardHelper(_ keyboardHelper: KeyboardHelper, keyboardDidShowWithState state: KeyboardState) {
  }
}

extension QASettingsViewController: UITextFieldDelegate {
  public func textFieldShouldReturn(_ textField: UITextField) -> Bool {
    textField.resignFirstResponder()
    return true
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

extension QASettingsViewController: UIDocumentPickerDelegate {
  public func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]) {
    guard let documentURL = urls.first, documentURL.pathExtension == "db" else { return }
    guard let appSupportPath = NSSearchPathForDirectoriesInDomains(.applicationSupportDirectory, .userDomainMask, true).first else { return }
    let dbPath = (appSupportPath as NSString).appendingPathComponent("ledger/Rewards.db")
    do {
      _ = try FileManager.default.replaceItemAt(URL(fileURLWithPath: dbPath), withItemAt: documentURL)
      if FileManager.default.fileExists(atPath: "\(dbPath)-journal") {
        try FileManager.default.removeItem(atPath: "\(dbPath)-journal")
      }
      let alert = UIAlertController(
        title: "Database Imported",
        message: "Brave must be restarted after importing a database for data to be read from it correctly.",
        preferredStyle: .alert
      )
      alert.addAction(UIAlertAction(title: "Exit Now", style: .destructive, handler: { _ in
        fatalError()
      }))
      alert.addAction(UIAlertAction(title: "Later…", style: .default, handler: nil))
      present(alert, animated: true)
    } catch {
      let alert = UIAlertController(title: "Failed To Import Database", message: error.localizedDescription, preferredStyle: .alert)
      alert.addAction(UIAlertAction(title: "OK", style: .default, handler: nil))
      present(alert, animated: true)
    }
  }
}
