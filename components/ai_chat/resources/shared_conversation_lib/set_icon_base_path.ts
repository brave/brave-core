// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { setIconBasePath } from '@brave/leo/react/icon'

// Set the nala icon path to be relative to this script, which should be
// the root of the output bundle.
// Need to store import.meta.url in a separate variable to the URL building
// otherwise webpack will try to resolve '.' locally (and probably fail).
//
// This lives in its own side-effect module (rather than inline in
// render_conversation.tsx) because `import.meta` cannot be parsed by the
// CommonJS-based unit test runner. Keeping it here lets render_conversation
// (and its exported renderConversation()) be imported in tests, which mock
// this module out.
const scriptUrl = import.meta.url
const relativePathUrl = new URL('./nala-icons', scriptUrl)
setIconBasePath(relativePathUrl.toString())
