// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveCore
import Static
import DeviceCheck
import Shared
import BraveUI

private class WarningCell: MultilineSubtitleCell {
  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: reuseIdentifier)

    textLabel?.font = .systemFont(ofSize: 16.0, weight: .semibold)
    detailTextLabel?.font = .systemFont(ofSize: 15.0)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}

/// A place where all rewards debugging information will live.
class RewardsInternalsViewController: TableViewController {

  private let rewardsAPI: BraveRewardsAPI
  private var internalsInfo: BraveCore.BraveRewards.RewardsInternalsInfo?

  init(rewardsAPI: BraveRewardsAPI) {
    self.rewardsAPI = rewardsAPI
    super.init(style: .insetGrouped)
    rewardsAPI.rewardsInternalInfo { [weak self] info in
      self?.internalsInfo = info
      self?.reloadSections()
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    title = Strings.RewardsInternals.title

    navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .action, target: self, action: #selector(tappedShare)).then {
      $0.accessibilityLabel = Strings.RewardsInternals.shareInternalsTitle
    }

    reloadSections()
  }

  @objc private func tappedShare() {
    let controller = RewardsInternalsShareController(rewardsAPI: self.rewardsAPI, initiallySelectedSharables: RewardsInternalsSharable.default)
    let container = UINavigationController(rootViewController: controller)
    present(container, animated: true)
  }

  func reloadSections() {
    guard let info = internalsInfo else { return }

    let dateFormatter = DateFormatter().then {
      $0.dateStyle = .short
    }

    let sections: [Static.Section] = [
      .init(
        rows: [
          Row(text: Strings.RewardsInternals.sharingWarningTitle, detailText: Strings.RewardsInternals.sharingWarningMessage, cellClass: WarningCell.self)
        ]
      ),
      .init(
        header: .title(Strings.RewardsInternals.walletInfoHeader),
        rows: [
          Row(text: Strings.RewardsInternals.keyInfoSeed, detailText: "\(info.isKeyInfoSeedValid ? Strings.RewardsInternals.valid : Strings.RewardsInternals.invalid)"),
          Row(
            text: Strings.RewardsInternals.walletPaymentID, detailText: info.paymentId,
            selection: { [unowned self] in
              if let index = self.dataSource.sections[safe: 1]?.rows.firstIndex(where: { $0.cellClass == PaymentIDCell.self }),
                let cell = self.tableView.cellForRow(at: IndexPath(item: index, section: 1)) as? PaymentIDCell {
                cell.showMenu()
              }
            }, cellClass: PaymentIDCell.self),
          Row(text: Strings.RewardsInternals.walletCreationDate, detailText: dateFormatter.string(from: Date(timeIntervalSince1970: TimeInterval(info.bootStamp)))),
        ]
      ),
      .init(
        header: .title(Strings.RewardsInternals.deviceInfoHeader),
        rows: [
          Row(text: Strings.RewardsInternals.status, detailText: DCDevice.current.isSupported ? Strings.RewardsInternals.supported : Strings.RewardsInternals.notSupported),
          Row(text: Strings.RewardsInternals.enrollmentState, detailText: DeviceCheckClient.isDeviceEnrolled() ? Strings.RewardsInternals.enrolled : Strings.RewardsInternals.notEnrolled),
        ]
      ),
    ]

    dataSource.sections = sections
  }
}

/// A file generator that creates a JSON file containing basic information such as wallet info, device info
/// and balance info
struct RewardsInternalsBasicInfoGenerator: RewardsInternalsFileGenerator {
  func generateFiles(at path: String, using builder: RewardsInternalsSharableBuilder, completion: @escaping (Error?) -> Void) {
    // Only 1 file to make here
    builder.rewardsAPI.rewardsInternalInfo { internals in
      guard let info = internals else {
        completion(RewardsInternalsSharableError.rewardsInternalsUnavailable)
        return
      }
      
      let data: [String: Any] = [
        "Rewards Profile Info": [
          "Key Info Seed": "\(info.isKeyInfoSeedValid ? "Valid" : "Invalid")",
          "Rewards Payment ID": info.paymentId,
          "Rewards Profile Creation Date": builder.dateFormatter.string(from: Date(timeIntervalSince1970: TimeInterval(info.bootStamp))),
        ],
        "Device Info": [
          "DeviceCheck Status": DCDevice.current.isSupported ? "Supported" : "Not supported",
          "DeviceCheck Enrollment State": DeviceCheckClient.isDeviceEnrolled() ? "Enrolled" : "Not enrolled",
          "OS": "\(UIDevice.current.systemName) \(UIDevice.current.systemVersion)",
          "Model": UIDevice.current.model,
        ],
      ]
      
      do {
        try builder.writeJSON(from: data, named: "basic", at: path)
        completion(nil)
      } catch {
        completion(error)
      }
    }
  }
}

private class PaymentIDCell: SubtitleCell {
  override func canPerformAction(_ action: Selector, withSender sender: Any?) -> Bool {
    action == #selector(copy(_:))
  }

  override func copy(_ sender: Any?) {
    if let text = self.detailTextLabel?.text {
      UIPasteboard.general.string = text
    }
  }

  override var canBecomeFirstResponder: Bool {
    true
  }

  func showMenu() {
    becomeFirstResponder()
    UIMenuController.shared.showMenu(from: self, rect: bounds)
  }
}
