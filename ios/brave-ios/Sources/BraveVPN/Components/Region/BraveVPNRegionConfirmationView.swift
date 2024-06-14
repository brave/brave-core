// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveUI
import Shared
import SwiftUI

public struct BraveVPNRegionConfirmationView: View {
  var regionCountry: String?
  var regionCity: String?
  var regionCountryFlag: Image?

  public init(regionCountry: String?, regionCity: String?, regionCountryISOCode: String?) {
    self.regionCountry = regionCountry
    self.regionCity = regionCity
    self.regionCountryFlag = regionCountryISOCode?.regionFlag ?? Image(braveSystemName: "leo.globe")
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
        if let regionCountryFlag = regionCountryFlag {
          regionCountryFlag
            .frame(width: 32, height: 32)
            .padding(4)
            .overlay(
              RoundedRectangle(cornerRadius: 12.0, style: .continuous)
                .strokeBorder(Color(.lightGray), lineWidth: 1.0)
            )
        }
        VStack(alignment: .leading) {
          if let regionCountry = regionCountry {
            Text(regionCountry)
              .font(.headline)
              .foregroundStyle(Color(braveSystemName: .textPrimary))
          }
          if let regionCity = regionCity {
            Text(regionCity)
              .font(.subheadline)
              .foregroundStyle(Color(braveSystemName: .textSecondary))
          }
        }
        Spacer()
      }
    }
    .padding(48)
    .background(Color(braveSystemName: .containerBackgroundMobile))
    .frame(maxWidth: 350)
  }
}

struct BraveVPNRegionConfirmationView_Previews: PreviewProvider {
  static var previews: some View {
    BraveVPNRegionConfirmationView(
      regionCountry: "Canada",
      regionCity: "Saskatchewan",
      regionCountryISOCode: "CA"
    )
  }
}

public struct BraveVPNRegionConfirmationContentView: UIViewControllerRepresentable {
  @Binding
  var isPresented: Bool

  var regionCountry: String?
  var regionCity: String?
  var regionCountryISOCode: String?

  public init(
    isPresented: Binding<Bool>,
    regionCountry: String?,
    regionCity: String? = nil,
    regionCountryISOCode: String? = nil
  ) {
    _isPresented = isPresented
    self.regionCountry = regionCountry
    self.regionCity = regionCity
    self.regionCountryISOCode = regionCountryISOCode
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
        rootView: BraveVPNRegionConfirmationView(
          regionCountry: regionCountry,
          regionCity: regionCity,
          regionCountryISOCode: regionCountryISOCode
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
