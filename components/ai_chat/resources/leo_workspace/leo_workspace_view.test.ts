// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { filePathToUrlPath } from './leo_workspace_view'

describe('filePathToUrlPath', () => {
  it('keeps the path structure', () => {
    expect(filePathToUrlPath('docs/page.html')).toBe('/docs/page.html')
  })

  it('escapes each segment, so the service worker sees the real name', () => {
    const urlPath = filePathToUrlPath('my docs/a b#1?.html')
    expect(urlPath).toBe('/my%20docs/a%20b%231%3F.html')
    // The URL keeps the whole name in its path, as the service worker reads
    // it, rather than splitting it off into a fragment or query. (Parsed
    // against an https base, as jsdom doesn't know chrome-untrusted is a
    // standard scheme.)
    const url = new URL(urlPath, 'https://view.abc.leo-workspace')
    expect(url.hash).toBe('')
    expect(url.search).toBe('')
    // Not compared after decodeURIComponent(), which the jest polyfills stub
    // out (see components/test/testPolyfills.ts).
    expect(url.pathname).toBe(urlPath)
  })
})
