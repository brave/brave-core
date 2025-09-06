// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Shared
import Static
import SwiftUI
import UIKit

class BraveVPNSmartProxyCellView: UIView {
  weak var settingsController: BraveVPNSettingsViewController?

  init(settingsController: BraveVPNSettingsViewController) {
    self.settingsController = settingsController
    super.init(frame: .zero)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}

class BraveVPNSmartProxyCell: UITableViewCell, Cell {

  private let hostingController = UIHostingController(
    rootView: VPNSmartProxyView(
      countryFlagIcon: nil,
      title: "",
      subtitle: "",
      isSmartProxyAvailable: false
    )
  ).then {
    $0.view.backgroundColor = .clear
  }

  public override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .default, reuseIdentifier: reuseIdentifier)
  }

  public required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  public func configure(row: Row) {
    guard case .view(let view) = row.accessory,
      let parentController = (view as? BraveVPNSmartProxyCellView)?.settingsController
    else {
      assertionFailure("row.accessory must be of type: BraveVPNSmartProxyCellView")
      return
    }

    if hostingController.parent != parentController {
      if hostingController.parent != nil {
        hostingController.willMove(toParent: nil)
        hostingController.view.removeFromSuperview()
        hostingController.removeFromParent()
      }

      parentController.addChild(hostingController)
      hostingController.didMove(toParent: parentController)

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

    self.accessoryType = .disclosureIndicator
    hostingController.rootView = VPNSmartProxyView(
      countryFlagIcon: row.image,
      title: row.text ?? "",
      subtitle: row.detailText ?? "",
      isSmartProxyAvailable: row.context?["smartRoutingProxyState"] as? Bool ?? false
    )
  }

  override func prepareForReuse() {
    super.prepareForReuse()

    if hostingController.parent != nil {
      hostingController.willMove(toParent: nil)
      hostingController.view.removeFromSuperview()
      hostingController.removeFromParent()
    }
  }

  private struct VPNSmartProxyView: View {
    let countryFlagIcon: UIImage?
    let title: String
    let subtitle: String
    let isSmartProxyAvailable: Bool

    @State
    private var showPopover = false

    public var body: some View {
      HStack {
        if let countryFlagIcon {
          Image(uiImage: countryFlagIcon)
        }

        VStack {
          HStack {
            Text(title)
              .font(.body)
              .foregroundStyle(Color(braveSystemName: .textPrimary))
              .fixedSize(horizontal: false, vertical: true)

            if isSmartProxyAvailable {
              Button {
                showPopover.toggle()
              } label: {
                Label {
                  Text(Strings.VPN.smartProxyPopoverTitle)
                } icon: {
                  Image(braveSystemName: "leo.smart.proxy-routing")
                    .foregroundStyle(Color(braveSystemName: .iconDefault))
                    .padding(4.0)
                    .background(Color(braveSystemName: .containerHighlight))
                    .clipShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
                }
                .labelStyle(.iconOnly)
              }
              .bravePopover(isPresented: $showPopover, arrowDirection: [.down]) {
                Text(Strings.VPN.smartProxyPopoverTitle)
                  .font(.subheadline)
                  .foregroundStyle(Color(braveSystemName: .textTertiary))
                  .fixedSize(horizontal: false, vertical: true)
                  .padding()
              }
            }
          }
          .frame(maxWidth: .infinity, alignment: .leading)

          Text(subtitle)
            .font(.footnote)
            .foregroundStyle(Color(braveSystemName: .textSecondary))
            .fixedSize(horizontal: false, vertical: true)
            .frame(maxWidth: .infinity, alignment: .leading)
        }
      }
    }
  }
}
