// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveStrings
import BraveUI
import DesignSystem
import Favicon
import Preferences
import Shared
import Strings
import SwiftUI

protocol OpenTabsModel {
  func getSyncedSessions() -> [OpenDistantSession]
  func add(_ observer: any OpenTabsSessionStateObserver) -> OpenTabsSessionStateListener
}

extension BraveOpenTabsAPI: OpenTabsModel {}

/// Wraps a BraveOpenTabsAPI instance in an observable model
@Observable
@MainActor private class SyncedOpenTabsModel {
  private let openTabs: any OpenTabsModel
  @ObservationIgnored
  private var listener: OpenTabsSessionStateListener?

  private(set) var sessions: [OpenDistantSession] = []

  init(openTabs: any OpenTabsModel) {
    self.openTabs = openTabs
    self.listener = openTabs.add(
      OpenTabsStateObserver { [weak self] state in
        self?.update()
      }
    )
    update()
  }

  private func update() {
    sessions = openTabs.getSyncedSessions()
  }
}

struct SyncedTabsView: View {
  var urlActionHandler: ToolbarUrlActionsDelegate?
  var openSyncSettings: () -> Void

  private var model: SyncedOpenTabsModel
  @State private var searchQuery: String = ""
  @ObservedObject private var syncOpenTabsEnabled = Preferences.Chromium.syncOpenTabsEnabled
  @ObservedObject private var syncEnabled = Preferences.Chromium.syncEnabled
  @Environment(\.dismiss) private var dismiss

  init(
    openTabs: OpenTabsModel,
    urlActionHandler: ToolbarUrlActionsDelegate? = nil,
    openSyncSettings: @escaping () -> Void = {}
  ) {
    self.model = .init(openTabs: openTabs)
    self.urlActionHandler = urlActionHandler
    self.openSyncSettings = openSyncSettings
  }

  private var filteredSessions: [OpenDistantSession] {
    if searchQuery.isEmpty {
      return model.sessions
    } else {
      return model.sessions.compactMap { session in
        let matchingTabs = session.tabs.filter { tab in
          tab.url.absoluteString.localizedCaseInsensitiveContains(searchQuery)
            || tab.title?.localizedCaseInsensitiveContains(searchQuery) == true
        }
        if matchingTabs.isEmpty {
          return nil
        }
        let session = session.copy() as! OpenDistantSession
        session.tabs = matchingTabs
        return session
      }
    }
  }

  var body: some View {
    NavigationStack {
      let sessions = filteredSessions
      List {
        ForEach(sessions, id: \.sessionTag) { session in
          Section {
            SessionDisclosureGroup(session: session) { tab in
              Button {
                urlActionHandler?.openInNewTab(tab.url, isPrivate: false)
                dismiss()
              } label: {
                SessionTab(tab: tab)
                  .padding(.horizontal, 16)
                  .padding(.vertical, 12)
              }
              .contextMenu {
                Section {
                  Button {
                    urlActionHandler?.openInNewTab(tab.url, isPrivate: false)
                    dismiss()
                  } label: {
                    Label(Strings.openNewTabButtonTitle, systemImage: "plus.square.on.square")
                  }
                }
                Section {
                  Button {
                    urlActionHandler?.copy(tab.url)
                  } label: {
                    Label(Strings.copyLinkActionTitle, braveSystemImage: "leo.copy")
                  }
                  Button {
                    urlActionHandler?.share(tab.url)
                  } label: {
                    Label(Strings.shareLinkActionTitle, braveSystemImage: "leo.share.macos")
                  }
                }
              }
            }
          } header: {
            if session.sessionTag == sessions.first?.sessionTag {
              // This is unfortunately needed to ensure there's proper padding at the top of the list
              // most likely due to using UIAppearance defaults across the app to manage the navigation
              // bar appearance
              Color.clear
            }
          }
        }
      }
      .scrollContentBackground(.hidden)
      .background(Color(uiColor: .braveGroupedBackground))
      .overlay {
        Group {
          if !syncEnabled.value {
            SyncDisabledView(action: openSyncSettings)
          } else if !syncOpenTabsEnabled.value {
            OpenTabSyncDisabled(action: openSyncSettings)
          } else if !searchQuery.isEmpty && sessions.isEmpty {
            Text(Strings.noSearchResultsfound)
          } else if sessions.isEmpty {
            NoSessionTabsView(action: openSyncSettings)
          }
        }
        .transition(.opacity.animation(.default))
      }
      .navigationTitle(Strings.OpenTabs.openTabsOnOtherDevices)
      .navigationBarTitleDisplayMode(.inline)
      .searchable(text: $searchQuery, prompt: Strings.OpenTabs.tabTrayOpenTabSearchBarTitle)
      .animation(.default, value: sessions)
      .toolbar {
        ToolbarItemGroup(placement: .topBarLeading) {
          Button {
            dismiss()
          } label: {
            Label(Strings.close, braveSystemImage: "leo.close")
          }
        }
        ToolbarItemGroup(placement: .topBarTrailing) {
          Button {
            openSyncSettings()
          } label: {
            Label(Strings.Sync.settingsTitle, braveSystemImage: "leo.settings")
          }
        }
      }
    }
  }
}

private struct SessionDisclosureGroup<Content: View>: View {
  var session: OpenDistantSession
  @ViewBuilder var content: (OpenDistantTab) -> Content

  @State private var isCollapsed: Bool = false

  var body: some View {
    Group {
      Button {
        withAnimation {
          isCollapsed.toggle()
        }
      } label: {
        Label {
          HStack {
            VStack(alignment: .leading) {
              if let name = session.name {
                Text(name)
                  .font(.callout.weight(.semibold))
                  .foregroundStyle(Color(braveSystemName: .textPrimary))
              }
              if let modifiedTime = session.modifiedTime {
                Text(modifiedTime, format: .relative(presentation: .named, unitsStyle: .wide))
                  .font(.footnote)
                  .foregroundStyle(Color(braveSystemName: .textTertiary))
              }
            }
            .frame(maxWidth: .infinity, alignment: .leading)
            Spacer()
            Image(braveSystemName: "leo.carat.up")
              .rotationEffect(.degrees(isCollapsed ? 180 : 0))
              .foregroundStyle(Color(braveSystemName: .iconDefault))
          }
        } icon: {
          Image(braveSystemName: session.deviceFormFactor.braveSystemImageName)
            .font(.subheadline)
            .foregroundStyle(Color(braveSystemName: .iconDefault))
            .padding(6)
            .background(
              Color(braveSystemName: .containerDisabled),
              in: .rect(cornerRadius: 8, style: .continuous)
            )
        }
        .padding()
      }
      .alignmentGuide(HorizontalAlignment.listRowSeparatorLeading) { dimension in
        dimension[.leading]
      }
      .listRowInsets(.zero)
      .listRowBackground(Color(uiColor: .secondaryBraveGroupedBackground))
      if !isCollapsed {
        ForEach(session.tabs, id: \.tabId) { tab in
          content(tab)
            .listRowBackground(Color(uiColor: .secondaryBraveGroupedBackground))
            .listRowInsets(.zero)
        }
      }
    }
  }
}

private struct SessionTab: View {
  var tab: OpenDistantTab

  var body: some View {
    Label {
      VStack {
        if let title = tab.title, !title.isEmpty {
          Text(title)
            .lineLimit(1)
            .truncationMode(.tail)
            .frame(maxWidth: .infinity, alignment: .leading)
            .fixedSize(horizontal: false, vertical: true)
            .foregroundStyle(Color(braveSystemName: .textPrimary))
        }

        URLElidedText(
          text: URLFormatter.formatURLOrigin(
            forDisplayOmitSchemePathAndTrivialSubdomains: tab.url.strippingBlobURLAuth
              .absoluteString
          )
        )
        .font(.footnote)
        .frame(maxWidth: .infinity, alignment: .leading)
        .fixedSize(horizontal: false, vertical: true)
        .foregroundStyle(Color(braveSystemName: .textTertiary))
      }
    } icon: {
      FaviconImage(url: tab.url, isPrivateBrowsing: false)
        .frame(width: 20, height: 20)
        .padding(6)
        .overlay(
          ContainerRelativeShape()
            .strokeBorder(Color(braveSystemName: .dividerSubtle), lineWidth: 1.0)
        )
        .containerShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
    }
  }
}

private struct NoSessionTabsView: View {
  var action: () -> Void

  var body: some View {
    ContentUnavailableView {
      Label {
        Text(Strings.OpenTabs.openTabsNoSyncedTabsTitle)
          .foregroundStyle(Color(braveSystemName: .textSecondary))
          .font(.title3)
      } icon: {
        Image(braveSystemName: "leo.product.sync")
          .foregroundStyle(Color(braveSystemName: .iconSecondary))
      }
    } description: {
      Text(Strings.OpenTabs.openTabsYourTabsAppearHere)
        .foregroundStyle(Color(braveSystemName: .textTertiary))
        .font(.callout)
    } actions: {
      Button {
        action()
      } label: {
        Text(Strings.OpenTabs.openSyncSettingsButtonTitle)
      }
      .buttonStyle(BraveFilledButtonStyle(size: .normal))
    }
  }
}

private struct SyncDisabledView: View {
  var action: () -> Void

  var body: some View {
    ContentUnavailableView {
      Label {
        Text(Strings.OpenTabs.noSyncSessionPlaceHolderViewTitle)
          .foregroundStyle(Color(braveSystemName: .textSecondary))
          .font(.title3)
      } icon: {
        Image(braveSystemName: "leo.product.sync")
          .foregroundStyle(Color(braveSystemName: .iconSecondary))
      }
    } description: {
      Text(Strings.OpenTabs.noSyncChainPlaceHolderViewDescription)
        .foregroundStyle(Color(braveSystemName: .textTertiary))
        .font(.callout)
    } actions: {
      Button {
        action()
      } label: {
        Text(Strings.OpenTabs.syncChainStartButtonTitle)
      }
      .buttonStyle(BraveFilledButtonStyle(size: .normal))
    }
  }
}

private struct OpenTabSyncDisabled: View {
  var action: () -> Void

  var body: some View {
    ContentUnavailableView {
      Label {
        Text(Strings.OpenTabs.noSyncSessionPlaceHolderViewTitle)
          .foregroundStyle(Color(braveSystemName: .textSecondary))
          .font(.title3)
      } icon: {
        Image(braveSystemName: "leo.product.sync")
          .foregroundStyle(Color(braveSystemName: .iconSecondary))
      }
    } description: {
      Text(Strings.OpenTabs.enableOpenTabsPlaceHolderViewDescription)
        .foregroundStyle(Color(braveSystemName: .textTertiary))
        .font(.callout)
    } actions: {
      VStack(spacing: 24) {
        Button {
          action()
        } label: {
          Text(Strings.OpenTabs.tabSyncEnableButtonTitle)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .normal))
        Text(Strings.OpenTabs.noSyncSessionPlaceHolderViewAdditionalDescription)
          .foregroundStyle(Color(braveSystemName: .textTertiary))
          .font(.footnote)
      }
    }
  }
}

extension SyncDeviceFormFactor {
  fileprivate var braveSystemImageName: String {
    switch self {
    case .phone, .tablet:
      return "leo.smartphone.tablet-portrait"
    case .desktop:
      return "leo.laptop"
    default:
      return "leo.smartphone.laptop"
    }
  }
}

#if DEBUG

private struct MockOpenTabsModel: OpenTabsModel {
  var sessions: [OpenDistantSession] = []
  func getSyncedSessions() -> [OpenDistantSession] {
    return sessions
  }
  func add(_ observer: any OpenTabsSessionStateObserver) -> OpenTabsSessionStateListener {
    class MockListener: NSObject, OpenTabsSessionStateListener {
      func destroy() {}
    }
    return MockListener()
  }
}

extension OpenDistantSession {
  fileprivate convenience init(
    name: String,
    dateCreated: Date = .now,
    deviceFormFactor: SyncDeviceFormFactor,
    tabs: [OpenDistantTab]
  ) {
    self.init(
      name: name,
      sessionTag: UUID().uuidString,
      dateCreated: dateCreated,
      deviceFormFactor: deviceFormFactor
    )
    self.tabs = tabs
  }
}

#Preview("Sync Disabled") {
  SyncedTabsView(openTabs: MockOpenTabsModel())
    .onAppear {
      Preferences.Chromium.syncEnabled.value = false
      Preferences.Chromium.syncOpenTabsEnabled.value = false
    }
}

#Preview("Open Tabs Sync Disabled") {
  SyncedTabsView(openTabs: MockOpenTabsModel())
    .onAppear {
      Preferences.Chromium.syncEnabled.value = true
      Preferences.Chromium.syncOpenTabsEnabled.value = false
    }
}

#Preview("No Synced Sessions") {
  SyncedTabsView(openTabs: MockOpenTabsModel())
    .onAppear {
      Preferences.Chromium.syncEnabled.value = true
      Preferences.Chromium.syncOpenTabsEnabled.value = true
    }
}

#Preview("Synced Sessions") {
  SyncedTabsView(
    openTabs: MockOpenTabsModel(
      sessions: [
        .init(
          name: "iPhone",
          deviceFormFactor: .phone,
          tabs: [
            .init(
              url: URL(string: "https://brave.com")!,
              title: "Brave",
              tabId: 1,
              sessionTag: "brave"
            )
          ]
        ),
        .init(
          name: "Mac",
          deviceFormFactor: .desktop,
          tabs: [
            .init(
              url: URL(string: "https://brave.com")!,
              title: "Brave",
              tabId: 1,
              sessionTag: "brave2"
            )
          ]
        ),
      ]
    )
  )
  .onAppear {
    Preferences.Chromium.syncEnabled.value = true
    Preferences.Chromium.syncOpenTabsEnabled.value = true
  }
}

#endif
