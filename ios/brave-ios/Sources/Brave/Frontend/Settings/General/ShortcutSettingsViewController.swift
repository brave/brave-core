// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveStrings
import IntentsUI
import SwiftUI

struct ShortcutSettingsView: View {
  private enum ShortcutSheet: Identifiable {
    case add(ActivityType)
    case edit(INVoiceShortcut)

    var id: String {
      switch self {
      case .add(let activityType):
        return "add-\(activityType.identifier)"
      case .edit(let voiceShortcut):
        return "edit-\(voiceShortcut.identifier)"
      }
    }
  }

  @State private var shortcutSheet: ShortcutSheet?
  @State private var isOpenSettingsAlertPresented: Bool = false

  var body: some View {
    Form {
      ForEach(ActivityType.allCases, id: \.identifier) { activityType in
        Section {
          Button {
            Task { @MainActor in
              let shortcuts = try await INVoiceShortcutCenter.shared.allVoiceShortcuts()
              if let shortcut = shortcuts.first(where: {
                $0.shortcut.userActivity?.activityType == activityType.identifier
              }) {
                shortcutSheet = .edit(shortcut)
              } else {
                shortcutSheet = .add(activityType)
              }
            }
          } label: {
            // Use NavigationLink pattern for disclosure indicator
            NavigationLink {
              EmptyView()
            } label: {
              Text(activityType.rowTitle)
                .frame(maxWidth: .infinity, alignment: .leading)
                .contentShape(.rect)
            }
          }
          .buttonStyle(.plain)
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        } footer: {
          Text(activityType.footerDescription)
        }
      }

      Section {
        Button {
          isOpenSettingsAlertPresented = true
        } label: {
          Text(Strings.Shortcuts.shortcutOpenApplicationSettingsTitle)
            .foregroundStyle(Color(braveSystemName: .textInteractive))
        }
        .alert(
          Strings.Shortcuts.shortcutOpenApplicationSettingsTitle,
          isPresented: $isOpenSettingsAlertPresented
        ) {
          Button(Strings.Shortcuts.shortcutOpenApplicationSettingsTitle) {
            if let settingsURL = URL(string: UIApplication.openSettingsURLString) {
              UIApplication.shared.open(settingsURL)
            }
          }
          Button(Strings.cancelButtonTitle, role: .cancel) {}
        } message: {
          Text(Strings.Shortcuts.shortcutOpenApplicationSettingsDescription)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } footer: {
        Text(Strings.Shortcuts.shortcutOpenApplicationSettingsDescription)
      }
    }
    .scrollContentBackground(.hidden)
    .background(Color(.braveGroupedBackground))
    .navigationTitle(Strings.Shortcuts.shortcutSettingsTitle)
    .sheet(item: $shortcutSheet) { sheet in
      switch sheet {
      case .add(let activityType):
        AddVoiceShortcutView(activityType: activityType)
      case .edit(let voiceShortcut):
        EditVoiceShortcutView(voiceShortcut: voiceShortcut)
      }
    }
  }
}

class ShortcutSettingsViewController: UIHostingController<ShortcutSettingsView> {
  init() {
    super.init(rootView: ShortcutSettingsView())
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    title = Strings.Shortcuts.shortcutSettingsTitle
  }
}

extension ActivityType {
  fileprivate var rowTitle: String {
    switch self {
    case .newTab:
      return Strings.Shortcuts.shortcutSettingsOpenNewTabTitle
    case .newPrivateTab:
      return Strings.Shortcuts.shortcutSettingsOpenNewPrivateTabTitle
    case .openBookmarks:
      return Strings.Shortcuts.shortcutSettingsOpenBookmarksTitle
    case .openHistoryList:
      return Strings.Shortcuts.shortcutSettingsOpenHistoryListTitle
    case .clearBrowsingHistory:
      return Strings.Shortcuts.shortcutSettingsClearBrowserHistoryTitle
    case .enableBraveVPN:
      return Strings.Shortcuts.shortcutSettingsEnableVPNTitle
    case .openBraveNews:
      return Strings.Shortcuts.shortcutSettingsOpenBraveNewsTitle
    case .openPlayList:
      return Strings.Shortcuts.shortcutSettingsOpenPlaylistTitle
    case .openSyncedTabs:
      return Strings.Shortcuts.shortcutSettingsOpenSyncedTabsTitle
    }
  }

  fileprivate var footerDescription: String {
    switch self {
    case .newTab:
      return Strings.Shortcuts.shortcutSettingsOpenNewTabDescription
    case .newPrivateTab:
      return Strings.Shortcuts.shortcutSettingsOpenNewPrivateTabDescription
    case .openBookmarks:
      return Strings.Shortcuts.shortcutSettingsOpenBookmarksDescription
    case .openHistoryList:
      return Strings.Shortcuts.shortcutSettingsOpenHistoryListDescription
    case .clearBrowsingHistory:
      return Strings.Shortcuts.shortcutSettingsClearBrowserHistoryDescription
    case .enableBraveVPN:
      return Strings.Shortcuts.shortcutSettingsEnableVPNDescription
    case .openBraveNews:
      return Strings.Shortcuts.shortcutSettingsOpenBraveNewsDescription
    case .openPlayList:
      return Strings.Shortcuts.shortcutSettingsOpenPlaylistDescription
    case .openSyncedTabs:
      return Strings.Shortcuts.shortcutSettingsOpenSyncedTabsDescription
    }
  }
}

private struct AddVoiceShortcutView: UIViewControllerRepresentable {
  let activityType: ActivityType

  func makeUIViewController(context: Context) -> INUIAddVoiceShortcutViewController {
    let userActivity = ActivityShortcutManager.shared.createShortcutActivity(
      type: activityType
    )
    let vc = INUIAddVoiceShortcutViewController(
      shortcut: INShortcut(userActivity: userActivity)
    )
    vc.delegate = context.coordinator
    return vc
  }

  func updateUIViewController(
    _ uiViewController: INUIAddVoiceShortcutViewController,
    context: Context
  ) {
  }

  func makeCoordinator() -> Coordinator {
    Coordinator()
  }

  class Coordinator: NSObject, INUIAddVoiceShortcutViewControllerDelegate {
    func addVoiceShortcutViewController(
      _ controller: INUIAddVoiceShortcutViewController,
      didFinishWith voiceShortcut: INVoiceShortcut?,
      error: Error?
    ) {
      controller.dismiss(animated: true)
    }

    func addVoiceShortcutViewControllerDidCancel(
      _ controller: INUIAddVoiceShortcutViewController
    ) {
      controller.dismiss(animated: true)
    }
  }
}

private struct EditVoiceShortcutView: UIViewControllerRepresentable {
  let voiceShortcut: INVoiceShortcut

  func makeUIViewController(context: Context) -> INUIEditVoiceShortcutViewController {
    let vc = INUIEditVoiceShortcutViewController(voiceShortcut: voiceShortcut)
    vc.delegate = context.coordinator
    return vc
  }

  func updateUIViewController(
    _ uiViewController: INUIEditVoiceShortcutViewController,
    context: Context
  ) {
  }

  func makeCoordinator() -> Coordinator {
    Coordinator()
  }

  class Coordinator: NSObject, INUIEditVoiceShortcutViewControllerDelegate {
    func editVoiceShortcutViewController(
      _ controller: INUIEditVoiceShortcutViewController,
      didUpdate voiceShortcut: INVoiceShortcut?,
      error: Error?
    ) {
      controller.dismiss(animated: true)
    }

    func editVoiceShortcutViewController(
      _ controller: INUIEditVoiceShortcutViewController,
      didDeleteVoiceShortcutWithIdentifier deletedVoiceShortcutIdentifier: UUID
    ) {
      controller.dismiss(animated: true)
    }

    func editVoiceShortcutViewControllerDidCancel(
      _ controller: INUIEditVoiceShortcutViewController
    ) {
      controller.dismiss(animated: true)
    }
  }
}
