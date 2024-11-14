// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import UIKit

class RewardsInternalsAutoContributeController: UITableViewController {

  let rewardsAPI: BraveRewardsAPI
  private var publishers: [BraveCore.BraveRewards.PublisherInfo] = []
  private let percentFormatter = NumberFormatter().then {
    $0.numberStyle = .percent
  }

  private let dateFormatter = DateFormatter().then {
    $0.dateStyle = .short
    $0.timeStyle = .none
  }

  init(rewardsAPI: BraveRewardsAPI) {
    self.rewardsAPI = rewardsAPI
    super.init(style: .grouped)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    tableView.register(AutoContributePublisherCell.self)
    title = "Auto-Contribute"

    rewardsAPI.listAutoContributePublishers { [weak self] list in
      guard let self = self else { return }
      self.publishers = list
      self.tableView.reloadData()
    }
  }

  override func numberOfSections(in tableView: UITableView) -> Int {
    2
  }

  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    switch section {
    case 0: return 1
    case 1: return publishers.count
    default: return 0
    }
  }

  override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String?
  {
    if section == 1 {
      return "Supported Verified Publishers"
    }
    return nil
  }

  override func tableView(
    _ tableView: UITableView,
    cellForRowAt indexPath: IndexPath
  ) -> UITableViewCell {
    let cell = tableView.dequeueReusableCell(for: indexPath) as AutoContributePublisherCell
    switch indexPath.section {
    case 0:
      cell.textLabel?.text = "Next Contribution Date"
      cell.detailTextLabel?.text = "–"
      return cell
    case 1:
      guard let publisher = publishers[safe: indexPath.item] else { return cell }
      cell.textLabel?.text = publisher.displayName
      cell.detailTextLabel?.text = percentFormatter.string(
        from: NSNumber(value: Double(publisher.percent) / 100.0)
      )
      return cell
    default:
      fatalError()
    }
  }
}

private class AutoContributePublisherCell: UITableViewCell, TableViewReusable {
  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .value1, reuseIdentifier: reuseIdentifier)
  }
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
