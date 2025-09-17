// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// securityOrigin is predefined by translate_script.cc.
const securityOriginHost = new URL(securityOrigin).host;

// A method to rewrite URL in the scripts:
// 1. change the domain to translate.brave.com;
// 2. adjust static paths to use braveTranslateStaticPath.
const rewriteUrl = (url) => {
  try {
    let new_url = new URL(url);
    if (new_url.pathname === '/translate_a/t') {
      // useGoogleTranslateEndpoint is predefined by translate_script.cc.
      // It's used only for local testing to disable the redirection of
      // translation requests.
      if (useGoogleTranslateEndpoint) {
        // Remove API key
        new_url.searchParams.set('key', '');

        // Leave the domain unchanged (translate.googleapis.com).
        return new_url.toString();
      }
    } else {
      // braveTranslateStaticPath is predefined by translate_script.cc.
      new_url.pathname = new_url.pathname.replace('/translate_static/',
        braveTranslateStaticPath);
    }
    new_url.host = securityOriginHost;
    return new_url.toString();
  } catch {
    return url;
  }
};

const emptySvgDataUrl = 'data:image/svg+xml;base64,' +
  btoa('<svg xmlns="http://www.w3.org/2000/svg"/>');

// Make replacements in loading .js files.
function processJavascript(text) {
  // Replace gen204 telemetry requests with loading an empty svg.
  text = text.replaceAll('"//"+po+"/gen204?"+Bo(b)',
    '"' + emptySvgDataUrl + '"');

  // Used in the injected elements, that are currently not visible. Replace it
  // to hide the loading error in devtools (because of CSP).
  text = text.replaceAll(
    'https://www.gstatic.com/images/branding/product/1x/translate_24dp.png',
    emptySvgDataUrl);
  return text;
}

// Make replacements in loading .css files.
function processCSS(text) {
  // Used in the injected elements, that are currently not visible. Replace it
  // to hide the loading error in devtools (because of CSP).
  text = text.replaceAll(
    '//www.gstatic.com/images/branding/product/2x/translate_24dp.png',
    emptySvgDataUrl);
  return text;
}

// Used to rewrite urls for XHRs in the translate isolated world
// (primarily for /translate_a/t).
if (typeof XMLHttpRequest.prototype.realOpen === 'undefined') {
  XMLHttpRequest.prototype.realOpen = XMLHttpRequest.prototype.open;
  XMLHttpRequest.prototype.open = function (method, url, async = true,
    user = '', password = '') {
    this.realOpen(method, rewriteUrl(url), async, user,
      password);
  }
};

// An overridden version of onLoadJavascript from translate.js, that fetches
// and evaluates secondary scripts (i.e. main.js).
// The differences:
// 1. change url via rewriteUrl();
// 2. process the loaded code via processJavascript().
cr.googleTranslate.onLoadJavascript = function (url) {
  const xhr = new XMLHttpRequest();
  xhr.open('GET', rewriteUrl(url), true);
  xhr.onreadystatechange = function () {
    if (this.readyState !== this.DONE) {
      return;
    }
    if (this.status !== 200) {
      errorCode = ERROR['SCRIPT_LOAD_ERROR'];
      return;
    }

    // nosemgrep
    new Function(processJavascript(this.responseText)).call(window);
  };
  xhr.send();
};

// The styles to hide root elements that are injected by the scripts in the DOM.
// Currently they are always invisible. The styles are added in case of changes
// in future versions.
const braveExtraStyles = `.goog-te-spinner-pos, #goog-gt-tt {display: none;}`

// An overridden version of onLoadCSS from translate.js.
// The differences:
// 1. change url via rewriteUrl();
// 2. process the loaded styles via processCSS().
// 3. Add braveExtraStyles in the end.
cr.googleTranslate.onLoadCSS = function (url) {
  const xhr = new XMLHttpRequest();
  xhr.open('GET', rewriteUrl(url), true);
  xhr.onreadystatechange = function () {
    if (this.readyState !== this.DONE || this.status !== 200) {
      return;
    }

    const element = document.createElement('style');
    element.type = 'text/css';
    element.charset = 'UTF-8';
    element.innerText = processCSS(this.responseText) + braveExtraStyles;
    document.head.appendChild(element);
  };
  xhr.send();
};

// Brave: On-device translation override functionality
// This section adds local translation capabilities while maintaining
// compatibility

// Local Translator API state
let localTranslator = null;
let localTranslationReady = false;
let originalTextMap = new Map();

/**
 * Check if Translator API is available in the browser
 * @return {boolean} True if Translator API is available
 */
function isTranslatorAPIAvailable() {
  return typeof window.Translator !== 'undefined' &&
         typeof window.Translator.create === 'function';
}

/**
 * Initialize local translator for the given language pair
 * @param {string} sourceLanguage Source language code
 * @param {string} targetLanguage Target language code
 * @return {Promise<boolean>} Promise resolving to success status
 */
async function initializeLocalTranslator(sourceLanguage, targetLanguage) {
  if (!isTranslatorAPIAvailable()) {
    console.log('Brave: Translator API not available, using server ' +
                'translation');
    return false;
  }

  try {
    console.log(`Brave: Creating local translator for ${sourceLanguage} -> ` +
                `${targetLanguage}`);

    localTranslator = await window.Translator.create({
      sourceLanguage: sourceLanguage,
      targetLanguage: targetLanguage,
      monitor(m) {
        m.addEventListener('downloadprogress', (e) => {
          console.log(`Brave: Translation model download ` +
                      `${(e.loaded * 100).toFixed(1)}%`);
        });
      },
    });

    localTranslationReady = true;
    console.log('Brave: Local translator initialized successfully');
    return true;

  } catch (error) {
    console.error('Brave: Failed to initialize local translator:', error);
    localTranslator = null;
    localTranslationReady = false;
    return false;
  }
}

/**
 * Translate text using local Translator API
 * @param {string} text Text to translate
 * @return {Promise<string>} Translated text
 */
async function translateTextLocally(text) {
  if (!localTranslator || !localTranslationReady) {
    throw new Error('Local translator not available');
  }

  try {
    const result = await localTranslator.translate(text);
    return result;
  } catch (error) {
    console.error('Brave: Local translation failed:', error);
    throw error;
  }
}

/**
 * Get all translatable text nodes from an element
 * @param {Element} element Root element
 * @return {Array<Text>} Array of text nodes
 */
function getTextNodes(element) {
  const textNodes = [];
  const walker = document.createTreeWalker(
    element,
    NodeFilter.SHOW_TEXT,
    {
      acceptNode: function(node) {
        // Skip empty text nodes and nodes in script/style elements
        if (!node.textContent.trim()) return NodeFilter.FILTER_REJECT;
        if (node.parentElement.tagName === 'SCRIPT' ||
            node.parentElement.tagName === 'STYLE' ||
            node.parentElement.tagName === 'NOSCRIPT') {
          return NodeFilter.FILTER_REJECT;
        }
        return NodeFilter.FILTER_ACCEPT;
      }
    }
  );

  let node;
  while (node = walker.nextNode()) {
    textNodes.push(node);
  }
  return textNodes;
}

/**
 * Perform local page translation using the Translator API
 * @param {string} sourceLang Source language code
 * @param {string} targetLang Target language code
 * @param {Function} onTranslateProgress Progress callback
 */
async function performLocalPageTranslation(
    sourceLang, targetLang, onTranslateProgress) {
  console.log('Brave: Starting local page translation');

  try {
    // Notify start of translation
    if (onTranslateProgress) onTranslateProgress(0, false, false);

    // Get all text nodes in the document
    const textNodes = getTextNodes(document.body);
    console.log(`Brave: Found ${textNodes.length} text nodes to translate`);

    if (textNodes.length === 0) {
      if (onTranslateProgress) onTranslateProgress(1, true, false);
      return;
    }

    const batchSize = 10;
    let processed = 0;
    originalTextMap.clear();

    // Process in batches to avoid overwhelming the API
    for (let i = 0; i < textNodes.length; i += batchSize) {
      const batch = textNodes.slice(i, i + batchSize);

      // Translate each text node in the batch
      const translations = await Promise.allSettled(
        batch.map(async (textNode) => {
          const originalText = textNode.textContent.trim();
          originalTextMap.set(textNode, originalText); // Store for revert
          const translated = await translateTextLocally(originalText);
          return { node: textNode, original: originalText, translated };
        })
      );

      // Apply successful translations
      translations.forEach((result) => {
        if (result.status === 'fulfilled' && result.value) {
          const { node, translated } = result.value;
          node.textContent = translated;
        }
      });

      processed += batch.length;
      const progress = processed / textNodes.length;
      if (onTranslateProgress) onTranslateProgress(progress, false, false);

      // Small delay to prevent UI blocking
      await new Promise(resolve => setTimeout(resolve, 10));
    }

    console.log('Brave: Local page translation completed');
    if (onTranslateProgress) onTranslateProgress(1, true, false);

  } catch (error) {
    console.error('Brave: Local page translation failed:', error);
    if (onTranslateProgress) {
      onTranslateProgress(0, false, true, 6); // TRANSLATION_ERROR
    }
  }
}

// Store original cr.googleTranslate methods for fallback
const originalTranslateObject = cr.googleTranslate;
const originalTranslate = cr.googleTranslate.translate;
const originalRevert = cr.googleTranslate.revert;
const originalLibReady = Object.getOwnPropertyDescriptor(
  cr.googleTranslate, 'libReady');

// Override the translate method with local-first approach
cr.googleTranslate.translate = function(sourceLang, targetLang) {
  console.log(`Brave: Hybrid translate requested: ${sourceLang} -> ` +
              `${targetLang}`);

  // Try local translation first if available
  if (isTranslatorAPIAvailable()) {
    console.log('Brave: Attempting local translation first');

    // Start async local translation
    (async () => {
      try {
        const localSuccess = await initializeLocalTranslator(
          sourceLang, targetLang);

        if (localSuccess) {
          await performLocalPageTranslation(sourceLang, targetLang, null);
          return; // Success, don't call original
        }

        console.log('Brave: Local translation not available, ' +
                    'falling back to server');
        // Fallback to original Google Translate
        originalTranslate.call(this, sourceLang, targetLang);
      } catch (error) {
        console.error('Brave: Local translation failed, ' +
                      'falling back to server:', error);
        // Fallback to original Google Translate
        originalTranslate.call(this, sourceLang, targetLang);
      }
    })();

    return true; // Indicate translation started
  }

  // No local API available, use original Google Translate
  console.log('Brave: Local Translator API not available, ' +
              'using server translation');
  return originalTranslate.call(this, sourceLang, targetLang);
};

// Override the revert method to handle both local and Google revert
cr.googleTranslate.revert = function() {
  console.log('Brave: Reverting translation');

  // Try local revert first (if we have original text stored)
  if (originalTextMap.size > 0) {
    originalTextMap.forEach((originalText, node) => {
      if (node.parentNode) { // Make sure node still exists in DOM
        node.textContent = originalText;
      }
    });
    originalTextMap.clear();
  }

  // Also call original revert for Google Translate
  originalRevert.call(this);
};

// Override libReady getter to include local translation readiness
Object.defineProperty(cr.googleTranslate, 'libReady', {
  get: function() {
    const originalReady = originalLibReady.get.call(this);
    // Ready if local API OR Google Translate ready
    return isTranslatorAPIAvailable() || originalReady;
  },
  configurable: true
});

console.log('Brave: Enhanced translate with on-device translation ' +
            'capabilities loaded');
