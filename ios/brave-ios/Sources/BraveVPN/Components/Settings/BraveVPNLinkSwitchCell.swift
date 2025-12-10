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

  static let textAccessoryKey = "textAccessoryImageName"

  public override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .default, reuseIdentifier: reuseIdentifier)
  }

  public required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  public func configure(row: Row) {
    guard case .view(let view) = row.accessory, let switchView = view as? BraveVPNLinkSwitchView
    else {
      fatalError("row.accessory must be of type: BraveVPNLinkSwitchView")
    }

    var titleAccessory: UIImage?
    if let imageName = row.context?[BraveVPNLinkSwitchCell.textAccessoryKey] as? String {
      titleAccessory = UIImage(braveSystemNamed: imageName)
    }

    self.contentConfiguration = UIHostingConfiguration {
      VPNToggleView(
        title: row.text ?? "",
        titleAccessory: titleAccessory,
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
      .padding(.vertical, 8)
      .background {
        Color.clear
      }
    }
  }

  private struct VPNToggleView: View {
    let title: String
    let titleAccessory: UIImage?
    var subtitle: String?
    var openURL: ((URL) -> Void)?
    @Binding var toggle: Bool

    public init(
      title: String,
      titleAccessory: UIImage? = nil,
      subtitle: String? = nil,
      openURL: ((URL) -> Void)? = nil,
      toggle: Binding<Bool>
    ) {
      self.title = title
      self.titleAccessory = titleAccessory
      self.subtitle = subtitle
      self.openURL = openURL
      _toggle = toggle
    }

    public var body: some View {
      Toggle(isOn: $toggle) {
        BraveVPNLabelView(
          title: title,
          titleAccessory: titleAccessory,
          subtitle: subtitle
        )
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
