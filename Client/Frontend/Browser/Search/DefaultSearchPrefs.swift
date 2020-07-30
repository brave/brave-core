/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import SwiftyJSON

/*
 This only makes sense if you look at the structure of List.json
*/
class DefaultSearchPrefs {
    fileprivate let defaultSearchList: [String]
    fileprivate let allLocalesSearchList: [String]
    fileprivate let locales: JSON
    fileprivate let regionOverrides: JSON
    fileprivate let globalDefaultEngine: String
    let priorityEngines: [String: JSON]?

    public init?(with filePath: URL) {
        guard let searchManifest = try? String(contentsOf: filePath) else {
            assertionFailure("Search list not found. Check bundle")
            return nil
        }

        let json = JSON(parseJSON: searchManifest)

        // Split up the JSON into useful parts
        locales = json["locales"]
        regionOverrides = json["regionOverrides"]
        priorityEngines = json["priorityEngines"].dictionary
        // These are the fallback defaults
        guard let searchList = json["default"]["visibleDefaultEngines"].array?.compactMap({ $0.string }),
            let engine = json["default"]["searchDefault"].string else {
                assertionFailure("Defaults are not set up correctly in List.json")
                return nil
        }
        defaultSearchList = searchList

        // These are to be used by all locales
        guard let allLocalesSearchList = json["allLocales"]["visibleDefaultEngines"].array?.compactMap({ $0.string }) else {
            assertionFailure("All locales are not set up correctly in List.json")
            return nil
        }
        self.allLocalesSearchList = allLocalesSearchList

        globalDefaultEngine = engine
    }
    
    /// Priority engine is placed at the top of search engines list, it has higher priority than currently selected engine.
    func priorityEngine(for locale: Locale) -> String? {
        guard let priorityEngines = priorityEngines else { return nil }
        let region = locale.regionCode ?? "US"
        return priorityEngines[region]?.string
    }

    /*
     Returns an array of the visibile engines. It overrides any of the returned engines from the regionOverrides list
     Each langauge in the locales list has a default list of engines and then a region override list.
     */
    open func visibleDefaultEngines(locales: [String], region: String, selected: [String] = []) -> [String] {
        let engineList = locales.compactMap({ self.locales[$0].dictionary }).compactMap({ (localDict) -> [JSON]? in
            return localDict[region]?["visibleDefaultEngines"].array ?? localDict["default"]?["visibleDefaultEngines"].array
        }).last?.compactMap({ $0.string })

        // If the engineList is empty then go ahead and use the default
        var usersEngineList = engineList ?? defaultSearchList

        // Append "all locales" search engines to the users engine list
        usersEngineList += allLocalesSearchList
        
        // Append preferences
        usersEngineList += engineNames(fromShortNames: selected)

        // Overrides for specfic regions.
        if let overrides = regionOverrides[region].dictionary {
            usersEngineList = usersEngineList.map({ overrides[$0]?.string ?? $0 })
        }
        
        return usersEngineList.unique { $0 == $1 }
    }
    
    private func engineNames(fromShortNames shortNames: [String]) -> [String] {
        guard let path = Bundle.main.path(forResource: "ShortNameToFileMapping", ofType: "json") else {
            assertionFailure("Search list not found. Check bundle")
            return []
        }
        
        do {
            let filePath = URL(fileURLWithPath: path)
            let jsonData = try Data(contentsOf: filePath)
            let mappings = try JSONDecoder().decode([String: String].self, from: jsonData)
            return shortNames.compactMap { mappings[$0] }
        } catch {
            // Fail silently, nothing needed for recovery here really
        }
        
        return []
    }

    /*
     Returns the default search given the possible locales and region
     The list.json locales list contains searchDefaults for a few locales.
     Create a list of these and return the last one. The globalDefault acts as the fallback in case the list is empty.
     */
    open func searchDefault(for possibileLocales: [String], and region: String) -> String {
        
        if let regionalEngine = SearchEngines.defaultRegionSearchEngines[region] {
            return regionalEngine
        }
        
        return possibileLocales.compactMap({ locales[$0].dictionary }).reduce(globalDefaultEngine) { (defaultEngine, localeJSON) -> String in
            return localeJSON[region]?["searchDefault"].string ?? defaultEngine
        }
    }
}
