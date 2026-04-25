// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Strings

extension Strings {
  struct PlaylistOnboarding {
    public static let introducingBravePlaylist = NSLocalizedString(
      "introducingBravePlaylist",
      bundle: .module,
      value: "Introducing Brave Playlist",
      comment:
        "A title shown on the first 2 steps of onboarding for Brave Playlist. Brave Playlist is a product name"
    )

    // -------------------
    // Onboarding Stage 1
    // -------------------

    public static let playlistOnboardingDescription = NSLocalizedString(
      "playlistOnboardingDescription",
      bundle: .module,
      value:
        "Create playlists with content you love. Add media from any page.\nPlay anytime. Even offline!",
      comment:
        "A body of text explaining the feature that will be displayed under a title reading 'Introducing Brave Playlist'"
    )
    public static let advanceInitialOnboardingButtonTitle = NSLocalizedString(
      "advanceInitialOnboardingButtonTitle",
      bundle: .module,
      value: "Discover How",
      comment:
        "A button title shown below language introducing the Brave Playlist feature. i.e. 'Discover how to use use this feature'"
    )
    public static let dismissOnboardingButtonTitle = NSLocalizedString(
      "dismissOnboardingButtonTitle",
      bundle: .module,
      value: "Dismiss",
      comment:
        "A button title that allows the user to dismiss the popup explaining the Brave Playlist feature"
    )

    // -------------------
    // Onboarding Stage 2
    // -------------------

    public static let playlistInfoFeaturePointTitleOne = NSLocalizedString(
      "playlistInfoFeaturePointTitleOne",
      bundle: .module,
      value: "Add videos to Brave Playlist. Playback anytime. Even offline",
      comment: "A list of features in Playlist"
    )
    public static let playlistInfoFeaturePointSubtitleOne = NSLocalizedString(
      "playlistInfoFeaturePointSubtitleOne",
      bundle: .module,
      value: "Make playlists of your favorite content. Save mobile data. And playback ad-free.",
      comment: "A list of features in Playlist"
    )
    public static let playlistInfoFeaturePointTitleTwo = NSLocalizedString(
      "playlistInfoFeaturePointTitleTwo",
      bundle: .module,
      value: "Just tap the button to add videos to a playlist",
      comment: "A list of features in Playlist"
    )
    public static let playlistInfoFeaturePointSubtitleTwo = NSLocalizedString(
      "playlistInfoFeaturePointSubtitleTwo",
      bundle: .module,
      value: "Stay in flow: Add videos while you browse, watch later.",
      comment: "A list of features in Playlist"
    )
    public static let playlistInfoFeaturePointTitleThree = NSLocalizedString(
      "playlistInfoFeaturePointTitleThree",
      bundle: .module,
      value: "Playback is better in Playlist",
      comment: "A list of features in Playlist"
    )
    public static let playlistInfoFeaturePointSubtitleThree = NSLocalizedString(
      "playlistInfoFeaturePointSubtitleThree",
      bundle: .module,
      value: "Picture-in-picture. Background play. Control from the lockscreen. And more…",
      comment: "A list of features in Playlist"
    )
    public static let advanceStep2OnboardingButtonTitle = NSLocalizedString(
      "advanceStep2OnboardingButtonTitle",
      bundle: .module,
      value: "Try It Out!",
      comment:
        "A button title that when tapped will advance the onboarding to guide a user to add some content to Playlist"
    )
    public static let dismissStep2OnboardingButtonTitle = NSLocalizedString(
      "dismissStep2OnboardingButtonTitle",
      bundle: .module,
      value: "Maybe Later",
      comment:
        "A button title that allows the user to dismiss the popup explaining the Brave Playlist feature"
    )

    // -------------------
    // Onboarding Stage 3
    // -------------------

    public static let userActionPrompt = NSLocalizedString(
      "userActionPrompt",
      bundle: .module,
      value: "Tap %@ to add your first item",
      comment:
        "%@ will be replaced by the text 'Add to Playlist' and an icon. Guides the user to tap an icon just above to add some content to the playlist feature"
    )

    public static let userActionPromptAddToPlaylist = NSLocalizedString(
      "userActionPromptFill",
      bundle: .module,
      value: "Add to Playlist",
      comment:
        "Will be shown next to an icon inside of some other text. i.e. \"Tap 'Add to Playlist' to add your first item\""
    )

    // -------------------
    // Onboarding Stage 4
    // -------------------

    public static let confirmationTitle = NSLocalizedString(
      "confirmationTitle",
      bundle: .module,
      value: "Good Job",
      comment:
        "Guides the user to tap on an icon where the onboarding is pointing to. '%@' will be replaced with said icon."
    )
    public static let confirmationSubtitle = NSLocalizedString(
      "confirmationSubtitle",
      bundle: .module,
      value: "Content added to %@",
      comment:
        "Shown below a title that confirms the user completed the task of adding something to playlist. %@ will be replaced with the folder the content was added to. i.e. 'Content added to Play Later'"
    )
    public static let confirmationPrompt = NSLocalizedString(
      "confirmationPrompt",
      bundle: .module,
      value: "Now, let’s take a look at your Playlist.",
      comment: "Shown above a button that will let the user open the playlist feature"
    )
    public static let confirmationActionButtonTitle = NSLocalizedString(
      "confirmationActionButtonTitle",
      bundle: .module,
      value: "Take me to my Playlist",
      comment: "A button title that when tapped opens Brave Playlist"
    )
    public static let dismissStep4OnboardingButtonTitle = NSLocalizedString(
      "dismissStep4OnboardingButtonTitle",
      bundle: .module,
      value: "Close",
      comment:
        "A button title that allows the user to dismiss the popup explaining the Brave Playlist feature"
    )
  }
}

extension Strings {
  struct BraveTranslateOnboarding {
    public static let translateTitle = NSLocalizedString(
      "BraveTranslateOnboarding.translateTitle",
      bundle: .module,
      value: "Page Translations",
      comment:
        "A text element that explains we're translating pages."
    )

    public static let translateDescription = NSLocalizedString(
      "BraveTranslateOnboarding.translateDescription",
      bundle: .module,
      value:
        "Pages can be translated to languages supported by your iOS device. You may be required to configure languages if this is your first time using this feature.",
      comment:
        "A text element that describes the translation feature and explains that hte user might have to download or configure languages on their device."
    )

    public static let disableTranslateButtonTitle = NSLocalizedString(
      "BraveTranslateOnboarding.disableTranslateButtonTitle",
      bundle: .module,
      value: "Disable This Feature",
      comment:
        "A button that allows the user to disable the brave translate feature. When pressed, the page will not be automatically translated to the user's language."
    )
  }
}
