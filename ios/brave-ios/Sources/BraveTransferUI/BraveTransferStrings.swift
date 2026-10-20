// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Strings

extension Strings {
  public struct BraveTransfer {
    public static let productName = NSLocalizedString(
      "braveTransfer.productName",
      bundle: .module,
      value: "Brave Transfer",
      comment:
        "The name of the feature that sends files, photos, text and links between the user's devices."
    )

    // MARK: Accept/Decline sheet

    public static let offerTitle = NSLocalizedString(
      "braveTransfer.offerTitle",
      bundle: .module,
      value: "%@ wants to send you",
      comment:
        "Title of the sheet asking whether to accept items from another device. %@ is the device name, for example \"James's MacBook\". The list of items follows."
    )
    public static let offerItemCount = NSLocalizedString(
      "braveTransfer.offerItemCount",
      bundle: .module,
      value: "%d items",
      comment:
        "Number of items in an incoming transfer, when there is more than one. %d is the count."
    )
    public static let offerMoreItems = NSLocalizedString(
      "braveTransfer.offerMoreItems",
      bundle: .module,
      value: "and %d more",
      comment:
        "Shown under the first few items of an incoming transfer. %d is how many more items there are."
    )
    public static let offerTotalSize = NSLocalizedString(
      "braveTransfer.offerTotalSize",
      bundle: .module,
      value: "Total: %@",
      comment: "Total size of an incoming transfer. %@ is a size such as \"12 MB\"."
    )
    public static let offerExpires = NSLocalizedString(
      "braveTransfer.offerExpires",
      bundle: .module,
      value: "Expires %@",
      comment:
        "When an unanswered incoming transfer is cancelled. %@ is a relative time such as \"in 2 minutes\"."
    )
    public static let offerSavedTo = NSLocalizedString(
      "braveTransfer.offerSavedTo",
      bundle: .module,
      value:
        "Photos and videos are saved to Photos. Other files are saved to Files › Brave › Downloads.",
      comment: "Explains where accepted items are saved."
    )
    public static let accept = NSLocalizedString(
      "braveTransfer.accept",
      bundle: .module,
      value: "Accept",
      comment: "Button that accepts an incoming transfer."
    )
    public static let decline = NSLocalizedString(
      "braveTransfer.decline",
      bundle: .module,
      value: "Decline",
      comment: "Button that declines an incoming transfer."
    )
    public static let textItem = NSLocalizedString(
      "braveTransfer.textItem",
      bundle: .module,
      value: "Text",
      comment: "Label for a text item in a transfer, when it has no other name."
    )
    public static let linkItem = NSLocalizedString(
      "braveTransfer.linkItem",
      bundle: .module,
      value: "Link",
      comment: "Label for a link item in a transfer, when it has no other name."
    )

    // MARK: Receive toast

    public static let receivedOne = NSLocalizedString(
      "braveTransfer.receivedOne",
      bundle: .module,
      value: "Received 1 item from %@",
      comment: "Toast after receiving one item. %@ is the sending device's name."
    )
    public static let receivedMany = NSLocalizedString(
      "braveTransfer.receivedMany",
      bundle: .module,
      value: "Received %1$d items from %2$@",
      comment:
        "Toast after receiving several items. %1$d is the count, %2$@ the sending device's name."
    )
    public static let view = NSLocalizedString(
      "braveTransfer.view",
      bundle: .module,
      value: "View",
      comment: "Toast button that opens the Brave Transfer page with the received items."
    )
    public static let keyMismatch = NSLocalizedString(
      "braveTransfer.keyMismatch",
      bundle: .module,
      value: "Blocked a connection from %@: its identity changed",
      comment:
        "Toast when a device in the user's Sync chain presented a different key than before and was rejected. %@ is the device name."
    )

    // MARK: Share extension

    public static let shareTitle = NSLocalizedString(
      "braveTransfer.shareTitle",
      bundle: .module,
      value: "Send to a computer",
      comment: "Title of the Brave Transfer share sheet."
    )
    public static let shareCancel = NSLocalizedString(
      "braveTransfer.shareCancel",
      bundle: .module,
      value: "Cancel",
      comment: "Closes the Brave Transfer share sheet."
    )
    public static let shareDone = NSLocalizedString(
      "braveTransfer.shareDone",
      bundle: .module,
      value: "Done",
      comment: "Closes the Brave Transfer share sheet after the transfer ended."
    )
    public static let sharePreparing = NSLocalizedString(
      "braveTransfer.sharePreparing",
      bundle: .module,
      value: "Preparing…",
      comment: "Shown while the share sheet reads the items to send."
    )
    public static let shareSearching = NSLocalizedString(
      "braveTransfer.shareSearching",
      bundle: .module,
      value: "Looking for your computers…",
      comment: "Shown while waiting for a computer to connect."
    )
    public static let shareEmptyState = NSLocalizedString(
      "braveTransfer.shareEmptyState",
      bundle: .module,
      value: "Open brave://transfer in Brave on your computer (same Wi-Fi).",
      comment:
        "Tells the user how to make their computer appear. brave://transfer is a page address and is not translated."
    )
    public static let shareDevicesHeader = NSLocalizedString(
      "braveTransfer.shareDevicesHeader",
      bundle: .module,
      value: "Your computers",
      comment: "Header above the list of computers the user can send to."
    )
    public static let shareContinueInBrave = NSLocalizedString(
      "braveTransfer.shareContinueInBrave",
      bundle: .module,
      value: "Continue in Brave",
      comment: "Button that hands the items to the Brave app, which sends them."
    )
    public static let shareWaitingForAccept = NSLocalizedString(
      "braveTransfer.shareWaitingForAccept",
      bundle: .module,
      value: "Waiting for %@ to accept…",
      comment: "Shown after sending an offer. %@ is the computer's name."
    )
    public static let shareSending = NSLocalizedString(
      "braveTransfer.shareSending",
      bundle: .module,
      value: "Sending to %@…",
      comment: "Shown while sending. %@ is the computer's name."
    )
    public static let shareCompleted = NSLocalizedString(
      "braveTransfer.shareCompleted",
      bundle: .module,
      value: "Sent to %@",
      comment: "Shown when the transfer finished. %@ is the computer's name."
    )
    public static let shareDeclined = NSLocalizedString(
      "braveTransfer.shareDeclined",
      bundle: .module,
      value: "%@ declined",
      comment: "Shown when the computer declined the transfer. %@ is the computer's name."
    )
    public static let shareExpired = NSLocalizedString(
      "braveTransfer.shareExpired",
      bundle: .module,
      value: "%@ didn't answer",
      comment: "Shown when the computer did not answer in time. %@ is the computer's name."
    )
    public static let shareCancelled = NSLocalizedString(
      "braveTransfer.shareCancelled",
      bundle: .module,
      value: "Cancelled",
      comment: "Shown when the transfer was cancelled."
    )
    public static let shareFailed = NSLocalizedString(
      "braveTransfer.shareFailed",
      bundle: .module,
      value: "Couldn't send",
      comment: "Shown when the transfer failed."
    )
    public static let shareDeviceUnavailable = NSLocalizedString(
      "braveTransfer.shareDeviceUnavailable",
      bundle: .module,
      value: "That computer is no longer connected.",
      comment: "Shown when the computer the user picked disconnected."
    )
    public static let shareNothingToSend = NSLocalizedString(
      "braveTransfer.shareNothingToSend",
      bundle: .module,
      value: "There's nothing here Brave Transfer can send.",
      comment: "Shown when the shared content has no supported items."
    )
    public static let handoffTooLarge = NSLocalizedString(
      "braveTransfer.handoffTooLarge",
      bundle: .module,
      value: "These items are large, so Brave will send them.",
      comment: "Explains why the share sheet is handing the items to the Brave app."
    )
    public static let handoffNoDevice = NSLocalizedString(
      "braveTransfer.handoffNoDevice",
      bundle: .module,
      value: "No computer found yet. Brave can keep looking and send when one appears.",
      comment: "Explains why the share sheet is handing the items to the Brave app."
    )
    public static let handoffLocalNetwork = NSLocalizedString(
      "braveTransfer.handoffLocalNetwork",
      bundle: .module,
      value: "Allow Brave to find devices on your network. Brave will ask, then send.",
      comment:
        "Explains that the Local Network permission is needed and that the Brave app will ask for it."
    )
    public static let handoffNotSetUp = NSLocalizedString(
      "braveTransfer.handoffNotSetUp",
      bundle: .module,
      value: "Brave Transfer needs Sync. Open Brave to set it up.",
      comment: "Explains that Sync must be on for Brave Transfer, and that the Brave app shows how."
    )
    public static let handoffOpenBrave = NSLocalizedString(
      "braveTransfer.handoffOpenBrave",
      bundle: .module,
      value: "Open Brave",
      comment: "Button that hands the items to the Brave app."
    )
    public static let handoffFailed = NSLocalizedString(
      "braveTransfer.handoffFailed",
      bundle: .module,
      value: "Couldn't hand these items to Brave.",
      comment: "Shown when copying the items for the Brave app failed."
    )
  }
}
