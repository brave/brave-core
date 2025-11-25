// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { UndefinedToOptional } from '$web-common/mojomUtils'
import * as Mojom from './mojom'

const eventTemplate: Mojom.ConversationEntryEvent = {
  // Mojom doesn't want us to have these undefined properties (it wants the properties
  // not to be present) but typescript, based on the generated mojom JS, needs them.
  completionEvent: undefined,
  searchQueriesEvent: undefined,
  searchStatusEvent: undefined,
  selectedLanguageEvent: undefined,
  conversationTitleEvent: undefined,
  sourcesEvent: undefined,
  contentReceiptEvent: undefined,
  toolUseEvent: undefined,
}

export function getEventTemplate() {
  return { ...eventTemplate }
}

export function getCompletionEvent(text: string): Mojom.ConversationEntryEvent {
  return {
    ...eventTemplate,
    completionEvent: { completion: text },
  }
}

export function getToolUseEvent(
  toolUseEvent: UndefinedToOptional<Mojom.ToolUseEvent>,
): Mojom.ConversationEntryEvent {
  return {
    ...eventTemplate,
    toolUseEvent: {
      output: undefined,
      permissionChallenge: undefined,
      ...toolUseEvent,
    },
  }
}

export function getWebSourcesEvent(
  sources: Mojom.WebSource[],
  richResults?: string[],
): Mojom.ConversationEntryEvent {
  return {
    ...eventTemplate,
    sourcesEvent: { sources, richResults: richResults ?? [], infoBoxes: [] },
  }
}

/**
 * Creates a conversation turn filling in any optional properties as undefined
 * and basic defaults for other properties.
 * @param props Overriding properties
 * @returns a valid ConversationTurn
 */
export function createConversationTurnWithDefaults(
  props: Partial<Mojom.ConversationTurn>,
): Mojom.ConversationTurn {
  return {
    uuid: '1',
    prompt: undefined,
    selectedText: undefined,
    events: undefined,
    createdTime: { internalValue: BigInt(Date.now()) },
    edits: undefined,
    uploadedFiles: undefined,
    skill: undefined,
    fromBraveSearchSERP: false,
    modelKey: undefined,
    actionType: Mojom.ActionType.UNSPECIFIED,
    characterType: Mojom.CharacterType.ASSISTANT,
    nearVerificationStatus: undefined,
    text: '',

    ...props,
  }
}
