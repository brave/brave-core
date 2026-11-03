// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// PoC glue. Firefox's TranslationsDocument (bundled above) walks the page.
// Translation requests go to Brave's backend (securityOrigin + /translate_a/t).
// securityOrigin and translateApiKey are defined by translate_script.cc.

let mozillaTranslatedDoc = null
let mozillaTranslateGeneration = 0
// translate.js clears the global translateApiKey immediately after
// TranslateService() returns, so keep the copy it passes in.
let mozillaTranslateApiKey = ''

function mozillaUnwrapTranslation(payload) {
  if (typeof payload === 'string') {
    return payload
  }
  if (Array.isArray(payload)) {
    const first = payload[0]
    if (typeof first === 'string') {
      return payload.length === 1 ? first : payload.join('')
    }
    if (Array.isArray(first) && typeof first[0] === 'string') {
      return first[0]
    }
  }
  throw new Error('Unexpected translate response')
}

function mozillaTranslateEndpoint() {
  const url = new URL('/translate_a/t', securityOrigin)
  url.searchParams.set('client', 'te')
  url.searchParams.set('v', '1.0')
  if (mozillaTranslateApiKey) {
    url.searchParams.set('key', mozillaTranslateApiKey)
  }
  return url
}

async function mozillaTranslateText(sourceText, sourceLang, targetLang, isHTML) {
  const url = mozillaTranslateEndpoint()
  url.searchParams.set('format', isHTML ? 'html' : 'text')
  url.searchParams.set('sl', sourceLang || 'auto')
  url.searchParams.set('tl', targetLang)
  const body = new URLSearchParams()
  body.append('q', sourceText)
  const response = await fetch(url, {
    method: 'POST',
    credentials: 'omit',
    headers: {
      'Content-Type': 'application/x-www-form-urlencoded;charset=utf-8',
    },
    body,
  })
  if (!response.ok) {
    throw new Error('translate failed: ' + response.status)
  }
  return mozillaUnwrapTranslation(await response.json())
}

function mozillaWirePort(port, sourceLang, targetLang, generation) {
  port.onmessage = (event) => {
    const data = event.data || {}
    if (data.type === 'TranslationsPort:GetEngineStatusRequest') {
      port.postMessage({
        type: 'TranslationsPort:GetEngineStatusResponse',
        status: 'ready',
      })
      return
    }
    if (data.type === 'TranslationsPort:Passthrough') {
      port.postMessage({
        type: 'TranslationsPort:TranslationResponse',
        translationId: data.translationId,
        targetText: null,
      })
      return
    }
    if (data.type === 'TranslationsPort:CachedTranslation') {
      port.postMessage({
        type: 'TranslationsPort:TranslationResponse',
        translationId: data.translationId,
        targetText: data.cachedTranslation,
      })
      return
    }
    if (data.type !== 'TranslationsPort:TranslationRequest') {
      return
    }
    mozillaTranslateText(
        data.sourceText, sourceLang, targetLang, !!data.isHTML)
        .then((targetText) => {
          if (generation !== mozillaTranslateGeneration) {
            return
          }
          port.postMessage({
            type: 'TranslationsPort:TranslationResponse',
            translationId: data.translationId,
            targetText,
          })
        })
        .catch((error) => {
          console.error('Brave translate:', error)
          if (generation !== mozillaTranslateGeneration) {
            return
          }
          // Keep the scheduler moving if the backend rejects one chunk.
          port.postMessage({
            type: 'TranslationsPort:TranslationResponse',
            translationId: data.translationId,
            targetText: data.sourceText,
          })
        })
  }
}

function mozillaWatchFinished(doc, onProgress, generation) {
  let idleTicks = 0
  const startedAt = performance.now()
  const timer = setInterval(() => {
    if (generation !== mozillaTranslateGeneration) {
      clearInterval(timer)
      return
    }
    if (doc.engineStatus === 'error') {
      clearInterval(timer)
      onProgress(0, true, 1)
      return
    }
    const busy = doc.hasPendingTranslationRequests() ||
        doc.hasPendingCallbackOnEventLoop()
    if (busy) {
      idleTicks = 0
      return
    }
    idleTicks += 1
    const elapsed = performance.now() - startedAt
    if ((doc.engineStatus === 'ready' && idleTicks >= 3) || elapsed > 60000) {
      clearInterval(timer)
      onProgress(100, true, 0)
    }
  }, 200)
}

var google = google || {}
google.translate = {
  TranslateService: function (options) {
    const key = options && options.key
    if (typeof key === 'string' && key) {
      mozillaTranslateApiKey = key
    }
    return {
      isAvailable: function () {
        return true
      },
      getDetectedLanguage: function () {
        return (document.documentElement && document.documentElement.lang) ||
            'und'
      },
      translatePage: function (sourceLang, targetLang, onProgress) {
        const generation = ++mozillaTranslateGeneration
        try {
          if (mozillaTranslatedDoc) {
            mozillaTranslatedDoc.destroy()
            mozillaTranslatedDoc = null
          }
          const languagePair = {
            sourceLanguage: sourceLang,
            targetLanguage: targetLang,
          }
          const cache = new LRUCache(languagePair)
          const channel = new MessageChannel()
          mozillaWirePort(channel.port1, sourceLang, targetLang, generation)
          const requestNewPort = () => {
            if (generation !== mozillaTranslateGeneration ||
                !mozillaTranslatedDoc) {
              return
            }
            const next = new MessageChannel()
            mozillaWirePort(next.port1, sourceLang, targetLang, generation)
            mozillaTranslatedDoc.acquirePort(next.port2)
          }
          mozillaTranslatedDoc = new TranslationsDocument(
              document,
              sourceLang,
              targetLang,
              1,
              channel.port2,
              requestNewPort,
              function () {},
              cache,
              false,
              null)
          mozillaWatchFinished(mozillaTranslatedDoc, onProgress, generation)
        } catch (error) {
          console.error('Brave translate:', error)
          onProgress(0, true, 1)
        }
      },
      // Firefox restores by reloading the page.
      restore: function () {
        mozillaTranslateGeneration++
        if (mozillaTranslatedDoc) {
          mozillaTranslatedDoc.destroy()
          mozillaTranslatedDoc = null
        }
        location.reload()
      },
    }
  },
}

cr.googleTranslate.onTranslateElementLoad()
