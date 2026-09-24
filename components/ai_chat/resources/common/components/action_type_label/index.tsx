/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { getLocale } from '$web-common/locale'
import * as Mojom from '../../mojom'

// Returns the localized label for an action type (e.g. "Explain"). Used to
// render actions as inline chips, matching skills.
export function getActionTypeLabel(
  actionType: Mojom.ActionType,
): string | undefined {
  switch (actionType) {
    case Mojom.ActionType.SUMMARIZE_SELECTED_TEXT:
      return getLocale(S.AI_CHAT_CONTEXT_SUMMARIZE_TEXT)
    case Mojom.ActionType.EXPLAIN:
      return getLocale(S.AI_CHAT_CONTEXT_EXPLAIN)
    case Mojom.ActionType.CREATE_TAGLINE:
      return getLocale(S.AI_CHAT_CONTEXT_CREATE_TAGLINE)
    case Mojom.ActionType.CREATE_SOCIAL_MEDIA_COMMENT_SHORT:
      return getLocale(S.AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_COMMENT_SHORT)
    case Mojom.ActionType.CREATE_SOCIAL_MEDIA_COMMENT_LONG:
      return getLocale(S.AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_COMMENT_LONG)
    case Mojom.ActionType.PARAPHRASE:
      return getLocale(S.AI_CHAT_CONTEXT_PARAPHRASE)
    case Mojom.ActionType.IMPROVE:
      return getLocale(S.AI_CHAT_CONTEXT_IMPROVE)
    case Mojom.ActionType.ACADEMICIZE:
      return getLocale(S.AI_CHAT_CONTEXT_ACADEMICIZE)
    case Mojom.ActionType.PROFESSIONALIZE:
      return getLocale(S.AI_CHAT_CONTEXT_PROFESSIONALIZE)
    case Mojom.ActionType.PERSUASIVE_TONE:
      return getLocale(S.AI_CHAT_CONTEXT_PERSUASIVE_TONE)
    case Mojom.ActionType.CASUALIZE:
      return getLocale(S.AI_CHAT_CONTEXT_CASUALIZE)
    case Mojom.ActionType.FUNNY_TONE:
      return getLocale(S.AI_CHAT_CONTEXT_FUNNY_TONE)
    case Mojom.ActionType.SHORTEN:
      return getLocale(S.AI_CHAT_CONTEXT_SHORTEN)
    case Mojom.ActionType.EXPAND:
      return getLocale(S.AI_CHAT_CONTEXT_EXPAND)
    default:
      return undefined
  }
}
