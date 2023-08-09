/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import Storage

struct SiteTableViewControllerUX {
  static let headerHeight = CGFloat(32)
  static let rowHeight = CGFloat(44)
  static let headerFont = UIFont.systemFont(ofSize: 12, weight: UIFont.Weight.medium)
  static let headerTextMargin = CGFloat(16)
}

class SiteTableViewHeader: UITableViewHeaderFooterView {
  let titleLabel = UILabel()

  override var textLabel: UILabel? {
    return titleLabel
  }

  override init(reuseIdentifier: String?) {
    super.init(reuseIdentifier: reuseIdentifier)

    titleLabel.font = DynamicFontHelper.defaultHelper.DeviceFontMediumBold
    titleLabel.textColor = .braveLabel

    contentView.addSubview(titleLabel)

    // A table view will initialize the header with CGSizeZero before applying the actual size. Hence, the label's constraints
    // must not impose a minimum width on the content view.
    titleLabel.snp.makeConstraints { make in
      make.left.equalTo(contentView).offset(SiteTableViewControllerUX.headerTextMargin).priority(999)
      make.right.equalTo(contentView).offset(-SiteTableViewControllerUX.headerTextMargin).priority(999)
      make.left.greaterThanOrEqualTo(contentView)  // Fallback for when the left space constraint breaks
      make.right.lessThanOrEqualTo(contentView)  // Fallback for when the right space constraint breaks
      make.centerY.equalTo(contentView)
    }
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}

/**
 * Provides base shared functionality for site rows and headers.
 */
@objcMembers
public class SiteTableViewController: LoadingViewController, UITableViewDelegate, UITableViewDataSource {
  fileprivate let CellIdentifier = "CellIdentifier"
  fileprivate let HeaderIdentifier = "HeaderIdentifier"
  var profile: Profile! {
    didSet {
      reloadData()
    }
  }

  var data = [Site]()
  var tableView = UITableView()

  override public func viewDidLoad() {
    super.viewDidLoad()

    view.addSubview(tableView)
    tableView.snp.makeConstraints { make in
      make.edges.equalTo(self.view)
      return
    }

    tableView.do {
      $0.delegate = self
      $0.dataSource = self
      $0.register(SiteTableViewCell.self, forCellReuseIdentifier: CellIdentifier)
      $0.register(SiteTableViewHeader.self, forHeaderFooterViewReuseIdentifier: HeaderIdentifier)
      $0.layoutMargins = .zero
      $0.keyboardDismissMode = .onDrag
      $0.backgroundColor = .secondaryBraveBackground
      $0.separatorColor = .braveSeparator
      $0.accessibilityIdentifier = "SiteTable"
      $0.cellLayoutMarginsFollowReadableWidth = false
      $0.sectionHeaderTopPadding = 5
    }

    // Set an empty footer to prevent empty cells from appearing in the list.
    tableView.tableFooterView = UIView()
  }

  deinit {
    // The view might outlive this view controller thanks to animations;
    // explicitly nil out its references to us to avoid crashes. Bug 1218826.
    tableView.dataSource = nil
    tableView.delegate = nil
  }

  func reloadData() {
    self.tableView.reloadData()
  }

  public func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return data.count
  }

  public func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = tableView.dequeueReusableCell(withIdentifier: CellIdentifier, for: indexPath)
    if self.tableView(tableView, hasFullWidthSeparatorForRowAtIndexPath: indexPath) {
      cell.separatorInset = .zero
    }
    return cell
  }

  public func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    return tableView.dequeueReusableHeaderFooterView(withIdentifier: HeaderIdentifier)
  }

  public func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
    return SiteTableViewControllerUX.headerHeight
  }

  public func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
    return SiteTableViewControllerUX.rowHeight
  }

  public func tableView(_ tableView: UITableView, hasFullWidthSeparatorForRowAtIndexPath indexPath: IndexPath) -> Bool {
    return false
  }

  public func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
    true
  }
}
