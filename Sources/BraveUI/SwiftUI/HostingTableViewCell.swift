/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import SwiftUI

/// A `UITableViewCell` that houses a SwiftUI view hierarchy.
///
/// In most cases you want `List` or `Form`, but for situations that require more in-depth
/// `UITableView` customization, you can use this to display simple SwiftUI View's inside a cell.
///
/// - warning: that only *static* SwiftUI View's should be placed within this cell. This implementation
/// is restricted to simply displaying data. Any actions should be handled by the controller handling
/// the `UITableView`. Anything that may adjust the size of the cell from within the `View` will not
/// adjust the containing `HostingTableViewCell`.
open class HostingTableViewCell<Content: View>: UITableViewCell {
  public let hostingController = UIHostingController<Content?>(rootView: nil)

  public override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: reuseIdentifier)

    hostingController.view.backgroundColor = .clear
    contentView.addSubview(hostingController.view)
    hostingController.view.snp.makeConstraints {
      $0.edges.equalTo(contentView)
    }
  }

  @available(*, unavailable)
  public required init(coder: NSCoder) {
    fatalError()
  }

  public func setRootView(_ rootView: Content, parent: UIViewController) {
    if hostingController.parent != parent {
      parent.addChild(hostingController)
      hostingController.didMove(toParent: parent)
    }
    hostingController.rootView = rootView
    hostingController.view.invalidateIntrinsicContentSize()
  }
}
