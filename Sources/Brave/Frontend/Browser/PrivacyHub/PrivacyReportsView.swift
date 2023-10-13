/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import BraveUI
import Shared
import Preferences
import Data

struct PrivacyReportsView: View {
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  let lastVPNAlerts: [BraveVPNAlert]?
  
  private(set) var isPrivateBrowsing: Bool
  var onDismiss: (() -> Void)?
  var openPrivacyReportsUrl: (() -> Void)?
  
  @ObservedObject private var showNotificationPermissionCallout = Preferences.PrivacyReports.shouldShowNotificationPermissionCallout
  
  @State private var correctAuthStatus: Bool = false
  
  @State private var showClearDataPrompt: Bool = false
  
  /// This is to cover a case where user has set up their notifications already, and pressing on 'Enable notifications' would do nothing.
  private func determineNotificationPermissionStatus() {
    UNUserNotificationCenter.current().getNotificationSettings { settings in
      DispatchQueue.main.async {
        correctAuthStatus =
        settings.authorizationStatus == .notDetermined || settings.authorizationStatus == .provisional
      }
    }
  }
  
  private func dismissView() {
    presentationMode.dismiss()
    onDismiss?()
  }
  
  private var clearAllDataButton: some View {
    Button(action: {
      showClearDataPrompt = true
    }, label: {
      Image(uiImage: .init(braveSystemNamed: "leo.trash")!.template)
    })
      .accessibility(label: Text(Strings.PrivacyHub.clearAllDataAccessibility))
      .foregroundColor(Color(.braveBlurpleTint))
      .actionSheet(isPresented: $showClearDataPrompt) {
        // Currently .actionSheet does not allow you leave empty title for the sheet.
        // This could get converted to .confirmationPrompt or Menu with destructive buttons
        // once iOS 15 is minimum supported version
        .init(title: Text(Strings.PrivacyHub.clearAllDataPrompt),
              buttons: [
                .destructive(Text(Strings.yes), action: {
                  PrivacyReportsManager.clearAllData()
                  // Dismiss to avoid having to observe for db changes to update the view.
                  dismissView()
                }),
                .cancel()
              ])
      }
  }
  
  private var doneButton: some View {
    Button(Strings.done, action: dismissView)
      .foregroundColor(Color(.braveBlurpleTint))
  }
  
  var body: some View {
    NavigationView {
      ScrollView(.vertical) {
        VStack(alignment: .leading, spacing: 16) {
          
          if showNotificationPermissionCallout.value && correctAuthStatus {
            NotificationCalloutView()
          }
                    
          PrivacyHubLastWeekSection()
          
          Divider()
          
          if Preferences.PrivacyReports.captureVPNAlerts.value, let lastVPNAlerts = lastVPNAlerts, !lastVPNAlerts.isEmpty {
            PrivacyHubVPNAlertsSection(lastVPNAlerts: lastVPNAlerts, onDismiss: dismissView)
            
            Divider()
          }
          
          PrivacyHubAllTimeSection(isPrivateBrowsing: isPrivateBrowsing, onDismiss: dismissView)
          
          VStack {
            Text(Strings.PrivacyHub.privacyReportsDisclaimer)
              .font(.caption)
              .multilineTextAlignment(.center)
            
            Button(action: {
              openPrivacyReportsUrl?()
              dismissView()
            }, label: {
              Text(Strings.learnMore)
                .underline()
                .font(.caption.weight(.bold))
                .frame(maxWidth: .infinity, alignment: .center)
            })
          }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .padding()
        .navigationTitle(Strings.PrivacyHub.privacyReportsTitle)
        .navigationBarTitleDisplayMode(.inline)
        .toolbar {
          ToolbarItem(placement: .confirmationAction) {
            doneButton
          }
          
          ToolbarItem(placement: .cancellationAction) {
            clearAllDataButton
          }
        }
      }
      .background(Color(.secondaryBraveBackground).ignoresSafeArea())
    }
    .navigationViewStyle(.stack)
    .environment(\.managedObjectContext, DataController.swiftUIContext)
    .onAppear(perform: determineNotificationPermissionStatus)
  }
}

#if DEBUG
struct PrivacyReports_Previews: PreviewProvider {
  static var previews: some View {
    
    Group {
      PrivacyReportsView(lastVPNAlerts: nil, isPrivateBrowsing: false)
      
      PrivacyReportsView(lastVPNAlerts: nil, isPrivateBrowsing: false)
        .preferredColorScheme(.dark)
    }
  }
}
#endif
