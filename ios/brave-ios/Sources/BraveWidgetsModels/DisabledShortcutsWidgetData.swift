// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import WidgetKit
import os.log

/// Shared storage describing which `WidgetShortcut`s are currently unavailable, for example when
/// disabled by Brave Origin or enterprise policies.
public class DisabledShortcutsWidgetData {
  private static var widgetDataRoot: URL? {
    FileManager.default.containerURL(
      forSecurityApplicationGroupIdentifier: AppInfo.sharedContainerIdentifier
    )?.appendingPathComponent("widget_data")
  }

  private static var widgetDataPath: URL? {
    widgetDataRoot?.appendingPathComponent("disabled_shortcuts.json")
  }

  /// The set of shortcuts that are currently unavailable. An empty set (the
  /// common case, including a missing file) means every feature is available.
  public static func loadDisabledShortcuts() async -> Set<WidgetShortcut> {
    guard let dataPath = widgetDataPath,
      FileManager.default.fileExists(atPath: dataPath.path)
    else {
      return []
    }
    do {
      let jsonData = try Data(contentsOf: dataPath)
      let rawValues = try JSONDecoder().decode([Int].self, from: jsonData)
      return Set(
        rawValues.compactMap(WidgetShortcut.init(rawValue:)).filter { $0 != .unknown }
      )
    } catch {
      Logger.module.error("loadDisabledShortcuts error: \(error.localizedDescription)")
      return []
    }
  }

  /// Writes the set of unavailable shortcuts and reloads the affected widgets.
  /// When the set is empty the backing file is removed to keep the common case
  /// clean.
  public static func updateDisabledShortcuts(_ shortcuts: Set<WidgetShortcut>) async {
    guard let rootPath = widgetDataRoot, let dataPath = widgetDataPath else { return }
    do {
      let rawValues = shortcuts.filter { $0 != .unknown }.map(\.rawValue).sorted()
      if rawValues.isEmpty {
        if FileManager.default.fileExists(atPath: dataPath.path) {
          try FileManager.default.removeItem(at: dataPath)
        }
      } else {
        let widgetData = try JSONEncoder().encode(rawValues)
        try FileManager.default.createDirectory(
          atPath: rootPath.path,
          withIntermediateDirectories: true
        )
        try widgetData.write(to: dataPath)
      }
      WidgetCenter.shared.reloadTimelines(ofKind: "ShortcutsWidget")
      WidgetCenter.shared.reloadTimelines(ofKind: "LockScreenShortcutWidget")
      WidgetCenter.shared.reloadTimelines(ofKind: "TopNewsWidget")
      WidgetCenter.shared.reloadTimelines(ofKind: "TopNewsListWidget")
    } catch {
      Logger.module.error("updateDisabledShortcuts error: \(error.localizedDescription)")
    }
  }
}
