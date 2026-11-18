// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * Extracts just the filename from a path (e.g., "src/foo/bar.ts" -> "bar.ts")
 */
export function getFilename(path: string): string {
  const parts = path.split('/')
  return parts[parts.length - 1] || path
}

/**
 * Maps the workspace URL to the origin of the workspace page.
 * Workspace URL: workspace://<uuid>
 * Origin: chrome-untrusted://<uuid>.leo-workspace
 * Returns '' for anything that isn't a workspace URL.
 */
export function getWorkspaceOrigin(workspaceUrl: string): string {
  try {
    const url = new URL(workspaceUrl)
    if (url.protocol !== 'workspace:' || !url.hostname) {
      return ''
    }
    return `chrome-untrusted://${url.hostname}.leo-workspace`
  } catch {
    return ''
  }
}

/**
 * Builds the iframe URL from the workspace origin and file path.
 * The workspace page handles the #file= fragment to display the file.
 * Iframe URL: chrome-untrusted://<uuid>.leo-workspace/#file=<path>
 */
export function buildViewerUrl(
  workspaceOrigin: string,
  filePath: string,
): string {
  return `${workspaceOrigin}/#file=${encodeURIComponent(filePath)}`
}
