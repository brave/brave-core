// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveUI
import Shared
import SwiftUI

public struct BraveVPNRegionChangedView: View {
  var regionTitle: String
  var regionSubtitle: String

  public init(regionTitle: String, regionSubtitle: String) {
    self.regionTitle = regionTitle
    self.regionSubtitle = regionSubtitle
  }

  public var body: some View {
    VStack(spacing: 24) {
      Image("region-change-banner", bundle: .module)
        .resizable()
        .frame(width: 96, height: 96)
      Text("VPN Region Changed")
        .multilineTextAlignment(.center)
        .font(.headline)
        .foregroundStyle(Color(braveSystemName: .textPrimary))
      HStack {
        "BR".regionFlag
          .frame(width: 32, height: 32)
          .padding(4)
          .overlay(
            RoundedRectangle(cornerRadius: 4.0, style: .continuous)
              .strokeBorder(Color(.lightGray), lineWidth: 1.0)
          )
        VStack(alignment: .leading) {
          Text(regionTitle)
            .font(.headline)
            .foregroundStyle(Color(braveSystemName: .textPrimary))
          Text(regionSubtitle)
            .font(.subheadline)
            .foregroundStyle(Color(braveSystemName: .textSecondary))
        }
        Spacer()
      }
    }
    .padding(48)
    .background(Color(braveSystemName: .containerBackgroundMobile))
  }
}

struct BraveVPNRegionChangedView_Previews: PreviewProvider {
  static var previews: some View {
    BraveVPNRegionChangedView(regionTitle: "Brazil", regionSubtitle: "Rio de Janeiro")
  }
}

public struct BraveVPNRegionChangedContentView: UIViewControllerRepresentable {
  @Binding
  var isPresented: Bool

  var regionTitle: String
  var regionSubtitle: String

  public init(
    isPresented: Binding<Bool>,
    regionTitle: String,
    regionSubtitle: String
  ) {
    _isPresented = isPresented
    self.regionTitle = regionTitle
    self.regionSubtitle = regionSubtitle
  }

  public func makeUIViewController(context: Context) -> UIViewController {
    .init()
  }

  public func updateUIViewController(_ uiViewController: UIViewController, context: Context) {
    if isPresented {
      if uiViewController.presentedViewController != nil {
        return
      }

      let controller = PopupViewController(
        rootView: BraveVPNRegionChangedView(
          regionTitle: regionTitle,
          regionSubtitle: regionSubtitle
        )
      )

      context.coordinator.presentedViewController = .init(controller)
      uiViewController.present(controller, animated: true)
    } else {
      if let presentedViewController = context.coordinator.presentedViewController?.value,
        presentedViewController == uiViewController.presentedViewController
      {
        uiViewController.presentedViewController?.dismiss(animated: true)
      }
    }
  }

  public class Coordinator {
    var presentedViewController: WeakRef<UIViewController>?
  }

  public func makeCoordinator() -> Coordinator {
    Coordinator()
  }
}
