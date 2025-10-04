// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Static
import SwiftUI
import UIKit

class BraveVPNServerLocationCell: UITableViewCell, Cell {

  static let textAccessoryKey = "textAccessoryImageName"

  private let hostingController = UIHostingController(
    rootView: ServerLocationView(
      image: nil,
      title: "",
      titleAccessory: nil,
      subtitle: nil
    )
  ).then {
    $0.view.backgroundColor = .clear
  }

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .default, reuseIdentifier: reuseIdentifier)
    contentView.addSubview(hostingController.view)
    hostingController.view.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      hostingController.view.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 16.0),
      hostingController.view.bottomAnchor.constraint(
        equalTo: contentView.bottomAnchor,
        constant: -16.0
      ),
      hostingController.view.leadingAnchor.constraint(
        equalTo: contentView.leadingAnchor,
        constant: 16.0
      ),
      hostingController.view.trailingAnchor.constraint(
        equalTo: contentView.trailingAnchor,
        constant: -16.0
      ),
    ])
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func configure(row: Row) {
    accessoryType = row.accessory.type
    var titleAccessory: UIImage?
    if let imageName = row.context?[BraveVPNServerLocationCell.textAccessoryKey] as? String {
      titleAccessory = UIImage(braveSystemNamed: imageName)
    }
    hostingController.rootView = ServerLocationView(
      image: row.image,
      title: row.text ?? "",
      titleAccessory: titleAccessory,
      subtitle: row.detailText
    )
  }

  private struct ServerLocationView: View {
    let image: UIImage?
    let title: String
    let titleAccessory: UIImage?
    let subtitle: String?

    var body: some View {
      HStack {
        if let image {
          Image(uiImage: image)
        }
        BraveVPNLabelView(
          title: title,
          titleAccessory: titleAccessory,
          subtitle: subtitle
        )
        Spacer()
      }
    }
  }
}
