/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import Shared
import BraveShared
import Data
import BraveUI

extension PrivacyReportsView {
  struct AllVPNAlertsView: View {
    @Environment(\.sizeCategory) private var sizeCategory
    @Environment(\.horizontalSizeClass) private var horizontalSizeClass
    
    @Environment(\.managedObjectContext) var context
    
    @FetchRequest(
      entity: BraveVPNAlert.entity(),
      sortDescriptors: [NSSortDescriptor(keyPath: \BraveVPNAlert.timestamp, ascending: false)],
      // For performance reasons we grab last month's alerts only.
      // Unlikely the user is going to scroll beyond last 30 days timeframe.
      predicate: NSPredicate(format: "timestamp > %lld", Int64(Date().timeIntervalSince1970 - 30.days))
    ) private var vpnAlerts: FetchedResults<BraveVPNAlert>
    
    @State private var alerts: (trackerCount: Int, locationPingCount: Int, emailTrackerCount: Int)?
    
    @State private var alertsLoading = true
    
    private(set) var onDismiss: () -> Void
    
    private var total: Int {
      guard let alerts = alerts else {
        return 0
      }
      
      return alerts.trackerCount + alerts.locationPingCount + alerts.emailTrackerCount
    }
    
    private var headerView: some View {
      VStack {
        HStack {
          Text(Strings.PrivacyHub.vpvnAlertsTotalCount.uppercased())
            .font(.subheadline.weight(.medium))
            .unredacted()
          Spacer()
          Text("\(total)")
            .font(.headline)
        }
        .padding()
        .background(Color("total_alerts_background", bundle: .module))
        .clipShape(RoundedRectangle(cornerRadius: 12, style: .continuous))
        
        if sizeCategory.isAccessibilityCategory && horizontalSizeClass == .compact {
          VPNAlertStat(
            assetName: "vpn_data_tracker",
            title: Strings.PrivacyHub.vpnAlertRegularTrackerTypePlural,
            count: alerts?.trackerCount ?? 0,
            compact: true)
          VPNAlertStat(
            assetName: "vpn_location_tracker",
            title: Strings.PrivacyHub.vpnAlertLocationTrackerTypePlural,
            count: alerts?.locationPingCount ?? 0,
            compact: true)
          VPNAlertStat(
            assetName: "vpn_mail_tracker",
            title: Strings.PrivacyHub.vpnAlertEmailTrackerTypePlural,
            count: alerts?.emailTrackerCount ?? 0,
            compact: true)
        } else {
          VPNAlertStat(
            assetName: "vpn_data_tracker",
            title: Strings.PrivacyHub.vpnAlertRegularTrackerTypePlural,
            count: alerts?.trackerCount ?? 0,
            compact: false)
          HStack {
            VPNAlertStat(
              assetName: "vpn_location_tracker",
              title: Strings.PrivacyHub.vpnAlertLocationTrackerTypePlural,
              count: alerts?.locationPingCount ?? 0,
              compact: true)
            VPNAlertStat(
              assetName: "vpn_mail_tracker",
              title: Strings.PrivacyHub.vpnAlertEmailTrackerTypePlural,
              count: alerts?.emailTrackerCount ?? 0,
              compact: true)
          }
        }
      }
      .padding(.vertical)
    }
    
    private func cell(for alert: BraveVPNAlert) -> some View {
      VPNAlertCell(vpnAlert: alert)
        .listRowInsets(.init())
        .background(Color(.braveBackground))
        .clipShape(RoundedRectangle(cornerRadius: 12, style: .continuous))
        .padding(.vertical, 4)
    }
    
    var body: some View {
      VStack(alignment: .leading) {
        List {
          Section {
            ForEach(vpnAlerts) { alert in
              cell(for: alert)
                .listRowBackground(Color.clear)
                .listRowSeparator(.hidden)
            }
          } header: {
            headerView
              .listRowInsets(.init())
              .redacted(reason: alertsLoading ? .placeholder : [])
          }
        }
        .listStyle(.insetGrouped)
        .listBackgroundColor(Color(UIColor.braveGroupedBackground))
        
        Spacer()
      }
      .frame(maxWidth: .infinity, maxHeight: .infinity)
      .background(Color(.secondaryBraveBackground).ignoresSafeArea())
      .ignoresSafeArea(.container, edges: .bottom)
      .navigationTitle(Strings.PrivacyHub.allVPNAlertsButtonText)
      .toolbar {
        ToolbarItem(placement: .confirmationAction) {
          Button(Strings.done, action: onDismiss)
            .foregroundColor(Color(.braveBlurpleTint))
        }
      }
      .onAppear {
        BraveVPNAlert.alertTotals { result in
          alerts = result
          alertsLoading = false
        }
      }
    }
  }
}

#if DEBUG
struct AllVPNAlertsView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      PrivacyReportsView.AllVPNAlertsView(onDismiss: {})
      PrivacyReportsView.AllVPNAlertsView(onDismiss: {})
        .preferredColorScheme(.dark)
    }
  }
}
#endif
