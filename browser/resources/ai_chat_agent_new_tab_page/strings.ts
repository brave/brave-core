// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$web-common/strings'

import { AiChatAgentNtpStrings } from 'gen/components/grit/brave_components_webui_strings'

declare global {
  // Expose the AiChatStrings enum on the global `S` enum.
  interface Strings {
    AiChatAgentNtpStrings: typeof AiChatAgentNtpStrings
  }
}
