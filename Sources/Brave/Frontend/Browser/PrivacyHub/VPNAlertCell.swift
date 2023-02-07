/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import Data
import Shared
import BraveShared

extension PrivacyReportsView {
  struct VPNAlertCell: View {
    @Environment(\.sizeCategory) private var sizeCategory
    @Environment(\.horizontalSizeClass) private var horizontalSizeClass
    
    var date: String {
      let formatter = DateFormatter()
      formatter.dateStyle = .medium
      formatter.timeStyle = .short
      return formatter.string(from: Date(timeIntervalSince1970: TimeInterval(vpnAlert.timestamp)))
    }
    
    private func assetName(for type: BraveVPNAlert.TrackerType) -> String {
      switch type {
      case .app: return "vpn_data_tracker"
      case .location: return "vpn_location_tracker"
      case .mail: return "vpn_mail_tracker"
      }
    }
    
    private func headerText(for type: BraveVPNAlert.TrackerType) -> String {
      switch type {
      case .app: return Strings.PrivacyHub.vpnAlertRegularTrackerTypeSingular
      case .location: return Strings.PrivacyHub.vpnAlertLocationTrackerTypeSingular
      case .mail: return Strings.PrivacyHub.vpnAlertEmailTrackerTypeSingular
      }
    }
    
    private let vpnAlert: BraveVPNAlert
    
    init(vpnAlert: BraveVPNAlert) {
      self.vpnAlert = vpnAlert
    }
    
    private var headerText: some View {
      Group {
        if let category = vpnAlert.categoryEnum {
          Text(headerText(for: category))
            .foregroundColor(Color(.secondaryBraveLabel))
            .font(.caption.weight(.semibold))
        } else {
          EmptyView()
        }
      }
    }
    
    var body: some View {
      HStack(alignment: .top) {
        if let category = vpnAlert.categoryEnum {
          Image(assetName(for: category), bundle: .module)
        }
        
        VStack(alignment: .leading) {
          Group {
            if sizeCategory.isAccessibilityCategory && horizontalSizeClass == .compact {
              VStack(alignment: .leading, spacing: 4) {
                headerText
                PrivacyReportsView.BlockedLabel()
              }
            } else {
              HStack(spacing: 4) {
                headerText
                PrivacyReportsView.BlockedLabel()
              }
            }
          }
          .font(.caption.weight(.semibold))
          
          VStack(alignment: .leading, spacing: 4) {
            Text(vpnAlert.message ?? "-")
              .font(.callout)
            
            Text(date)
              .font(.caption)
              .foregroundColor(Color(.secondaryBraveLabel))
          }
        }
        Spacer()
      }
      .background(Color(.braveBackground))
      .frame(maxWidth: .infinity)
      .fixedSize(horizontal: false, vertical: true)
      .padding(.horizontal)
      .padding(.vertical, 8)
      .clipShape(RoundedRectangle(cornerRadius: 12, style: .continuous))
    }
  }
}

