/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import BraveShared
import Shared

protocol Themeable {
    
    var themeableChildren: [Themeable?]? { get }
    
    // This method should _always_ call `styleChildren`, regardless of `themableChildren` value.
    func applyTheme(_ theme: Theme)
}

extension Themeable {
    var themeableChildren: [Themeable?]? { return nil }
    
    func applyTheme(_ theme: Theme) {
        styleChildren(theme: theme)
    }
    
    func styleChildren(theme: Theme) {
        self.themeableChildren?.forEach { $0?.applyTheme(theme) }
    }
}

class Theme: Equatable, Decodable {
    
    enum DefaultTheme: String, RepresentableOptionType {
        case system = "Z71ED37E-EC3E-436E-AD5F-B22748306A6B"
        case light = "ACE618A3-D6FC-45A4-94F2-1793C40AE927"
        case dark = "B900A41F-2C02-4664-9DE4-C170956339AC"
        case `private` = "C5CB0D9A-5467-432C-AB35-1A78C55CFB41"
        
        var theme: Theme {
           return Theme.from(id: self.rawValue)
        }
        
        static var normalThemesOptions: [DefaultTheme] {
            if #available(iOS 13.0, *) {
                return [DefaultTheme.system, DefaultTheme.light, DefaultTheme.dark]
            } else {
                // iOS 12 .system is treated as .light
                return [DefaultTheme.system, DefaultTheme.dark]
            }
        }
        
        public var displayString: String {
            // Due to translations needs, titles are hardcoded here, ideally they would be pulled from the
            //  theme files themselves.
            
            if #available(iOS 13.0, *) {
                // continue
            } else if self == .system {
                // iOS 12 .system is treated as .light
                return Strings.ThemesLightOption
            }
            
            switch self {
            case .system: return Strings.ThemesAutomaticOption
            case .light: return Strings.ThemesLightOption
            case .dark: return Strings.ThemesDarkOption
                
            // Should not be visible, but making explicit so compiler will capture any `DefaultTheme` modifications
            case .private: return "<invalid>"
            }
        }
    }
    
    fileprivate static let ThemeDirectory = Bundle.main.resourceURL!.appendingPathComponent("Themes")

    required init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: ThemeCodingKeys.self)
        uuid = try container.decode(String.self, forKey: .uuid)
        title = try container.decode(String.self, forKey: .title)
        url = try container.decode(URL.self, forKey: .url)
        description = try container.decode(String.self, forKey: .description)
        thumbnail = try container.decode(URL.self, forKey: .thumbnail)
        isDark = try container.decode(Bool.self, forKey: .isDark)
        enabled = try container.decode(Bool.self, forKey: .enabled)

        colors = try container.decode(Color.self, forKey: .colors)
        images = try container.decode(Image.self, forKey: .images)
    }
    
    let uuid: String
    let title: String
    let url: URL
    let description: String
    let thumbnail: URL
    let isDark: Bool
    let enabled: Bool
    
    let colors: Color
    struct Color: Decodable {
        
        init(from decoder: Decoder) throws {
            
            let container = try decoder.container(keyedBy: ThemeCodingKeys.ColorCodingKeys.self)
            let headerStr = try container.decode(String.self, forKey: .header)
            let footerStr = try container.decode(String.self, forKey: .footer)
            let homeStr = try container.decode(String.self, forKey: .home)
            let addressBarStr = try container.decode(String.self, forKey: .addressBar)
            let borderStr = try container.decode(String.self, forKey: .border)
            let accentStr = try container.decode(String.self, forKey: .accent)

            header = UIColor(colorString: headerStr)
            footer = UIColor(colorString: footerStr)
            home = UIColor(colorString: homeStr)
            addressBar = UIColor(colorString: addressBarStr)
            border = UIColor(colorString: borderStr)
            accent = UIColor(colorString: accentStr)
            
            stats = try container.decode(Stat.self, forKey: .stats)
            tints = try container.decode(Tint.self, forKey: .tints)
            transparencies = try container.decode(Transparency.self, forKey: .transparencies)
        }
        
        let header: UIColor
        let footer: UIColor
        let home: UIColor
        let addressBar: UIColor
        let border: UIColor
        let accent: UIColor
        
        let stats: Stat
        struct Stat: Decodable {
            init(from decoder: Decoder) throws {

                let container = try decoder.container(keyedBy: ThemeCodingKeys.ColorCodingKeys.StatCodingKeys.self)
                let adsStr = try container.decode(String.self, forKey: .ads)
                let trackersStr = try container.decode(String.self, forKey: .trackers)
                let httpseStr = try container.decode(String.self, forKey: .httpse)
                let timeSavedStr = try container.decode(String.self, forKey: .timeSaved)

                ads = UIColor(colorString: adsStr)
                trackers = UIColor(colorString: trackersStr)
                httpse = UIColor(colorString: httpseStr)
                timeSaved = UIColor(colorString: timeSavedStr)
            }

            let ads: UIColor
            let trackers: UIColor
            let httpse: UIColor
            let timeSaved: UIColor
        }
        
        let tints: Tint
        struct Tint: Decodable {
            init(from decoder: Decoder) throws {

                let container = try decoder.container(keyedBy: ThemeCodingKeys.ColorCodingKeys.TintCodingKeys.self)
                let homeStr = try container.decode(String.self, forKey: .home)
                let headerStr = try container.decode(String.self, forKey: .header)
                let footerStr = try container.decode(String.self, forKey: .footer)
                let addressBarStr = try container.decode(String.self, forKey: .addressBar)

                home = UIColor(colorString: homeStr)
                header = UIColor(colorString: headerStr)
                footer = UIColor(colorString: footerStr)
                addressBar = UIColor(colorString: addressBarStr)
            }

            let home: UIColor
            let header: UIColor
            let footer: UIColor
            let addressBar: UIColor
        }
        
        let transparencies: Transparency
        struct Transparency: Decodable {
            let addressBarAlpha: CGFloat
            let borderAlpha: CGFloat
        }
    }
    
    let images: Image
    struct Image: Decodable {
        init(from decoder: Decoder) throws {
            let container = try decoder.container(keyedBy: ThemeCodingKeys.ImageCodingKeys.self)
            header = try container.decode(URL.self, forKey: .header)
            footer = try container.decode(URL.self, forKey: .footer)
            home = try container.decode(URL.self, forKey: .home)
        }

        let header: URL
        let footer: URL
        let home: URL
    }
    
    /// Textual representation suitable for debugging.
    var debugDescription: String {
        return description
    }
    
    /// Returns the theme of the given Tab, if the tab is nil returns a regular theme.
    ///
    /// - parameter tab: An object representing a Tab.
    /// - returns: A Tab theme.
    static func of(_ tab: Tab?) -> Theme {
       guard let tab = tab else {
           // This is important, used when switching modes when no tab is present
           // TODO: Theme: unit test
            let themeBasedOnMode = PrivateBrowsingManager.shared.isPrivateBrowsing
                ? Preferences.General.themePrivateMode
                : Preferences.General.themeNormalMode
            return Theme.from(id: themeBasedOnMode.value)
       }
        
        let themeType = { () -> Preferences.Option<String> in
            switch TabType.of(tab) {
            case .regular:
                return Preferences.General.themeNormalMode
            case .private:
                return Preferences.General.themePrivateMode
            }
        }()
        
        let chosenTheme = DefaultTheme(rawValue: themeType.value)
        return chosenTheme?.theme ?? DefaultTheme.system.theme
    }
    
    static var themeMemoryBank: [String: Theme] = [:]
    static func from(id: String) -> Theme {
        var id = id
        if id == DefaultTheme.system.rawValue {
            id = currentSystemThemeId
        }
        
        if let inMemoryTheme = themeMemoryBank[id] {
            return inMemoryTheme
        }
        
        let themePath = Theme.ThemeDirectory.appendingPathComponent(id).appendingPathExtension("json").path
        guard
            let themeData = FileManager.default.contents(atPath: themePath),
            let theme = try? JSONDecoder().decode(Theme.self, from: themeData) else {
                // TODO: Theme: Maybe throw error, but fallback to `system` / default / light
                fatalError("Theme file not found for: \(id)... no good")
        }
        
        themeMemoryBank[id] = theme
        return theme
    }
    
    private static var currentSystemThemeId: String {
        // Should really be based off of preferences, a dark theme and light theme preference
        
        let fallback = DefaultTheme.light.rawValue
        if #available(iOS 13.0, *) {
            let isDark = UITraitCollection.current.userInterfaceStyle == .dark
            return isDark ? DefaultTheme.dark.rawValue : fallback
        }
        return fallback
    }
    
    static let allThemes: [Theme] = {
        do {
            let filenames = try FileManager.default.contentsOfDirectory(at: Theme.ThemeDirectory, includingPropertiesForKeys: [])
            
            let final = filenames.filter {
                $0.pathExtension == "json"
            }.compactMap { fullPath -> Theme? in
                var path = fullPath.lastPathComponent
                path.removeLast(5) // Removing JSON extension
                return Theme.from(id: path)
            }.filter {
                $0.enabled
            }
            
            return final
        } catch {
            fatalError("`Themes` directory is not available!")
        }
    }()
    
    static func == (lhs: Theme, rhs: Theme) -> Bool {
        return lhs.uuid == rhs.uuid
    }
}

fileprivate enum ThemeCodingKeys: String, CodingKey {
    case uuid
    case title
    case url
    case description
    case thumbnail
    case isDark
    case enabled
    
    case colors
    enum ColorCodingKeys: String, CodingKey {
        case header
        case footer
        case home
        case addressBar
        case border
        case accent
        
        case tints
        enum TintCodingKeys: String, CodingKey {
            case home
            case header
            case footer
            case addressBar
        }
        
        case transparencies
        enum TransparencyCodingKeys: String, CodingKey {
            case addressBarAlpha
            case borderAlpha
        }
        
        case stats
        enum StatCodingKeys: String, CodingKey {
            case ads
            case trackers
            case httpse
            case timeSaved
        }
    }
    
    case images
    enum ImageCodingKeys: String, CodingKey {
        case header
        case footer
        case home
    }
}
