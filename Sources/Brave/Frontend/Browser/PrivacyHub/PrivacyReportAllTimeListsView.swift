/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import Shared
import BraveShared
import Data
import BraveUI

struct PrivacyReportAllTimeListsView: View {
  @Environment(\.horizontalSizeClass) private var horizontalSizeClass
  @Environment(\.sizeCategory) private var sizeCategory
  
  @State private var trackers: [PrivacyReportsTracker] = []
  @State private var websites: [PrivacyReportsWebsite] = []
  
  @State private var trackersLoading = true
  @State private var websitesLoading = true
  
  private(set) var isPrivateBrowsing: Bool
  private(set) var onDismiss: () -> Void
  
  enum Page: String, CaseIterable, Identifiable {
    case trackersAndAds, websites
    
    var id: String {
      rawValue
    }
    
    var displayString: String {
      switch self {
      case .trackersAndAds: return Strings.PrivacyHub.allTimeListsTrackersView
      case .websites: return Strings.PrivacyHub.allTimeListsWebsitesView
      }
    }
  }
  
  @State private var currentPage: Page = .trackersAndAds
  
  private var selectionPicker: some View {
    Picker("", selection: $currentPage) {
      ForEach(Page.allCases) {
        Text($0.displayString)
          .tag($0)
      }
    }
    .pickerStyle(.segmented)
    .padding(.horizontal, 20)
    .padding(.vertical, 12)
  }
  
  @ViewBuilder
  private func blockedLabels(by source: PrivacyReportsTracker.Source?) -> some View {
    switch source {
    case .shields:
      PrivacyReportsView.BlockedByShieldsLabel()
    case .vpn:
      PrivacyReportsView.BlockedByVPNLabel()
    case .both:
      PrivacyReportsView.BlockedByShieldsLabel()
      PrivacyReportsView.BlockedByVPNLabel()
    case .none:
      EmptyView()
    }
  }
  
  private var trackersList: some View {
    List {
      Section {
        ForEach(trackers) { item in
          HStack {
            VStack(alignment: .leading, spacing: 4) {
              
              VStack(alignment: .leading, spacing: 0) {
                Text(item.name)
                  .font(.callout)
                  .foregroundColor(Color(.bravePrimary))
                
                if let url = URL(string: item.name),
                   let humanFriendlyTrackerName = BlockedTrackerParser.parse(url: url, fallbackToDomainURL: false) {
                  Text(humanFriendlyTrackerName)
                    .font(.footnote)
                    .foregroundColor(Color(.braveLabel))
                }
              }
              
              Group {
                if sizeCategory.isAccessibilityCategory {
                  VStack(alignment: .leading, spacing: 4) {
                    Text(Strings.PrivacyHub.blockedBy)
                      .foregroundColor(Color(.secondaryBraveLabel))
                    
                    blockedLabels(by: item.source)
                  }
                } else {
                  HStack(spacing: 4) {
                    Text(Strings.PrivacyHub.blockedBy)
                      .foregroundColor(Color(.secondaryBraveLabel))
                    
                    blockedLabels(by: item.source)
                  }
                }
              }
              .font(.caption)
            }
            
            Spacer()
            Text("\(item.count)")
              .font(.headline)
          }
        }
      } header: {
        Text(Strings.PrivacyHub.allTimeListTrackersHeaderTitle)
          .listRowInsets(.init())
          .padding(.vertical, 8)
          .font(.footnote)
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .listStyle(.insetGrouped)
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
  }
  
  private var websitesList: some View {
    List {
      Section {
        ForEach(websites) { item in
          HStack {
            FaviconImage(url: item.faviconUrl, isPrivateBrowsing: isPrivateBrowsing)
            Text(item.domain)
            Spacer()
            Text("\(item.count)")
              .font(.headline)
          }
        }
      } header: {
        Text(Strings.PrivacyHub.allTimeListWebsitesHeaderTitle)
          .font(.footnote)
          .listRowInsets(.init())
          .padding(.vertical, 8)
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .listStyle(.insetGrouped)
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
  }
  
  var body: some View {
    VStack(spacing: 0) {
      Picker("", selection: $currentPage) {
        ForEach(Page.allCases) {
          Text($0.displayString)
            .tag($0)
        }
      }
      .pickerStyle(.segmented)
      .padding(.horizontal, 20)
      .padding(.vertical, 12)
      
      if trackersLoading || websitesLoading {
        ProgressView()
          .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .center)
      } else {
        switch currentPage {
        case .trackersAndAds: trackersList
        case .websites: websitesList
        }
      }
    }
    .background(Color(.braveGroupedBackground).ignoresSafeArea())
    .ignoresSafeArea(.container, edges: .bottom)
    .navigationTitle(Strings.PrivacyHub.allTimeListsButtonText)
    .toolbar {
      ToolbarItem(placement: .confirmationAction) {
        Button(Strings.done, action: onDismiss)
          .foregroundColor(Color(.braveBlurpleTint))
      }
    }
    .onAppear {
      BlockedResource.allTimeMostFrequentTrackers() { allTimeListTrackers in
        BraveVPNAlert.allByHostCount { vpnItems in
          trackers = PrivacyReportsTracker.merge(shieldItems: allTimeListTrackers, vpnItems: vpnItems)
          trackersLoading = false
        }
      }
      
      BlockedResource.allTimeMostRiskyWebsites { riskyWebsites in
        websites = riskyWebsites.map {
          PrivacyReportsWebsite(domain: $0.domain, faviconUrl: $0.faviconUrl, count: $0.count)
        }
        
        websitesLoading = false
      }
    }
  }
}

#if DEBUG
struct PrivacyReportAllTimeListsView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      PrivacyReportAllTimeListsView(isPrivateBrowsing: false, onDismiss: {})
      PrivacyReportAllTimeListsView(isPrivateBrowsing: false, onDismiss: {})
        .preferredColorScheme(.dark)
    }
  }
}
#endif
