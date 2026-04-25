// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import DesignSystem
import Foundation
import OSLog
import Strings
import SwiftUI

extension Bundle {
  var primaryIconName: String? {
    guard let icons = object(forInfoDictionaryKey: "CFBundleIcons") as? [String: Any],
      let primaryIcon = icons["CFBundlePrimaryIcon"] as? [String: Any],
      let iconFiles = primaryIcon["CFBundleIconFiles"] as? [String]
    else {
      return nil
    }
    return iconFiles.last
  }
}

private struct AppIconView: View {
  var altIcon: AltIcon?
  var isSelected: Bool

  private var icon: Image {
    if let altIcon {
      return Image(altIcon.assetName, bundle: .module)
    } else {
      let iconName = Bundle.main.primaryIconName ?? "AppIcon"
      return Image(uiImage: UIImage(named: iconName) ?? .init())
    }
  }

  var body: some View {
    HStack(spacing: 10) {
      icon
        .resizable()
        .aspectRatio(contentMode: .fit)
        .frame(width: 56, height: 56)
        .clipShape(.rect(cornerRadius: 14, style: .continuous))
        .overlay {
          RoundedRectangle(cornerRadius: 14, style: .continuous)
            .strokeBorder(Color.black.opacity(0.1))
        }
      HStack {
        Text(altIcon.map(\.displayName) ?? Strings.AltAppIcon.defaultAppIcon)
          .foregroundStyle(Color(braveSystemName: .textPrimary))
        Spacer()
        if isSelected {
          Image(braveSystemName: "leo.check.circle-filled")
        }
      }
    }
  }
}

struct AltIconsView: View {
  @ObservedObject var model: AltIconsModel

  @State private var isErrorPresented: Bool = false

  private func selectIcon(_ icon: AltIcon?) {
    model.setAlternateAppIcon(icon) { error in
      if let error {
        Logger.module.error("Failed to set alternate app icon: \(error.localizedDescription)")
        self.isErrorPresented = true
      }
    }
  }

  var body: some View {
    List {
      Section {
        Button {
          selectIcon(nil)
        } label: {
          AppIconView(altIcon: nil, isSelected: model.selectedAltAppIcon == nil)
        }
        .listRowBackground(Color(uiColor: UIColor.secondaryBraveGroupedBackground))
        ForEach(AltIcon.allBraveIcons) { icon in
          Button {
            selectIcon(icon)
          } label: {
            AppIconView(altIcon: icon, isSelected: model.selectedAltAppIcon == icon.assetName)
          }
          .listRowBackground(Color(uiColor: UIColor.secondaryBraveGroupedBackground))
        }
      } header: {
        Text(Strings.AltAppIcon.braveIconsHeader)
      }
    }
    .scrollContentBackground(.hidden)
    .background(Color(uiColor: UIColor.braveGroupedBackground))
    .navigationTitle(Strings.AltAppIcon.changeAppIcon)
    .alert(Strings.AltAppIcon.errorTitle, isPresented: $isErrorPresented) {
      Button(Strings.OKString) {}
    }
  }
}
