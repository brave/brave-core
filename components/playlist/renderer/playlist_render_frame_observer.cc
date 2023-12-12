/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/renderer/playlist_render_frame_observer.h"

#include <vector>

#include "brave/components/playlist/common/features.h"
#include "brave/components/playlist/renderer/playlist_js_handler.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/platform/web_isolated_world_info.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace playlist {

PlaylistRenderFrameObserver::PlaylistRenderFrameObserver(
    content::RenderFrame* render_frame,
    const int32_t isolated_world_id)
    : RenderFrameObserver(render_frame),
      RenderFrameObserverTracker<PlaylistRenderFrameObserver>(render_frame),
      isolated_world_id_(isolated_world_id),
      javascript_handler_(std::make_unique<PlaylistJSHandler>(render_frame)) {}

PlaylistRenderFrameObserver::~PlaylistRenderFrameObserver() = default;

void PlaylistRenderFrameObserver::RunScriptsAtDocumentStart() {
  const auto& blink_preferences = render_frame()->GetBlinkPreferences();
  if (blink_preferences.hide_media_src_api) {
    HideMediaSourceAPI();
  }

  if (blink_preferences.should_detect_media_files) {
    InstallMediaDetector();
  }

  if (blink_preferences.should_inject_media_source_downloader) {
    InjectMediaSourceDownloader();
  }
}

void PlaylistRenderFrameObserver::HideMediaSourceAPI() {
  // Hide MediaSource API so that we can get downloadable URL from the page.
  // Otherwise, we get "blob: " url which we can't handle.
  // This script is from
  // https://github.com/brave/brave-ios/blob/development/Client/Frontend/UserContent/UserScripts/PlaylistSwizzler.js
  static const char16_t kScriptToHideMediaSourceAPI[] =
      uR"-(
    (function() {
      // Stub out the MediaSource API so video players do not attempt to use `blob` for streaming
      if (window.MediaSource || window.WebKitMediaSource || window.HTMLMediaElement && HTMLMediaElement.prototype.webkitSourceAddId) {
        window.MediaSource = null;
        window.WebKitMediaSource = null;
        delete window.MediaSource;
        delete window.WebKitMediaSource;
      }
    })();
    )-";

  blink::WebLocalFrame* web_frame = render_frame()->GetWebFrame();
  if (web_frame->IsProvisional())
    return;

  web_frame->ExecuteScript(blink::WebScriptSource(
      blink::WebString::FromUTF16(kScriptToHideMediaSourceAPI)));
}

void PlaylistRenderFrameObserver::InstallMediaDetector() {
  DVLOG(2) << __FUNCTION__;

  static const char16_t kScriptToDetectVideoAndAudio[] =
      uR"-(
    (function() {
      // Firstly, we try to get find all <video> or <audio> tags periodically,
      // for a a while from the start up. If we find them, then we attach 
      // MutationObservers to them to detect source URL.
      // After a given amount of time, we do this in requestIdleCallback().
      // Note that there's a global object named |pl_worker|. This worker is
      // created and bound by PlaylistJSHandler.

      const mutationSources = new Set();
      const mutationObserver = new MutationObserver(mutations => {
          mutations.forEach(mutation => { pl_worker.onMediaUpdated(mutation.target.src); })
      });
      const findNewMediaAndObserveMutation = () => {
          return document.querySelectorAll('video, audio').forEach((mediaNode) => {
              if (mutationSources.has(mediaNode)) return

              mutationSources.add(mediaNode)
              pl_worker.onMediaUpdated(mediaNode.src)
              mutationObserver.observe(mediaNode, { attributeFilter: ['src'] })
          });
      }

      const pollingIntervalId = window.setInterval(findNewMediaAndObserveMutation, 1000);
      window.setTimeout(() => {
          window.clearInterval(pollingIntervalId)
          window.requestIdleCallback(findNewMediaAndObserveMutation)
          // TODO(sko) We might want to check if idle callback is waiting too long.
          // In that case, we should get back to the polling style. And also, this
          // time could be too long for production.
      }, 20000)
    })();
    )-";

  blink::WebLocalFrame* web_frame = render_frame()->GetWebFrame();
  if (web_frame->IsProvisional()) {
    return;
  }

  web_frame->ExecuteScriptInIsolatedWorld(
      isolated_world_id_,
      blink::WebScriptSource(
          blink::WebString::FromUTF16(kScriptToDetectVideoAndAudio)),
      blink::BackForwardCacheAware::kAllow);
}

void PlaylistRenderFrameObserver::InjectMediaSourceDownloader() {
  DVLOG(2) << __FUNCTION__;

  blink::WebLocalFrame* web_frame = render_frame()->GetWebFrame();
  if (web_frame->IsProvisional())
    return;

  web_frame->ExecuteScript(blink::WebScriptSource(
      blink::WebString::FromUTF16(uR"(
// Boyer-Moore:
// https://codereview.stackexchange.com/questions/20136/uint8array-indexof-method-that-allows-to-search-for-byte-sequences
// https://gist.github.com/etrepum/6235082

function asUint8Array(input) {
  if (input instanceof Uint8Array) {
    return input
  } else if (typeof input === 'string') {
    // This naive transform only supports ASCII patterns. UTF-8 support
    // not necessary for the intended use case here.
    var arr = new Uint8Array(input.length)
    for (var i = 0; i < input.length; i++) {
      var c = input.charCodeAt(i)
      if (c > 127) {
        throw new TypeError('Only ASCII patterns are supported')
      }
      arr[i] = c
    }
    return arr
  } else {
    // Assume that it's already something that can be coerced.
    return new Uint8Array(input)
  }
}
function boyerMoore(patternBuffer) {
  // Implementation of Boyer-Moore substring search ported from page 772 of
  // Algorithms Fourth Edition (Sedgewick, Wayne)
  // http://algs4.cs.princeton.edu/53substring/BoyerMoore.java.html
  /*
  USAGE:
     // needle should be ASCII string, ArrayBuffer, or Uint8Array
     // haystack should be an ArrayBuffer or Uint8Array
     var search = boyerMoore(needle);
     var skip = search.byteLength;
     var indexes = [];
     for (var i = search(haystack); i !== -1; i = search(haystack, i + skip)) {
       indexes.push(i);
     }
  */
  var pattern = asUint8Array(patternBuffer)
  var M = pattern.length
  if (M === 0) {
    throw new TypeError('patternBuffer must be at least 1 byte long')
  }
  // radix
  var R = 256
  var rightmost_positions = new Int32Array(R)
  // position of the rightmost occurrence of the byte c in the pattern
  for (var c = 0; c < R; c++) {
    // -1 for bytes not in pattern
    rightmost_positions[c] = -1
  }
  for (var j = 0; j < M; j++) {
    // rightmost position for bytes in pattern
    rightmost_positions[pattern[j]] = j
  }
  function boyerMooreSearch(txtBuffer, start, end) {
    // Return offset of first match, -1 if no match.
    var txt = asUint8Array(txtBuffer)
    if (start === undefined) start = 0
    if (end === undefined) end = txt.length
    var pat = pattern
    var right = rightmost_positions
    var lastIndex = end - pat.length
    var lastPatIndex = pat.length - 1
    var skip
    for (var i = start; i <= lastIndex; i += skip) {
      skip = 0
      for (var j = lastPatIndex; j >= 0; j--) {
        var c = txt[i + j]
        if (pat[j] !== c) {
          skip = Math.max(1, j - right[c])
          break
        }
      }
      if (skip === 0) {
        return i
      }
    }
    return -1
  }
  boyerMooreSearch.byteLength = pattern.byteLength
  return boyerMooreSearch
}

function approxEqual(v1, v2, epsilon) {
  return Math.abs(v1 - v2) < epsilon
}

function noQueryNoFragment(url) {
  const copy = new URL(url)
  copy.searchParams.delete('vqmmojqlas')
  return copy.toString()
}

function waitForElement(root, selector) {
  return new Promise((resolve) => {
    const element = root.querySelector(selector)
    if (element) {
      return resolve(element)
    }

    const observer = new MutationObserver((_) => {
      const element = root.querySelector(selector)
      if (element) {
        observer.disconnect()
        resolve(element)
      }
    })

    observer.observe(root, { childList: true, subtree: true })
  })
}

function waitUntil(predicate) {
  const executor = (resolve) =>
    predicate() ? resolve() : setTimeout(() => executor(resolve), 50)
  return new Promise(executor)
}


const boyerMoorePatternLength = 256 // bytes
const seekDelay = 500 // ms
const videoPlayerClass = 'html5-video-player'

function reportProgress(value) {
  pl_worker.onProgress(value.toString())
}

class MediaBuilder {
  constructor() {
    this.headers = undefined
    this.parts = []
    this.adFlags = []
  }

  append(part, adFlag) {
    if (!this.headers) {
      this.headers = part
      return
    }

    this.parts.push(part)
    this.adFlags.push(adFlag)
  }

  concat(withHeaders) {
    let length = 0
    length += withHeaders ? this.headers.length : 0
    this.parts.forEach((part) => (length += part.length))

    let concated = new Uint8Array(length)
    let offset = 0
    if (withHeaders) {
      concated.set(this.headers, offset)
      offset += this.headers.length
    }

    this.parts.forEach((part) => {
      concated.set(part, offset)
      offset += part.length
    })

    return concated
  }

  isAd() {
    return (
      this.adFlags.filter((flag) => flag !== false).length >
      this.adFlags.length / 5
    )
  }
}

class MediaSourceExtension {
  constructor(id) {
    this.duration = 0
    this.id = id
    // Although MediaSource has `sourceBuffers`, and `activeSourceBuffers`,
    // we still have to maintain internal references to its source buffers,
    // as once YouTube is done playing a certain MediaSource,
    // it gets rid of its source buffers (both `sourceBuffers`, and `activeSourceBuffers` become empty).
    // This effectively disables combining content source buffers if there's mid-roll ads during the video,
    // as source buffers from MediaSources (other than the last one) are gone.
    this.sourceBuffers = []
    this.timeout = undefined
    this.url = undefined
    this.videoElement = undefined
  }

  isAd() {
    return this.sourceBuffers.some((sourceBuffer) =>
      _(sourceBuffer).mediaBuilder.isAd()
    )
  }

  reportProgress() {
    if (this.isAd()) {
      return
    }

    if (!this.videoElement || !this.url || !this.duration) {
      return
    }

    if (this.videoElement.src !== this.url) {
      return
    }

    if (this.videoElement.buffered.length !== 1) {
      return
    }

    reportProgress(
      Math.floor((this.videoElement.buffered.end(0) / this.duration) * 100)
    )
  }

  seek() {
    if (!this.videoElement) {
      // Either of the SourceBuffers' appendBuffer()
      // will initialize this.videoElement
      // as soon as <video src=${this.url} /> is available.
      // Can't seek until then.
      return
    }

    if (this.videoElement.src !== this.url) {
      // If this MediaSource (that is, the one referred to by this.url)
      // is no longer attached to <video />, don't seek.
      return
    }

    if (this.videoElement.buffered.length === 0) {
      // If <video /> has no buffered ranges yet, can't seek.
      return
    }

    if (this.videoElement.buffered.length > 1) {
      return console.error(
        `Something's wrong with our seeking logic, as there are multiple buffered ranges (as opposed to a single, continuous buffered range)! Try raising seekDelay (currently ${seekDelay})!`
      )
    }

    const bufferedEnd = this.videoElement.buffered.end(0)
    console.log(`%cSeeking to ${bufferedEnd}`, 'background: white; color: red')
    this.videoElement.currentTime = bufferedEnd
  }
}

class SourceBufferExtension {
  constructor(sourceBuffer) {
    this.bytesSoFar = 0
    this.color = `color: #${(
      '000000' + ((Math.random() * 0xffffff) << 0).toString(16)
    ).slice(-6)}`
    this.dataId = 0
    this.mediaBuilder = new MediaBuilder()
    this.mediaSource = undefined
    this.mimeType = undefined
    this.sourceBuffer = sourceBuffer
  }

  log(where, firstLine, rest) {
    console[where](
      `%c${firstLine}${rest !== undefined ? `\n${rest}` : ''}`,
      `${this.color}`
    )
  }

  isDetached() {
    let found = false
    // this.mediaSource.sourceBuffers
    // (that is, MediaSource's sourceBuffers property),
    // and not _(this.mediaSource).sourceBuffers
    for (const sourceBuffer of this.mediaSource.sourceBuffers) {
      if (sourceBuffer === this.sourceBuffer) {
        found = true
        break
      }
    }
    return !found
  }

  onUpdateStart(dataId) {
    this.log('log', `onUpdateStart() (${dataId})`)
  }

  onUpdate(dataId, data) {
    if (!(data instanceof Uint8Array)) {
      data = new Uint8Array(data)
    }
    this.mediaBuilder.append(data, adShowing)
    this.bytesSoFar += data.length

    const firstLine = `onUpdate() (${dataId}) - ${data.length.toLocaleString()} bytes (${this.bytesSoFar.toLocaleString()} bytes so far)`

    if (this.isDetached()) {
      return this.log(
        'log',
        firstLine,
        `${this.mimeType} has already been detached from MediaSource ${
          _(this.mediaSource).id
        } (${_(this.mediaSource).url}).`
      )
    }

    let rest = `MediaSource ${_(this.mediaSource).id} (${
      _(this.mediaSource).url
    }) - ${this.mimeType}${this.sourceBuffer.buffered.length !== 0 ? ':' : ''}`
    for (let i = 0; i < this.sourceBuffer.buffered.length; ++i) {
      const start = this.sourceBuffer.buffered.start(i)
      const end = this.sourceBuffer.buffered.end(i)
      rest += `\n   TimeRange ${i}: start(${start}), end(${end})`
    }

    this.log('log', firstLine, rest)
  }

  onUpdateEnd(dataId) {
    const firstLine = `onUpdateEnd() (${dataId})`

    if (this.isDetached()) {
      return this.log(
        'log',
        firstLine,
        `${this.mimeType} has already been detached from MediaSource ${
          _(this.mediaSource).id
        } (${_(this.mediaSource).url}).`
      )
    }

    this.log('log', firstLine)

    if (this.sourceBuffer.buffered.length > 1) {
      return this.log(
        'error',
        `Something's wrong with our seeking logic, as there are multiple buffered ranges (as opposed to a single, continuous buffered range) (${dataId})! Try increasing seekDelay (currently ${seekDelay})!`
      )
    }

    clearTimeout(_(this.mediaSource).timeout)
    _(this.mediaSource).timeout = setTimeout(() => {
      if (this.mediaSource.readyState !== 'ended') {
        _(this.mediaSource).seek()
      }
    }, seekDelay)
  }
}

let adShowing = location.host.includes('youtube') ? undefined : false
window.addEventListener('DOMContentLoaded', () => {
  reportProgress(0)

  waitForElement(document.body, `div[class*='${videoPlayerClass}']`).then(
    (player) => {
      console.log('%cobserving', 'color: cyan', player)

      new MutationObserver((mutations) => {
        mutations.forEach((mutation) => {
          const isAd = mutation.target.classList
            .toString()
            .includes('ad-showing')
          if (adShowing !== isAd) {
            adShowing = isAd
            console.log(
              `%c${adShowing ? 'ad' : 'content'} showing`,
              `background: ${adShowing ? 'red' : 'green'}; color: white`
            )
          }
        })
      }).observe(player, { attributeFilter: ['class'] })
    }
  )

  // A few things to keep in mind when muting the video:
  //   - can't mute via YouTube's player settings, as those are synced via cookies,
  //     hence any changes made offscreen are reflected when viewing YouTube videos onscreen
  //   - YouTube clears the muted flag on <video> quite often, it also replaces the element itself sometimes,
  //     therefore it's tricky to find the right trigger to set it.
  waitForElement(document.body, 'video').then((video) => {
    video.muted = true
    video.play()
    new MutationObserver((mutations) => {
      mutations.forEach((mutation) => {
        switch (mutation.type) {
          case 'attributes':
            if (mutation.target.nodeName === 'VIDEO') {
              mutation.target.muted = true
            }
            break
          case 'childList':
            mutation.addedNodes.forEach((node) => {
              if (node.nodeName === 'VIDEO') {
                node.muted = true
              }
            })
            break
        }
      })
    }).observe(video.parentNode, {
      attributes: true,
      childList: true,
      subtree: true
    })
  })
})

const createObjectURL = URL.createObjectURL
URL.createObjectURL = function (object) {
  const url = createObjectURL(object)
  if (object instanceof MediaSource) {
    setUpMediaSource(object, url)
  }

  return url
}

let mediaSources = []
function setUpMediaSource(mediaSource, url) {
  const mediaSourceId = _(mediaSource).id
  console.log(
    `%cSetting up MediaSource ${mediaSourceId} (${url})...`,
    'background: black; color: white'
  )

  _(mediaSource).url = url

  waitUntil(() => {
    console.log(
      `%cWaiting for MediaSource ${mediaSourceId}'s duration to be !isNaN...`,
      'background: black; color: white'
    )
    return !isNaN(mediaSource.duration)
  }).then(() => {
    // We need to make a copy of duration,
    // as YouTube sets it to 0 when it finishes playing the MediaSource.
    _(mediaSource).duration = mediaSource.duration
    console.log(
      `%cMediaSource ${mediaSourceId}'s duration: ${mediaSource.duration} seconds`,
      'background: black; color: white'
    )

    setInterval(() => {
      _(mediaSource).reportProgress()
    }, 500)
  })

  mediaSource.addEventListener('sourceended', () => {
    console.log(
      `%cMediaSource ${mediaSourceId} ended`,
      'background: red; color: white'
    )

    if (_(mediaSource).isAd()) {
      // sendMediaForPlayback(mediaSource)
      // Since mediaSource.readyState === 'ended',
      // this will seek to the end of the ad.
      return _(mediaSource).seek()
    }

    const bufferedEnd = _(mediaSource).videoElement.buffered.end(0)
    const duration = _(mediaSource).duration
    console.log(
      `%c${bufferedEnd}, ${duration}`,
      'background: blue; color: white'
    )
    if ((approxEqual(bufferedEnd, duration), 3)) {
      sendMediaForPlayback()
    }
  })

  mediaSource.addEventListener('sourceclose', () => {
    console.log(
      `%cMediaSource ${mediaSourceId} closed`,
      'background: red; color: white'
    )
  })

  mediaSources.push(mediaSource)

  const addSourceBuffer = mediaSource.addSourceBuffer
  mediaSource.addSourceBuffer = function (mimeType) {
    const sourceBuffer = addSourceBuffer.call(this, mimeType)
    setUpSourceBuffer(sourceBuffer, this, mimeType)
    return sourceBuffer
  }
}

function setUpSourceBuffer(sourceBuffer, mediaSource, mimeType) {
  console.log(
    `%cSetting up the ${mimeType} SourceBuffer for MediaSource ${
      _(mediaSource).id
    }...`,
    'background: black; color: white'
  )

  // Never set sourceBuffer.mode to 'sequence'!
  // Issues:
  //   - it disables manual seeking in the video
  //   - if you have mid-roll ads, then when the player gets back to playing the content after the ad, it starts buffering all over again, resulting in the entire video playing from the start
  _(sourceBuffer).mediaSource = mediaSource
  _(sourceBuffer).mimeType = mimeType
  _(mediaSource).sourceBuffers.push(sourceBuffer)

  const appendBuffer = sourceBuffer.appendBuffer
  sourceBuffer.appendBuffer = function (data) {
    if (!_(_(this).mediaSource).videoElement) {
      _(_(this).mediaSource).videoElement = document.querySelector(
        `video[src='${_(_(this).mediaSource).url}']`
      )
    }

    const dataId = _(this).dataId++
    _(this).log('log', `appendBuffer() (${dataId})`)

    // TODO: consider removing this
    const duration = _(_(this).mediaSource).duration
    if (duration !== 0 && this.buffered.length > 0) {
      if (this.buffered.end(0) >= duration) {
        _(this).log(
          'warn',
          `${_(this).mimeType} over-buffering (${dataId}): ${this.buffered.end(
            0
          )} >= ${duration}!`
        )
        // return
      }
    }

    this.addEventListener(
      'updatestart',
      _(this).onUpdateStart.bind(_(this), dataId),
      { once: true }
    )
    this.addEventListener(
      'update',
      _(this).onUpdate.bind(_(this), dataId, data),
      { once: true }
    )
    this.addEventListener(
      'updateend',
      _(this).onUpdateEnd.bind(_(this), dataId),
      { once: true }
    )

    return appendBuffer.call(this, data)
  }

  const abort = sourceBuffer.abort
  sourceBuffer.abort = function () {
    const where = !_(_(this).mediaSource).isAd() ? 'error' : 'log'
    _(this).log(where, 'abort() has been called!')

    return abort.call(this)
  }
}

const _ = (() => {
  let mediaSourceId = 0
  const extensionObjects = new Map()

  return (object) => {
    if (!extensionObjects.get(object)) {
      if (object instanceof MediaSource) {
        extensionObjects.set(object, new MediaSourceExtension(mediaSourceId++))
      } else if (object instanceof SourceBuffer) {
        extensionObjects.set(object, new SourceBufferExtension(object))
      } else {
        throw new Error('Unknown type!')
      }
    }

    return extensionObjects.get(object)
  }
})()

function sendMediaForPlayback(mediaSource) {
  const condition = !mediaSource
    ? (ms) => !_(ms).isAd()
    : (ms) => ms === mediaSource

  const mediaSourceIds = mediaSources.filter(condition).map((ms) => _(ms).id)
  const partsByMimeType = getPartsByMimeType(mediaSourceIds)
  removeDuplicates(partsByMimeType)
  const media = getMedia(mediaSourceIds, partsByMimeType)
  if (media) {
    pl_worker.onBlobURL(media.blobInfo[1].url)
    parent.postMessage({ type: 'media', media }, '*')
  }
}

function getPartsByMimeType(mediaSourceIds) {
  return mediaSourceIds.reduce((partsByMimeType, id, i) => {
    _(mediaSources[id]).sourceBuffers.forEach((sb) => {
      if (!partsByMimeType.get(_(sb).mimeType)) {
        partsByMimeType.set(_(sb).mimeType, [])
      }

      partsByMimeType.get(_(sb).mimeType).push(_(sb).mediaBuilder.concat(!i))
    })

    return partsByMimeType
  }, new Map())
}

function removeDuplicates(partsByMimeType) {
  partsByMimeType.forEach((parts, mimeType) => {
    for (let current = 0; current < parts.length; ++current) {
      if (current > 0) {
        const previous = current - 1
        const search = boyerMoore(
          parts[current].subarray(0, boyerMoorePatternLength)
        )
        const skip = search.byteLength
        let indices = []
        for (
          let j = search(parts[previous]);
          j !== -1;
          j = search(parts[previous], j + skip)
        ) {
          indices.push(j)
        }

        if (indices.length === 0) {
          console.warn('This is fine, but usually there are overlapping areas.')
        } else if (indices.length === 1) {
          const length = parts[previous].length
          parts[previous] = parts[previous].subarray(0, indices[0])
          console.log(
            `Removed the last ${(
              length - indices[0]
            ).toLocaleString()} bytes from part ${previous}'s ${mimeType} buffer`
          )
        } else {
          console.error(
            `The byte pattern occurs at multiple places! Try increasing boyerMoorePatternLength (currently ${boyerMoorePatternLength})!`
          )
        }
      }
    }
  })
}

function getMedia(mediaSourceIds, partsByMimeType) {
  if (mediaSourceIds.length === 0) {
    console.error('mediaSourceIds is empty!')
    return undefined
  }

  let media = {
    videoTitle: noQueryNoFragment(location.href),
    type:
      mediaSourceIds.length === 1 && _(mediaSources[mediaSourceIds[0]]).isAd()
        ? 'ad'
        : 'content',
    mediaSourceIds: mediaSourceIds.join(', '),
    blobInfo: []
  }

  let log = `${media.type} (${media.mediaSourceIds}):`
  partsByMimeType.forEach((parts, mimeType) => {
    const blob = new Blob(parts, { type: 'application/octet-stream' })
    media.blobInfo.push({ mimeType, url: URL.createObjectURL(blob) })
    log += `\n  ${mimeType}: ${blob.size.toLocaleString()} bytes`
  })
  console.log(log)

  return media
}
      )")));
}

void PlaylistRenderFrameObserver::OnDestruct() {
  delete this;
}

void PlaylistRenderFrameObserver::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (world_id != isolated_world_id_ && world_id != blink::kMainDOMWorldId) {
    return;
  }

  DVLOG(2) << __FUNCTION__
           << "Will add Playlist worker object to the frame (world_id: "
           << world_id << ")";
  javascript_handler_->AddWorkerObjectToFrame(context, world_id);
}

}  // namespace playlist
