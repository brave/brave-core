// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Foundation
import Static

import Shared
import BraveShared
import BraveCore
import BraveUI
import DeviceCheck
import Combine

class IPFSSettingsViewController: TableViewController {
  
  private let ipfsAPI: IpfsAPI
  
  init(ipfsAPI: IpfsAPI) {
    self.ipfsAPI = ipfsAPI
    super.init(style: .insetGrouped)
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  open override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    reloadSections()
  }
  
  private func reloadSections() {
    dataSource.sections.removeAll()
    dataSource.sections.append(
      Section(
        rows: [
          Row(
            text: Strings.BraveIPFS.nftGatewaySetting,
            detailText: self.ipfsAPI.nftIpfsGateway?.absoluteString,
            selection: { [unowned self] in
              let controller = IPFSCustomGatewayViewController(ipfsAPI: self.ipfsAPI)
              self.navigationController?.pushViewController(controller, animated: true)
            },
            accessory: .disclosureIndicator,
            cellClass: MultilineSubtitleCell.self),
        ],
        footer: .title(Strings.BraveIPFS.nftGatewayDescription))
    )
  }
  
}
