// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { ContentSettingsTypes } from './constants.js';
import { getLocalizationStringForContentType as upstream } from './site_settings_util-chromium.js'

export function getLocalizationStringForContentType(
  contentSettingsType: ContentSettingsTypes): string | null {
    // Note: Upstream throws an error if it sees an unknown content settings type.
    // So we need to return null for the ones that are not supported (which is
    // what they do too)
  switch (contentSettingsType) {
    case ContentSettingsTypes.AUTOPLAY:
    case ContentSettingsTypes.BRAVE_OPEN_AI_CHAT:
    case ContentSettingsTypes.BRAVE_SHIELDS:
    case ContentSettingsTypes.CARDANO:
    case ContentSettingsTypes.ETHEREUM:
    case ContentSettingsTypes.GOOGLE_SIGN_IN:
    case ContentSettingsTypes.LOCALHOST_ACCESS:
    case ContentSettingsTypes.SOLANA:
      return null;
  }
  return upstream(contentSettingsType);
}


export * from './site_settings_util-chromium.js'