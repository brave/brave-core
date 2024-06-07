// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
    VStack {
      VStack {
        Image("region-change-banner", bundle: .module)
          .resizable()
          .frame(width: 96, height: 96)
        Text("example")
          .multilineTextAlignment(.center)
          .padding(.horizontal, 25)
      }
      .padding(.bottom, 25)
      Text(regionTitle)
        .font(.footnote)
        .multilineTextAlignment(.center)
        .foregroundColor(Color(.secondaryBraveLabel))
        .padding(.horizontal, 25)
        .padding(.bottom, 25)
    }
    .padding()
    .background(Color(.secondaryBraveBackground).ignoresSafeArea())
  }
}

struct BraveVPNRegionChangedView_Previews: PreviewProvider {
  static var previews: some View {
    BraveVPNRegionChangedView(regionTitle: "VPN Region Changed", regionSubtitle: "Rio Brazil")
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
        rootView: BraveVPNRegionChangedView(regionTitle: regionTitle, regionSubtitle: regionSubtitle)
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
