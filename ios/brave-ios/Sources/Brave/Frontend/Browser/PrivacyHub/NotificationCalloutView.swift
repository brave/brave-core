// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveUI
import Shared
import Preferences
import DesignSystem
import os.log

extension PrivacyReportsView {
  struct NotificationCalloutView: View {
    @Environment(\.horizontalSizeClass) private var horizontalSizeClass
    @Environment(\.sizeCategory) private var sizeCategory
    
    private func askForNotificationAuthorization() {
      let center = UNUserNotificationCenter.current()
      
      center.requestAuthorization(options: [.alert, .sound, .badge]) { granted, error in
        
        if let error = error {
          Logger.module.warning("requestAuthorization: \(error.localizedDescription)")
          return
        }
        
        DispatchQueue.main.async {
          Preferences.PrivacyReports.shouldShowNotificationPermissionCallout.value = false
          PrivacyReportsManager.scheduleNotification(debugMode: !AppConstants.buildChannel.isPublic)
        }
      }
    }
    
    var closeButton: some View {
      Button(
        action: {
          Preferences.PrivacyReports.shouldShowNotificationPermissionCallout.value = false
        },
        label: {
          Image(systemName: "xmark")
        })
        .accessibilityLabel(Text(Strings.close))
    }
    
    private var enableNotificationsButton: some View {
      Button(
        action: askForNotificationAuthorization,
        label: {
          Group {
            if sizeCategory.isAccessibilityCategory {
              Text(Strings.PrivacyHub.notificationCalloutButtonText)
            } else {
              Label(Strings.PrivacyHub.notificationCalloutButtonText, braveSystemImage: "leo.notification")
            }
          }
          .font(.callout)
          .padding(.vertical, 12)
          .frame(maxWidth: .infinity)
          .background(
            VisualEffectView(effect: UIBlurEffect(style: .systemUltraThinMaterial))
              .edgesIgnoringSafeArea(.all)
          )
          .clipShape(Capsule())
          .fixedSize(horizontal: false, vertical: true)
        })
    }
    
    var body: some View {
      Group {
        VStack {
          if horizontalSizeClass == .compact
              || (horizontalSizeClass == .regular && sizeCategory.isAccessibilityCategory) {
            HStack(alignment: .top) {
              HStack {
                if !sizeCategory.isAccessibilityCategory {
                  Image("brave_document", bundle: .module)
                }
                Text(Strings.PrivacyHub.notificationCalloutBody)
                  .font(.headline)
                  .fixedSize(horizontal: false, vertical: true)
              }
              Spacer()
              closeButton
            }
            .frame(maxWidth: .infinity)
            
            enableNotificationsButton
              .frame(maxWidth: .infinity)
          } else {
            HStack {
              Spacer()
              closeButton
            }
            
            HStack(spacing: 24) {
              Image("brave_document", bundle: .module)
              Text(Strings.PrivacyHub.notificationCalloutBody)
                .font(.headline)
                .fixedSize(horizontal: false, vertical: true)
              Spacer()
              enableNotificationsButton
            }
            .padding()
            // Extra bottom padding to offset the close button we have in top right.
            .padding(.bottom)
          }
        }
        .padding()
        .foregroundColor(Color.white)
        .background(LinearGradient(braveGradient: .gradient05))
        .clipShape(RoundedRectangle(cornerRadius: 12.0, style: .continuous))
      }
    }
  }
}

#if DEBUG
struct NotificationCalloutView_Previews: PreviewProvider {
  static var previews: some View {
    PrivacyReportsView.NotificationCalloutView()
  }
}
#endif
