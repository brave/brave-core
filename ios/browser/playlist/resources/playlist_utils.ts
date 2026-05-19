// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { sendWebKitMessage } from '//ios/web/public/js_messaging/resources/utils.js'

// Non-enumerable property used to stamp media elements with a unique
// identifier so a specific element can later be resolved from native code
// (e.g. to query its current playback time). The value lives on the element
// itself, which keeps it consistent across the separately injected feature
// scripts.
export const kTagPropertyName = '__brave_playlist_tag_id'

// A detected media item reported to the browser. The keys mirror the native
// `PlaylistInfo` decoder so the same payload can be consumed by both the
// legacy PlaylistScript handler and PlaylistJavaScriptFeature.
export interface PlaylistItem {
  tagId: string
  name: string
  src: string
  pageSrc: string
  pageTitle: string
  mimeType: string
  duration: number
  detected: boolean
  invisible: boolean
}

function isInfinite(value: number): boolean {
  return value === Infinity || value === -Infinity
}

// Clamps a media duration into a finite value the browser can consume. NaN
// durations (unknown) become 0 and infinite durations (live streams) are
// capped to the maximum representable value.
export function clampDuration(value: number): number {
  if (Number.isNaN(value)) {
    return 0.0
  }
  if (isInfinite(value)) {
    return Number.MAX_VALUE
  }
  return value
}

// Obtains all of the media elements on the page (video/audio) in reverse
// document order.
export function getAllMediaElements(): HTMLMediaElement[] {
  return [
    ...document.querySelectorAll<HTMLMediaElement>('video, audio'),
  ].reverse()
}

// Stamps a media element with a unique identifier (if it doesn't already have
// one) and suppresses presentation mode change events that would otherwise
// interfere with native playback. The identifier is regenerated on every call
// to match the original script's behavior: websites frequently reuse a single
// media element across in-page navigations, so re-tagging forces the element
// to be treated as newly detected media.
function tagNode(node: HTMLMediaElement): void {
  if (!(node as any)[kTagPropertyName]) {
    node.addEventListener(
      'webkitpresentationmodechanged',
      (event) => event.stopPropagation(),
      true,
    )
  }
  ;(node as any)[kTagPropertyName] = window.crypto.randomUUID()
}

// Reports a single detected media item to the browser. `sourceNode` provides
// the media `src` (which may be a child <source> element), while `target` is
// the owning <video>/<audio> element used for duration, tagging and
// visibility.
function reportMedia(
  name: string,
  sourceNode: HTMLMediaElement | HTMLSourceElement,
  target: HTMLMediaElement,
  mimeType: string,
  detected: boolean,
): void {
  // Prefer the top frame's location/title, falling back to the current frame
  // when cross-origin access throws.
  let pageSrc = ''
  let pageTitle = ''
  try {
    pageSrc = window.top!.location.href
    pageTitle = window.top!.document.title
  } catch (error) {
    pageSrc = window.location.href
    pageTitle = document.title
  }

  const item: PlaylistItem = {
    tagId: (target as any)[kTagPropertyName] ?? '',
    name: name,
    src: sourceNode.src,
    pageSrc: pageSrc,
    pageTitle: pageTitle,
    mimeType: mimeType,
    duration: clampDuration(target.duration),
    detected: detected,
    invisible: !target.parentNode,
  }
  sendWebKitMessage('PlaylistMessageHandler', item)
}

// Detects media on `target` and reports it. When the element has no direct
// `src`, its child <source> elements are inspected instead (unless
// `ignoreSource` forces direct reporting).
export function notifyNode(
  target: HTMLMediaElement,
  mimeType: string,
  detected: boolean,
  ignoreSource: boolean,
): void {
  let name = target.title
  if (!name) {
    try {
      name = window.top!.document.title
    } catch (error) {
      name = document.title
    }
  }

  // Infer the media type from the element when one wasn't provided.
  if (!mimeType) {
    if (target instanceof HTMLVideoElement) {
      mimeType = 'video'
    } else if (target instanceof HTMLAudioElement) {
      mimeType = 'audio'
    }
  }

  if (ignoreSource || target.src) {
    tagNode(target)
    reportMedia(name, target, target, mimeType, detected)
    return
  }

  for (const node of target.children) {
    if (node instanceof HTMLSourceElement && node.src) {
      tagNode(target)
      reportMedia(name, node, target, mimeType, detected)
    }
  }
}

// Scans the document for media elements and reports each to the browser.
// `ignoreSource` forces elements without a resolved `src` to still be
// inspected via their child <source> elements.
export function checkPageForMedia(ignoreSource: boolean = false): void {
  const fetchMedia = () => {
    for (const node of getAllMediaElements()) {
      const mimeType = node instanceof HTMLVideoElement ? 'video' : 'audio'
      notifyNode(node, mimeType, true, ignoreSource)
    }
  }

  fetchMedia()

  // Some pages (e.g. slow or lazily initialized players such as DailyMotion)
  // only attach their media well after load, so re-scan once more after a
  // short delay.
  setTimeout(fetchMedia, 5000)
}
