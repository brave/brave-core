// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Static
import SwiftUI
import UIKit

class BraveVPNLinkSwitchView: UIView {

  let isOn: () -> Bool
  let valueChange: (Bool) -> Void
  let openURL: ((URL) -> Void)?

  init(isOn: @escaping () -> Bool, valueChange: @escaping (Bool) -> Void, openURL: ((URL) -> Void)?)
  {
    self.isOn = isOn
    self.valueChange = valueChange
    self.openURL = openURL
    super.init(frame: .zero)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}

class BraveVPNLinkSwitchCell: UITableViewCell, Cell {

  private let hostingController = UIHostingController(
    rootView: VPNToggleView(title: "", toggle: .constant(false))
  ).then {
    $0.view.backgroundColor = .clear
  }

  public override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
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

  public required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  public func configure(row: Row) {
    guard case .view(let view) = row.accessory, let switchView = view as? BraveVPNLinkSwitchView
    else {
      fatalError("row.accessory must be of type: BraveVPNLinkSwitchView")
    }

    hostingController.rootView = VPNToggleView(
      title: row.text ?? "",
      subtitle: row.detailText,
      openURL: switchView.openURL,
      toggle: .init(
        get: { [weak switchView] in
          switchView?.isOn() ?? false
        },
        set: { [weak switchView] in
          switchView?.valueChange($0)
        }
      )
    )
  }

  private struct VPNToggleView: View {
    let title: String
    var subtitle: String?
    var openURL: ((URL) -> Void)?
    @Binding var toggle: Bool

    public init(
      title: String,
      subtitle: String? = nil,
      openURL: ((URL) -> Void)? = nil,
      toggle: Binding<Bool>
    ) {
      self.title = title
      self.subtitle = subtitle
      self.openURL = openURL
      _toggle = toggle
    }

    public var body: some View {
      Toggle(isOn: $toggle) {
        LabelView(title: title, subtitle: subtitle)
          .environment(
            \.openURL,
            OpenURLAction { url in
              openURL?(url)
              return .handled
            }
          )
      }
      .toggleStyle(SwitchToggleStyle(tint: .accentColor))
    }
  }
}
