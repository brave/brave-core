/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import BraveShared
import Storage
import XCGLogger

private let log = Logger.browserLogger

private let customSearchEnginesFileName = "customEngines.plist"

// MARK: - SearchEngineError

enum SearchEngineError: Error {
    case duplicate
    case failedToSave
    case invalidQuery
    case missingInformation
    case invalidURL
}

// BRAVE TODO: Move to newer Preferences class(#259)
enum DefaultEngineType: String {
    case standard = "search.default.name"
    case privateMode = "search.defaultprivate.name"
    
    var option: Preferences.Option<String?> {
        switch self {
        case .standard: return Preferences.Search.defaultEngineName
        case .privateMode: return Preferences.Search.defaultPrivateEngineName
        }
    }
}

/**
 * Manage a set of Open Search engines.
 *
 * The search engines are ordered.  Individual search engines can be enabled and disabled.  The
 * first search engine is distinguished and labeled the "default" search engine; it can never be
 * disabled.  Search suggestions should always be sourced from the default search engine.
 *
 * Two additional bits of information are maintained: whether the user should be shown "opt-in to
 * search suggestions" UI, and whether search suggestions are enabled.
 *
 * Users can set standard tab default search engine and private tab search engine.
 *
 * Consumers will almost always use `defaultEngine` if they want a single search engine, and
 * `quickSearchEngines()` if they want a list of enabled quick search engines (possibly empty,
 * since the default engine is never included in the list of enabled quick search engines, and
 * it is possible to disable every non-default quick search engine).
 *
 * The search engines are backed by a write-through cache into a ProfilePrefs instance.  This class
 * is not thread-safe -- you should only access it on a single thread (usually, the main thread)!
 */
class SearchEngines {
    fileprivate let fileAccessor: FileAccessor
    
    private let initialSearchEngines: InitialSearchEngines
    private let locale: Locale
    
    init(files: FileAccessor, locale: Locale = .current) {
        initialSearchEngines = InitialSearchEngines(locale: locale)
        self.locale = locale
        self.fileAccessor = files
        self.disabledEngineNames = getDisabledEngineNames()
        self.orderedEngines = getOrderedEngines()
    }
    
    func searchEngineSetup() {
        setInitialDefaultEngine(initialSearchEngines.defaultSearchEngine.rawValue)
    }
    
    /// If no engine type is specified this method returns search engine for regular browsing.
    func defaultEngine(forType type: DefaultEngineType? = nil) -> OpenSearchEngine {
        let engineType = type ?? (PrivateBrowsingManager.shared.isPrivateBrowsing ? .privateMode : .standard)
            
        if let name = engineType.option.value,
           let defaultEngine = orderedEngines.first(where: { $0.engineID == name || $0.shortName == name }) {
            return defaultEngine
        }
        
        let defaultEngineName = initialSearchEngines.defaultSearchEngine.rawValue
        
        let defaultEngine = orderedEngines.first(where: { $0.engineID == defaultEngineName })
        return defaultEngine ?? orderedEngines[0]
    }
    
    /// Initialize default engine and set order of remaining search engines.
    /// Call this method only at initialization(app launch or onboarding).
    /// For updating search engines use `updateDefaultEngine()` method.
    func setInitialDefaultEngine(_ engine: String) {
        // update engine
        DefaultEngineType.standard.option.value = engine
        DefaultEngineType.privateMode.option.value = engine
        
        let priorityEngine = initialSearchEngines.priorityEngine?.rawValue
        let defEngine = defaultEngine(forType: .standard)
        
        // Sort engines, priority engine at first place
        var newlyOrderedEngines = orderedEngines
            .filter { engine in engine.shortName != defEngine.shortName }
            .sorted { e1, e2 in e1.shortName < e2.shortName }
            .sorted { e, _ in e.engineID == priorityEngine }
        
        newlyOrderedEngines.insert(defEngine, at: 0)
        orderedEngines = newlyOrderedEngines
    }

    /// Updates selected default engine, order of remaining search engines remains intact.
    func updateDefaultEngine(_ engine: String, forType type: DefaultEngineType) {
        type.option.value = engine
        
        // The default engine is always enabled.
        enableEngine(defaultEngine(forType: type))
        
        // When re-sorting engines only look at default search for standard browsing.
        if type == .standard {
            // Make sure we don't alter the private mode's default since it relies on order when its not set
            if Preferences.Search.defaultPrivateEngineName.value == nil, let firstEngine = orderedEngines.first {
                // So set the default engine for private mode to whatever the default was before we changed the standard
                updateDefaultEngine(firstEngine.shortName, forType: .privateMode)
            }
            // The default engine is always first in the list.
            var newlyOrderedEngines =
                orderedEngines.filter { engine in engine.shortName != defaultEngine(forType: type).shortName }
            newlyOrderedEngines.insert(defaultEngine(forType: type), at: 0)
            orderedEngines = newlyOrderedEngines
        }
            
    }

    func isEngineDefault(_ engine: OpenSearchEngine, type: DefaultEngineType? = nil) -> Bool {
        return defaultEngine(forType: type).shortName == engine.shortName
    }

    // The keys of this dictionary are used as a set.
    fileprivate var disabledEngineNames: [String: Bool]! {
        didSet {
            Preferences.Search.disabledEngines.value = Array(self.disabledEngineNames.keys)
        }
    }

    var orderedEngines: [OpenSearchEngine]! {
        didSet {
            Preferences.Search.orderedEngines.value = self.orderedEngines.map { $0.shortName }
        }
    }

    var quickSearchEngines: [OpenSearchEngine]! {
        get {
            return self.orderedEngines.filter({ (engine) in !self.isEngineDefault(engine) && self.isEngineEnabled(engine) })
        }
    }

    var shouldShowSearchSuggestionsOptIn: Bool {
        get { return Preferences.Search.shouldShowSuggestionsOptIn.value }
        set { Preferences.Search.shouldShowSuggestionsOptIn.value = newValue }
    }
    
    var shouldShowSearchSuggestions: Bool {
        get { return Preferences.Search.showSuggestions.value }
        set { Preferences.Search.showSuggestions.value = newValue }
    }
    
    var shouldShowRecentSearchesOptIn: Bool {
        get { return Preferences.Search.shouldShowRecentSearchesOptIn.value }
        set { Preferences.Search.shouldShowRecentSearchesOptIn.value = newValue }
    }
    
    var shouldShowRecentSearches: Bool {
        get { return Preferences.Search.shouldShowRecentSearches.value }
        set { Preferences.Search.shouldShowRecentSearches.value = newValue }
    }

    func isEngineEnabled(_ engine: OpenSearchEngine) -> Bool {
        return disabledEngineNames.index(forKey: engine.shortName) == nil
    }

    func enableEngine(_ engine: OpenSearchEngine) {
        disabledEngineNames.removeValue(forKey: engine.shortName)
    }

    func disableEngine(_ engine: OpenSearchEngine) {
        if isEngineDefault(engine) {
            // Can't disable default engine.
            return
        }
        disabledEngineNames[engine.shortName] = true
    }

    func deleteCustomEngine(_ engine: OpenSearchEngine) throws {
        // We can't delete a preinstalled engine or an engine that is currently the default.
        if !engine.isCustomEngine || isEngineDefault(engine) {
            return
        }

        customEngines.remove(at: customEngines.firstIndex(of: engine)!)
        do {
            try saveCustomEngines()
        } catch {
            throw SearchEngineError.failedToSave
        }

        orderedEngines = getOrderedEngines()
    }

    /// Adds an engine to the front of the search engines list.
    func addSearchEngine(_ engine: OpenSearchEngine) throws {
        guard orderedEngines.contains(where: { $0.searchTemplate != engine.searchTemplate}) else {
            throw SearchEngineError.duplicate
        }
        
        customEngines.append(engine)
        orderedEngines.insert(engine, at: 1)
        
        do {
            try saveCustomEngines()
        } catch {
            throw SearchEngineError.failedToSave
        }
    }

    func queryForSearchURL(_ url: URL?) -> String? {
        return defaultEngine().queryForSearchURL(url)
    }

    fileprivate func getDisabledEngineNames() -> [String: Bool] {
        if let disabledEngineNames = Preferences.Search.disabledEngines.value {
            var disabledEngineDict = [String: Bool]()
            for engineName in disabledEngineNames {
                disabledEngineDict[engineName] = true
            }
            return disabledEngineDict
        } else {
            return [String: Bool]()
        }
    }

    fileprivate func customEngineFilePath() -> String {
        let profilePath = try! self.fileAccessor.getAndEnsureDirectory() as NSString // swiftlint:disable:this force_try
        return profilePath.appendingPathComponent(customSearchEnginesFileName)
    }

    fileprivate lazy var customEngines: [OpenSearchEngine] = {
        do {
            let data = try Data(contentsOf: URL(fileURLWithPath: customEngineFilePath()))
            return (try NSKeyedUnarchiver.unarchiveTopLevelObjectWithData(data) as? [OpenSearchEngine]) ?? []
        } catch {
            log.error("Failed to load custom search engines: \(error)")
            return []
        }
    }()

    fileprivate func saveCustomEngines() throws {
        do {
            let data = try NSKeyedArchiver.archivedData(withRootObject: customEngines, requiringSecureCoding: true)
            try data.write(to: URL(fileURLWithPath: customEngineFilePath()))
        } catch {
            log.error("Failed to save custom engines: \(customEngines) - \(error.localizedDescription)")
        }
    }

    /// Return all possible language identifiers in the order of most specific to least specific.
    /// For example, zh-Hans-CN will return [zh-Hans-CN, zh-CN, zh].
    class func possibilitiesForLanguageIdentifier(_ languageIdentifier: String) -> [String] {
        var possibilities: [String] = []
        let components = languageIdentifier.components(separatedBy: "-")
        possibilities.append(languageIdentifier)

        if components.count == 3, let first = components.first, let last = components.last {
            possibilities.append("\(first)-\(last)")
        }
        if components.count >= 2, let first = components.first {
            possibilities.append("\(first)")
        }
        return possibilities
    }

    /// Get all bundled (not custom) search engines, with the default search engine first,
    /// but the others in no particular order.
    class func getUnorderedBundledEngines(for selectedEngines: [String] = [],
                                          isOnboarding: Bool,
                                          locale: Locale) -> [OpenSearchEngine] {
        let parser = OpenSearchParser(pluginMode: true)

        guard let pluginDirectory = Bundle.main.resourceURL?.appendingPathComponent("SearchPlugins") else {
            assertionFailure("Search plugins not found. Check bundle")
            return []
        }

        let se = InitialSearchEngines(locale: locale)
        let engines = isOnboarding ? se.onboardingEngines : se.engines
        let engineIdentifiers: [(id: String, reference: String?)] = engines.map { (id: ($0.customId ?? $0.id.rawValue).lowercased(), reference: $0.reference) }
        assert(!engineIdentifiers.isEmpty, "No search engines")

        return engineIdentifiers.map({ (name: $0.id, path: pluginDirectory.appendingPathComponent("\($0.id).xml").path, reference: $0.reference) })
            .filter({ FileManager.default.fileExists(atPath: $0.path) })
            .compactMap({ parser.parse($0.path, engineID: $0.name, referenceURL: $0.reference) })
    }

    /// Get all known search engines, possibly as ordered by the user.
    fileprivate func getOrderedEngines() -> [OpenSearchEngine] {
        let selectedSearchEngines = [Preferences.Search.defaultEngineName, Preferences.Search.defaultPrivateEngineName].compactMap { $0.value }
        let unorderedEngines =
        SearchEngines.getUnorderedBundledEngines(for: selectedSearchEngines,
                                                    isOnboarding: false,
                                                    locale: locale) + customEngines

        // might not work to change the default.
        guard let orderedEngineNames = Preferences.Search.orderedEngines.value else {
            // We haven't persisted the engine order, so return whatever order we got from disk.
            return unorderedEngines
        }
        
        // We have a persisted order of engines, so try to use that order.
        // We may have found engines that weren't persisted in the ordered list
        // (if the user changed locales or added a new engine); these engines
        // will be appended to the end of the list.
        return unorderedEngines.sorted { engine1, engine2 in
            let index1 = orderedEngineNames.firstIndex(of: engine1.shortName)
            let index2 = orderedEngineNames.firstIndex(of: engine2.shortName)

            if index1 == nil && index2 == nil {
                return engine1.shortName < engine2.shortName
            }

            // nil < N for all non-nil values of N.
            if index1 == nil || index2 == nil {
                return index1 ?? -1 > index2 ?? -1
            }

            return index1! < index2!
        }
    }
    
    /// Function for Yahoo Search Engine Migration
    /// If Default Search Engine is Yahoo or Yahoo! JAPAN,
    /// the engine will be migrated as Custom Search Engine and set as default
    /// In Private Mode the default engine will be set as Brave Search
    func migrateDefaultYahooSearchEngines() {
        // Checking Standard Tab Engine is Yahoo and create a new custom engine for it
        let standardTabEngineName = Preferences.Search.defaultEngineName.value
        let privateTabEngineName = Preferences.Search.defaultPrivateEngineName.value

        if standardTabEngineName == OpenSearchEngine.EngineNames.yahoo {
            createDefaultYahooCustomEngine(isJapan: false, forType: .standard)
        } else if standardTabEngineName == OpenSearchEngine.EngineNames.yahooJP {
            createDefaultYahooCustomEngine(isJapan: true, forType: .standard)
        }
        
        if privateTabEngineName == OpenSearchEngine.EngineNames.yahoo {
            createDefaultYahooCustomEngine(isJapan: false, forType: .privateMode)
        } else if privateTabEngineName == OpenSearchEngine.EngineNames.yahooJP {
            createDefaultYahooCustomEngine(isJapan: true, forType: .privateMode)
        }
        
        // Marking Migration completed
        Preferences.Search.yahooEngineMigrationCompleted.value = true
    }
        
    /// Function for creating Yahoo / Yahoo JAPAN as OpenSearchEngine and setting as default
    /// - Parameter isJapan: The boolean to determine Yahoo Engine for JP locale
    private func createDefaultYahooCustomEngine(isJapan: Bool, forType type: DefaultEngineType) {
        
        // MARK: SearchEngineDetails
        
        enum SearchEngineDetails {
            case yahoo, yahooJapan
            
            var engineName: String {
                switch self {
                    case .yahooJapan:
                        return "Yahoo! JAPAN"
                    case .yahoo:
                        return "Yahoo"
                }
            }
            
            var engineTemplate: String {
                switch self {
                    case .yahooJapan:
                        return "https://search.yahoo.co.jp/search?p={searchTerms}"
                    case .yahoo:
                        return "https://search.yahoo.com/search?q={searchTerms}"
                }
            }
            
            var searchEngineImage: UIImage {
                switch self {
                    case .yahooJapan:
                        return #imageLiteral(resourceName: "faviconYahoo")
                    case .yahoo:
                        return #imageLiteral(resourceName: "faviconYahooJP")
                }
            }
        }
        
        var engineDetails =  SearchEngineDetails.yahoo
        
        if isJapan {
            engineDetails = SearchEngineDetails.yahooJapan
        }
        
        let searchEngine = OpenSearchEngine(
            engineID: OpenSearchEngine.migratedYahooEngineID,
            shortName: engineDetails.engineName,
            image: engineDetails.searchEngineImage,
            searchTemplate: engineDetails.engineTemplate,
            isCustomEngine: true)
        
        let customMigrationEngineCreated = customEngines.first(where: {
            $0.shortName == searchEngine.shortName
        })
        
        if customMigrationEngineCreated == nil {
            do {
                try addSearchEngine(searchEngine)
            } catch {
                log.error("Search Engine migration Failed for \(engineDetails.engineName)")
            }
        }
        
        updateDefaultEngine(engineDetails.engineName, forType: type)
    }
}
