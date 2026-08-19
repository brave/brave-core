// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from 'gen/brave/components/ai_chat/core/common/mojom/common.mojom.m.js'
export * from 'gen/brave/components/ai_chat/core/common/mojom/ai_chat.mojom.m.js'
export * from 'gen/brave/components/ai_chat/core/common/mojom/common.mojom.m.js'
export * from 'gen/brave/components/ai_chat/core/common/mojom/untrusted_frame.mojom.m.js'
export * from 'gen/brave/components/ai_chat/core/common/mojom/tab_tracker.mojom.m.js'
export * from 'gen/brave/components/ai_chat/core/common/mojom/bookmarks.mojom.m.js'
export * from 'gen/brave/components/ai_chat/core/common/mojom/history.mojom.m.js'
export * from 'gen/brave/components/ai_chat/core/common/mojom/ollama.mojom.m.js'

// Until this is needed outside of the UI, we don't need to add the field to the mojom.
export type AssociatedContentWithFavicon = Mojom.AssociatedContent & {
  faviconUrl?: string
}
