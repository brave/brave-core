// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * Shared test utilities for assistant response components
 */

export const setupMemoryToolStringConstants = () => {
  global.S = {
    ...(global.S || {}),
    CHAT_UI_MEMORY_UPDATED_WITH_CONTENT_LABEL:
      'CHAT_UI_MEMORY_UPDATED_WITH_CONTENT_LABEL',
    CHAT_UI_MEMORY_UNDO_BUTTON_LABEL:
      'CHAT_UI_MEMORY_UNDO_BUTTON_LABEL',
    CHAT_UI_MEMORY_MANAGE_ALL_BUTTON_LABEL:
      'CHAT_UI_MEMORY_MANAGE_ALL_BUTTON_LABEL',
    CHAT_UI_MEMORY_ERROR_LABEL: 'CHAT_UI_MEMORY_ERROR_LABEL',
    CHAT_UI_MEMORY_UNDONE_LABEL: 'CHAT_UI_MEMORY_UNDONE_LABEL'
  }
}