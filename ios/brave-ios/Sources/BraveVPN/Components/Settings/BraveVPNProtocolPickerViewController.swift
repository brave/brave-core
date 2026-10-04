// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import GuardianConnect
import Lottie
import NetworkExtension
import Shared
import UIKit

class BraveVPNProtocolPickerViewController: BraveVPNPickerViewController {

  private let tunnelProtocolList: [TransportProtocol]

  /// This group monitors vpn connection status.
  private var dispatchGroup: DispatchGroup?
  private var vpnRegionChangeSuccess = false

  override init() {
    self.tunnelProtocolList = [.wireGuard, .ikEv2]

    super.init()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) { fatalError() }

  override func viewDidLoad() {
    title = Strings.VPN.protocolPickerTitle

    tableView.delegate = self
    tableView.dataSource = self

    super.viewDidLoad()
  }

  override func vpnConfigChanged(notification: NSNotification) {
    guard let connection = notification.object as? NEVPNConnection else { return }

    if connection.status == .connected {
      dispatchGroup?.leave()
      self.vpnRegionChangeSuccess = true
      dispatchGroup = nil
    }
  }
}

// MARK: - UITableView Data Source & Delegate

extension BraveVPNProtocolPickerViewController: UITableViewDelegate, UITableViewDataSource {

  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    tunnelProtocolList.count
  }

  func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
    return Strings.VPN.protocolPickerDescription
  }

  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = tableView.dequeueReusableCell(for: indexPath) as VPNRegionCell
    cell.accessoryType = .none

    guard let tunnelProtocol = tunnelProtocolList[safe: indexPath.row] else { return cell }
    cell.textLabel?.text = GRDTransportProtocol.prettyTransportProtocolString(for: tunnelProtocol)

    let activeProtocolOption = GRDTransportProtocol.getUserPreferredTransportProtocol()

    if activeProtocolOption == tunnelProtocol {
      cell.accessoryType = .checkmark
    }

    return cell
  }

  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)

    guard let tunnelProtocol = tunnelProtocolList[safe: indexPath.row] else { return }

    let activeProtocolOption = GRDTransportProtocol.getUserPreferredTransportProtocol()

    // Same option is selected do nothing
    if activeProtocolOption == tunnelProtocol {
      return
    }

    isLoading = true

    BraveVPN.changePreferredTransportProtocol(with: tunnelProtocol) { [weak self] success in
      guard let self else { return }

      DispatchQueue.main.async {
        self.isLoading = false

        if success {
          self.dismiss(animated: true) {
            self.showSuccessAlert(text: Strings.VPN.protocolSwitchSuccessPopupText)
          }
        } else {
          self.showErrorAlert(
            title: Strings.VPN.protocolPickerErrorTitle,
            message: Strings.VPN.protocolPickerErrorMessage
          )
        }
      }
    }
  }
}
