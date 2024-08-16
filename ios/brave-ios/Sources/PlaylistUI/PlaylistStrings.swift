// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Strings

extension Strings {
  struct Playlist {
    static let somethingWentWrong = NSLocalizedString(
      "playlist.somethingWentWrong",
      bundle: .module,
      value: "Something went wrong",
      comment: "A fallback error message displayed when a video fails to play"
    )
    static let noMediaToPickFromEmptyState = NSLocalizedString(
      "playlist.noMediaToPickFromEmptyState",
      bundle: .module,
      value: "Add media from your favorite sites and it’ll play here.",
      comment: "Shown when there is no content available to play for a given folder"
    )
    static let noMediaSelectedEmptyState = NSLocalizedString(
      "playlist.noMediaSelectedEmptyState",
      bundle: .module,
      value: "Tap videos from your playlist to play.",
      comment: "Shown when the user has not selected any media to play."
    )
    static let sleepTimerEndOfItem = NSLocalizedString(
      "playlist.sleepTimerEndOfItem",
      bundle: .module,
      value: "End of Item",
      comment:
        "An option in the sleep timer menu which describes that playback will stop when the current media item that is playing finishes."
    )
    static let sleepTimerStopPlaybackAfter = NSLocalizedString(
      "playlist.sleepTimerStopPlaybackAfter",
      bundle: .module,
      value: "Stop Playback After…",
      comment:
        "A title shown above a set of time based options that describe when playback should stop."
    )
    static let sleepTimerCancelTimer = NSLocalizedString(
      "playlist.sleepTimerCancelTimer",
      bundle: .module,
      value: "Cancel Timer",
      comment:
        "A button title shown when the sleep timer is currently active and allows the user to turn off the sleep timer."
    )
    static let sleepTimer = NSLocalizedString(
      "playlist.sleepTimer",
      bundle: .module,
      value: "Sleep Timer",
      comment:
        "Describes an icon which allows the user to set a timer that when reaches 0 will stop playback of any media."
    )
    static let newPlaylistButtonTitle = NSLocalizedString(
      "playlist.newPlaylistButtonTitle",
      bundle: .module,
      value: "Create",
      comment: "A button title that when tapped will prompt the user to create a new playlist"
    )
    static let newPlaylistPlaceholder = NSLocalizedString(
      "playlist.newPlaylistPlaceholder",
      bundle: .module,
      value: "My New Playlist",
      comment: "A placeholder shown in the text field when creating a new playlist."
    )
    static let createNewPlaylistButtonTitle = NSLocalizedString(
      "playlist.createNewPlaylistButtonTitle",
      bundle: .module,
      value: "Create",
      comment: "A button title that when tapped will create a new playlist"
    )
    static let fillNewPlaylistTitle = NSLocalizedString(
      "playlist.fillNewPlaylistTitle",
      bundle: .module,
      value: "Add from your **%@** playlist",
      comment:
        "A title shown above a list of videos in your default playlist. '%@' will be replaced with the default playlist name which has already been localized, for example: 'Add from your Play Later playlist'"
    )
    static let fillNewPlaylistSubtitle = NSLocalizedString(
      "playlist.fillNewPlaylistSubtitle",
      bundle: .module,
      value: "Tap to select media",
      comment: "A title shown above a list of videos in your default playlist."
    )
    static let saveButtonTitle = NSLocalizedString(
      "playlist.saveButtonTitle",
      bundle: .module,
      value: "Save",
      comment: "A button call to action that allows the user to save their selection"
    )
    static let doneButtonTitle = NSLocalizedString(
      "playlist.doneButtonTitle",
      bundle: .module,
      value: "Done",
      comment: "A button call to action that allows the user to dismiss the active modal"
    )
    static let liveIndicator = NSLocalizedString(
      "playlist.liveIndicator",
      bundle: .module,
      value: "Live",
      comment:
        "A label shown next to the media progress bar that when the particular media being played is being live streamed."
    )
    static let accessibilityCurrentMediaTime = NSLocalizedString(
      "playlist.accessibilityCurrentMediaTime",
      bundle: .module,
      value: "Current Media Time",
      comment:
        "A label that VoiceOver accessibility technology will read out if the user taps the media progress bar"
    )
    static let itemDownloadStatusPreparing = NSLocalizedString(
      "playlist.itemDownloadStatusPreparing",
      bundle: .module,
      value: "Preparing",
      comment: "A label shown on an item when it is being prepared for offline play"
    )
    static let accessibilityItemDownloadStatusReady = NSLocalizedString(
      "playlist.accessibilityItemDownloadStatusReady",
      bundle: .module,
      value: "Item Ready",
      comment: "A label shown on an item when the item is ready for offline play"
    )
    static let deletePlaylist = NSLocalizedString(
      "playlist.deletePlaylist",
      bundle: .module,
      value: "Delete Playlist…",
      comment:
        "A menu button that when tapped will present a confirmation dialog to delete a playlist and all its contents"
    )
    static let deletePlaylistConfirmationMessage = NSLocalizedString(
      "playlist.deletePlaylistConfirmationMessage",
      bundle: .module,
      value: "All videos on this playlist will be removed",
      comment: "A message display when the user attempts to delete a playlist."
    )
    static let renamePlaylist = NSLocalizedString(
      "playlist.renamePlaylist",
      bundle: .module,
      value: "Rename Playlist",
      comment:
        "A menu button that when tapped will present the user with a text box to change the name of the selected playlist"
    )
    static let editPlaylist = NSLocalizedString(
      "playlist.editPlaylist",
      bundle: .module,
      value: "Edit Playlist",
      comment: "A button title that when tapped will allow the user to edit the selected playlist."
    )
    static let editPlaylistShortTitle = NSLocalizedString(
      "playlist.editPlaylistShortTitle",
      bundle: .module,
      value: "Edit",
      comment: "A button title that when tapped will allow the user to edit the selected playlist."
    )
    static let accessibilityMoreMenuButtonTitle = NSLocalizedString(
      "playlist.accessibilityMoreMenuButtonTitle",
      bundle: .module,
      value: "More",
      comment:
        "A button title that VoiceOver accessibility technology will read out when highlighting the more menu button (ellipsis icon)"
    )
    static let moveItem = NSLocalizedString(
      "playlist.moveItem",
      bundle: .module,
      value: "Move",
      comment:
        "A button title that when tapped will allow the user to move the selected item to another playlist."
    )
    static let deleteItem = NSLocalizedString(
      "playlist.deleteItem",
      bundle: .module,
      value: "Delete",
      comment: "A button title that when tapped will allow the user to delete the selected item."
    )
    static let itemCountSingular = NSLocalizedString(
      "playlist.itemCountSingular",
      bundle: .module,
      value: "1 item",
      comment: "A count of how many items there are in the playlist"
    )
    static let itemCountPlural = NSLocalizedString(
      "playlist.itemCountPlural",
      bundle: .module,
      value: "%lld items",
      comment:
        "A count of how many items there are in the playlist. '%lld' would be replaced with a number, for example: 10 items"
    )
    static let repeatModeOptionNone = NSLocalizedString(
      "playlist.repeatModeOptionNone",
      bundle: .module,
      value: "None",
      comment:
        "A repeat mode option, where selecting None will mean that repeat mode is off and nothing will repeat."
    )
    static let repeatModeOptionOne = NSLocalizedString(
      "playlist.repeatModeOptionOne",
      bundle: .module,
      value: "One",
      comment:
        "A repeat mode option, where selecting One will mean that the current playing item will repeat itself when completing."
    )
    static let repeatModeOptionAll = NSLocalizedString(
      "playlist.repeatModeOptionAll",
      bundle: .module,
      value: "All",
      comment:
        "A repeat mode option, where selecting All will mean that the entire playlist will repeat from the beginning when the last item in it completes."
    )
    static let removeOfflineData = NSLocalizedString(
      "playlist.removeOfflineData",
      bundle: .module,
      value: "Remove Offline Data",
      comment: "A button title that will delete the selected items data saved to the users device."
    )
    static let saveOfflineData = NSLocalizedString(
      "playlist.saveOfflineData",
      bundle: .module,
      value: "Save Offline Data",
      comment: "A button title that will attempt to save the selected item for offline playback."
    )
    static let openInNewTab = NSLocalizedString(
      "playlist.openInNewTab",
      bundle: .module,
      value: "Open In New Tab",
      comment: "A button title that will open the selected item in a new tab."
    )
    static let openInNewPrivateTab = NSLocalizedString(
      "playlist.openInNewPrivateTab",
      bundle: .module,
      value: "Open In New Private Tab",
      comment: "A button title that will open the selected item in a new private tab."
    )
    static let share = NSLocalizedString(
      "playlist.share",
      bundle: .module,
      value: "Share",
      comment: "A button title that will present a share sheet for the selected item"
    )
    static let moveMenuItemTitle = NSLocalizedString(
      "playlist.moveMenuItemTitle",
      bundle: .module,
      value: "Move…",
      comment:
        "A button title that will display a list of menu options to move an item to a different playlist"
    )
    static let accessibilityPlay = NSLocalizedString(
      "playlist.accessibilityPlay",
      bundle: .module,
      value: "Play",
      comment:
        "A label read by VoiceOver accessibility technology that indicates the button will play some media"
    )
    static let accessibilityPause = NSLocalizedString(
      "playlist.accessibilityPause",
      bundle: .module,
      value: "Pause",
      comment:
        "A label read by VoiceOver accessibility technology that indicates the button will pause some currently playing media"
    )
    static let accessibilityShuffleModeOff = NSLocalizedString(
      "playlist.accessibilityShuffleModeOff",
      bundle: .module,
      value: "Shuffle Mode: Off",
      comment:
        "A label read by VoiceOver accessibility technology that indicates the shuffle mode button is toggled off"
    )
    static let accessibilityShuffleModeOn = NSLocalizedString(
      "playlist.accessibilityShuffleModeOn",
      bundle: .module,
      value: "Shuffle Mode: On",
      comment:
        "A label read by VoiceOver accessibility technology that indicates the shuffle mode button is toggled off"
    )
    static let accessibilityRepeatModeOff = NSLocalizedString(
      "playlist.accessibilityRepeatModeOff",
      bundle: .module,
      value: "Repeat Mode: Off",
      comment:
        "A label read by VoiceOver accessibility technology that indicates the repeat mode button is toggled off"
    )
    static let accessibilityRepeatModeOne = NSLocalizedString(
      "playlist.accessibilityRepeatModeOne",
      bundle: .module,
      value: "Repeat Mode: One",
      comment:
        "A label read by VoiceOver accessibility technology that indicates the repeat mode button is toggled to the One option (repeats single item)"
    )
    static let accessibilityRepeatModeAll = NSLocalizedString(
      "playlist.accessibilityRepeatModeAll",
      bundle: .module,
      value: "Repeat Mode: All",
      comment:
        "A label read by VoiceOver accessibility technology that indicates the repeat mode button is toggled to the All option (repeats entire playlist)"
    )
    static let accessibilityStepBack = NSLocalizedString(
      "playlist.accessibilityStepBack",
      bundle: .module,
      value: "Step Back",
      comment:
        "A label read by VoiceOver accessibility technology that indicates the button will move the media progress by a step backwards"
    )
    static let accessibilityStepForwards = NSLocalizedString(
      "playlist.accessibilityStepForwards",
      bundle: .module,
      value: "Step Forward",
      comment:
        "A label read by VoiceOver accessibility technology that indicates the button will move the media progress by a step forward"
    )
    static let accessibilityNextItem = NSLocalizedString(
      "playlist.accessibilityNextItem",
      bundle: .module,
      value: "Next Item",
      comment:
        "A label read by VoiceOver accessibility technology that indicates the button will play the next item in the queue"
    )
    static let accessibilityEnterFullscreen = NSLocalizedString(
      "playlist.accessibilityEnterFullscreen",
      bundle: .module,
      value: "Enter Fullscreen",
      comment:
        "A label read by VoiceOver accessibility technology that indicates the button will toggle fullscreen mode on"
    )
    static let accessibilityExitFullscreen = NSLocalizedString(
      "playlist.accessibilityExitFullscreen",
      bundle: .module,
      value: "Exit Fullscreen",
      comment:
        "A label read by VoiceOver accessibility technology that indicates the button will toggle fullscreen mode on"
    )
    static let accessibilityPlaybackSpeed = NSLocalizedString(
      "playlist.accessibilityPlaybackSpeed",
      bundle: .module,
      value: "Playback Speed",
      comment:
        "A label read by VoiceOver accessibility technology that indicates the button will display playback speed options"
    )
    static let accessibilityTapToToggleControls = NSLocalizedString(
      "playlist.accessibilityTapToToggleControls",
      bundle: .module,
      value: "Tap to toggle controls",
      comment:
        "A label read by VoiceOver accessibility technology that indicates that tapping the screen will display playback controls"
    )
    static let accessibilityEnterPictureInPicture = NSLocalizedString(
      "playlist.accessibilityEnterPictureInPicture",
      bundle: .module,
      value: "Enter Picture in Picture",
      comment:
        "A label read by VoiceOver accessibility technology that indicates the button will enter picture in picture mode"
    )
    static let accessibilityAirPlay = NSLocalizedString(
      "playlist.accessibilityAirPlay",
      bundle: .module,
      value: "AirPlay",
      comment:
        "A label read by VoiceOver accessibility technology that indicates the button will toggle AirPlay"
    )
  }
}
