// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveUI
import DesignSystem
import Shared
import SwiftUI

public enum VPNChurnPromoType {
  case autoRenewSoonExpire
  case autoRenewDiscount
  case autoRenewFreeMonth
  case updateBillingSoonExpire
  case updateBillingExpired
  case subscribeDiscount
  case subscribeVPNProtection
  case subscribeAllDevices

  var promoImage: String {
    switch self {
    case .autoRenewSoonExpire:
      return "auto_renew _soon_image"
    case .autoRenewDiscount:
      return "auto_renew _discount_image"
    case .autoRenewFreeMonth:
      return "auto_renew _free_image"
    case .updateBillingSoonExpire, .updateBillingExpired:
      return "update_billing_expired"
    case .subscribeDiscount:
      return "auto_renew _soon_image"
    case .subscribeVPNProtection:
      return "subscribe_protection_image"
    case .subscribeAllDevices:
      return "subscribe_all-devices_image"
    }
  }

  var title: String {
    switch self {
    case .autoRenewSoonExpire:
      return Strings.VPN.autoRenewSoonExpirePopOverTitle
    case .autoRenewDiscount:
      return Strings.VPN.autoRenewDiscountPopOverTitle
    case .autoRenewFreeMonth:
      return Strings.VPN.autoRenewFreeMonthPopOverTitle
    case .updateBillingSoonExpire:
      return Strings.VPN.updateBillingSoonExpirePopOverTitle
    case .updateBillingExpired:
      return Strings.VPN.updateBillingExpiredPopOverTitle
    case .subscribeDiscount:
      return Strings.VPN.subscribeVPNDiscountPopOverTitle
    case .subscribeVPNProtection:
      return Strings.VPN.subscribeVPNProtectionPopOverTitle
    case .subscribeAllDevices:
      return Strings.VPN.subscribeVPNAllDevicesPopOverTitle
    }
  }

  var description: String? {
    switch self {
    case .autoRenewSoonExpire:
      return Strings.VPN.autoRenewSoonExpirePopOverDescription
    case .updateBillingSoonExpire:
      return Strings.VPN.updateBillingSoonExpirePopOverDescription
    case .updateBillingExpired:
      return Strings.VPN.updateBillingExpiredPopOverDescription
    case .subscribeVPNProtection:
      return Strings.VPN.subscribeVPNProtectionPopOverDescription
    case .subscribeAllDevices:
      return Strings.VPN.subscribeVPNAllDevicesPopOverDescription
    default:
      return nil
    }
  }

  var subDescription: String? {
    switch self {
    case .autoRenewSoonExpire:
      return Strings.VPN.autoReneSoonExpirePopOverSubDescription
    case .subscribeVPNProtection, .subscribeAllDevices:
      return Strings.VPN.subscribeVPNPopOverSubDescription
    default:
      return nil
    }
  }

  var buttonTitle: String {
    switch self {
    case .autoRenewSoonExpire, .autoRenewDiscount, .autoRenewFreeMonth:
      return Strings.VPN.autoRenewActionButtonTitle
    case .updateBillingSoonExpire, .updateBillingExpired:
      return Strings.VPN.updatePaymentActionButtonTitle
    case .subscribeDiscount, .subscribeVPNProtection, .subscribeAllDevices:
      return Strings.VPN.subscribeVPNActionButtonTitle
    }
  }
}

public struct VPNChurnPromoView: View {
  @Environment(\.presentationMode) @Binding private var presentationMode
  @State private var height: CGFloat?

  public var renewAction: (() -> Void)?

  public var churnPromoType: VPNChurnPromoType

  private let descriptionItems = [
    Strings.VPN.popupCheckboxBlockAdsAlternate,
    Strings.VPN.popupCheckmarkSecureConnections,
    Strings.VPN.popupCheckboxFastAlternate,
    Strings.VPN.popupCheckmark247Support,
  ]

  public init(churnPromoType: VPNChurnPromoType) {
    self.churnPromoType = churnPromoType
  }

  public var body: some View {
    ScrollView {
      VStack(spacing: 24) {
        headerView
        detailView
          .padding(.bottom, 8)
        footerView
      }
      .background {
        GeometryReader { proxy in
          Color.clear
            .onAppear { height = proxy.size.height }
            .onChange(of: proxy.size.height) { newValue in
              height = newValue
            }
        }
      }
      .padding(.horizontal, 32)
    }
    .background(Color(.braveBackground))
    .frame(maxWidth: BraveUX.baseDimensionValue, maxHeight: height)
    .overlay {
      Button {
        presentationMode.dismiss()
      } label: {
        Image(braveSystemName: "leo.close")
          .renderingMode(.template)
          .foregroundColor(Color(.bravePrimary))
      }
      .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topTrailing)
      .padding([.top, .trailing], 10)
    }
    .padding(.vertical, 16)
    .osAvailabilityModifiers { content in
      #if compiler(>=5.8)
      if #available(iOS 16.4, *) {
        content
          .scrollBounceBehavior(.basedOnSize)
      } else {
        content
          .introspectScrollView { scrollView in
            scrollView.alwaysBounceVertical = false
          }
      }
      #else
      content
        .introspectScrollView { scrollView in
          scrollView.alwaysBounceVertical = false
        }
      #endif
    }
  }

  private var headerView: some View {
    VStack(spacing: 24) {
      Image(churnPromoType.promoImage, bundle: .module)

      Text(churnPromoType.title)
        .font(.title)
        .multilineTextAlignment(.center)
    }
  }

  @ViewBuilder
  private var detailView: some View {
    switch churnPromoType {
    case .autoRenewSoonExpire, .subscribeVPNProtection, .subscribeAllDevices:
      let description = churnPromoType.description ?? ""
      let subDescription = churnPromoType.subDescription ?? ""

      VStack(spacing: 24) {
        Text(description)
          .font(.title3)
          .multilineTextAlignment(.center)
        Text(subDescription)
          .font(.callout)
          .multilineTextAlignment(.center)
      }
    case .autoRenewDiscount, .autoRenewFreeMonth, .subscribeDiscount:
      VStack(alignment: .leading, spacing: 8) {
        ForEach(descriptionItems, id: \.self) { itemDescription in
          HStack(spacing: 8) {
            Image(sharedName: "vpn_checkmark_popup")
              .renderingMode(.template)
              .foregroundColor(Color(.red))
              .frame(alignment: .leading)
            Text(itemDescription)
              .multilineTextAlignment(.leading)
              .foregroundColor(Color(.bravePrimary))
              .fixedSize(horizontal: false, vertical: true)
          }
        }
      }
    case .updateBillingExpired, .updateBillingSoonExpire:
      let description = churnPromoType.description ?? ""

      Text(description)
        .font(.title3)
        .multilineTextAlignment(.center)
    }
  }

  private var footerView: some View {
    VStack(spacing: 24) {
      Button {
        renewAction?()
        presentationMode.dismiss()
      } label: {
        Text(churnPromoType.buttonTitle)
          .padding(.vertical, 4)
          .frame(maxWidth: .infinity)
      }
      .buttonStyle(BraveFilledButtonStyle(size: .large))

      HStack(spacing: 8) {
        Text(Strings.VPN.poweredBy)
          .font(.footnote)
          .foregroundColor(Color(.secondaryBraveLabel))
          .multilineTextAlignment(.center)
        Image(sharedName: "vpn_brand")
          .renderingMode(.template)
          .foregroundColor(Color(.secondaryBraveLabel))
      }
      .padding(.bottom, 16)
    }
  }
}

#if DEBUG
struct VPNChurnPromoView_Previews: PreviewProvider {
  static var previews: some View {
    VPNChurnPromoView(churnPromoType: .autoRenewSoonExpire)
      .previewLayout(.sizeThatFits)

    VPNChurnPromoView(churnPromoType: .autoRenewDiscount)
      .previewLayout(.sizeThatFits)

    VPNChurnPromoView(churnPromoType: .autoRenewFreeMonth)
      .previewLayout(.sizeThatFits)

    VPNChurnPromoView(churnPromoType: .updateBillingSoonExpire)
      .previewLayout(.sizeThatFits)

    VPNChurnPromoView(churnPromoType: .updateBillingExpired)
      .previewLayout(.sizeThatFits)
  }
}
#endif
