// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Which workspace file a viewer page is pointed at, carried in the URL
// fragment. Shared by the viewer (view.tsx), which shows the file, and the
// workspace page (index.tsx), which frames a viewer for it.
//
// The fragment is used rather than a query so that retargeting a page is a
// same-document navigation: the document survives, so the workspace keeps the
// directory handle it was launched with and the viewer keeps the worker that
// serves its files. A query would also reach the WebUI data source as part of
// the request path, which is keyed on resource paths.

export const kFileHashParam = 'file'

// The workspace-relative path the fragment asks for, or null when it asks for
// no file, or for something other than a file.
export function filePathFromHash(hash: string): string | null {
  // URLSearchParams does the percent-decoding, and tolerates the leading '#'
  // being there or not.
  return new URLSearchParams(hash.replace(/^#/, '')).get(kFileHashParam) || null
}

// The fragment that asks for |path|, to append to a viewer's URL.
export function fileFragment(path: string): string {
  return `#${kFileHashParam}=${encodeURIComponent(path)}`
}
