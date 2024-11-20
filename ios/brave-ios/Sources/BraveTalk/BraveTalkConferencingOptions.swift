// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import JitsiMeetSDK

extension JitsiMeetConferenceOptions {
  /// Creates conferencing options for use in a Brave Talk room
  static func braveTalkOptions(
    room: String,
    token: String,
    host: String
  ) -> Self {
    Self.fromBuilder { builder in
      builder.room = room
      builder.token = token
      if host == "talk.brave.software" {
        builder.serverURL = URL(string: "https://stage.8x8.vc")
      } else {
        builder.serverURL = URL(string: "https://8x8.vc")
      }
      builder.setFeatureFlag("calendar.enabled", withBoolean: false)
      builder.setFeatureFlag("resolution", withValue: 360)
      builder.setFeatureFlag("add-people.enabled", withBoolean: false)
      builder.setFeatureFlag("invite-dial-in.enabled", withBoolean: false)
      builder.setSubject("Brave Talk")
      builder.setConfigOverride("localSubject", withValue: "Brave Talk")
      builder.setConfigOverride("analytics", with: ["disabled": true, "rtcstatsEnabled": false])
      if let roomId = room.split(separator: "/").last {
        builder.setConfigOverride("brandingRoomAlias", withValue: roomId)
      }
      builder.setConfigOverride("callStatsID", withBoolean: false)
      builder.setConfigOverride("callStatsSecret", withBoolean: false)
      builder.setConfigOverride("disabledSounds", with: ["E2EE_OFF_SOUND", "E2EE_ON_SOUND"])
      builder.setConfigOverride("disableGTM", withBoolean: true)
      builder.setConfigOverride("doNotStoreRoom", withBoolean: true)
      builder.setConfigOverride("disableBeforeUnloadHandlers", withBoolean: true)
      builder.setConfigOverride("disableInviteFunctions", withBoolean: false)
      builder.setConfigOverride("disableTileEnlargement", withBoolean: true)
      builder.setConfigOverride("dropbox", with: ["appKey": NSNull()])
      builder.setConfigOverride(
        "e2eeLabels",
        with: [
          "e2ee": "Video Bridge Encryption",
          "labelToolTip":
            "Audio and Video Communication on this call is encrypted on the video bridge",
          "description":
            "Video Bridge Encryption is currently EXPERIMENTAL. Please keep in mind that turning it on will effectively disable server-side provided services such as: phone participation. Also keep in mind that the meeting will only work for people joining from browsers with support for insertable streams.  Note that chats will not use this encryption.",
          "label": "Enable Video Bridge Encryption",
          "warning":
            "WARNING: Not all participants in this meeting seem to have support for Video Bridge Encryption. If you enable it they won't be able to see nor hear you.",
        ]
      )
      builder.setConfigOverride("enableTalkWhileMuted", withBoolean: false)
      builder.setConfigOverride("hideEmailInSettings", withBoolean: true)
      builder.setConfigOverride("inviteAppName", withValue: "Brave Talk")
      builder.setConfigOverride("prejoinPageEnabled", withBoolean: true)
      builder.setConfigOverride("startWithAudioMuted", withBoolean: true)
      builder.setConfigOverride("startWithVideoMuted", withBoolean: true)
      builder.setConfigOverride("toolbarConfig", with: ["autoHideWhileChatIsOpen": true])
      builder.setConfigOverride("transcribingEnabled", withBoolean: false)
      builder.setConfigOverride("useHostPageLocalStorage", withBoolean: true)
      builder.setConfigOverride("videoQuality", with: ["persist": true])
      builder.setConfigOverride("testing.mobileXmppWsThreshold", withValue: 100)
    }
  }
}
