// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Preferences
import SwiftUI

public enum UserAgentOverride: Equatable, Identifiable, Hashable, CaseIterable {
  case none
  case braveMobile
  case braveDesktop
  case safariMobile
  case safariDesktop
  case chromeMobile
  case chromeDesktop
  case chromeMobileBraveComment
  case chromeDesktopBraveComment
  case safariMobileBraveComment
  case safariDesktopBraveComment
  case custom

  /// Name to display in the Picker / drop down
  var displayName: String {
    switch self {
    case .none:
      return "None"
    case .braveMobile:
      return "Brave Mobile"
    case .braveDesktop:
      return "Brave Desktop"
    case .safariMobile:
      return "Safari Mobile"
    case .safariDesktop:
      return "Safari Desktop"
    case .chromeMobile:
      return "Chrome Mobile"
    case .chromeDesktop:
      return "Chrome Desktop"
    case .chromeMobileBraveComment:
      return "Chrome Mobile w/ Brave comment"
    case .chromeDesktopBraveComment:
      return "Chrome Desktop w/ Brave comment"
    case .safariMobileBraveComment:
      return "Safari Mobile w/ Brave comment"
    case .safariDesktopBraveComment:
      return "Safari Desktop w/ Brave comment"
    case .custom:
      return "Custom"
    }
  }

  /// The user agent string for the selected UserAgentOverride case.
  public var userAgent: String? {
    switch self {
    case .none:
      return nil
    case .braveMobile:
      return UserAgent.mobile
    case .braveDesktop:
      return UserAgent.desktop
    case .safariMobile:
      return UserAgent.mobileMasked
    case .safariDesktop:
      return UserAgent.desktopMasked
    case .chromeMobile:
      return UserAgentBuilder(appIdentifier: "CriOS").build(
        desktopMode: false,
        useSafariUA: false,
        useChromiumVersion: true
      )
    case .chromeDesktop:
      return UserAgentBuilder(appIdentifier: "CriOS").build(
        desktopMode: true,
        useSafariUA: false,
        useChromiumVersion: true
      )
    case .chromeMobileBraveComment:
      return UserAgentBuilder(appIdentifier: "CriOS (Brave)").build(
        desktopMode: false,
        useSafariUA: false,
        useChromiumVersion: true
      )
    case .chromeDesktopBraveComment:
      return UserAgentBuilder(appIdentifier: "CriOS (Brave)").build(
        desktopMode: true,
        useSafariUA: false,
        useChromiumVersion: true
      )
    case .safariMobileBraveComment:
      let safariUA = UserAgent.mobileMasked
      // add ` (Brave)` after last occurance of `Safari`
      if let range = safariUA.range(of: "Safari", options: [.backwards]) {
        return safariUA.replacingCharacters(in: range, with: "Safari (Brave)")
      }
      return safariUA
    case .safariDesktopBraveComment:
      let safariUA = UserAgent.desktopMasked
      // add ` (Brave)` after last occurance of `Safari`
      if let range = safariUA.range(of: "Safari", options: [.backwards]) {
        return safariUA.replacingCharacters(in: range, with: "Safari (Brave)")
      }
      return safariUA
    case .custom:
      return nil
    }
  }

  public var id: String {
    return displayName
  }
}

public struct UserAgentOverrideView: View {

  /// The value used in the Picker / drop down
  @State private var userAgentOverride: UserAgentOverride
  /// The stored user agent value to be used as the overridden user agent
  @ObservedObject private var userAgent = Preferences.Debug.userAgentOverride
  /// If the user agent text editor is focused
  @FocusState private var isTextEditorFocused

  public init() {
    let currentValue = Preferences.Debug.userAgentOverride.value
    if currentValue.isEmpty {
      self.userAgentOverride = .none
    } else {
      self.userAgentOverride =
        UserAgentOverride.allCases.first(
          where: { $0.userAgent == currentValue }
        ) ?? .custom
    }
  }

  public var body: some View {
    Form {
      Section(
        content: {
          Menu(
            content: {
              Picker(selection: $userAgentOverride) {
                ForEach(UserAgentOverride.allCases) { userAgentOverride in
                  Group {
                    Text(userAgentOverride.displayName)
                  }
                  .tag(userAgentOverride)
                }
              } label: {
                EmptyView()
              }
            },
            label: {
              HStack {
                Text(userAgentOverride.displayName)
                  .foregroundStyle(
                    userAgentOverride == .none ? Color(.placeholderText) : Color.primary
                  )
                Spacer()
                Image(systemName: "chevron.up.chevron.down")
                  .foregroundStyle(Color.secondary)
              }
            }
          )
          .onChange(
            of: userAgentOverride,
            { _, userAgentOverride in
              if userAgentOverride == .custom {
                // focus the text editor when user selects custom
                self.isTextEditorFocused = true
              } else {
                // update the TextEditor if user picks non-custom user agent
                self.userAgent.value = userAgentOverride.userAgent ?? ""
              }
            }
          )
          if userAgentOverride != .none {
            TextEditor(text: $userAgent.value)
              .focused($isTextEditorFocused)
              .frame(height: 300)
              .onChange(of: userAgent.value) { _, userAgent in
                // update the picker value if text changes
                self.userAgentOverride =
                  UserAgentOverride.allCases.first(
                    where: { $0.userAgent == userAgent }
                  ) ?? .custom
              }
          }
        },
        header: {
          Text("User Agent Override")
        },
        footer: {
          Text(
            "This will override user agent in all scenarios, including when requesting desktop site."
          )
          .font(.caption)
        }
      )
    }
    .navigationTitle("User Agent Override")
  }
}
