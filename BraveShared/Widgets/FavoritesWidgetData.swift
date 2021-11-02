// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SDWebImage
import Shared
import WidgetKit

private let log = Logger.browserLogger

public struct WidgetFavorite: Codable {
    public var url: URL
    public var favicon: FaviconAttributes?
    
    public init(url: URL, favicon: FaviconAttributes) {
        self.url = url
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
            log.error("loadWidgetData error: \(error)")
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
            log.error("updateWidgetData error: \(error)")
        }
    }
}
