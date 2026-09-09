/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_WORKSPACE_SERVICE_WORKER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_WORKSPACE_SERVICE_WORKER_H_

namespace content {
class BrowserContext;
}

namespace ai_chat {

// Registers the service worker that serves workspace folders, scoped to the
// whole leo-workspace host. It answers requests under
// chrome-untrusted://leo-workspace/<uuid>/files/ by asking that workspace's
// page for the file, so the folder is read through the page's
// FileSystemDirectoryHandle rather than by path in the browser.
//
// Serving from a worker is what lets a folder's files be given a response of
// their own: the worker sets each one's Content-Type, and scopes each one's CSP
// to the folder it came from. A WebUI data source can do neither - its MIME
// types come from a fixed list that answers "text/html" for anything else, and
// its CSP is per-source, so it is shared with the page.
//
// SECURITY: served files share this origin with the workspace pages, which are
// auto-granted File System Access read/write and are the only origin allowed
// navigator.modelContext, so model-authored script served from here runs with
// the latter. Sandboxing the response is not the answer: an opaque origin
// cannot be controlled by a service worker, so a sandboxed page's own
// subresources would never reach this worker.
// TODO(https://github.com/brave/brave-browser/issues/58792): Support dynamic
// origins from WebUI, so each workspace's files get an origin of their own.
//
// Registration is idempotent, and browser-side because
// OriginCanRegisterServiceWorkerFromJavascript() refuses WebUI schemes: the
// page cannot register (or unregister) a worker of its own.
void RegisterWorkspaceServiceWorker(content::BrowserContext* browser_context);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_WORKSPACE_SERVICE_WORKER_H_
