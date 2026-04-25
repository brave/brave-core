// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Strings

extension Strings {
  enum CredentialProvider {
    public static let onboardingViewTitle = NSLocalizedString(
      "CredentialProvider.onboardingViewTitle",
      bundle: .module,
      value: "AutoFill Is On",
      comment:
        "The title on a screen shown when the user turns on Brave as a password autofill provider"
    )
    public static let onboardingViewSubtitle = NSLocalizedString(
      "CredentialProvider.onboardingViewSubtitle",
      bundle: .module,
      value: "You can use saved passwords in other apps on your device",
      comment:
        "The subtitle on a screen shown when the user turns on Brave as a password autofill provider"
    )
    public static let onboardingViewFootnote = NSLocalizedString(
      "CredentialProvider.onboardingViewFootnote",
      bundle: .module,
      value: "To get to your passwords faster, deselect iCloud Keychain",
      comment:
        "A footnote shown on a screen shown when the user turns on Brave as a password autofill provider. 'iCloud Keychain' is an Apple product."
    )
    public static let onboardingViewContinueCTA = NSLocalizedString(
      "CredentialProvider.onboardingViewContinueCTA",
      bundle: .module,
      value: "Continue",
      comment: "The title on a button that dismisses the onboarding view"
    )
    public static let credentialListTitle = NSLocalizedString(
      "CredentialProvider.credentialListTitle",
      bundle: .module,
      value: "Brave Passwords",
      comment:
        "The title shown at the top of a list of credentials. 'Brave' in this case is the company name"
    )
    public static let loginsForWebsite = NSLocalizedString(
      "CredentialProvider.loginsForWebsite",
      bundle: .module,
      value: "Logins for %@",
      comment:
        "A header label shown above a list of logins (username/password combinations) for a particular website. '%@' is replaced by a website host for example: 'Logins for apple.com'"
    )
    public static let otherLogins = NSLocalizedString(
      "CredentialProvider.otherLogins",
      bundle: .module,
      value: "Other Logins",
      comment:
        "A header label shown above a list of website logins (username/password combinations)"
    )
    public static let viewCredentialDetails = NSLocalizedString(
      "CredentialProvider.viewCredentialDetails",
      bundle: .module,
      value: "View Details",
      comment:
        "An accessibility label read out when a user activates VoiceOver on a button that when taps drills down into a credentials details"
    )
    public static let cancelButtonTitle = NSLocalizedString(
      "CredentialProvider.cancelButtonTitle",
      bundle: .module,
      value: "Cancel",
      comment: "A button that cancels the autofill request and dismisses the screen"
    )
    public static let searchBarPrompt = NSLocalizedString(
      "CredentialProvider.searchBarPrompt",
      bundle: .module,
      value: "Search Logins",
      comment: "The placeholder/prompt that is shown in the search bar"
    )
    public static let emptySuggestions = NSLocalizedString(
      "CredentialProvider.emptySuggestions",
      bundle: .module,
      value: "No Suggestions",
      comment:
        "Some text shown when there are no suggested logins for the website the user is trying to autofill on"
    )
    public static let searchEmptyResults = NSLocalizedString(
      "CredentialProvider.searchEmptyResults",
      bundle: .module,
      value: "No results for \"%@\"",
      comment: "A label shown when the users search has no results"
    )
    public static let detailsFormURLField = NSLocalizedString(
      "CredentialProvider.detailsFormURLField",
      bundle: .module,
      value: "URL",
      comment: "The URL field in the details form. Shows a website URL next to it"
    )
    public static let detailsFormUsernameField = NSLocalizedString(
      "CredentialProvider.detailsFormUsernameField",
      bundle: .module,
      value: "Username",
      comment:
        "The username field in the details form. Shows the username for the website next to it"
    )
    public static let detailsFormPasswordField = NSLocalizedString(
      "CredentialProvider.detailsFormPasswordField",
      bundle: .module,
      value: "Password",
      comment:
        "The password field in the details form. Shows the password (if revealed) or a mask of the users password for the website next to it"
    )
    public static let revealPasswordMenuItemTitle = NSLocalizedString(
      "CredentialProvider.revealPasswordMenuItemTitle",
      bundle: .module,
      value: "Reveal Password",
      comment:
        "A title for a button that when tapped reveals a users password that is currently masked"
    )
    public static let hidePasswordMenuItemTitle = NSLocalizedString(
      "CredentialProvider.hidePasswordMenuItemTitle",
      bundle: .module,
      value: "Hide Password",
      comment: "A title for a button that when tapped hides a password that was previously revealed"
    )
    public static let copyMenuItemTitle = NSLocalizedString(
      "CredentialProvider.copyMenuItemTitle",
      bundle: .module,
      value: "Copy",
      comment:
        "A title for a button that when tapped copies the content highlighted into the users clipboard"
    )
    public static let useButtonTitle = NSLocalizedString(
      "CredentialProvider.useButtonTitle",
      bundle: .module,
      value: "Use",
      comment:
        "A title for a button shown as a confirmation action on a login's detail screen. Tapping it uses the password when autofilling into the website"
    )
    public static let authenticationUnlockButtonTitle = NSLocalizedString(
      "CredentialProvider.authenticationUnlockButtonTitle",
      bundle: .module,
      value: "Unlock",
      comment:
        "A title for a button shown to a user to authenticate themselves to display their passwords"
    )
    public static let authenticationReason = NSLocalizedString(
      "CredentialProvider.authenticationReason",
      bundle: .module,
      value: "Authenticate to access your passwords",
      comment:
        "A message shown when a user tries to access their passwords and are presented with a Touch ID or passcode prompt."
    )
    public static let setPasscodeAlertTitle =
      NSLocalizedString(
        "CredentialProvider.setPasscodeAlertTitle",
        bundle: .module,
        value: "Set a Passcode",
        comment: "The title displayed in alert when a user needs to set passcode"
      )
    public static let setPasscodeAlertMessage =
      NSLocalizedString(
        "CredentialProvider.setPasscodeAlertMessage",
        bundle: .module,
        value: "To access your passwords, you must first set a passcode on your device.",
        comment: "The message displayed in alert when a user needs to set a passcode"
      )
  }
}
