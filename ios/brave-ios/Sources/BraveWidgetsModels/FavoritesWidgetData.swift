// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import WidgetKit
import FaviconModels
import os.log

public struct WidgetFavorite: Codable {
  public var url: URL
  public var title: String?
  public var favicon: Favicon?

  public init(url: URL, title: String?, favicon: Favicon?) {
    self.url = url
    self.title = title
    self.favicon = favicon
  }
}

public class FavoritesWidgetData {
  private static var widgetDataRoot: URL? {
    FileManager.default.containerURL(forSecurityApplicationGroupIdentifier: AppInfo.sharedContainerIdentifier)?.appendingPathComponent("widget_data")
  }

  private static var widgetDataPath: URL? {
    widgetDataRoot?.appendingPathComponent("favs.json")
  }

  public static var dataExists: Bool {
    guard let url = widgetDataPath else { return false }
    return FileManager.default.fileExists(atPath: url.path)
  }

  public static func loadWidgetData() -> [WidgetFavorite]? {
    guard let dataPath = widgetDataPath else { return nil }
    do {
      let jsonData = try Data(contentsOf: dataPath)
      return try JSONDecoder().decode([WidgetFavorite].self, from: jsonData)
    } catch {
      Logger.module.error("loadWidgetData error: \(error.localizedDescription)")
      return nil
    }
  }

  public static func updateWidgetData(_ favs: [WidgetFavorite]) {
    guard let rootPath = widgetDataRoot, let dataPath = widgetDataPath else { return }
    do {
      let widgetData = try JSONEncoder().encode(favs)
      try FileManager.default.createDirectory(atPath: rootPath.path, withIntermediateDirectories: true)
      try widgetData.write(to: dataPath)
      WidgetCenter.shared.reloadTimelines(ofKind: "FavoritesWidget")
    } catch {
      Logger.module.error("updateWidgetData error: \(error.localizedDescription)")
    }
  }
}
