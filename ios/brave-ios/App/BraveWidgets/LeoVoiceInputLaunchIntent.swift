// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AppIntents
import BraveShared
import BraveWidgetsModels
import Strings

/// Must be compiled into both the app and widget extension. `OpenIntent` is
/// what tells Control Center to foreground the app; the URL-representable
/// target then arrives through the same `brave://shortcut` path as the
/// lock screen widget.
struct LeoVoiceInputLaunchIntent: OpenIntent, URLRepresentableIntent {
  static var title: LocalizedStringResource = .init(stringLiteral: Strings.Widgets.leoVoiceInputWidgetTitle)

  @Parameter(title: "Target", default: LeoVoiceInputOpenTarget.voiceInput)
  var target: LeoVoiceInputOpenTarget
}

enum LeoVoiceInputOpenTarget: String, AppEnum, URLRepresentableEnum {
  case voiceInput

  static var typeDisplayRepresentation = TypeDisplayRepresentation("Leo")
  static var caseDisplayRepresentations: [LeoVoiceInputOpenTarget: DisplayRepresentation] = [
    .voiceInput: "Voice Input"
  ]

  static var urlRepresentation: EnumURLRepresentation<LeoVoiceInputOpenTarget> {
    let url =
      "\(AppURLScheme.appURLScheme)://shortcut?path=\(WidgetShortcut.braveLeoVoiceInput.rawValue)"
    return EnumURLRepresentation(url)
  }
}
