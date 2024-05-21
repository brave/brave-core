// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Data
import Foundation
import Preferences
import UIKit
import os.log

struct AppDebugComposer {

  private static var printDeviceInfo: String {
    let device = UIDevice.current
    let model = device.userInterfaceIdiom == .pad ? "iPad" : "iPhone"
    let iOSVersion = "\(device.systemName) \(device.systemVersion)"
    let appVersion =
      Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? "-"

    return "Setup: \(model), \(iOSVersion), Brave \(appVersion)"
  }

  /// This function prepares data to help us identify any app storage problems users may have.
  static func composeAppSize() -> String {
    var printFolderTreeStructure: String {
      let fm = FileManager.default
      guard
        let enumerator = fm.enumerator(
          at: URL(fileURLWithPath: NSHomeDirectory()),
          includingPropertiesForKeys: nil
        )
      else { return "" }

      let formatter = ByteCountFormatter().then {
        $0.countStyle = .file
        $0.allowsNonnumericFormatting = false
        $0.allowedUnits = [.useKB, .useMB, .useGB, .useTB]
      }

      var result = ""

      while let file = enumerator.nextObject() as? URL {
        do {

          let isDirectory =
            (try file.resourceValues(forKeys: [.isDirectoryKey])).isDirectory == true
          // Skip individual files, skip folders below 4th level of nesting.
          guard isDirectory, enumerator.level <= 4 else { continue }

          let _1MB = 1000 * 1000
          // Skip folders smaller than 1MB
          guard let directorySize = try fm.directorySize(at: file), directorySize > _1MB else {
            continue
          }

          let formattedSize = formatter.string(fromByteCount: Int64(directorySize))

          let indentation = String(repeating: "\t", count: enumerator.level - 1)
          result += indentation + file.lastPathComponent + "(\(formattedSize))\n"
        } catch {
          Logger.module.error("AppStorageDebug error: \(error)")
          continue
        }
      }

      return result
    }

    let result =
      """
      \(printDeviceInfo)
      \(printFolderTreeStructure)
      """

    return result
  }

  static func composeTabDebug(_ tabManager: TabManager) -> String {
    let storageTabs = SessionTab.all()
    let windows = SessionWindow.all()

    // We want to catch if there's disprepancy between ids of windows and tab's windows.
    let windowUniqueUUIDs = Set<String>(windows.compactMap { $0.windowId.uuidString })
    let tabWindowUUIDs = Set<String>(
      storageTabs.compactMap { $0.sessionWindow?.windowId.uuidString }
    )

    let result =
      """
      \(printDeviceInfo)
      \("Managed tabs: \(tabManager.allTabs.count)")
      \("Managed regular tabs: \(tabManager.tabsCountForMode(isPrivate: false))")
      \("Managed private tabs: \(tabManager.tabsCountForMode(isPrivate: true))")
      \("Storage Tabs: \(storageTabs.count)")
      \("Storage Regular Tabs: \(storageTabs.filter { !$0.isPrivate }.count)")
      \("Storage Private Tabs: \(storageTabs.filter { $0.isPrivate }.count)")
      \("Storage TabGroup: \(SessionTabGroup.tabGroupCount)")
      \("Storage Window: \(windows.count)")
      \("Recently Closed tabs: \(RecentlyClosed.all().count)")
      \("Tab sync enabled: \(Preferences.Chromium.syncOpenTabsEnabled.value)")
      \("Window IDs of Tabs: \(tabWindowUUIDs.map { $0.prefix(8) })")
      \("Unique Window IDs: \(windowUniqueUUIDs.map { $0.prefix(8) })")
      \("Tabs without window: \(storageTabs.filter { $0.sessionWindow == nil }.count)")
      \("Tabs without group: \(storageTabs.filter { $0.sessionTabGroup == nil }.count)")
      """

    print("bxx: \(result)")
    return result
  }
}
