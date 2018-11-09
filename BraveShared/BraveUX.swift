/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

public struct BraveUX {    
    public static let BraveCommunityURL = URL(string: "https://community.brave.com/")!
    public static let BravePrivacyURL = URL(string: "https://brave.com/privacy/")!
    public static let BraveTermsOfUseURL = URL(string: "https://www.brave.com/terms_of_use")!
    public static let PrefKeyOptInDialogWasSeen = "OptInDialogWasSeen"
    public static let PrefKeyUserAllowsTelemetry = "userallowstelemetry"
    
    public static let MaxTabsInMemory = 9
    
    static var PanelShadowWidth = 11
    
    public static let ReaderModeBarHeight = 28
    
    public static let BraveOrange = UIColor(rgb: 0xfb542b)
    
    public static let Blue = UIColor(rgb: 0x424acb)
    public static let LightBlue = UIColor(rgb: 0x4A90E2)
    public static let Purple = UIColor(rgb: 0x8236b9)
    public static let Green = UIColor(rgb: 0x1bc760)
    public static let Red = UIColor(rgb: 0xE2052A)
    
    public static let White = UIColor.white
    public static let Black = UIColor.black
    
    public static let GreyA = UIColor(rgb: 0xF7F8F9)
    public static let GreyB = UIColor(rgb: 0xE7EBEE)
    public static let GreyC = UIColor(rgb: 0xDBDFE3)
    public static let GreyD = UIColor(rgb: 0xCDD1D5)
    public static let GreyE = UIColor(rgb: 0xA7ACB2)
    public static let GreyF = UIColor(rgb: 0x999EA2)
    public static let GreyG = UIColor(rgb: 0x818589)
    public static let GreyH = UIColor(rgb: 0x606467)
    public static let GreyI = UIColor(rgb: 0x484B4E)
    public static let GreyJ = UIColor(rgb: 0x222326)
    
    public static let BraveButtonMessageInUrlBarColor = BraveOrange
    public static let BraveButtonMessageInUrlBarShowTime = 0.5
    public static let BraveButtonMessageInUrlBarFadeTime = 0.7
    
    public static let lockIconColor = GreyJ
    
    public static let TabsBarPlusButtonWidth = (UIDevice.current.userInterfaceIdiom == .pad) ? 40 : 0
    
    public static let SwitchTintColor = GreyC
    
    public static let ToolbarsBackgroundSolidColor = White
    public static let DarkToolbarsBackgroundSolidColor = GreyJ
    public static let DarkToolbarsBackgroundColor = GreyJ
    
    public static let TopSitesStatTitleColor = GreyF
    
    // I am considering using DeviceInfo.isBlurSupported() to set this, and reduce heavy animations
    static var IsHighLoadAnimationAllowed = true
    
    public static let PullToReloadDistance = 100
    
    public static let PanelClosingThresholdWhenDragging = 0.3 // a percent range 1.0 to 0
    
    public static let BrowserViewAlphaWhenShowingTabTray = 0.3
    
    public static let BackgroundColorForBookmarksHistoryAndTopSites = UIColor.white
    
    public static let BackgroundColorForTopSitesPrivate = GreyJ
    
    public static let BackgroundColorForSideToolbars = GreyA
    
    public static let ColorForSidebarLineSeparators = GreyB
    
    public static let PopupDialogColorLight = UIColor.white
    
    // debug settings
    //  static var IsToolbarHidingOff = false
    //  static var IsOverrideScrollingSpeedAndMakeSlower = false // overrides IsHighLoadAnimationAllowed effect
    
    // set to true to show borders around views
    public static let DebugShowBorders = false
    
    public static let BackForwardDisabledButtonAlpha = CGFloat(0.3)
    public static let BackForwardEnabledButtonAlpha = CGFloat(1.0)
    
    public static let TopLevelBackgroundColor = UIColor.white
    
    // LocationBar Coloring
    public static let LocationBarTextColor = GreyJ
    
    // Setting this to clearColor() and setting LocationContainerBackgroundColor to a definitive color
    //  with transparency (e.g. allwhile 0.3 alpha) is how to make a non-opaque URL bar (e.g. for blurring).
    // Not currently needed since top bar is entirely opaque
    public static let LocationBarBackgroundColor = GreyB
    public static let LocationContainerBackgroundColor = LocationBarBackgroundColor
    
    // Editing colors same as standard coloring
    public static let LocationBarEditModeTextColor = LocationBarTextColor
    public static let LocationBarEditModeBackgroundColor = LocationBarBackgroundColor
    
    // LocationBar Private Coloring
    // TODO: Add text coloring
    // See comment for LocationBarBackgroundColor is semi-transparent location bar is desired
    public static let LocationBarBackgroundColor_PrivateMode = Black
    public static let LocationContainerBackgroundColor_PrivateMode = LocationBarBackgroundColor_PrivateMode
    
    public static let LocationBarEditModeBackgroundColor_Private = GreyJ
    public static let LocationBarEditModeTextColor_Private = GreyA
    
    // Interesting: compontents of the url can be colored differently: http://www.foo.com
    // Base: http://www and Host: foo.com
    public static let LocationBarTextColor_URLBaseComponent = GreyG
    public static let LocationBarTextColor_URLHostComponent = LocationBarTextColor
    
    public static let TextFieldCornerRadius: CGFloat = 8.0
    public static let TextFieldBorderColor_HasFocus = GreyJ
    public static let TextFieldBorderColor_NoFocus = GreyJ
    
    public static let CancelTextColor = LocationBarTextColor
    // The toolbar button color (for the Normal state). Using default highlight color ATM
    public static let ActionButtonTintColor = GreyI
    public static let ActionButtonPrivateTintColor = GreyG
    
    // The toolbar button color when (for the Selected state).
    public static let ActionButtonSelectedTintColor = LightBlue
    
    public static let AutocompleteTextFieldHighlightColor = Blue
    
    // Yes it could be detected, just make life easier and set this number for now
    public static let BottomToolbarNumberButtonsToRightOfBackForward = 3
    public static let BackForwardButtonLeftOffset = CGFloat(10)
    
    public static let ProgressBarColor = GreyB
    public static let ProgressBarDarkColor = GreyI
    
    public static let TabTrayCellCornerRadius = CGFloat(6.0)
    public static let TabTrayCellBackgroundColor = UIColor.white
    
    /** Determines how fast the swipe needs to be to trigger navigation action(go back/go forward).
     To determine its value, see `UIPanGestureRecognizer.velocity()` */
    public static let fastSwipeVelocity: CGFloat = 300
    
    public static let faviconBorderColor = UIColor(white: 0, alpha: 0.2)
    public static let faviconBorderWidth = 1.0 / UIScreen.main.scale
}

