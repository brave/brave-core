/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

public struct BraveUX {    
    public static let braveCommunityURL = URL(string: "https://community.brave.com/")!
    public static let braveVPNFaqURL = URL(string: "https://support.brave.com/hc/en-us/articles/360045045952")!
    public static let bravePrivacyURL = URL(string: "https://brave.com/privacy/")!
    public static let braveTodayPrivacyURL = URL(string: "https://brave.com/privacy/#brave-today")!
    public static let braveOffersURL = URL(string: "https://offers.brave.com/")!
    public static let braveTermsOfUseURL = URL(string: "https://www.brave.com/terms_of_use")!
    public static let prefKeyOptInDialogWasSeen = "OptInDialogWasSeen"
    public static let prefKeyUserAllowsTelemetry = "userallowstelemetry"
    
    public static let maxTabsInMemory = 9
    
    static var panelShadowWidth = 11
    
    public static let readerModeBarHeight = 28
    
    public static let braveOrange = UIColor(rgb: 0xfb542b)
    
    public static let blue = UIColor(rgb: 0x424acb)
    public static let lightBlue = UIColor(rgb: 0x4A90E2)
    public static let purple = UIColor(rgb: 0x8236b9)
    public static let green = UIColor(rgb: 0x1bc760)
    public static let red = UIColor(rgb: 0xE2052A)
    
    public static let white = UIColor.white
    public static let black = UIColor.black
    
    public static let greyA = UIColor(rgb: 0xF7F8F9)
    public static let greyB = UIColor(rgb: 0xE7EBEE)
    public static let greyC = UIColor(rgb: 0xDBDFE3)
    public static let greyD = UIColor(rgb: 0xCDD1D5)
    public static let greyE = UIColor(rgb: 0xA7ACB2)
    public static let greyF = UIColor(rgb: 0x999EA2)
    public static let greyG = UIColor(rgb: 0x818589)
    public static let greyH = UIColor(rgb: 0x606467)
    public static let greyI = UIColor(rgb: 0x484B4E)
    public static let greyJ = UIColor(rgb: 0x222326)
    
    public static let braveButtonMessageInUrlBarColor = braveOrange
    public static let braveButtonMessageInUrlBarShowTime = 0.5
    public static let braveButtonMessageInUrlBarFadeTime = 0.7
    
    public static let lockIconColor = greyJ
    
    public static let tabsBarPlusButtonWidth = (UIDevice.current.userInterfaceIdiom == .pad) ? 40 : 0
    
    public static let switchTintColor = greyC
    
    public static let toolbarsBackgroundSolidColor = white
    public static let darkToolbarsBackgroundSolidColor = greyJ
    public static let darkToolbarsBackgroundColor = greyJ
    
    public static let topSitesStatTitleColor = greyF
    
    // I am considering using DeviceInfo.isBlurSupported() to set this, and reduce heavy animations
    static var isHighLoadAnimationAllowed = true
    
    public static let pullToReloadDistance = 100
    
    public static let panelClosingThresholdWhenDragging = 0.3 // a percent range 1.0 to 0
    
    public static let browserViewAlphaWhenShowingTabTray = 0.3
    
    public static let backgroundColorForBookmarksHistoryAndTopSites = UIColor.white
    
    public static let backgroundColorForTopSitesPrivate = greyJ
    
    public static let backgroundColorForSideToolbars = greyA
    
    public static let colorForSidebarLineSeparators = greyB
    
    public static let popupDialogColorLight = UIColor.white
    
    // debug settings
    //  static var isToolbarHidingOff = false
    //  static var isOverrideScrollingSpeedAndMakeSlower = false // overrides IsHighLoadAnimationAllowed effect
    
    // set to true to show borders around views
    public static let debugShowBorders = false
    
    public static let backForwardDisabledButtonAlpha = CGFloat(0.3)
    public static let backForwardEnabledButtonAlpha = CGFloat(1.0)
    
    public static let topLevelBackgroundColor = UIColor.white
    
    // LocationBar Coloring
    public static let locationBarTextColor = greyJ
    
    // Setting this to clearColor() and setting LocationContainerBackgroundColor to a definitive color
    //  with transparency (e.g. allwhile 0.3 alpha) is how to make a non-opaque URL bar (e.g. for blurring).
    // Not currently needed since top bar is entirely opaque
    public static let locationBarBackgroundColor = #colorLiteral(red: 0.8431372549, green: 0.8431372549, blue: 0.8784313725, alpha: 1)
    public static let locationContainerBackgroundColor = locationBarBackgroundColor
    
    // Editing colors same as standard coloring
    public static let locationBarEditModeTextColor = locationBarTextColor
    public static let locationBarEditModeBackgroundColor = locationBarBackgroundColor
    
    // LocationBar Private Coloring
    // TODO: Add text coloring
    // See comment for LocationBarBackgroundColor is semi-transparent location bar is desired
    public static let locationBarBackgroundColorPrivateMode = black
    public static let locationContainerBackgroundColorPrivateMode = locationBarBackgroundColorPrivateMode
    
    public static let locationBarEditModeBackgroundColorPrivate = greyJ
    public static let locationBarEditModeTextColorPrivate = greyA
    
    // Interesting: compontents of the url can be colored differently: http://www.foo.com
    // Base: http://www and Host: foo.com
    public static let locationBarTextColorURLBaseComponent = greyG
    public static let locationBarTextColorURLHostComponent = locationBarTextColor
    
    public static let textFieldCornerRadius: CGFloat = 8.0
    public static let textFieldBorderColorHasFocus = greyJ
    public static let textFieldBorderColorNoFocus = greyJ
    
    public static let cancelTextColor = locationBarTextColor
    // The toolbar button color (for the Normal state). Using default highlight color ATM
    public static let actionButtonTintColor = greyI
    public static let actionButtonPrivateTintColor = greyG
    
    // The toolbar button color when (for the Selected state).
    public static let actionButtonSelectedTintColor = lightBlue
    
    public static let autocompleteTextFieldHighlightColor = blue
    
    // Yes it could be detected, just make life easier and set this number for now
    public static let bottomToolbarNumberButtonsToRightOfBackForward = 3
    public static let backForwardButtonLeftOffset = CGFloat(10)
    
    public static let progressBarColor = greyB
    public static let progressBarDarkColor = greyI
    
    public static let tabTrayCellCornerRadius = CGFloat(6.0)
    public static let tabTrayCellBackgroundColor = UIColor.white
    
    /** Determines how fast the swipe needs to be to trigger navigation action(go back/go forward).
     To determine its value, see `UIPanGestureRecognizer.velocity()` */
    public static let fastSwipeVelocity: CGFloat = 300
    
    public static let faviconBorderColor = UIColor(white: 0, alpha: 0.2)
    public static let faviconBorderWidth = 1.0 / UIScreen.main.scale
}

