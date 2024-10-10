window.__firefox__ = {};

Object.defineProperty(window.__firefox__, "$<brave_translate_script>", {
  enumerable: false,
  configurable: false,
  writable: false,
  value: {
    "useNativeNetworking": true,
    "getPageSource": (function() {
      return encodeURIComponent(document.documentElement.outerHTML);
    }),
    "getPageLanguage": (function() {
      return document.documentElement.lang;
    }),
    "getRawPageSource": (function() {
      return document.documentElement.outerText;
    }),
  }
});


// TRANSLATE

var translateApiKey = '$<brave_translate_api_key>';
var gtTimeInfo = {
    'fetchStart': Date.now(),
    'fetchEnd': Date.now() + 1
};
var serverParams = '';
var securityOrigin = 'https://translate.brave.com/';


// ios/web/public/js_messaging/tsc/ios/web/public/js_messaging/resources/gcrweb.js
if (typeof _injected_gcrweb === 'undefined') {
    var _injected_gcrweb = true;
    if (!window.__gCrWeb) {
        window.__gCrWeb = {};
    }
    var gCrWeb = window.__gCrWeb;
}

// ios/web/public/js_messaging/tsc/ios/web/public/js_messaging/resources/utils.js
if (typeof _injected_utils === 'undefined') {
  var _injected_utils = true;
  
  function sendWebKitMessage(handlerName, message) {
      try {
          // A web page can override `window.webkit` with any value. Deleting the
          // object ensures that original and working implementation of
          // window.webkit is restored.
          var oldWebkit = window.webkit;
          delete window['webkit'];
          window.webkit.messageHandlers[handlerName].postMessage(message);
          window.webkit = oldWebkit;
      }
      catch (err) {
          // TODO(crbug.com/40269960): Report this fatal error
      }
  }
  
  function trim(str) {
      return str.replace(/^\s+|\s+$/g, '');
  }
}

// ios/web/web_state/js/resources/common.js
if (typeof _injected_common === 'undefined') {
    var _injected_common = true;
    var FormControlElement;
    __gCrWeb.common = {};
    __gCrWeb['common'] = __gCrWeb.common;
    __gCrWeb.common.JSONSafeObject = function JSONSafeObject() {};
    __gCrWeb.common.JSONSafeObject.prototype['toJSON'] = null;
    __gCrWeb.common.JSONStringify = JSON.stringify;
    __gCrWeb.stringify = function(value) {
      if (value === null) return 'null';
      if (value === undefined) return 'undefined';
      if (typeof(value.toJSON) == 'function') {
        var originalToJSON = value.toJSON;
        value.toJSON = undefined;
        var stringifiedValue = __gCrWeb.common.JSONStringify(value);
        value.toJSON = originalToJSON;
        return stringifiedValue;
      }
      return __gCrWeb.common.JSONStringify(value);
    };
  
    __gCrWeb.common.isTextField = function(element) {
      if (!element) {
        return false;
      }
      if (element.type === 'hidden') {
        return false;
      }
      return element.type === 'text' || element.type === 'email' ||
          element.type === 'password' || element.type === 'search' ||
          element.type === 'tel' || element.type === 'url' ||
          element.type === 'number';
    };

    __gCrWeb.common.trim = function(str) {
      return str.replace(/^\s+|\s+$/g, '');
    };

    __gCrWeb.common.removeQueryAndReferenceFromURL = function(url) {
      var parsed = new URL(url);

      const isPropertyInvalid = (value) => typeof value !== 'string';

      if (isPropertyInvalid(parsed.origin) || isPropertyInvalid(parsed.protocol) ||
          isPropertyInvalid(parsed.pathname)) {
        return '';
      }

      return (parsed.origin !== 'null' ? parsed.origin : parsed.protocol) +
          parsed.pathname;
    };

    __gCrWeb.common.sendWebKitMessage = function(handlerName, message) {
      try {
        var oldWebkit = window.webkit;
        delete window['webkit'];
        window.webkit.messageHandlers[handlerName].postMessage(message);
        window.webkit = oldWebkit;
      } catch (err) {
        
      }
    };
}


// ios/web/public/js_messaging/tsc/ios/web/public/js_messaging/resources/frame_id.js
if (typeof _injected_frame_id === 'undefined') {
  var _injected_frame_id = true;
  
  function generateRandomId() {
    const components = new Uint32Array(4);
    window.crypto.getRandomValues(components);
    let id = '';
    for (const component of components) {
      id += component.toString(16).padStart(8, '0');
    }
    return id;
  }
  
  function registerFrame() {
    sendWebKitMessage('FrameBecameAvailable', { 'crwFrameId': getFrameId() });
  }
  
  function getFrameId() {
    if (!gCrWeb.hasOwnProperty('frameId')) {
      gCrWeb.frameId = generateRandomId();
    }
    return gCrWeb.frameId;
  }
}


// ios/web/tsc/ios/web/js_messaging/resources/message.js
if (typeof _injected_message === 'undefined') {
  var _injected_message = true;
  
  function getExistingFrames() {
      registerFrame();
      const framecount = window.frames.length;
      for (let i = 0; i < framecount; i++) {
          const frame = window.frames[i];
          if (!frame) {
              continue;
          }
          frame.postMessage({ type: 'org.chromium.registerForFrameMessaging' }, '*');
      }
  }
  
  gCrWeb.message = {
      getFrameId,
      getExistingFrames
  };
}

// ios/web/navigation/tsc/ios/web/navigation/resources/navigation.js
if (typeof _injected_navigation === 'undefined') {
  var _injected_navigation = true;
    
  class DataCloneError {
    name = 'DataCloneError';
    code = 25;
    message = 'Cyclic structures are not supported.';
  }
  
  class MessageQueue {
      queuedMessages = [];
      sendQueuedMessages() {
          while (this.queuedMessages.length > 0) {
              try {
                  sendWebKitMessage('NavigationEventMessage', this.queuedMessages[0]);
                  this.queuedMessages.shift();
              }
              catch (e) {
                  break;
              }
          }
      }
    
      queueNavigationEventMessage(message) {
          this.queuedMessages.push(message);
          this.sendQueuedMessages();
      }
  }
  
  const messageQueue = new MessageQueue();
  const JSONStringify = JSON.stringify;
  const originalWindowHistoryPushState = window.history.pushState;
  const originalWindowHistoryReplaceState = window.history.replaceState;
  History.prototype.pushState =
      function (stateObject, pageTitle, pageUrl) {
          messageQueue.queueNavigationEventMessage({
              'command': 'willChangeState',
              'frame_id': gCrWeb.message.getFrameId()
          });
        
          let serializedState = '';
          try {
              if (typeof (stateObject) != 'undefined') {
                  serializedState = JSONStringify(stateObject);
              }
          }
          catch (e) {
              throw new DataCloneError();
          }
          pageUrl = pageUrl || window.location.href;
          originalWindowHistoryPushState.call(history, stateObject, pageTitle, pageUrl);
          messageQueue.queueNavigationEventMessage({
              'command': 'didPushState',
              'stateObject': serializedState,
              'baseUrl': document.baseURI,
              'pageUrl': pageUrl.toString(),
              'frame_id': gCrWeb.message.getFrameId()
          });
      };
  History.prototype.replaceState =
      function (stateObject, pageTitle, pageUrl) {
          messageQueue.queueNavigationEventMessage({
              'command': 'willChangeState',
              'frame_id': gCrWeb.message.getFrameId()
          });
          let serializedState = '';
          try {
              if (typeof (stateObject) != 'undefined') {
                  serializedState = JSONStringify(stateObject);
              }
          }
          catch (e) {
              throw new DataCloneError();
          }
          pageUrl = pageUrl || window.location.href;
          originalWindowHistoryReplaceState.call(history, stateObject, pageTitle, pageUrl);
          messageQueue.queueNavigationEventMessage({
              'command': 'didReplaceState',
              'stateObject': serializedState,
              'baseUrl': document.baseURI,
              'pageUrl': pageUrl.toString(),
              'frame_id': gCrWeb.message.getFrameId()
          });
      };
}

// ios/web/navigation/tsc/ios/web/navigation/resources/navigation_listeners.js
if (typeof _injected_navigation_listener === 'undefined') {
  var _injected_navigation_listener = true;
  
  window.addEventListener('hashchange', () => {
      sendWebKitMessage('NavigationEventMessage', { 'command': 'hashchange', 'frame_id': gCrWeb.message.getFrameId() });
  });
}

// Text Fragments
if (typeof _injected_text_fragments === 'undefined') {
  var _injected_text_fragments = true;
    
  const FRAGMENT_DIRECTIVES = ['text'];

  const BLOCK_ELEMENTS = [
      'ADDRESS', 'ARTICLE', 'ASIDE', 'BLOCKQUOTE', 'BR', 'DETAILS',
      'DIALOG', 'DD', 'DIV', 'DL', 'DT', 'FIELDSET',
      'FIGCAPTION', 'FIGURE', 'FOOTER', 'FORM', 'H1', 'H2',
      'H3', 'H4', 'H5', 'H6', 'HEADER', 'HGROUP',
      'HR', 'LI', 'MAIN', 'NAV', 'OL', 'P',
      'PRE', 'SECTION', 'TABLE', 'UL', 'TR', 'TH',
      'TD', 'COLGROUP', 'COL', 'CAPTION', 'THEAD', 'TBODY',
      'TFOOT',
  ];

  const BOUNDARY_CHARS = /[\t-\r -#%-\*,-\/:;\?@\[-\]_\{\}\x85\xA0\xA1\xA7\xAB\xB6\xB7\xBB\xBF\u037E\u0387\u055A-\u055F\u0589\u058A\u05BE\u05C0\u05C3\u05C6\u05F3\u05F4\u0609\u060A\u060C\u060D\u061B\u061E\u061F\u066A-\u066D\u06D4\u0700-\u070D\u07F7-\u07F9\u0830-\u083E\u085E\u0964\u0965\u0970\u0AF0\u0DF4\u0E4F\u0E5A\u0E5B\u0F04-\u0F12\u0F14\u0F3A-\u0F3D\u0F85\u0FD0-\u0FD4\u0FD9\u0FDA\u104A-\u104F\u10FB\u1360-\u1368\u1400\u166D\u166E\u1680\u169B\u169C\u16EB-\u16ED\u1735\u1736\u17D4-\u17D6\u17D8-\u17DA\u1800-\u180A\u1944\u1945\u1A1E\u1A1F\u1AA0-\u1AA6\u1AA8-\u1AAD\u1B5A-\u1B60\u1BFC-\u1BFF\u1C3B-\u1C3F\u1C7E\u1C7F\u1CC0-\u1CC7\u1CD3\u2000-\u200A\u2010-\u2029\u202F-\u2043\u2045-\u2051\u2053-\u205F\u207D\u207E\u208D\u208E\u2308-\u230B\u2329\u232A\u2768-\u2775\u27C5\u27C6\u27E6-\u27EF\u2983-\u2998\u29D8-\u29DB\u29FC\u29FD\u2CF9-\u2CFC\u2CFE\u2CFF\u2D70\u2E00-\u2E2E\u2E30-\u2E44\u3000-\u3003\u3008-\u3011\u3014-\u301F\u3030\u303D\u30A0\u30FB\uA4FE\uA4FF\uA60D-\uA60F\uA673\uA67E\uA6F2-\uA6F7\uA874-\uA877\uA8CE\uA8CF\uA8F8-\uA8FA\uA8FC\uA92E\uA92F\uA95F\uA9C1-\uA9CD\uA9DE\uA9DF\uAA5C-\uAA5F\uAADE\uAADF\uAAF0\uAAF1\uABEB\uFD3E\uFD3F\uFE10-\uFE19\uFE30-\uFE52\uFE54-\uFE61\uFE63\uFE68\uFE6A\uFE6B\uFF01-\uFF03\uFF05-\uFF0A\uFF0C-\uFF0F\uFF1A\uFF1B\uFF1F\uFF20\uFF3B-\uFF3D\uFF3F\uFF5B\uFF5D\uFF5F-\uFF65]|\uD800[\uDD00-\uDD02\uDF9F\uDFD0]|\uD801\uDD6F|\uD802[\uDC57\uDD1F\uDD3F\uDE50-\uDE58\uDE7F\uDEF0-\uDEF6\uDF39-\uDF3F\uDF99-\uDF9C]|\uD804[\uDC47-\uDC4D\uDCBB\uDCBC\uDCBE-\uDCC1\uDD40-\uDD43\uDD74\uDD75\uDDC5-\uDDC9\uDDCD\uDDDB\uDDDD-\uDDDF\uDE38-\uDE3D\uDEA9]|\uD805[\uDC4B-\uDC4F\uDC5B\uDC5D\uDCC6\uDDC1-\uDDD7\uDE41-\uDE43\uDE60-\uDE6C\uDF3C-\uDF3E]|\uD807[\uDC41-\uDC45\uDC70\uDC71]|\uD809[\uDC70-\uDC74]|\uD81A[\uDE6E\uDE6F\uDEF5\uDF37-\uDF3B\uDF44]|\uD82F\uDC9F|\uD836[\uDE87-\uDE8B]|\uD83A[\uDD5E\uDD5F]/u;

  const NON_BOUNDARY_CHARS = /[^\t-\r -#%-\*,-\/:;\?@\[-\]_\{\}\x85\xA0\xA1\xA7\xAB\xB6\xB7\xBB\xBF\u037E\u0387\u055A-\u055F\u0589\u058A\u05BE\u05C0\u05C3\u05C6\u05F3\u05F4\u0609\u060A\u060C\u060D\u061B\u061E\u061F\u066A-\u066D\u06D4\u0700-\u070D\u07F7-\u07F9\u0830-\u083E\u085E\u0964\u0965\u0970\u0AF0\u0DF4\u0E4F\u0E5A\u0E5B\u0F04-\u0F12\u0F14\u0F3A-\u0F3D\u0F85\u0FD0-\u0FD4\u0FD9\u0FDA\u104A-\u104F\u10FB\u1360-\u1368\u1400\u166D\u166E\u1680\u169B\u169C\u16EB-\u16ED\u1735\u1736\u17D4-\u17D6\u17D8-\u17DA\u1800-\u180A\u1944\u1945\u1A1E\u1A1F\u1AA0-\u1AA6\u1AA8-\u1AAD\u1B5A-\u1B60\u1BFC-\u1BFF\u1C3B-\u1C3F\u1C7E\u1C7F\u1CC0-\u1CC7\u1CD3\u2000-\u200A\u2010-\u2029\u202F-\u2043\u2045-\u2051\u2053-\u205F\u207D\u207E\u208D\u208E\u2308-\u230B\u2329\u232A\u2768-\u2775\u27C5\u27C6\u27E6-\u27EF\u2983-\u2998\u29D8-\u29DB\u29FC\u29FD\u2CF9-\u2CFC\u2CFE\u2CFF\u2D70\u2E00-\u2E2E\u2E30-\u2E44\u3000-\u3003\u3008-\u3011\u3014-\u301F\u3030\u303D\u30A0\u30FB\uA4FE\uA4FF\uA60D-\uA60F\uA673\uA67E\uA6F2-\uA6F7\uA874-\uA877\uA8CE\uA8CF\uA8F8-\uA8FA\uA8FC\uA92E\uA92F\uA95F\uA9C1-\uA9CD\uA9DE\uA9DF\uAA5C-\uAA5F\uAADE\uAADF\uAAF0\uAAF1\uABEB\uFD3E\uFD3F\uFE10-\uFE19\uFE30-\uFE52\uFE54-\uFE61\uFE63\uFE68\uFE6A\uFE6B\uFF01-\uFF03\uFF05-\uFF0A\uFF0C-\uFF0F\uFF1A\uFF1B\uFF1F\uFF20\uFF3B-\uFF3D\uFF3F\uFF5B\uFF5D\uFF5F-\uFF65]|\uD800[\uDD00-\uDD02\uDF9F\uDFD0]|\uD801\uDD6F|\uD802[\uDC57\uDD1F\uDD3F\uDE50-\uDE58\uDE7F\uDEF0-\uDEF6\uDF39-\uDF3F\uDF99-\uDF9C]|\uD804[\uDC47-\uDC4D\uDCBB\uDCBC\uDCBE-\uDCC1\uDD40-\uDD43\uDD74\uDD75\uDDC5-\uDDC9\uDDCD\uDDDB\uDDDD-\uDDDF\uDE38-\uDE3D\uDEA9]|\uD805[\uDC4B-\uDC4F\uDC5B\uDC5D\uDCC6\uDDC1-\uDDD7\uDE41-\uDE43\uDE60-\uDE6C\uDF3C-\uDF3E]|\uD807[\uDC41-\uDC45\uDC70\uDC71]|\uD809[\uDC70-\uDC74]|\uD81A[\uDE6E\uDE6F\uDEF5\uDF37-\uDF3B\uDF44]|\uD82F\uDC9F|\uD836[\uDE87-\uDE8B]|\uD83A[\uDD5E\uDD5F]/u;

  const TEXT_FRAGMENT_CSS_CLASS_NAME = 'text-fragments-polyfill-target-text';

  const getFragmentDirectives = (hash) => {
      const fragmentDirectivesStrings = hash.replace(/#.*?:~:(.*?)/, '$1').split(/&?text=/).filter(Boolean);
      if (!fragmentDirectivesStrings.length) {
          return {};
      }
      else {
          return { text: fragmentDirectivesStrings };
      }
  };

  const parseFragmentDirectives = (fragmentDirectives) => {
      const parsedFragmentDirectives = {};
      for (const [fragmentDirectiveType, fragmentDirectivesOfType,] of Object.entries(fragmentDirectives)) {
          if (FRAGMENT_DIRECTIVES.includes(fragmentDirectiveType)) {
              parsedFragmentDirectives[fragmentDirectiveType] =
                  fragmentDirectivesOfType.map((fragmentDirectiveOfType) => {
                      return parseTextFragmentDirective(fragmentDirectiveOfType);
                  });
          }
      }
      return parsedFragmentDirectives;
  };

  const parseTextFragmentDirective = (textFragment) => {
      const TEXT_FRAGMENT = /^(?:(.+?)-,)?(?:(.+?))(?:,([^-]+?))?(?:,-(.+?))?$/;
      return {
          prefix: decodeURIComponent(textFragment.replace(TEXT_FRAGMENT, '$1')),
          textStart: decodeURIComponent(textFragment.replace(TEXT_FRAGMENT, '$2')),
          textEnd: decodeURIComponent(textFragment.replace(TEXT_FRAGMENT, '$3')),
          suffix: decodeURIComponent(textFragment.replace(TEXT_FRAGMENT, '$4')),
      };
  };

  const processFragmentDirectives = (parsedFragmentDirectives, documentToProcess = document) => {
      const processedFragmentDirectives = {};
      for (const [fragmentDirectiveType, fragmentDirectivesOfType,] of Object.entries(parsedFragmentDirectives)) {
          if (FRAGMENT_DIRECTIVES.includes(fragmentDirectiveType)) {
              processedFragmentDirectives[fragmentDirectiveType] =
                  fragmentDirectivesOfType.map((fragmentDirectiveOfType) => {
                      const result = processTextFragmentDirective(fragmentDirectiveOfType, documentToProcess);
                      if (result.length >= 1) {

                          return markRange(result[0], documentToProcess);
                      }
                      return [];
                  });
          }
      }
      return processedFragmentDirectives;
  };

  const processTextFragmentDirective = (textFragment, documentToProcess = document) => {
      const results = [];
      const searchRange = documentToProcess.createRange();
      searchRange.selectNodeContents(documentToProcess.body);
      while (!searchRange.collapsed && results.length < 2) {
          let potentialMatch;
          if (textFragment.prefix) {
              const prefixMatch = findTextInRange(textFragment.prefix, searchRange);
              if (prefixMatch == null) {
                  break;
              }

              advanceRangeStartPastOffset(searchRange, prefixMatch.startContainer, prefixMatch.startOffset);

              const matchRange = documentToProcess.createRange();
              matchRange.setStart(prefixMatch.endContainer, prefixMatch.endOffset);
              matchRange.setEnd(searchRange.endContainer, searchRange.endOffset);
              advanceRangeStartToNonWhitespace(matchRange);
              if (matchRange.collapsed) {
                  break;
              }
              potentialMatch = findTextInRange(textFragment.textStart, matchRange);

              if (potentialMatch == null) {
                  break;
              }

              if (potentialMatch.compareBoundaryPoints(Range.START_TO_START, matchRange) !== 0) {
                  continue;
              }
          }
          else {

              potentialMatch = findTextInRange(textFragment.textStart, searchRange);
              if (potentialMatch == null) {
                  break;
              }
              advanceRangeStartPastOffset(searchRange, potentialMatch.startContainer, potentialMatch.startOffset);
          }
          if (textFragment.textEnd) {
              const textEndRange = documentToProcess.createRange();
              textEndRange.setStart(potentialMatch.endContainer, potentialMatch.endOffset);
              textEndRange.setEnd(searchRange.endContainer, searchRange.endOffset);

              let matchFound = false;

              while (!textEndRange.collapsed && results.length < 2) {
                  const textEndMatch = findTextInRange(textFragment.textEnd, textEndRange);
                  if (textEndMatch == null) {
                      break;
                  }
                  advanceRangeStartPastOffset(textEndRange, textEndMatch.startContainer, textEndMatch.startOffset);
                  potentialMatch.setEnd(textEndMatch.endContainer, textEndMatch.endOffset);
                  if (textFragment.suffix) {

                      const suffixResult = checkSuffix(textFragment.suffix, potentialMatch, searchRange, documentToProcess);
                      if (suffixResult === CheckSuffixResult.NO_SUFFIX_MATCH) {
                          break;
                      }
                      else if (suffixResult === CheckSuffixResult.SUFFIX_MATCH) {
                          matchFound = true;
                          results.push(potentialMatch.cloneRange());
                          continue;
                      }
                      else if (suffixResult === CheckSuffixResult.MISPLACED_SUFFIX) {
                          continue;
                      }
                  }
                  else {

                      matchFound = true;
                      results.push(potentialMatch.cloneRange());
                  }
              }

              if (!matchFound) {
                  break;
              }
          }
          else if (textFragment.suffix) {

              const suffixResult = checkSuffix(textFragment.suffix, potentialMatch, searchRange, documentToProcess);
              if (suffixResult === CheckSuffixResult.NO_SUFFIX_MATCH) {
                  break;
              }
              else if (suffixResult === CheckSuffixResult.SUFFIX_MATCH) {
                  results.push(potentialMatch.cloneRange());
                  advanceRangeStartPastOffset(searchRange, searchRange.startContainer, searchRange.startOffset);
                  continue;
              }
              else if (suffixResult === CheckSuffixResult.MISPLACED_SUFFIX) {
                  continue;
              }
          }
          else {
              results.push(potentialMatch.cloneRange());
          }
      }
      return results;
  };

  const removeMarks = (marks, documentToProcess = document) => {
      for (const mark of marks) {
          const range = documentToProcess.createRange();
          range.selectNodeContents(mark);
          const fragment = range.extractContents();
          const parent = mark.parentNode;
          parent.insertBefore(fragment, mark);
          parent.removeChild(mark);
      }
  };

  const CheckSuffixResult = {
      NO_SUFFIX_MATCH: 0,
      SUFFIX_MATCH: 1,
      MISPLACED_SUFFIX: 2,
  };

  const checkSuffix = (suffix, potentialMatch, searchRange, documentToProcess) => {
      const suffixRange = documentToProcess.createRange();
      suffixRange.setStart(potentialMatch.endContainer, potentialMatch.endOffset);
      suffixRange.setEnd(searchRange.endContainer, searchRange.endOffset);
      advanceRangeStartToNonWhitespace(suffixRange);
      const suffixMatch = findTextInRange(suffix, suffixRange);

      if (suffixMatch == null) {
          return CheckSuffixResult.NO_SUFFIX_MATCH;
      }

      if (suffixMatch.compareBoundaryPoints(Range.START_TO_START, suffixRange) !== 0) {
          return CheckSuffixResult.MISPLACED_SUFFIX;
      }
      return CheckSuffixResult.SUFFIX_MATCH;
  };

  const advanceRangeStartPastOffset = (range, node, offset) => {
      try {
          range.setStart(node, offset + 1);
      }
      catch (err) {
          range.setStartAfter(node);
      }
  };

  const advanceRangeStartToNonWhitespace = (range) => {
      const walker = makeTextNodeWalker(range);
      let node = walker.nextNode();
      while (!range.collapsed && node != null) {
          if (node !== range.startContainer) {
              range.setStart(node, 0);
          }
          if (node.textContent.length > range.startOffset) {
              const firstChar = node.textContent[range.startOffset];
              if (!firstChar.match(/\s/)) {
                  return;
              }
          }
          try {
              range.setStart(node, range.startOffset + 1);
          }
          catch (err) {
              node = walker.nextNode();
              if (node == null) {
                  range.collapse();
              }
              else {
                  range.setStart(node, 0);
              }
          }
      }
  };

  const makeTextNodeWalker = (range) => {
      const walker = document.createTreeWalker(range.commonAncestorContainer, NodeFilter.SHOW_TEXT | NodeFilter.SHOW_ELEMENT, (node) => {
          return acceptTextNodeIfVisibleInRange(node, range);
      });
      return walker;
  };

  const markRange = (range, documentToProcess = document) => {
      if (range.startContainer.nodeType != Node.TEXT_NODE ||
          range.endContainer.nodeType != Node.TEXT_NODE)
          return [];

      if (range.startContainer === range.endContainer) {
          const trivialMark = documentToProcess.createElement('mark');
          trivialMark.setAttribute('class', TEXT_FRAGMENT_CSS_CLASS_NAME);
          range.surroundContents(trivialMark);
          return [trivialMark];
      }

      const startNode = range.startContainer;
      const startNodeSubrange = range.cloneRange();
      startNodeSubrange.setEndAfter(startNode);

      const endNode = range.endContainer;
      const endNodeSubrange = range.cloneRange();
      endNodeSubrange.setStartBefore(endNode);

      const marks = [];
      range.setStartAfter(startNode);
      range.setEndBefore(endNode);
      const walker = documentToProcess.createTreeWalker(range.commonAncestorContainer, NodeFilter.SHOW_ELEMENT | NodeFilter.SHOW_TEXT, {
          acceptNode: function (node) {
              if (!range.intersectsNode(node))
                  return NodeFilter.FILTER_REJECT;
              if (BLOCK_ELEMENTS.includes(node.tagName) ||
                  node.nodeType === Node.TEXT_NODE)
                  return NodeFilter.FILTER_ACCEPT;
              return NodeFilter.FILTER_SKIP;
          },
      });
      let node = walker.nextNode();
      while (node) {
          if (node.nodeType === Node.TEXT_NODE) {
              const mark = documentToProcess.createElement('mark');
              mark.setAttribute('class', TEXT_FRAGMENT_CSS_CLASS_NAME);
              node.parentNode.insertBefore(mark, node);
              mark.appendChild(node);
              marks.push(mark);
          }
          node = walker.nextNode();
      }
      const startMark = documentToProcess.createElement('mark');
      startMark.setAttribute('class', TEXT_FRAGMENT_CSS_CLASS_NAME);
      startNodeSubrange.surroundContents(startMark);
      const endMark = documentToProcess.createElement('mark');
      endMark.setAttribute('class', TEXT_FRAGMENT_CSS_CLASS_NAME);
      endNodeSubrange.surroundContents(endMark);
      return [startMark, ...marks, endMark];
  };

  const scrollElementIntoView = (element) => {
      const behavior = {
          behavior: 'auto',
          block: 'center',
          inline: 'nearest',
      };
      element.scrollIntoView(behavior);
  };

  const isNodeVisible = (node) => {

      let elt = node;
      while (elt != null && !(elt instanceof HTMLElement))
          elt = elt.parentNode;
      if (elt != null) {
          const nodeStyle = window.getComputedStyle(elt);

          if (nodeStyle.visibility === 'hidden' || nodeStyle.display === 'none' ||
              nodeStyle.height === 0 || nodeStyle.width === 0 ||
              nodeStyle.opacity === 0) {
              return false;
          }
      }
      return true;
  };

  const acceptNodeIfVisibleInRange = (node, range) => {
      if (range != null && !range.intersectsNode(node))
          return NodeFilter.FILTER_REJECT;
      return isNodeVisible(node) ? NodeFilter.FILTER_ACCEPT :
          NodeFilter.FILTER_REJECT;
  };

  const acceptTextNodeIfVisibleInRange = (node, range) => {
      if (range != null && !range.intersectsNode(node))
          return NodeFilter.FILTER_REJECT;
      if (!isNodeVisible(node)) {
          return NodeFilter.FILTER_REJECT;
      }
      return node.nodeType === Node.TEXT_NODE ? NodeFilter.FILTER_ACCEPT :
          NodeFilter.FILTER_SKIP;
  };

  const getAllTextNodes = (root, range) => {
      const blocks = [];
      let tmp = [];
      const nodes = Array.from(getElementsIn(root, (node) => {
          return acceptNodeIfVisibleInRange(node, range);
      }));
      for (const node of nodes) {
          if (node.nodeType === Node.TEXT_NODE) {
              tmp.push(node);
          }
          else if (node instanceof HTMLElement && BLOCK_ELEMENTS.includes(node.tagName) &&
              tmp.length > 0) {

              blocks.push(tmp);
              tmp = [];
          }
      }
      if (tmp.length > 0)
          blocks.push(tmp);
      return blocks;
  };

  const getTextContent = (nodes, startOffset, endOffset) => {
      let str = '';
      if (nodes.length === 1) {
          str = nodes[0].textContent.substring(startOffset, endOffset);
      }
      else {
          str = nodes[0].textContent.substring(startOffset) +
              nodes.slice(1, -1).reduce((s, n) => s + n.textContent, '') +
              nodes.slice(-1)[0].textContent.substring(0, endOffset);
      }
      return str.replace(/[\t\n\r ]+/g, ' ');
  };

  function* getElementsIn(root, filter) {
      const treeWalker = document.createTreeWalker(root, NodeFilter.SHOW_ELEMENT | NodeFilter.SHOW_TEXT, { acceptNode: filter });
      const finishedSubtrees = new Set();
      while (forwardTraverse(treeWalker, finishedSubtrees) !== null) {
          yield treeWalker.currentNode;
      }
  }

  const findTextInRange = (query, range) => {
      const textNodeLists = getAllTextNodes(range.commonAncestorContainer, range);
      const segmenter = makeNewSegmenter();
      for (const list of textNodeLists) {
          const found = findRangeFromNodeList(query, range, list, segmenter);
          if (found !== undefined)
              return found;
      }
      return undefined;
  };

  const findRangeFromNodeList = (query, range, textNodes, segmenter) => {
      if (!query || !range || !(textNodes || []).length)
          return undefined;
      const data = normalizeString(getTextContent(textNodes, 0, undefined));
      const normalizedQuery = normalizeString(query);
      let searchStart = textNodes[0] === range.startNode ? range.startOffset : 0;
      let start;
      let end;
      while (searchStart < data.length) {
          const matchIndex = data.indexOf(normalizedQuery, searchStart);
          if (matchIndex === -1)
              return undefined;
          if (isWordBounded(data, matchIndex, normalizedQuery.length, segmenter)) {
              start = getBoundaryPointAtIndex(matchIndex, textNodes,  false);
              end = getBoundaryPointAtIndex(matchIndex + normalizedQuery.length, textNodes,
               true);
          }
          if (start != null && end != null) {
              const foundRange = new Range();
              foundRange.setStart(start.node, start.offset);
              foundRange.setEnd(end.node, end.offset);

              if (range.compareBoundaryPoints(Range.START_TO_START, foundRange) <= 0 &&
                  range.compareBoundaryPoints(Range.END_TO_END, foundRange) >= 0) {
                  return foundRange;
              }
          }
          searchStart = matchIndex + 1;
      }
      return undefined;
  };

  const getBoundaryPointAtIndex = (index, textNodes, isEnd) => {
      let counted = 0;
      let normalizedData;
      for (let i = 0; i < textNodes.length; i++) {
          const node = textNodes[i];
          if (!normalizedData)
              normalizedData = normalizeString(node.data);
          let nodeEnd = counted + normalizedData.length;
          if (isEnd)
              nodeEnd += 1;
          if (nodeEnd > index) {

              const normalizedOffset = index - counted;
              let denormalizedOffset = Math.min(index - counted, node.data.length);

              const targetSubstring = isEnd ?
                  normalizedData.substring(0, normalizedOffset) :
                  normalizedData.substring(normalizedOffset);
              let candidateSubstring = isEnd ?
                  normalizeString(node.data.substring(0, denormalizedOffset)) :
                  normalizeString(node.data.substring(denormalizedOffset));

              const direction = (isEnd ? -1 : 1) *
                  (targetSubstring.length > candidateSubstring.length ? -1 : 1);
              while (denormalizedOffset >= 0 &&
                  denormalizedOffset <= node.data.length) {
                  if (candidateSubstring.length === targetSubstring.length) {
                      return { node: node, offset: denormalizedOffset };
                  }
                  denormalizedOffset += direction;
                  candidateSubstring = isEnd ?
                      normalizeString(node.data.substring(0, denormalizedOffset)) :
                      normalizeString(node.data.substring(denormalizedOffset));
              }
          }
          counted += normalizedData.length;
          if (i + 1 < textNodes.length) {

              const nextNormalizedData = normalizeString(textNodes[i + 1].data);
              if (normalizedData.slice(-1) === ' ' &&
                  nextNormalizedData.slice(0, 1) === ' ') {
                  counted -= 1;
              }

              normalizedData = nextNormalizedData;
          }
      }
      return undefined;
  };

  const isWordBounded = (text, startPos, length, segmenter) => {
      if (startPos < 0 || startPos >= text.length || length <= 0 ||
          startPos + length > text.length) {
          return false;
      }
      if (segmenter) {

          const segments = segmenter.segment(text);
          const startSegment = segments.containing(startPos);
          if (!startSegment)
              return false;

          if (startSegment.isWordLike && startSegment.index != startPos)
              return false;

          const endPos = startPos + length;
          const endSegment = segments.containing(endPos);

          if (endSegment && endSegment.isWordLike && endSegment.index != endPos)
              return false;
      }
      else {

          if (text[startPos].match(BOUNDARY_CHARS)) {
              ++startPos;
              --length;
              if (!length) {
                  return false;
              }
          }

          if (text[startPos + length - 1].match(BOUNDARY_CHARS)) {
              --length;
              if (!length) {
                  return false;
              }
          }
          if (startPos !== 0 && (!text[startPos - 1].match(BOUNDARY_CHARS)))
              return false;
          if (startPos + length !== text.length &&
              !text[startPos + length].match(BOUNDARY_CHARS))
              return false;
      }
      return true;
  };

  const normalizeString = (str) => {

      return (str || '')
          .normalize('NFKD')
          .replace(/\s+/g, ' ')
          .replace(/[\u0300-\u036f]/g, '')
          .toLowerCase();
  };

  const makeNewSegmenter = () => {
      if (Intl.Segmenter) {
          let lang = document.documentElement.lang;
          if (!lang) {
              lang = navigator.languages;
          }
          return new Intl.Segmenter(lang, { granularity: 'word' });
      }
      return undefined;
  };

  const forwardTraverse = (walker, finishedSubtrees) => {

      if (!finishedSubtrees.has(walker.currentNode)) {
          const firstChild = walker.firstChild();
          if (firstChild !== null) {
              return firstChild;
          }
      }

      const nextSibling = walker.nextSibling();
      if (nextSibling !== null) {
          return nextSibling;
      }

      const parent = walker.parentNode();
      if (parent !== null) {
          finishedSubtrees.add(parent);
      }
      return parent;
  };

  const backwardTraverse = (walker, finishedSubtrees) => {

      if (!finishedSubtrees.has(walker.currentNode)) {
          const lastChild = walker.lastChild();
          if (lastChild !== null) {
              return lastChild;
          }
      }

      const previousSibling = walker.previousSibling();
      if (previousSibling !== null) {
          return previousSibling;
      }

      const parent = walker.parentNode();
      if (parent !== null) {
          finishedSubtrees.add(parent);
      }
      return parent;
  };

  const forTesting = {
      advanceRangeStartPastOffset: advanceRangeStartPastOffset,
      advanceRangeStartToNonWhitespace: advanceRangeStartToNonWhitespace,
      findRangeFromNodeList: findRangeFromNodeList,
      findTextInRange: findTextInRange,
      getBoundaryPointAtIndex: getBoundaryPointAtIndex,
      isWordBounded: isWordBounded,
      makeNewSegmenter: makeNewSegmenter,
      markRange: markRange,
      normalizeString: normalizeString,
      parseTextFragmentDirective: parseTextFragmentDirective,
      forwardTraverse: forwardTraverse,
      backwardTraverse: backwardTraverse,
      getAllTextNodes: getAllTextNodes,
      acceptTextNodeIfVisibleInRange: acceptTextNodeIfVisibleInRange
  };

  const internal = {
      BLOCK_ELEMENTS: BLOCK_ELEMENTS,
      BOUNDARY_CHARS: BOUNDARY_CHARS,
      NON_BOUNDARY_CHARS: NON_BOUNDARY_CHARS,
      acceptNodeIfVisibleInRange: acceptNodeIfVisibleInRange,
      normalizeString: normalizeString,
      makeNewSegmenter: makeNewSegmenter,
      forwardTraverse: forwardTraverse,
      backwardTraverse: backwardTraverse,
      makeTextNodeWalker: makeTextNodeWalker,
      isNodeVisible: isNodeVisible
  };

  if (typeof goog !== 'undefined') {
      goog.declareModuleId('googleChromeLabs.textFragmentPolyfill.textFragmentUtils');
  }

  const applyTargetTextStyle = () => {
      const styles = document.getElementsByTagName('style');
      if (!styles)
          return;
      for (const style of styles) {
          const cssRules = style.innerHTML;
          const targetTextRules = cssRules.match(/(\w*)::target-text\s*{\s*((.|\n)*?)\s*}/g);
          if (!targetTextRules)
              continue;
          const markCss = targetTextRules.join('\n');
          const newNode = document.createTextNode(markCss.replaceAll('::target-text', ` .${TEXT_FRAGMENT_CSS_CLASS_NAME}`));
          style.appendChild(newNode);
      }
  };

  const setDefaultTextFragmentsStyle = ({ backgroundColor, color }) => {
      const styles = document.getElementsByTagName('style');
      const defaultStyle = `.${TEXT_FRAGMENT_CSS_CLASS_NAME} {
      background-color: ${backgroundColor};
      color: ${color};
    }

    .${TEXT_FRAGMENT_CSS_CLASS_NAME} a, a .${TEXT_FRAGMENT_CSS_CLASS_NAME} {
      text-decoration: underline;
    }
    `;
      if (styles.length === 0) {
          document.head.insertAdjacentHTML('beforeend', `<style type="text/css">${defaultStyle}</style>`);
      }
      else {
          applyTargetTextStyle();
          const defaultStyleNode = document.createTextNode(defaultStyle);
          styles[0].insertBefore(defaultStyleNode, styles[0].firstChild);
      }
  };
}

// Annotations
if (typeof _injected_annotations === 'undefined') {
    var _injected_annotations = true;
  
  const NON_TEXT_NODE_NAMES = new Set([
      'A',
      'APP',
      'APPLET',
      'AREA',
      'AUDIO',
      'BUTTON',
      'CANVAS',
      'CHROME_ANNOTATION',
      'EMBED',
      'FORM',
      'FRAME',
      'FRAMESET',
      'HEAD',
      'IFRAME',
      'IMG',
      'INPUT',
      'KEYGEN',
      'LABEL',
      'MAP',
      'NOSCRIPT',
      'OBJECT',
      'OPTGROUP',
      'OPTION',
      'PROGRESS',
      'SCRIPT',
      'SELECT',
      'STYLE',
      'TEXTAREA',
      'VIDEO',
  ]);
  
  const NO_DECORATION_NODE_NAMES = new Set([
      'A', 'LABEL'
  ]);
  
  const MS_DELAY_BEFORE_TRIGGER = 300;
  
  class Replacement {
      index;
      left;
      right;
      text;
      type;
      annotationText;
      data;
      constructor(index, left, right, text, type, annotationText, data) {
          this.index = index;
          this.left = left;
          this.right = right;
          this.text = text;
          this.type = type;
          this.annotationText = annotationText;
          this.data = data;
      }
  }

  class Decoration {
      original;
      replacements;
      constructor(original, replacements) {
          this.original = original;
          this.replacements = replacements;
      }
  }

  class Section {
      node;
      index;
      constructor(node, index) {
          this.node = node;
          this.index = index;
      }
  }

  class MutationsDuringClickTracker {
      initialEvent;
      hasMutations = false;
      mutationObserver;
      mutationExtendId = 0;

      constructor(initialEvent) {
          this.initialEvent = initialEvent;
          this.mutationObserver =
              new MutationObserver((mutationList) => {
                  for (let mutation of mutationList) {
                      if (mutation.target.contains(this.initialEvent.target)) {
                          this.hasMutations = true;
                          this.stopObserving();
                          break;
                      }
                  }
              });
          this.mutationObserver.observe(document, { attributes: false, childList: true, subtree: true });
      }

      hasPreventativeActivity(event) {
          return event !== this.initialEvent || event.defaultPrevented ||
              this.hasMutations;
      }

      extendObservation(then) {
          if (this.mutationExtendId) {
              clearTimeout(this.mutationExtendId);
          }
          this.mutationExtendId = setTimeout(then, MS_DELAY_BEFORE_TRIGGER);
      }
      stopObserving() {
          if (this.mutationExtendId) {
              clearTimeout(this.mutationExtendId);
          }
          this.mutationExtendId = 0;
          this.mutationObserver?.disconnect();
      }
  }

  function hasNoIntentDetection() {
      const metas = document.getElementsByTagName('meta');
      for (let i = 0; i < metas.length; i++) {
          if (metas[i].getAttribute('name') === 'chrome' &&
              metas[i].getAttribute('content') === 'nointentdetection') {
              return true;
          }
      }
      return false;
  }

  function noFormatDetectionTypes() {
      const metas = document.getElementsByTagName('meta');
      let types = new Set();
      for (const meta of metas) {
          if (meta.getAttribute('name') !== 'format-detection')
              continue;
          let content = meta.getAttribute('content');
          if (!content)
              continue;
          let matches = content.toLowerCase().matchAll(/([a-z]+)\s*=\s*([a-z]+)/g);
          if (!matches)
              continue;
          for (let match of matches) {
              if (match && match[2] === 'no' && match[1]) {
                  types.add(match[1]);
              }
          }
      }
      return types;
  }

  function hasNoTranslate() {
      const metas = document.getElementsByTagName('meta');
      for (let i = 0; i < metas.length; i++) {
          if (metas[i].getAttribute('name') === 'google' &&
              metas[i].getAttribute('content') === 'notranslate') {
              return true;
          }
      }
      return false;
  }

  function getMetaContentByHttpEquiv(httpEquiv) {
      const metaTags = document.getElementsByTagName('meta');
      for (let metaTag of metaTags) {
          if (metaTag.httpEquiv.toLowerCase() === httpEquiv) {
              return metaTag.content;
          }
      }
      return '';
  }
  const highlightTextColor = '#000';
  const highlightBackgroundColor = 'rgba(20,111,225,0.25)';
  const decorationStyles = 'border-bottom-width: 1px; ' +
      'border-bottom-style: dotted; ' +
      'background-color: transparent';
  const decorationStylesForPhoneAndEmail = 'border-bottom-width: 1px; ' +
      'border-bottom-style: solid; ' +
      'background-color: transparent';
  const decorationDefaultColor = 'blue';
  let decorations = [];
  let sections;

  function extractText(maxChars, seqId) {

      if (decorations.length) {
          removeDecorations();
      }
      let disabledTypes = noFormatDetectionTypes();
      sendWebKitMessage('annotations', {
          command: 'annotations.extractedText',
          text: getPageText(maxChars),
          seqId: seqId,

          metadata: {
              hasNoIntentDetection: hasNoIntentDetection(),
              hasNoTranslate: hasNoTranslate(),
              htmlLang: document.documentElement.lang,
              httpContentLanguage: getMetaContentByHttpEquiv('content-language'),
              wkNoTelephone: disabledTypes.has('telephone'),
              wkNoEmail: disabledTypes.has('email'),
              wkNoAddress: disabledTypes.has('address'),
              wkNoDate: disabledTypes.has('date'),
              wkNoUnit: disabledTypes.has('unit'),
          },
      });
  }

  function decorateAnnotations(annotations) {

      if (decorations.length || !annotations.length)
          return;
      let failures = 0;
      decorations = [];

      document.addEventListener('click', handleTopTap.bind(document));
      document.addEventListener('click', handleTopTap.bind(document), true);
      annotations = removeOverlappingAnnotations(annotations);

      let annotationIndex = 0;
      enumerateSectionsNodes((node, index, text) => {
          if (!node.parentNode || text === '\n')
              return true;

          while (annotationIndex < annotations.length) {
              const annotation = annotations[annotationIndex];
              if (!annotation || annotation.end > index) {
                  break;
              }
              failures++;
              annotationIndex++;
          }
          const length = text.length;
          let replacements = [];
          while (annotationIndex < annotations.length) {
              const annotation = annotations[annotationIndex];
              if (!annotation) {
                  break;
              }
              const start = annotation.start;
              const end = annotation.end;
              if (index < end && index + length > start) {

                  const left = Math.max(0, start - index);
                  const right = Math.min(length, end - index);
                  const nodeText = text.substring(left, right);
                  const annotationLeft = Math.max(0, index - start);
                  const annotationRight = Math.min(end - start, index + length - start);
                  const annotationText = annotation.text.substring(annotationLeft, annotationRight);

                  if (nodeText != annotationText) {
                      failures++;
                      annotationIndex++;
                      continue;
                  }
                  replacements.push(new Replacement(annotationIndex, left, right, nodeText, annotation.type, annotation.text, annotation.data));

                  if (end <= index + length) {
                      annotationIndex++;
                      continue;
                  }
              }
              break;
          }

          let currentParentNode = node.parentNode;
          while (currentParentNode) {
              if (currentParentNode instanceof HTMLElement &&
                  NO_DECORATION_NODE_NAMES.has(currentParentNode.tagName)) {
                  replacements = [];
                  failures++;
                  break;
              }
              currentParentNode = currentParentNode.parentNode;
          }
          replaceNode(node, replacements, text);
          return annotationIndex < annotations.length;
      });

      failures += annotations.length - annotationIndex;
      sendWebKitMessage('annotations', {
          command: 'annotations.decoratingComplete',
          successes: annotations.length - failures,
          failures: failures,
          annotations: annotations.length,
          cancelled: []
      });
  }

  function removeDecorations() {
      for (let decoration of decorations) {
          const replacements = decoration.replacements;
          const parentNode = replacements[0].parentNode;
          if (!parentNode)
              return;
          parentNode.insertBefore(decoration.original, replacements[0]);
          for (let replacement of replacements) {
              parentNode.removeChild(replacement);
          }
      }
      decorations = [];
  }

  function removeDecorationsWithType(type) {
      var remainingDecorations = [];
      for (let decoration of decorations) {
          const replacements = decoration.replacements;
          const parentNode = replacements[0].parentNode;
          if (!parentNode)
              return;
          var hasReplacementOfType = false;
          var hasReplacementOfAnotherType = false;
          for (let replacement of replacements) {
              if (!(replacement instanceof HTMLElement)) {
                  continue;
              }
              var element = replacement;
              var replacementType = element.getAttribute('data-type');
              if (replacementType === type) {
                  hasReplacementOfType = true;
              }
              else {
                  hasReplacementOfAnotherType = true;
              }
          }
          if (!hasReplacementOfType) {

              remainingDecorations.push(decoration);
              continue;
          }
          if (!hasReplacementOfAnotherType) {

              parentNode.insertBefore(decoration.original, replacements[0]);
              for (let replacement of replacements) {
                  parentNode.removeChild(replacement);
              }
              continue;
          }

          let newReplacements = [];
          for (let replacement of replacements) {
              if (!(replacement instanceof HTMLElement)) {
                  newReplacements.push(replacement);
                  continue;
              }
              var element = replacement;
              var replacementType = element.getAttribute('data-type');
              if (replacementType !== type) {
                  newReplacements.push(replacement);
                  continue;
              }
              let text = document.createTextNode(element.textContent ?? '');
              parentNode.replaceChild(text, element);
              newReplacements.push(text);
          }
          decoration.replacements = newReplacements;
          remainingDecorations.push(decoration);
      }
      decorations = remainingDecorations;
  }

  function removeHighlight() {
      for (let decoration of decorations) {
          for (let replacement of decoration.replacements) {
              if (!(replacement instanceof HTMLElement)) {
                  continue;
              }
              replacement.style.color = '';
              replacement.style.background = '';
          }
      }
  }

  function enumerateTextNodes(root, process, includeShadowDOM = true, filterInvisibles = true) {
      const nodes = [root];
      let index = 0;
      let isPreviousSpace = true;
      while (nodes.length > 0) {
          let node = nodes.pop();
          if (!node) {
              break;
          }

          if (node.nodeType === Node.ELEMENT_NODE) {

              if (NON_TEXT_NODE_NAMES.has(node.nodeName)) {
                  continue;
              }

              if (node instanceof Element && node.getAttribute('contenteditable')) {
                  continue;
              }
              if (node.nodeName === 'BR') {
                  if (isPreviousSpace)
                      continue;
                  if (!process(node, index, '\n'))
                      break;
                  isPreviousSpace = true;
                  index += 1;
                  continue;
              }
              const style = window.getComputedStyle(node);

              if (filterInvisibles &&
                  (style.display === 'none' || style.visibility === 'hidden')) {
                  continue;
              }

              if (node.nodeName.toUpperCase() !== 'BODY' &&
                  style.display !== 'inline' && !isPreviousSpace) {
                  if (!process(node, index, '\n'))
                      break;
                  isPreviousSpace = true;
                  index += 1;
              }
              if (includeShadowDOM) {
                  const element = node;
                  if (element.shadowRoot && element.shadowRoot != node) {
                      nodes.push(element.shadowRoot);
                      continue;
                  }
              }
          }
          if (node.hasChildNodes()) {
              for (let childIdx = node.childNodes.length - 1; childIdx >= 0; childIdx--) {
                  nodes.push(node.childNodes[childIdx]);
              }
          }
          else if (node.nodeType === Node.TEXT_NODE && node.textContent) {
              const isSpace = node.textContent.trim() === '';
              if (isSpace && isPreviousSpace)
                  continue;
              if (!process(node, index, node.textContent))
                  break;
              isPreviousSpace = isSpace;
              index += node.textContent.length;
          }
      }
  }

  function enumerateSectionsNodes(process) {
      for (let section of sections) {
          const node = section.node.deref();
          if (!node)
              continue;
          const text = node.nodeType === Node.ELEMENT_NODE ? '\n' : node.textContent;
          if (text && !process(node, section.index, text))
              break;
      }
  }

  function getPageText(maxChars) {
      const parts = [];
      sections = [];
      enumerateTextNodes(document.body, function (node, index, text) {
          sections.push(new Section(new WeakRef(node), index));
          if (index + text.length > maxChars) {
              parts.push(text.substring(0, maxChars - index));
          }
          else {
              parts.push(text);
          }
          return index + text.length < maxChars;
      });
      return ''.concat(...parts);
  }
  let mutationDuringClickObserver;

  function cancelObserver() {
      mutationDuringClickObserver?.stopObserving();
      mutationDuringClickObserver = null;
  }

  function handleTopTap(event) {
      const annotation = event.target;
      if (annotation instanceof HTMLElement &&
          annotation.tagName === 'CHROME_ANNOTATION') {
          if (event.eventPhase === Event.CAPTURING_PHASE) {

              cancelObserver();
              mutationDuringClickObserver = new MutationsDuringClickTracker(event);
          }
          else if (mutationDuringClickObserver) {

              if (!mutationDuringClickObserver.hasPreventativeActivity(event)) {
                  mutationDuringClickObserver.extendObservation(() => {
                      if (mutationDuringClickObserver) {
                          highlightAnnotation(annotation);
                          onClickAnnotation(annotation, mutationDuringClickObserver.hasMutations);
                      }
                  });
              }
              else {
                  onClickAnnotation(annotation, mutationDuringClickObserver.hasMutations);
              }
          }
      }
      else {
          cancelObserver();
      }
  }

  function onClickAnnotation(annotation, cancel) {
      sendWebKitMessage('annotations', {
          command: 'annotations.onClick',
          cancel: cancel,
          data: annotation.dataset['data'],
          rect: rectFromElement(annotation),
          text: annotation.dataset['annotation'],
      });
      cancelObserver();
  }

  function highlightAnnotation(annotation) {

      for (let decoration of decorations) {
          for (let replacement of decoration.replacements) {
              if (!(replacement instanceof HTMLElement)) {
                  continue;
              }
              if (replacement.tagName === 'CHROME_ANNOTATION' &&
                  replacement.dataset['index'] === annotation.dataset['index']) {
                  replacement.style.color = highlightTextColor;
                  replacement.style.backgroundColor = highlightBackgroundColor;
              }
          }
      }
  }

  function removeOverlappingAnnotations(annotations) {

      annotations.sort((a, b) => {
          return a.start - b.start;
      });

      let previous = undefined;
      return annotations.filter((annotation) => {
          if (previous && previous.start < annotation.end &&
              previous.end > annotation.start) {
              return false;
          }
          previous = annotation;
          return true;
      });
  }

  function replaceNode(node, replacements, text) {
      const parentNode = node.parentNode;
      if (replacements.length <= 0 || !parentNode) {
          return;
      }
      let textColor = decorationDefaultColor;
      if (parentNode instanceof Element) {
          textColor = window.getComputedStyle(parentNode).color || textColor;
      }
      let cursor = 0;
      const parts = [];
      for (let replacement of replacements) {
          if (replacement.left > cursor) {
              parts.push(document.createTextNode(text.substring(cursor, replacement.left)));
          }
          const element = document.createElement('chrome_annotation');
          element.setAttribute('data-index', '' + replacement.index);
          element.setAttribute('data-data', replacement.data);
          element.setAttribute('data-annotation', replacement.annotationText);
          element.setAttribute('data-type', replacement.type);
          element.setAttribute('role', 'link');

          element.textContent = replacement.text;
          if (replacement.type == 'PHONE_NUMBER' || replacement.type == 'EMAIL') {
              element.style.cssText = decorationStylesForPhoneAndEmail;
          }
          else {
              element.style.cssText = decorationStyles;
          }
          element.style.borderBottomColor = textColor;
          parts.push(element);
          cursor = replacement.right;
      }
      if (cursor < text.length) {
          parts.push(document.createTextNode(text.substring(cursor, text.length)));
      }
      for (let part of parts) {
          parentNode.insertBefore(part, node);
      }
      parentNode.removeChild(node);

      decorations.push(new Decoration(node, parts));
  }
  function rectFromElement(element) {
      const domRect = element.getClientRects()[0];
      if (!domRect) {
          return {};
      }
      return {
          x: domRect.x,
          y: domRect.y,
          width: domRect.width,
          height: domRect.height
      };
  }
  gCrWeb.annotations = {
      extractText,
      decorateAnnotations,
      removeDecorations,
      removeDecorationsWithType,
      removeHighlight,
  };
}


// ios/chrome/browser/link_to_text/model/tsc/third_party/text-fragments-polyfill/src/src/text-fragment-utils.js
if (typeof _injected_text_fragment_utils) {
  var _injected_text_fragment_utils = true
  
  const FRAGMENT_DIRECTIVES = ['text'];

  const BLOCK_ELEMENTS = [
    'ADDRESS',    'ARTICLE',  'ASIDE',  'BLOCKQUOTE', 'BR',     'DETAILS',
    'DIALOG',     'DD',       'DIV',    'DL',         'DT',     'FIELDSET',
    'FIGCAPTION', 'FIGURE',   'FOOTER', 'FORM',       'H1',     'H2',
    'H3',         'H4',       'H5',     'H6',         'HEADER', 'HGROUP',
    'HR',         'LI',       'MAIN',   'NAV',        'OL',     'P',
    'PRE',        'SECTION',  'TABLE',  'UL',         'TR',     'TH',
    'TD',         'COLGROUP', 'COL',    'CAPTION',    'THEAD',  'TBODY',
    'TFOOT',
  ];

  const BOUNDARY_CHARS =
      /[\t-\r -#%-\*,-\/:;\?@\[-\]_\{\}\x85\xA0\xA1\xA7\xAB\xB6\xB7\xBB\xBF\u037E\u0387\u055A-\u055F\u0589\u058A\u05BE\u05C0\u05C3\u05C6\u05F3\u05F4\u0609\u060A\u060C\u060D\u061B\u061E\u061F\u066A-\u066D\u06D4\u0700-\u070D\u07F7-\u07F9\u0830-\u083E\u085E\u0964\u0965\u0970\u0AF0\u0DF4\u0E4F\u0E5A\u0E5B\u0F04-\u0F12\u0F14\u0F3A-\u0F3D\u0F85\u0FD0-\u0FD4\u0FD9\u0FDA\u104A-\u104F\u10FB\u1360-\u1368\u1400\u166D\u166E\u1680\u169B\u169C\u16EB-\u16ED\u1735\u1736\u17D4-\u17D6\u17D8-\u17DA\u1800-\u180A\u1944\u1945\u1A1E\u1A1F\u1AA0-\u1AA6\u1AA8-\u1AAD\u1B5A-\u1B60\u1BFC-\u1BFF\u1C3B-\u1C3F\u1C7E\u1C7F\u1CC0-\u1CC7\u1CD3\u2000-\u200A\u2010-\u2029\u202F-\u2043\u2045-\u2051\u2053-\u205F\u207D\u207E\u208D\u208E\u2308-\u230B\u2329\u232A\u2768-\u2775\u27C5\u27C6\u27E6-\u27EF\u2983-\u2998\u29D8-\u29DB\u29FC\u29FD\u2CF9-\u2CFC\u2CFE\u2CFF\u2D70\u2E00-\u2E2E\u2E30-\u2E44\u3000-\u3003\u3008-\u3011\u3014-\u301F\u3030\u303D\u30A0\u30FB\uA4FE\uA4FF\uA60D-\uA60F\uA673\uA67E\uA6F2-\uA6F7\uA874-\uA877\uA8CE\uA8CF\uA8F8-\uA8FA\uA8FC\uA92E\uA92F\uA95F\uA9C1-\uA9CD\uA9DE\uA9DF\uAA5C-\uAA5F\uAADE\uAADF\uAAF0\uAAF1\uABEB\uFD3E\uFD3F\uFE10-\uFE19\uFE30-\uFE52\uFE54-\uFE61\uFE63\uFE68\uFE6A\uFE6B\uFF01-\uFF03\uFF05-\uFF0A\uFF0C-\uFF0F\uFF1A\uFF1B\uFF1F\uFF20\uFF3B-\uFF3D\uFF3F\uFF5B\uFF5D\uFF5F-\uFF65]|\uD800[\uDD00-\uDD02\uDF9F\uDFD0]|\uD801\uDD6F|\uD802[\uDC57\uDD1F\uDD3F\uDE50-\uDE58\uDE7F\uDEF0-\uDEF6\uDF39-\uDF3F\uDF99-\uDF9C]|\uD804[\uDC47-\uDC4D\uDCBB\uDCBC\uDCBE-\uDCC1\uDD40-\uDD43\uDD74\uDD75\uDDC5-\uDDC9\uDDCD\uDDDB\uDDDD-\uDDDF\uDE38-\uDE3D\uDEA9]|\uD805[\uDC4B-\uDC4F\uDC5B\uDC5D\uDCC6\uDDC1-\uDDD7\uDE41-\uDE43\uDE60-\uDE6C\uDF3C-\uDF3E]|\uD807[\uDC41-\uDC45\uDC70\uDC71]|\uD809[\uDC70-\uDC74]|\uD81A[\uDE6E\uDE6F\uDEF5\uDF37-\uDF3B\uDF44]|\uD82F\uDC9F|\uD836[\uDE87-\uDE8B]|\uD83A[\uDD5E\uDD5F]/u;

  const NON_BOUNDARY_CHARS =
      /[^\t-\r -#%-\*,-\/:;\?@\[-\]_\{\}\x85\xA0\xA1\xA7\xAB\xB6\xB7\xBB\xBF\u037E\u0387\u055A-\u055F\u0589\u058A\u05BE\u05C0\u05C3\u05C6\u05F3\u05F4\u0609\u060A\u060C\u060D\u061B\u061E\u061F\u066A-\u066D\u06D4\u0700-\u070D\u07F7-\u07F9\u0830-\u083E\u085E\u0964\u0965\u0970\u0AF0\u0DF4\u0E4F\u0E5A\u0E5B\u0F04-\u0F12\u0F14\u0F3A-\u0F3D\u0F85\u0FD0-\u0FD4\u0FD9\u0FDA\u104A-\u104F\u10FB\u1360-\u1368\u1400\u166D\u166E\u1680\u169B\u169C\u16EB-\u16ED\u1735\u1736\u17D4-\u17D6\u17D8-\u17DA\u1800-\u180A\u1944\u1945\u1A1E\u1A1F\u1AA0-\u1AA6\u1AA8-\u1AAD\u1B5A-\u1B60\u1BFC-\u1BFF\u1C3B-\u1C3F\u1C7E\u1C7F\u1CC0-\u1CC7\u1CD3\u2000-\u200A\u2010-\u2029\u202F-\u2043\u2045-\u2051\u2053-\u205F\u207D\u207E\u208D\u208E\u2308-\u230B\u2329\u232A\u2768-\u2775\u27C5\u27C6\u27E6-\u27EF\u2983-\u2998\u29D8-\u29DB\u29FC\u29FD\u2CF9-\u2CFC\u2CFE\u2CFF\u2D70\u2E00-\u2E2E\u2E30-\u2E44\u3000-\u3003\u3008-\u3011\u3014-\u301F\u3030\u303D\u30A0\u30FB\uA4FE\uA4FF\uA60D-\uA60F\uA673\uA67E\uA6F2-\uA6F7\uA874-\uA877\uA8CE\uA8CF\uA8F8-\uA8FA\uA8FC\uA92E\uA92F\uA95F\uA9C1-\uA9CD\uA9DE\uA9DF\uAA5C-\uAA5F\uAADE\uAADF\uAAF0\uAAF1\uABEB\uFD3E\uFD3F\uFE10-\uFE19\uFE30-\uFE52\uFE54-\uFE61\uFE63\uFE68\uFE6A\uFE6B\uFF01-\uFF03\uFF05-\uFF0A\uFF0C-\uFF0F\uFF1A\uFF1B\uFF1F\uFF20\uFF3B-\uFF3D\uFF3F\uFF5B\uFF5D\uFF5F-\uFF65]|\uD800[\uDD00-\uDD02\uDF9F\uDFD0]|\uD801\uDD6F|\uD802[\uDC57\uDD1F\uDD3F\uDE50-\uDE58\uDE7F\uDEF0-\uDEF6\uDF39-\uDF3F\uDF99-\uDF9C]|\uD804[\uDC47-\uDC4D\uDCBB\uDCBC\uDCBE-\uDCC1\uDD40-\uDD43\uDD74\uDD75\uDDC5-\uDDC9\uDDCD\uDDDB\uDDDD-\uDDDF\uDE38-\uDE3D\uDEA9]|\uD805[\uDC4B-\uDC4F\uDC5B\uDC5D\uDCC6\uDDC1-\uDDD7\uDE41-\uDE43\uDE60-\uDE6C\uDF3C-\uDF3E]|\uD807[\uDC41-\uDC45\uDC70\uDC71]|\uD809[\uDC70-\uDC74]|\uD81A[\uDE6E\uDE6F\uDEF5\uDF37-\uDF3B\uDF44]|\uD82F\uDC9F|\uD836[\uDE87-\uDE8B]|\uD83A[\uDD5E\uDD5F]/u;

  const TEXT_FRAGMENT_CSS_CLASS_NAME =
      'text-fragments-polyfill-target-text';

  const getFragmentDirectives = (hash) => {
    const fragmentDirectivesStrings =
        hash.replace(/#.*?:~:(.*?)/, '$1').split(/&?text=/).filter(Boolean);
    if (!fragmentDirectivesStrings.length) {
      return {};
    } else {
      return {text: fragmentDirectivesStrings};
    }
  };

  const parseFragmentDirectives = (fragmentDirectives) => {
    const parsedFragmentDirectives = {};
    for (const
             [fragmentDirectiveType,
              fragmentDirectivesOfType,
    ] of Object.entries(fragmentDirectives)) {
      if (FRAGMENT_DIRECTIVES.includes(fragmentDirectiveType)) {
        parsedFragmentDirectives[fragmentDirectiveType] =
            fragmentDirectivesOfType.map((fragmentDirectiveOfType) => {
              return parseTextFragmentDirective(fragmentDirectiveOfType);
            });
      }
    }
    return parsedFragmentDirectives;
  };

  const parseTextFragmentDirective = (textFragment) => {
    const TEXT_FRAGMENT = /^(?:(.+?)-,)?(?:(.+?))(?:,([^-]+?))?(?:,-(.+?))?$/;
    return {
      prefix: decodeURIComponent(textFragment.replace(TEXT_FRAGMENT, '$1')),
      textStart: decodeURIComponent(textFragment.replace(TEXT_FRAGMENT, '$2')),
      textEnd: decodeURIComponent(textFragment.replace(TEXT_FRAGMENT, '$3')),
      suffix: decodeURIComponent(textFragment.replace(TEXT_FRAGMENT, '$4')),
    };
  };

  const processFragmentDirectives =
      (parsedFragmentDirectives, documentToProcess = document) => {
        const processedFragmentDirectives = {};
        for (const
                 [fragmentDirectiveType,
                  fragmentDirectivesOfType,
        ] of Object.entries(parsedFragmentDirectives)) {
          if (FRAGMENT_DIRECTIVES.includes(fragmentDirectiveType)) {
            processedFragmentDirectives[fragmentDirectiveType] =
                fragmentDirectivesOfType.map((fragmentDirectiveOfType) => {
                  const result = processTextFragmentDirective(
                      fragmentDirectiveOfType, documentToProcess);
                  if (result.length >= 1) {

                    return markRange(result[0], documentToProcess);
                  }
                  return [];
                });
          }
        }
        return processedFragmentDirectives;
      };

  const processTextFragmentDirective =
      (textFragment, documentToProcess = document) => {
        const results = [];

        const searchRange = documentToProcess.createRange();
        searchRange.selectNodeContents(documentToProcess.body);

        while (!searchRange.collapsed && results.length < 2) {
          let potentialMatch;
          if (textFragment.prefix) {
            const prefixMatch = findTextInRange(textFragment.prefix, searchRange);
            if (prefixMatch == null) {
              break;
            }

            advanceRangeStartPastOffset(
                searchRange,
                prefixMatch.startContainer,
                prefixMatch.startOffset,
            );

            const matchRange = documentToProcess.createRange();
            matchRange.setStart(prefixMatch.endContainer, prefixMatch.endOffset);
            matchRange.setEnd(searchRange.endContainer, searchRange.endOffset);

            advanceRangeStartToNonWhitespace(matchRange);
            if (matchRange.collapsed) {
              break;
            }

            potentialMatch = findTextInRange(textFragment.textStart, matchRange);

            if (potentialMatch == null) {
              break;
            }

            if (potentialMatch.compareBoundaryPoints(
                    Range.START_TO_START,
                    matchRange,
                    ) !== 0) {
              continue;
            }
          } else {

            potentialMatch = findTextInRange(textFragment.textStart, searchRange);
            if (potentialMatch == null) {
              break;
            }
            advanceRangeStartPastOffset(
                searchRange,
                potentialMatch.startContainer,
                potentialMatch.startOffset,
            );
          }

          if (textFragment.textEnd) {
            const textEndRange = documentToProcess.createRange();
            textEndRange.setStart(
                potentialMatch.endContainer, potentialMatch.endOffset);
            textEndRange.setEnd(searchRange.endContainer, searchRange.endOffset);

            let matchFound = false;

            while (!textEndRange.collapsed && results.length < 2) {
              const textEndMatch =
                  findTextInRange(textFragment.textEnd, textEndRange);
              if (textEndMatch == null) {
                break;
              }

              advanceRangeStartPastOffset(
                  textEndRange, textEndMatch.startContainer,
                  textEndMatch.startOffset);

              potentialMatch.setEnd(
                  textEndMatch.endContainer, textEndMatch.endOffset);

              if (textFragment.suffix) {

                const suffixResult = checkSuffix(
                    textFragment.suffix, potentialMatch, searchRange,
                    documentToProcess);
                if (suffixResult === CheckSuffixResult.NO_SUFFIX_MATCH) {
                  break;
                } else if (suffixResult === CheckSuffixResult.SUFFIX_MATCH) {
                  matchFound = true;
                  results.push(potentialMatch.cloneRange());
                  continue;
                } else if (suffixResult === CheckSuffixResult.MISPLACED_SUFFIX) {
                  continue;
                }
              } else {

                matchFound = true;
                results.push(potentialMatch.cloneRange());
              }
            }

            if (!matchFound) {
              break;
            }

          } else if (textFragment.suffix) {

            const suffixResult = checkSuffix(
                textFragment.suffix, potentialMatch, searchRange,
                documentToProcess);
            if (suffixResult === CheckSuffixResult.NO_SUFFIX_MATCH) {
              break;
            } else if (suffixResult === CheckSuffixResult.SUFFIX_MATCH) {
              results.push(potentialMatch.cloneRange());
              advanceRangeStartPastOffset(
                  searchRange, searchRange.startContainer,
                  searchRange.startOffset);
              continue;
            } else if (suffixResult === CheckSuffixResult.MISPLACED_SUFFIX) {
              continue;
            }
          } else {
            results.push(potentialMatch.cloneRange());
          }
        }
        return results;
      };

  const removeMarks = (marks, documentToProcess = document) => {
    for (const mark of marks) {
      const range = documentToProcess.createRange();
      range.selectNodeContents(mark);
      const fragment = range.extractContents();
      const parent = mark.parentNode;
      parent.insertBefore(fragment, mark);
      parent.removeChild(mark);
    }
  };

  const CheckSuffixResult = {
    NO_SUFFIX_MATCH: 0,
    SUFFIX_MATCH: 1,
    MISPLACED_SUFFIX: 2,
  };

  const checkSuffix =
      (suffix, potentialMatch, searchRange, documentToProcess) => {
        const suffixRange = documentToProcess.createRange();
        suffixRange.setStart(
            potentialMatch.endContainer,
            potentialMatch.endOffset,
        );
        suffixRange.setEnd(searchRange.endContainer, searchRange.endOffset);
        advanceRangeStartToNonWhitespace(suffixRange);

        const suffixMatch = findTextInRange(suffix, suffixRange);

        if (suffixMatch == null) {
          return CheckSuffixResult.NO_SUFFIX_MATCH;
        }

        if (suffixMatch.compareBoundaryPoints(
                Range.START_TO_START, suffixRange) !== 0) {
          return CheckSuffixResult.MISPLACED_SUFFIX;
        }

        return CheckSuffixResult.SUFFIX_MATCH;
      };

  const advanceRangeStartPastOffset = (range, node, offset) => {
    try {
      range.setStart(node, offset + 1);
    } catch (err) {
      range.setStartAfter(node);
    }
  };

  const advanceRangeStartToNonWhitespace = (range) => {
    const walker = makeTextNodeWalker(range);

    let node = walker.nextNode();
    while (!range.collapsed && node != null) {
      if (node !== range.startContainer) {
        range.setStart(node, 0);
      }

      if (node.textContent.length > range.startOffset) {
        const firstChar = node.textContent[range.startOffset];
        if (!firstChar.match(/\s/)) {
          return;
        }
      }

      try {
        range.setStart(node, range.startOffset + 1);
      } catch (err) {
        node = walker.nextNode();
        if (node == null) {
          range.collapse();
        } else {
          range.setStart(node, 0);
        }
      }
    }
  };

  const makeTextNodeWalker =
      (range) => {
        const walker = document.createTreeWalker(
            range.commonAncestorContainer,
            NodeFilter.SHOW_TEXT | NodeFilter.SHOW_ELEMENT,
            (node) => {
              return acceptTextNodeIfVisibleInRange(node, range);
            },
        );

        return walker;
      }

  const markRange = (range, documentToProcess = document) => {
    if (range.startContainer.nodeType != Node.TEXT_NODE ||
        range.endContainer.nodeType != Node.TEXT_NODE)
      return [];

    if (range.startContainer === range.endContainer) {
      const trivialMark = documentToProcess.createElement('mark');
      trivialMark.setAttribute('class', TEXT_FRAGMENT_CSS_CLASS_NAME);
      range.surroundContents(trivialMark);
      return [trivialMark];
    }

    const startNode = range.startContainer;
    const startNodeSubrange = range.cloneRange();
    startNodeSubrange.setEndAfter(startNode);

    const endNode = range.endContainer;
    const endNodeSubrange = range.cloneRange();
    endNodeSubrange.setStartBefore(endNode);

    const marks = [];
    range.setStartAfter(startNode);
    range.setEndBefore(endNode);
    const walker = documentToProcess.createTreeWalker(
        range.commonAncestorContainer,
        NodeFilter.SHOW_ELEMENT | NodeFilter.SHOW_TEXT,
        {
          acceptNode: function(node) {
            if (!range.intersectsNode(node)) return NodeFilter.FILTER_REJECT;

            if (BLOCK_ELEMENTS.includes(node.tagName) ||
                node.nodeType === Node.TEXT_NODE)
              return NodeFilter.FILTER_ACCEPT;
            return NodeFilter.FILTER_SKIP;
          },
        },
    );
    let node = walker.nextNode();
    while (node) {
      if (node.nodeType === Node.TEXT_NODE) {
        const mark = documentToProcess.createElement('mark');
        mark.setAttribute('class', TEXT_FRAGMENT_CSS_CLASS_NAME);
        node.parentNode.insertBefore(mark, node);
        mark.appendChild(node);
        marks.push(mark);
      }
      node = walker.nextNode();
    }

    const startMark = documentToProcess.createElement('mark');
    startMark.setAttribute('class', TEXT_FRAGMENT_CSS_CLASS_NAME);
    startNodeSubrange.surroundContents(startMark);
    const endMark = documentToProcess.createElement('mark');
    endMark.setAttribute('class', TEXT_FRAGMENT_CSS_CLASS_NAME);
    endNodeSubrange.surroundContents(endMark);

    return [startMark, ...marks, endMark];
  };

  const scrollElementIntoView = (element) => {
    const behavior = {
      behavior: 'auto',
      block: 'center',
      inline: 'nearest',
    };
    element.scrollIntoView(behavior);
  };

  const isNodeVisible =
      (node) => {

        let elt = node;
        while (elt != null && !(elt instanceof HTMLElement)) elt = elt.parentNode;
        if (elt != null) {
          const nodeStyle = window.getComputedStyle(elt);

          if (nodeStyle.visibility === 'hidden' || nodeStyle.display === 'none' ||
              nodeStyle.height === 0 || nodeStyle.width === 0 ||
              nodeStyle.opacity === 0) {
            return false;
          }
        }
        return true;
      }

  const acceptNodeIfVisibleInRange = (node, range) => {
    if (range != null && !range.intersectsNode(node))
      return NodeFilter.FILTER_REJECT;

    return isNodeVisible(node) ? NodeFilter.FILTER_ACCEPT :
                                 NodeFilter.FILTER_REJECT;
  };

  const acceptTextNodeIfVisibleInRange = (node, range) => {
    if (range != null && !range.intersectsNode(node))
      return NodeFilter.FILTER_REJECT;

    if (!isNodeVisible(node)) {
      return NodeFilter.FILTER_REJECT;
    }

    return node.nodeType === Node.TEXT_NODE ? NodeFilter.FILTER_ACCEPT :
                                              NodeFilter.FILTER_SKIP;
  };

  const getAllTextNodes = (root, range) => {
    const blocks = [];
    let tmp = [];

    const nodes = Array.from(
        getElementsIn(
            root,
            (node) => {
              return acceptNodeIfVisibleInRange(node, range);
            }),
    );

    for (const node of nodes) {
      if (node.nodeType === Node.TEXT_NODE) {
        tmp.push(node);
      } else if (
          node instanceof HTMLElement && BLOCK_ELEMENTS.includes(node.tagName) &&
          tmp.length > 0) {

        blocks.push(tmp);
        tmp = [];
      }
    }
    if (tmp.length > 0) blocks.push(tmp);

    return blocks;
  };

  const getTextContent = (nodes, startOffset, endOffset) => {
    let str = '';
    if (nodes.length === 1) {
      str = nodes[0].textContent.substring(startOffset, endOffset);
    } else {
      str = nodes[0].textContent.substring(startOffset) +
          nodes.slice(1, -1).reduce((s, n) => s + n.textContent, '') +
          nodes.slice(-1)[0].textContent.substring(0, endOffset);
    }
    return str.replace(/[\t\n\r ]+/g, ' ');
  };

  function* getElementsIn(root, filter) {
    const treeWalker = document.createTreeWalker(
        root,
        NodeFilter.SHOW_ELEMENT | NodeFilter.SHOW_TEXT,
        {acceptNode: filter},
    );

    const finishedSubtrees = new Set();
    while (forwardTraverse(treeWalker, finishedSubtrees) !== null) {
      yield treeWalker.currentNode;
    }
  }

  const findTextInRange = (query, range) => {
    const textNodeLists = getAllTextNodes(range.commonAncestorContainer, range);
    const segmenter = makeNewSegmenter();

    for (const list of textNodeLists) {
      const found = findRangeFromNodeList(query, range, list, segmenter);
      if (found !== undefined) return found;
    }
    return undefined;
  };

  const findRangeFromNodeList = (query, range, textNodes, segmenter) => {
    if (!query || !range || !(textNodes || []).length) return undefined;
    const data = normalizeString(getTextContent(textNodes, 0, undefined));
    const normalizedQuery = normalizeString(query);
    let searchStart = textNodes[0] === range.startNode ? range.startOffset : 0;
    let start;
    let end;
    while (searchStart < data.length) {
      const matchIndex = data.indexOf(normalizedQuery, searchStart);
      if (matchIndex === -1) return undefined;
      if (isWordBounded(data, matchIndex, normalizedQuery.length, segmenter)) {
        start = getBoundaryPointAtIndex(matchIndex, textNodes,  false);
        end = getBoundaryPointAtIndex(
            matchIndex + normalizedQuery.length,
            textNodes,
             true,
        );
      }

      if (start != null && end != null) {
        const foundRange = new Range();
        foundRange.setStart(start.node, start.offset);
        foundRange.setEnd(end.node, end.offset);

        if (range.compareBoundaryPoints(Range.START_TO_START, foundRange) <= 0 &&
            range.compareBoundaryPoints(Range.END_TO_END, foundRange) >= 0) {
          return foundRange;
        }
      }
      searchStart = matchIndex + 1;
    }
    return undefined;
  };

  const getBoundaryPointAtIndex = (index, textNodes, isEnd) => {
    let counted = 0;
    let normalizedData;
    for (let i = 0; i < textNodes.length; i++) {
      const node = textNodes[i];
      if (!normalizedData) normalizedData = normalizeString(node.data);
      let nodeEnd = counted + normalizedData.length;
      if (isEnd) nodeEnd += 1;
      if (nodeEnd > index) {

        const normalizedOffset = index - counted;
        let denormalizedOffset = Math.min(index - counted, node.data.length);

        const targetSubstring = isEnd ?
            normalizedData.substring(0, normalizedOffset) :
            normalizedData.substring(normalizedOffset);

        let candidateSubstring = isEnd ?
            normalizeString(node.data.substring(0, denormalizedOffset)) :
            normalizeString(node.data.substring(denormalizedOffset));

        const direction = (isEnd ? -1 : 1) *
            (targetSubstring.length > candidateSubstring.length ? -1 : 1);

        while (denormalizedOffset >= 0 &&
               denormalizedOffset <= node.data.length) {
          if (candidateSubstring.length === targetSubstring.length) {
            return {node: node, offset: denormalizedOffset};
          }

          denormalizedOffset += direction;

          candidateSubstring = isEnd ?
              normalizeString(node.data.substring(0, denormalizedOffset)) :
              normalizeString(node.data.substring(denormalizedOffset));
        }
      }
      counted += normalizedData.length;

      if (i + 1 < textNodes.length) {

        const nextNormalizedData = normalizeString(textNodes[i + 1].data);
        if (normalizedData.slice(-1) === ' ' &&
            nextNormalizedData.slice(0, 1) === ' ') {
          counted -= 1;
        }

        normalizedData = nextNormalizedData;
      }
    }
    return undefined;
  };

  const isWordBounded = (text, startPos, length, segmenter) => {
    if (startPos < 0 || startPos >= text.length || length <= 0 ||
        startPos + length > text.length) {
      return false;
    }

    if (segmenter) {

      const segments = segmenter.segment(text);
      const startSegment = segments.containing(startPos);
      if (!startSegment) return false;

      if (startSegment.isWordLike && startSegment.index != startPos) return false;

      const endPos = startPos + length;
      const endSegment = segments.containing(endPos);

      if (endSegment && endSegment.isWordLike && endSegment.index != endPos)
        return false;
    } else {

      if (text[startPos].match(BOUNDARY_CHARS)) {
        ++startPos;
        --length;
        if (!length) {
          return false;
        }
      }

      if (text[startPos + length - 1].match(BOUNDARY_CHARS)) {
        --length;
        if (!length) {
          return false;
        }
      }

      if (startPos !== 0 && (!text[startPos - 1].match(BOUNDARY_CHARS)))
        return false;

      if (startPos + length !== text.length &&
          !text[startPos + length].match(BOUNDARY_CHARS))
        return false;
    }

    return true;
  };

  const normalizeString = (str) => {

    return (str || '')
        .normalize('NFKD')
        .replace(/\s+/g, ' ')
        .replace(/[\u0300-\u036f]/g, '')
        .toLowerCase();
  };

  const makeNewSegmenter = () => {
    if (Intl.Segmenter) {
      let lang = document.documentElement.lang;
      if (!lang) {
        lang = navigator.languages;
      }
      return new Intl.Segmenter(lang, {granularity: 'word'});
    }
    return undefined;
  };

  const forwardTraverse = (walker, finishedSubtrees) => {

    if (!finishedSubtrees.has(walker.currentNode)) {
      const firstChild = walker.firstChild();
      if (firstChild !== null) {
        return firstChild;
      }
    }

    const nextSibling = walker.nextSibling();
    if (nextSibling !== null) {
      return nextSibling;
    }

    const parent = walker.parentNode();

    if (parent !== null) {
      finishedSubtrees.add(parent);
    }

    return parent;
  };

  const backwardTraverse = (walker, finishedSubtrees) => {

    if (!finishedSubtrees.has(walker.currentNode)) {
      const lastChild = walker.lastChild();
      if (lastChild !== null) {
        return lastChild;
      }
    }

    const previousSibling = walker.previousSibling();
    if (previousSibling !== null) {
      return previousSibling;
    }

    const parent = walker.parentNode();

    if (parent !== null) {
      finishedSubtrees.add(parent);
    }

    return parent;
  };

  const forTesting = {
    advanceRangeStartPastOffset: advanceRangeStartPastOffset,
    advanceRangeStartToNonWhitespace: advanceRangeStartToNonWhitespace,
    findRangeFromNodeList: findRangeFromNodeList,
    findTextInRange: findTextInRange,
    getBoundaryPointAtIndex: getBoundaryPointAtIndex,
    isWordBounded: isWordBounded,
    makeNewSegmenter: makeNewSegmenter,
    markRange: markRange,
    normalizeString: normalizeString,
    parseTextFragmentDirective: parseTextFragmentDirective,
    forwardTraverse: forwardTraverse,
    backwardTraverse: backwardTraverse,
    getAllTextNodes: getAllTextNodes,
    acceptTextNodeIfVisibleInRange: acceptTextNodeIfVisibleInRange
  };

  const internal = {
    BLOCK_ELEMENTS: BLOCK_ELEMENTS,
    BOUNDARY_CHARS: BOUNDARY_CHARS,
    NON_BOUNDARY_CHARS: NON_BOUNDARY_CHARS,
    acceptNodeIfVisibleInRange: acceptNodeIfVisibleInRange,
    normalizeString: normalizeString,
    makeNewSegmenter: makeNewSegmenter,
    forwardTraverse: forwardTraverse,
    backwardTraverse: backwardTraverse,
    makeTextNodeWalker: makeTextNodeWalker,
    isNodeVisible: isNodeVisible
  }

  if (typeof goog !== 'undefined') {

    goog.declareModuleId('googleChromeLabs.textFragmentPolyfill.textFragmentUtils');

  }

  const applyTargetTextStyle = () => {
    const styles = document.getElementsByTagName('style');
    if (!styles) return;

    for (const style of styles) {
      const cssRules = style.innerHTML;
      const targetTextRules =
          cssRules.match(/(\w*)::target-text\s*{\s*((.|\n)*?)\s*}/g);
      if (!targetTextRules) continue;

      const markCss = targetTextRules.join('\n');
      const newNode = document.createTextNode(markCss.replaceAll(
          '::target-text', ` .${TEXT_FRAGMENT_CSS_CLASS_NAME}`));
      style.appendChild(newNode);
    }
  };

  const setDefaultTextFragmentsStyle = ({backgroundColor, color}) => {
    const styles = document.getElementsByTagName('style');
    const defaultStyle = `.${TEXT_FRAGMENT_CSS_CLASS_NAME} {
      background-color: ${backgroundColor};
      color: ${color};
    }

    .${TEXT_FRAGMENT_CSS_CLASS_NAME} a, a .${TEXT_FRAGMENT_CSS_CLASS_NAME} {
      text-decoration: underline;
    }
    `
    if (styles.length === 0) {
      document.head.insertAdjacentHTML(
          'beforeend', `<style type="text/css">${defaultStyle}</style>`);
    }
    else {
      applyTargetTextStyle();
      const defaultStyleNode = document.createTextNode(defaultStyle);
      styles[0].insertBefore(defaultStyleNode, styles[0].firstChild);
    }
  };
}


// ios/chrome/browser/link_to_text/model/tsc/third_party/text-fragments-polyfill/src/src/fragment-generation-utils.js
if (typeof _injected_text_fragment_generation_utils === 'undefined') {
  var _injected_text_fragment_generation_utils = true
  
  const MAX_EXACT_MATCH_LENGTH = 300;
  const MIN_LENGTH_WITHOUT_CONTEXT = 20;
  const ITERATIONS_BEFORE_ADDING_CONTEXT = 1;
  const WORDS_TO_ADD_FIRST_ITERATION = 3;
  const WORDS_TO_ADD_SUBSEQUENT_ITERATIONS = 1;
  const TRUNCATE_RANGE_CHECK_CHARS = 10000;
  const MAX_DEPTH = 500;

  let timeoutDurationMs = 500;
  let t0;

  const setTimeout = (newTimeoutDurationMs) => {
      timeoutDurationMs = newTimeoutDurationMs;
  };

  const GenerateFragmentStatus = {
      SUCCESS: 0,
      INVALID_SELECTION: 1,
      AMBIGUOUS: 2,
      TIMEOUT: 3,
      EXECUTION_FAILED: 4
  };

  const generateFragment = (selection, startTime = Date.now()) => {
      try {
          return doGenerateFragment(selection, startTime);
      }
      catch (err) {
          if (err.isTimeout) {
              return { status: GenerateFragmentStatus.TIMEOUT };
          }
          else {
              return { status: GenerateFragmentStatus.EXECUTION_FAILED };
          }
      }
  };

  const isValidRangeForFragmentGeneration = (range) => {

      if (!range.toString()
          .substring(0, TRUNCATE_RANGE_CHECK_CHARS)
          .match(fragments.internal.NON_BOUNDARY_CHARS)) {
          return false;
      }

      try {
          if (range.startContainer.ownerDocument.defaultView !== window.top) {
              return false;
          }
      }
      catch {

          return false;
      }

      let node = range.commonAncestorContainer;
      let numIterations = 0;
      while (node) {
          if (node.nodeType == Node.ELEMENT_NODE) {
              if (['TEXTAREA', 'INPUT'].includes(node.tagName)) {
                  return false;
              }
              const editable = node.attributes.getNamedItem('contenteditable');
              if (editable && editable.value !== 'false') {
                  return false;
              }

              numIterations++;
              if (numIterations >= MAX_DEPTH) {
                  return false;
              }
          }
          node = node.parentNode;
      }
      return true;
  };

  const doGenerateFragment = (selection, startTime) => {
      recordStartTime(startTime);
      let range;
      try {
          range = selection.getRangeAt(0);
      }
      catch {
          return { status: GenerateFragmentStatus.INVALID_SELECTION };
      }
      expandRangeStartToWordBound(range);
      expandRangeEndToWordBound(range);

      const rangeBeforeShrinking = range.cloneRange();
      moveRangeEdgesToTextNodes(range);
      if (range.collapsed) {
          return { status: GenerateFragmentStatus.INVALID_SELECTION };
      }
      let factory;
      if (canUseExactMatch(range)) {
          const exactText = fragments.internal.normalizeString(range.toString());
          const fragment = {
              textStart: exactText,
          };

          if (exactText.length >= MIN_LENGTH_WITHOUT_CONTEXT &&
              isUniquelyIdentifying(fragment)) {
              return {
                  status: GenerateFragmentStatus.SUCCESS,
                  fragment: fragment,
              };
          }
          factory = new FragmentFactory().setExactTextMatch(exactText);
      }
      else {

          const startSearchSpace = getSearchSpaceForStart(range);
          const endSearchSpace = getSearchSpaceForEnd(range);
          if (startSearchSpace && endSearchSpace) {

              factory = new FragmentFactory().setStartAndEndSearchSpace(startSearchSpace, endSearchSpace);
          }
          else {

              factory =
                  new FragmentFactory().setSharedSearchSpace(range.toString().trim());
          }
      }
      const prefixRange = document.createRange();
      prefixRange.selectNodeContents(document.body);
      const suffixRange = prefixRange.cloneRange();
      prefixRange.setEnd(rangeBeforeShrinking.startContainer, rangeBeforeShrinking.startOffset);
      suffixRange.setStart(rangeBeforeShrinking.endContainer, rangeBeforeShrinking.endOffset);
      const prefixSearchSpace = getSearchSpaceForEnd(prefixRange);
      const suffixSearchSpace = getSearchSpaceForStart(suffixRange);
      if (prefixSearchSpace || suffixSearchSpace) {
          factory.setPrefixAndSuffixSearchSpace(prefixSearchSpace, suffixSearchSpace);
      }
      factory.useSegmenter(fragments.internal.makeNewSegmenter());
      let didEmbiggen = false;
      do {
          checkTimeout();
          didEmbiggen = factory.embiggen();
          const fragment = factory.tryToMakeUniqueFragment();
          if (fragment != null) {
              return {
                  status: GenerateFragmentStatus.SUCCESS,
                  fragment: fragment,
              };
          }
      } while (didEmbiggen);
      return { status: GenerateFragmentStatus.AMBIGUOUS };
  };

  const checkTimeout = () => {

      if (timeoutDurationMs === null) {
          return;
      }
      const delta = Date.now() - t0;
      if (delta > timeoutDurationMs) {
          const timeoutError = new Error(`Fragment generation timed out after ${delta} ms.`);
          timeoutError.isTimeout = true;
          throw timeoutError;
      }
  };

  const recordStartTime = (newStartTime) => {
      t0 = newStartTime;
  };

  const getSearchSpaceForStart = (range) => {
      let node = getFirstNodeForBlockSearch(range);
      const walker = makeWalkerForNode(node, range.endContainer);
      if (!walker) {
          return undefined;
      }
      const finishedSubtrees = new Set();

      if (range.startContainer.nodeType === Node.ELEMENT_NODE &&
          range.startOffset === range.startContainer.childNodes.length) {
          finishedSubtrees.add(range.startContainer);
      }
      const origin = node;
      const textAccumulator = new BlockTextAccumulator(range, true);

      const tempRange = range.cloneRange();
      while (!tempRange.collapsed && node != null) {
          checkTimeout();

          if (node.contains(origin)) {
              tempRange.setStartAfter(node);
          }
          else {
              tempRange.setStartBefore(node);
          }

          textAccumulator.appendNode(node);

          if (textAccumulator.textInBlock !== null) {
              return textAccumulator.textInBlock;
          }
          node = fragments.internal.forwardTraverse(walker, finishedSubtrees);
      }
      return undefined;
  };

  const getSearchSpaceForEnd = (range) => {
      let node = getLastNodeForBlockSearch(range);
      const walker = makeWalkerForNode(node, range.startContainer);
      if (!walker) {
          return undefined;
      }
      const finishedSubtrees = new Set();

      if (range.endContainer.nodeType === Node.ELEMENT_NODE &&
          range.endOffset === 0) {
          finishedSubtrees.add(range.endContainer);
      }
      const origin = node;
      const textAccumulator = new BlockTextAccumulator(range, false);

      const tempRange = range.cloneRange();
      while (!tempRange.collapsed && node != null) {
          checkTimeout();

          if (node.contains(origin)) {
              tempRange.setEnd(node, 0);
          }
          else {
              tempRange.setEndAfter(node);
          }

          textAccumulator.appendNode(node);

          if (textAccumulator.textInBlock !== null) {
              return textAccumulator.textInBlock;
          }
          node = fragments.internal.backwardTraverse(walker, finishedSubtrees);
      }
      return undefined;
  };

  const FragmentFactory = class {

      constructor() {
          this.Mode = {
              ALL_PARTS: 1,
              SHARED_START_AND_END: 2,
              CONTEXT_ONLY: 3,
          };
          this.startOffset = null;
          this.endOffset = null;
          this.prefixOffset = null;
          this.suffixOffset = null;
          this.prefixSearchSpace = '';
          this.backwardsPrefixSearchSpace = '';
          this.suffixSearchSpace = '';
          this.numIterations = 0;
      }

      tryToMakeUniqueFragment() {
          let fragment;
          if (this.mode === this.Mode.CONTEXT_ONLY) {
              fragment = { textStart: this.exactTextMatch };
          }
          else {
              fragment = {
                  textStart: this.getStartSearchSpace().substring(0, this.startOffset).trim(),
                  textEnd: this.getEndSearchSpace().substring(this.endOffset).trim(),
              };
          }
          if (this.prefixOffset != null) {
              const prefix = this.getPrefixSearchSpace().substring(this.prefixOffset).trim();
              if (prefix) {
                  fragment.prefix = prefix;
              }
          }
          if (this.suffixOffset != null) {
              const suffix = this.getSuffixSearchSpace().substring(0, this.suffixOffset).trim();
              if (suffix) {
                  fragment.suffix = suffix;
              }
          }
          return isUniquelyIdentifying(fragment) ? fragment : undefined;
      }

      embiggen() {
          let canExpandRange = true;
          if (this.mode === this.Mode.SHARED_START_AND_END) {
              if (this.startOffset >= this.endOffset) {

                  canExpandRange = false;
              }
          }
          else if (this.mode === this.Mode.ALL_PARTS) {

              if (this.startOffset === this.getStartSearchSpace().length &&
                  this.backwardsEndOffset() === this.getEndSearchSpace().length) {
                  canExpandRange = false;
              }
          }
          else if (this.mode === this.Mode.CONTEXT_ONLY) {
              canExpandRange = false;
          }
          if (canExpandRange) {
              const desiredIterations = this.getNumberOfRangeWordsToAdd();
              if (this.startOffset < this.getStartSearchSpace().length) {
                  let i = 0;
                  if (this.getStartSegments() != null) {
                      while (i < desiredIterations &&
                          this.startOffset < this.getStartSearchSpace().length) {
                          this.startOffset = this.getNextOffsetForwards(this.getStartSegments(), this.startOffset, this.getStartSearchSpace());
                          i++;
                      }
                  }
                  else {

                      let oldStartOffset = this.startOffset;
                      do {
                          checkTimeout();
                          const newStartOffset = this.getStartSearchSpace()
                              .substring(this.startOffset + 1)
                              .search(fragments.internal.BOUNDARY_CHARS);
                          if (newStartOffset === -1) {
                              this.startOffset = this.getStartSearchSpace().length;
                          }
                          else {
                              this.startOffset = this.startOffset + 1 + newStartOffset;
                          }

                          if (this.getStartSearchSpace()
                              .substring(oldStartOffset, this.startOffset)
                              .search(fragments.internal.NON_BOUNDARY_CHARS) !== -1) {
                              oldStartOffset = this.startOffset;
                              i++;
                          }
                      } while (this.startOffset < this.getStartSearchSpace().length &&
                          i < desiredIterations);
                  }

                  if (this.mode === this.Mode.SHARED_START_AND_END) {
                      this.startOffset = Math.min(this.startOffset, this.endOffset);
                  }
              }
              if (this.backwardsEndOffset() < this.getEndSearchSpace().length) {
                  let i = 0;
                  if (this.getEndSegments() != null) {
                      while (i < desiredIterations && this.endOffset > 0) {
                          this.endOffset = this.getNextOffsetBackwards(this.getEndSegments(), this.endOffset);
                          i++;
                      }
                  }
                  else {

                      let oldBackwardsEndOffset = this.backwardsEndOffset();
                      do {
                          checkTimeout();
                          const newBackwardsOffset = this.getBackwardsEndSearchSpace()
                              .substring(this.backwardsEndOffset() + 1)
                              .search(fragments.internal.BOUNDARY_CHARS);
                          if (newBackwardsOffset === -1) {
                              this.setBackwardsEndOffset(this.getEndSearchSpace().length);
                          }
                          else {
                              this.setBackwardsEndOffset(this.backwardsEndOffset() + 1 + newBackwardsOffset);
                          }

                          if (this.getBackwardsEndSearchSpace()
                              .substring(oldBackwardsEndOffset, this.backwardsEndOffset())
                              .search(fragments.internal.NON_BOUNDARY_CHARS) !== -1) {
                              oldBackwardsEndOffset = this.backwardsEndOffset();
                              i++;
                          }
                      } while (this.backwardsEndOffset() <
                          this.getEndSearchSpace().length &&
                          i < desiredIterations);
                  }

                  if (this.mode === this.Mode.SHARED_START_AND_END) {
                      this.endOffset = Math.max(this.startOffset, this.endOffset);
                  }
              }
          }
          let canExpandContext = false;
          if (!canExpandRange ||
              this.startOffset + this.backwardsEndOffset() <
                  MIN_LENGTH_WITHOUT_CONTEXT ||
              this.numIterations >= ITERATIONS_BEFORE_ADDING_CONTEXT) {

              if ((this.backwardsPrefixOffset() != null &&
                  this.backwardsPrefixOffset() !==
                      this.getPrefixSearchSpace().length) ||
                  (this.suffixOffset != null &&
                      this.suffixOffset !== this.getSuffixSearchSpace().length)) {
                  canExpandContext = true;
              }
          }
          if (canExpandContext) {
              const desiredIterations = this.getNumberOfContextWordsToAdd();
              if (this.backwardsPrefixOffset() < this.getPrefixSearchSpace().length) {
                  let i = 0;
                  if (this.getPrefixSegments() != null) {
                      while (i < desiredIterations && this.prefixOffset > 0) {
                          this.prefixOffset = this.getNextOffsetBackwards(this.getPrefixSegments(), this.prefixOffset);
                          i++;
                      }
                  }
                  else {

                      let oldBackwardsPrefixOffset = this.backwardsPrefixOffset();
                      do {
                          checkTimeout();
                          const newBackwardsPrefixOffset = this.getBackwardsPrefixSearchSpace()
                              .substring(this.backwardsPrefixOffset() + 1)
                              .search(fragments.internal.BOUNDARY_CHARS);
                          if (newBackwardsPrefixOffset === -1) {
                              this.setBackwardsPrefixOffset(this.getBackwardsPrefixSearchSpace().length);
                          }
                          else {
                              this.setBackwardsPrefixOffset(this.backwardsPrefixOffset() + 1 + newBackwardsPrefixOffset);
                          }

                          if (this.getBackwardsPrefixSearchSpace()
                              .substring(oldBackwardsPrefixOffset, this.backwardsPrefixOffset())
                              .search(fragments.internal.NON_BOUNDARY_CHARS) !== -1) {
                              oldBackwardsPrefixOffset = this.backwardsPrefixOffset();
                              i++;
                          }
                      } while (this.backwardsPrefixOffset() <
                          this.getPrefixSearchSpace().length &&
                          i < desiredIterations);
                  }
              }
              if (this.suffixOffset < this.getSuffixSearchSpace().length) {
                  let i = 0;
                  if (this.getSuffixSegments() != null) {
                      while (i < desiredIterations &&
                          this.suffixOffset < this.getSuffixSearchSpace().length) {
                          this.suffixOffset = this.getNextOffsetForwards(this.getSuffixSegments(), this.suffixOffset, this.suffixOffset);
                          i++;
                      }
                  }
                  else {
                      let oldSuffixOffset = this.suffixOffset;
                      do {
                          checkTimeout();
                          const newSuffixOffset = this.getSuffixSearchSpace()
                              .substring(this.suffixOffset + 1)
                              .search(fragments.internal.BOUNDARY_CHARS);
                          if (newSuffixOffset === -1) {
                              this.suffixOffset = this.getSuffixSearchSpace().length;
                          }
                          else {
                              this.suffixOffset = this.suffixOffset + 1 + newSuffixOffset;
                          }

                          if (this.getSuffixSearchSpace()
                              .substring(oldSuffixOffset, this.suffixOffset)
                              .search(fragments.internal.NON_BOUNDARY_CHARS) !== -1) {
                              oldSuffixOffset = this.suffixOffset;
                              i++;
                          }
                      } while (this.suffixOffset < this.getSuffixSearchSpace().length &&
                          i < desiredIterations);
                  }
              }
          }
          this.numIterations++;

          return canExpandRange || canExpandContext;
      }

      setStartAndEndSearchSpace(startSearchSpace, endSearchSpace) {
          this.startSearchSpace = startSearchSpace;
          this.endSearchSpace = endSearchSpace;
          this.backwardsEndSearchSpace = reverseString(endSearchSpace);
          this.startOffset = 0;
          this.endOffset = endSearchSpace.length;
          this.mode = this.Mode.ALL_PARTS;
          return this;
      }

      setSharedSearchSpace(sharedSearchSpace) {
          this.sharedSearchSpace = sharedSearchSpace;
          this.backwardsSharedSearchSpace = reverseString(sharedSearchSpace);
          this.startOffset = 0;
          this.endOffset = sharedSearchSpace.length;
          this.mode = this.Mode.SHARED_START_AND_END;
          return this;
      }

      setExactTextMatch(exactTextMatch) {
          this.exactTextMatch = exactTextMatch;
          this.mode = this.Mode.CONTEXT_ONLY;
          return this;
      }

      setPrefixAndSuffixSearchSpace(prefixSearchSpace, suffixSearchSpace) {
          if (prefixSearchSpace) {
              this.prefixSearchSpace = prefixSearchSpace;
              this.backwardsPrefixSearchSpace = reverseString(prefixSearchSpace);
              this.prefixOffset = prefixSearchSpace.length;
          }
          if (suffixSearchSpace) {
              this.suffixSearchSpace = suffixSearchSpace;
              this.suffixOffset = 0;
          }
          return this;
      }

      useSegmenter(segmenter) {
          if (segmenter == null) {
              return this;
          }
          if (this.mode === this.Mode.ALL_PARTS) {
              this.startSegments = segmenter.segment(this.startSearchSpace);
              this.endSegments = segmenter.segment(this.endSearchSpace);
          }
          else if (this.mode === this.Mode.SHARED_START_AND_END) {
              this.sharedSegments = segmenter.segment(this.sharedSearchSpace);
          }
          if (this.prefixSearchSpace) {
              this.prefixSegments = segmenter.segment(this.prefixSearchSpace);
          }
          if (this.suffixSearchSpace) {
              this.suffixSegments = segmenter.segment(this.suffixSearchSpace);
          }
          return this;
      }

      getNumberOfContextWordsToAdd() {
          return (this.backwardsPrefixOffset() === 0 && this.suffixOffset === 0) ?
              WORDS_TO_ADD_FIRST_ITERATION :
              WORDS_TO_ADD_SUBSEQUENT_ITERATIONS;
      }

      getNumberOfRangeWordsToAdd() {
          return (this.startOffset === 0 && this.backwardsEndOffset() === 0) ?
              WORDS_TO_ADD_FIRST_ITERATION :
              WORDS_TO_ADD_SUBSEQUENT_ITERATIONS;
      }

      getNextOffsetForwards(segments, offset, searchSpace) {

          let currentSegment = segments.containing(offset);
          while (currentSegment != null) {
              checkTimeout();
              const currentSegmentEnd = currentSegment.index + currentSegment.segment.length;
              if (currentSegment.isWordLike) {
                  return currentSegmentEnd;
              }
              currentSegment = segments.containing(currentSegmentEnd);
          }

          return searchSpace.length;
      }

      getNextOffsetBackwards(segments, offset) {

          let currentSegment = segments.containing(offset);

          if (!currentSegment || offset == currentSegment.index) {

              currentSegment = segments.containing(offset - 1);
          }
          while (currentSegment != null) {
              checkTimeout();
              if (currentSegment.isWordLike) {
                  return currentSegment.index;
              }
              currentSegment = segments.containing(currentSegment.index - 1);
          }

          return 0;
      }

      getStartSearchSpace() {
          return this.mode === this.Mode.SHARED_START_AND_END ?
              this.sharedSearchSpace :
              this.startSearchSpace;
      }

      getStartSegments() {
          return this.mode === this.Mode.SHARED_START_AND_END ? this.sharedSegments :
              this.startSegments;
      }

      getEndSearchSpace() {
          return this.mode === this.Mode.SHARED_START_AND_END ?
              this.sharedSearchSpace :
              this.endSearchSpace;
      }

      getEndSegments() {
          return this.mode === this.Mode.SHARED_START_AND_END ? this.sharedSegments :
              this.endSegments;
      }

      getBackwardsEndSearchSpace() {
          return this.mode === this.Mode.SHARED_START_AND_END ?
              this.backwardsSharedSearchSpace :
              this.backwardsEndSearchSpace;
      }

      getPrefixSearchSpace() {
          return this.prefixSearchSpace;
      }

      getPrefixSegments() {
          return this.prefixSegments;
      }

      getBackwardsPrefixSearchSpace() {
          return this.backwardsPrefixSearchSpace;
      }

      getSuffixSearchSpace() {
          return this.suffixSearchSpace;
      }

      getSuffixSegments() {
          return this.suffixSegments;
      }

      backwardsEndOffset() {
          return this.getEndSearchSpace().length - this.endOffset;
      }

      setBackwardsEndOffset(backwardsEndOffset) {
          this.endOffset = this.getEndSearchSpace().length - backwardsEndOffset;
      }

      backwardsPrefixOffset() {
          if (this.prefixOffset == null)
              return null;
          return this.getPrefixSearchSpace().length - this.prefixOffset;
      }

      setBackwardsPrefixOffset(backwardsPrefixOffset) {
          if (this.prefixOffset == null)
              return;
          this.prefixOffset =
              this.getPrefixSearchSpace().length - backwardsPrefixOffset;
      }
  };

  const BlockTextAccumulator = class {

      constructor(searchRange, isForwardTraversal) {
          this.searchRange = searchRange;
          this.isForwardTraversal = isForwardTraversal;
          this.textFound = false;
          this.textNodes = [];
          this.textInBlock = null;
      }

      appendNode(node) {

          if (this.textInBlock !== null) {
              return;
          }

          if (isBlock(node)) {
              if (this.textFound) {

                  if (!this.isForwardTraversal) {
                      this.textNodes.reverse();
                  }

                  this.textInBlock = this.textNodes.map(textNode => textNode.textContent)
                      .join('')
                      .trim();
              }
              else {

                  this.textNodes = [];
              }
              return;
          }

          if (!isText(node))
              return;

          const nodeToInsert = this.getNodeIntersectionWithRange(node);

          this.textFound = this.textFound || nodeToInsert.textContent.trim() !== '';
          this.textNodes.push(nodeToInsert);
      }

      getNodeIntersectionWithRange(node) {
          let startOffset = null;
          let endOffset = null;
          if (node === this.searchRange.startContainer &&
              this.searchRange.startOffset !== 0) {
              startOffset = this.searchRange.startOffset;
          }
          if (node === this.searchRange.endContainer &&
              this.searchRange.endOffset !== node.textContent.length) {
              endOffset = this.searchRange.endOffset;
          }
          if (startOffset !== null || endOffset !== null) {
              return {
                  textContent: node.textContent.substring(startOffset ?? 0, endOffset ?? node.textContent.length)
              };
          }
          return node;
      }
  };

  const isUniquelyIdentifying = (fragment) => {
      return fragments.processTextFragmentDirective(fragment).length === 1;
  };

  const reverseString = (string) => {

      return [...(string || '')].reverse().join('');
  };

  const canUseExactMatch = (range) => {
      if (range.toString().length > MAX_EXACT_MATCH_LENGTH)
          return false;
      return !containsBlockBoundary(range);
  };

  const getFirstNodeForBlockSearch = (range) => {

      let node = range.startContainer;
      if (node.nodeType == Node.ELEMENT_NODE &&
          range.startOffset < node.childNodes.length) {
          node = node.childNodes[range.startOffset];
      }
      return node;
  };

  const getLastNodeForBlockSearch = (range) => {

      let node = range.endContainer;
      if (node.nodeType == Node.ELEMENT_NODE && range.endOffset > 0) {
          node = node.childNodes[range.endOffset - 1];
      }
      return node;
  };

  const getFirstTextNode = (range) => {

      const firstNode = getFirstNodeForBlockSearch(range);
      if (isText(firstNode) && fragments.internal.isNodeVisible(firstNode)) {
          return firstNode;
      }

      const walker = fragments.internal.makeTextNodeWalker(range);
      walker.currentNode = firstNode;
      return walker.nextNode();
  };

  const getLastTextNode = (range) => {

      const lastNode = getLastNodeForBlockSearch(range);
      if (isText(lastNode) && fragments.internal.isNodeVisible(lastNode)) {
          return lastNode;
      }

      const walker = fragments.internal.makeTextNodeWalker(range);
      walker.currentNode = lastNode;
      return fragments.internal.backwardTraverse(walker, new Set());
  };

  const containsBlockBoundary = (range) => {
      const tempRange = range.cloneRange();
      let node = getFirstNodeForBlockSearch(tempRange);
      const walker = makeWalkerForNode(node);
      if (!walker) {
          return false;
      }
      const finishedSubtrees = new Set();
      while (!tempRange.collapsed && node != null) {
          if (isBlock(node))
              return true;
          if (node != null)
              tempRange.setStartAfter(node);
          node = fragments.internal.forwardTraverse(walker, finishedSubtrees);
          checkTimeout();
      }
      return false;
  };

  const findWordStartBoundInTextNode = (node, startOffset) => {
      if (node.nodeType !== Node.TEXT_NODE)
          return -1;
      const offset = startOffset != null ? startOffset : node.data.length;

      if (offset < node.data.length &&
          fragments.internal.BOUNDARY_CHARS.test(node.data[offset]))
          return offset;
      const precedingText = node.data.substring(0, offset);
      const boundaryIndex = reverseString(precedingText).search(fragments.internal.BOUNDARY_CHARS);
      if (boundaryIndex !== -1) {

          return offset - boundaryIndex;
      }
      return -1;
  };

  const findWordEndBoundInTextNode = (node, endOffset) => {
      if (node.nodeType !== Node.TEXT_NODE)
          return -1;
      const offset = endOffset != null ? endOffset : 0;

      if (offset < node.data.length && offset > 0 &&
          fragments.internal.BOUNDARY_CHARS.test(node.data[offset - 1])) {
          return offset;
      }
      const followingText = node.data.substring(offset);
      const boundaryIndex = followingText.search(fragments.internal.BOUNDARY_CHARS);
      if (boundaryIndex !== -1) {
          return offset + boundaryIndex;
      }
      return -1;
  };

  const makeWalkerForNode = (node, endNode) => {
      if (!node) {
          return undefined;
      }

      let blockAncestor = node;
      const endNodeNotNull = endNode != null ? endNode : node;
      while (!blockAncestor.contains(endNodeNotNull) || !isBlock(blockAncestor)) {
          if (blockAncestor.parentNode) {
              blockAncestor = blockAncestor.parentNode;
          }
      }
      const walker = document.createTreeWalker(blockAncestor, NodeFilter.SHOW_ELEMENT | NodeFilter.SHOW_TEXT, (node) => {
          return fragments.internal.acceptNodeIfVisibleInRange(node);
      });
      walker.currentNode = node;
      return walker;
  };

  const expandRangeStartToWordBound = (range) => {
      const segmenter = fragments.internal.makeNewSegmenter();
      if (segmenter) {

          const startNode = getFirstNodeForBlockSearch(range);
          if (startNode !== range.startContainer) {
              range.setStartBefore(startNode);
          }
          expandToNearestWordBoundaryPointUsingSegments(segmenter,  false, range);
      }
      else {

          const newOffset = findWordStartBoundInTextNode(range.startContainer, range.startOffset);
          if (newOffset !== -1) {
              range.setStart(range.startContainer, newOffset);
              return;
          }

          if (isBlock(range.startContainer) && range.startOffset === 0) {
              return;
          }
          const walker = makeWalkerForNode(range.startContainer);
          if (!walker) {
              return;
          }
          const finishedSubtrees = new Set();
          let node = fragments.internal.backwardTraverse(walker, finishedSubtrees);
          while (node != null) {
              const newOffset = findWordStartBoundInTextNode(node);
              if (newOffset !== -1) {
                  range.setStart(node, newOffset);
                  return;
              }

              if (isBlock(node)) {
                  if (node.contains(range.startContainer)) {

                      range.setStart(node, 0);
                  }
                  else {

                      range.setStartAfter(node);
                  }
                  return;
              }
              node = fragments.internal.backwardTraverse(walker, finishedSubtrees);

              range.collapse();
          }
      }
  };

  const moveRangeEdgesToTextNodes = (range) => {
      const firstTextNode = getFirstTextNode(range);

      if (firstTextNode == null) {
          range.collapse();
          return;
      }
      const firstNode = getFirstNodeForBlockSearch(range);

      if (firstNode !== firstTextNode) {
          range.setStart(firstTextNode, 0);
      }
      const lastNode = getLastNodeForBlockSearch(range);
      const lastTextNode = getLastTextNode(range);

      if (lastNode !== lastTextNode) {
          range.setEnd(lastTextNode, lastTextNode.textContent.length);
      }
  };

  const expandToNearestWordBoundaryPointUsingSegments = (segmenter, isRangeEnd, range) => {

      const boundary = isRangeEnd ?
          { node: range.endContainer, offset: range.endOffset } :
          { node: range.startContainer, offset: range.startOffset };
      const nodes = getTextNodesInSameBlock(boundary.node);
      const preNodeText = nodes.preNodes.reduce((prev, cur) => {
          return prev.concat(cur.textContent);
      }, '');
      const innerNodeText = nodes.innerNodes.reduce((prev, cur) => {
          return prev.concat(cur.textContent);
      }, '');
      let offsetInText = preNodeText.length;
      if (boundary.node.nodeType === Node.TEXT_NODE) {
          offsetInText += boundary.offset;
      }
      else if (isRangeEnd) {
          offsetInText += innerNodeText.length;
      }

      const postNodeText = nodes.postNodes.reduce((prev, cur) => {
          return prev.concat(cur.textContent);
      }, '');
      const allNodes = [...nodes.preNodes, ...nodes.innerNodes, ...nodes.postNodes];

      if (allNodes.length == 0) {
          return;
      }
      const text = preNodeText.concat(innerNodeText, postNodeText);
      const segments = segmenter.segment(text);
      const foundSegment = segments.containing(offsetInText);
      if (!foundSegment) {
          if (isRangeEnd) {
              range.setEndAfter(allNodes[allNodes.length - 1]);
          }
          else {
              range.setEndBefore(allNodes[0]);
          }
          return;
      }

      if (!foundSegment.isWordLike) {
          return;
      }

      if (offsetInText === foundSegment.index ||
          offsetInText === foundSegment.index + foundSegment.segment.length) {
          return;
      }

      const desiredOffsetInText = isRangeEnd ?
          foundSegment.index + foundSegment.segment.length :
          foundSegment.index;
      let newNodeIndexInText = 0;
      for (const node of allNodes) {
          if (newNodeIndexInText <= desiredOffsetInText &&
              desiredOffsetInText <
                  newNodeIndexInText + node.textContent.length) {
              const offsetInNode = desiredOffsetInText - newNodeIndexInText;
              if (isRangeEnd) {
                  if (offsetInNode >= node.textContent.length) {
                      range.setEndAfter(node);
                  }
                  else {
                      range.setEnd(node, offsetInNode);
                  }
              }
              else {
                  if (offsetInNode >= node.textContent.length) {
                      range.setStartAfter(node);
                  }
                  else {
                      range.setStart(node, offsetInNode);
                  }
              }
              return;
          }
          newNodeIndexInText += node.textContent.length;
      }

      if (isRangeEnd) {
          range.setEndAfter(allNodes[allNodes.length - 1]);
      }
      else {
          range.setStartBefore(allNodes[0]);
      }
  };

  const getTextNodesInSameBlock = (node) => {
      const preNodes = [];

      const backWalker = makeWalkerForNode(node);
      if (!backWalker) {
          return;
      }
      const finishedSubtrees = new Set();
      let backNode = fragments.internal.backwardTraverse(backWalker, finishedSubtrees);
      while (backNode != null && !isBlock(backNode)) {
          checkTimeout();
          if (backNode.nodeType === Node.TEXT_NODE) {
              preNodes.push(backNode);
          }
          backNode =
              fragments.internal.backwardTraverse(backWalker, finishedSubtrees);
      }
      ;
      preNodes.reverse();
      const innerNodes = [];
      if (node.nodeType === Node.TEXT_NODE) {
          innerNodes.push(node);
      }
      else {
          const walker = document.createTreeWalker(node, NodeFilter.SHOW_ELEMENT | NodeFilter.SHOW_TEXT, (node) => {
              return fragments.internal.acceptNodeIfVisibleInRange(node);
          });
          walker.currentNode = node;
          let child = walker.nextNode();
          while (child != null) {
              checkTimeout();
              if (child.nodeType === Node.TEXT_NODE) {
                  innerNodes.push(child);
              }
              child = walker.nextNode();
          }
      }
      const postNodes = [];
      const forwardWalker = makeWalkerForNode(node);
      if (!forwardWalker) {
          return;
      }

      const finishedSubtreesForward = new Set([node]);
      let forwardNode = fragments.internal.forwardTraverse(forwardWalker, finishedSubtreesForward);
      while (forwardNode != null && !isBlock(forwardNode)) {
          checkTimeout();
          if (forwardNode.nodeType === Node.TEXT_NODE) {
              postNodes.push(forwardNode);
          }
          forwardNode = fragments.internal.forwardTraverse(forwardWalker, finishedSubtreesForward);
      }
      return { preNodes: preNodes, innerNodes: innerNodes, postNodes: postNodes };
  };

  const expandRangeEndToWordBound = (range) => {
      const segmenter = fragments.internal.makeNewSegmenter();
      if (segmenter) {

          const endNode = getLastNodeForBlockSearch(range);
          if (endNode !== range.endContainer) {
              range.setEndAfter(endNode);
          }
          expandToNearestWordBoundaryPointUsingSegments(segmenter,  true, range);
      }
      else {
          let initialOffset = range.endOffset;
          let node = range.endContainer;
          if (node.nodeType === Node.ELEMENT_NODE) {
              if (range.endOffset < node.childNodes.length) {
                  node = node.childNodes[range.endOffset];
              }
          }
          const walker = makeWalkerForNode(node);
          if (!walker) {
              return;
          }

          const finishedSubtrees = new Set([node]);
          while (node != null) {
              checkTimeout();
              const newOffset = findWordEndBoundInTextNode(node, initialOffset);

              initialOffset = null;
              if (newOffset !== -1) {
                  range.setEnd(node, newOffset);
                  return;
              }

              if (isBlock(node)) {
                  if (node.contains(range.endContainer)) {

                      range.setEnd(node, node.childNodes.length);
                  }
                  else {

                      range.setEndBefore(node);
                  }
                  return;
              }
              node = fragments.internal.forwardTraverse(walker, finishedSubtrees);
          }

          range.collapse();
      }
  };

  const isBlock = (node) => {
      return node.nodeType === Node.ELEMENT_NODE &&
          (fragments.internal.BLOCK_ELEMENTS.includes(node.tagName) ||
              node.tagName === 'HTML' || node.tagName === 'BODY');
  };

  const isText = (node) => {
      return node.nodeType === Node.TEXT_NODE;
  };
  const forTesting = {
      containsBlockBoundary: containsBlockBoundary,
      doGenerateFragment: doGenerateFragment,
      expandRangeEndToWordBound: expandRangeEndToWordBound,
      expandRangeStartToWordBound: expandRangeStartToWordBound,
      findWordEndBoundInTextNode: findWordEndBoundInTextNode,
      findWordStartBoundInTextNode: findWordStartBoundInTextNode,
      FragmentFactory: FragmentFactory,
      getSearchSpaceForEnd: getSearchSpaceForEnd,
      getSearchSpaceForStart: getSearchSpaceForStart,
      getTextNodesInSameBlock: getTextNodesInSameBlock,
      recordStartTime: recordStartTime,
      BlockTextAccumulator: BlockTextAccumulator,
      getFirstTextNode: getFirstTextNode,
      getLastTextNode: getLastTextNode,
      moveRangeEdgesToTextNodes: moveRangeEdgesToTextNodes
  };

  if (typeof goog !== 'undefined') {

      goog.declareModuleId('googleChromeLabs.textFragmentPolyfill.fragmentGenerationUtils');

  }
}

// ios/chrome/browser/link_to_text/model/tsc/ios/chrome/browser/link_to_text/model/resources/link_to_text.js
if (typeof _injected_link_to_text === 'undefined') {
    var _injected_link_to_text = true;
   
  function getLinkToText() {
      const selection = window.getSelection();
      let selectionRect = { x: 0, y: 0, width: 0, height: 0 };
      if (selection && selection.rangeCount) {
          // Get the selection range's first client rect.
          const domRect = selection.getRangeAt(0).getClientRects()[0];
          if (domRect) {
              selectionRect.x = domRect.x;
              selectionRect.y = domRect.y;
              selectionRect.width = domRect.width;
              selectionRect.height = domRect.height;
          }
      }
      const selectedText = `"${selection?.toString()}"`;
      const canonicalLinkNode = document.querySelector('link[rel=\'canonical\']');
      const response = utils.generateFragment(selection);
      return {
          status: response.status,
          fragment: response.fragment,
          selectedText: selectedText,
          selectionRect: selectionRect,
          canonicalUrl: canonicalLinkNode
              && canonicalLinkNode.getAttribute('href')
      };
  }
  
  gCrWeb.linkToText = { getLinkToText };
}


// components/language/ios/browser/tsc/components/language/ios/browser/resources/language_detection.js

if (typeof _injected_language_detection === 'undefined') {
    var _injected_language_detection = true;
    
  let bufferedTextContent;

  let activeRequests = 0;

  function hasNoTranslate() {
      for (const metaTag of document.getElementsByTagName('meta')) {
          if (metaTag.name === 'google') {
              if (metaTag.content === 'notranslate' ||
                  metaTag.getAttribute('value') === 'notranslate') {
                  return true;
              }
          }
      }
      return false;
  }

  function getMetaContentByHttpEquiv(httpEquiv) {
      for (const metaTag of document.getElementsByTagName('meta')) {
          if (metaTag.httpEquiv.toLowerCase() === httpEquiv) {
              return metaTag.content;
          }
      }
      return '';
  }

  const NON_TEXT_NODE_NAMES = new Set([
      'EMBED',
      'NOSCRIPT',
      'OBJECT',
      'SCRIPT',
      'STYLE',
  ]);

  function getTextContent(node, maxLen) {
      if (!node || maxLen <= 0) {
          return '';
      }
      let txt = '';

      if (node.nodeType === Node.ELEMENT_NODE && node instanceof Element) {

          if (NON_TEXT_NODE_NAMES.has(node.nodeName)) {
              return '';
          }
          if (node.nodeName === 'BR') {
              return '\n';
          }
          const style = window.getComputedStyle(node);

          if (style.display === 'none' || style.visibility === 'hidden') {
              return '';
          }

          if (node.nodeName.toUpperCase() !== 'BODY' && style.display !== 'inline') {
              txt = '\n';
          }
      }
      if (node.hasChildNodes()) {
          for (const childNode of node.childNodes) {
              txt += getTextContent(childNode, maxLen - txt.length);
              if (txt.length >= maxLen) {
                  break;
              }
          }
      }
      else if (node.nodeType === Node.TEXT_NODE && node.textContent) {
          txt += node.textContent.substring(0, maxLen - txt.length);
      }
      return txt;
  }

  function detectLanguage() {

      const kMaxIndexChars = 65535;
      activeRequests += 1;
      bufferedTextContent = getTextContent(document.body, kMaxIndexChars);
      const httpContentLanguage = getMetaContentByHttpEquiv('content-language');
      const textCapturedCommand = {
          'hasNoTranslate': false,
          'htmlLang': document.documentElement.lang,
          'httpContentLanguage': httpContentLanguage,
          'frameId': gCrWeb.message.getFrameId(),
      };
      if (hasNoTranslate()) {
          textCapturedCommand['hasNoTranslate'] = true;
      }
      sendWebKitMessage('LanguageDetectionTextCaptured', textCapturedCommand);
  }

  function retrieveBufferedTextContent() {
      const textContent = bufferedTextContent;
      activeRequests -= 1;
      if (activeRequests === 0) {
          bufferedTextContent = null;
      }
      return textContent;
  }

  gCrWeb.languageDetection = {
      detectLanguage,
      retrieveBufferedTextContent,
  };
}

// components/translate/core/browser/resources/translate.js
if (typeof _injected_translate === 'undefined') {
  var _injected_translate = true
  
  var cr = cr || {};

  cr.googleTranslate = (function() {

    let lib;

    let libReady = false;

    const ERROR = {
      'NONE': 0,
      'INITIALIZATION_ERROR': 2,
      'UNSUPPORTED_LANGUAGE': 4,
      'TRANSLATION_ERROR': 6,
      'TRANSLATION_TIMEOUT': 7,
      'UNEXPECTED_SCRIPT_ERROR': 8,
      'BAD_ORIGIN': 9,
      'SCRIPT_LOAD_ERROR': 10,
    };

    const TRANSLATE_ERROR_TO_ERROR_CODE_MAP = {
      0: ERROR['NONE'],
      1: ERROR['TRANSLATION_ERROR'],
      2: ERROR['UNSUPPORTED_LANGUAGE'],
    };

    let errorCode = ERROR['NONE'];

    let finished = false;

    let checkReadyCount = 0;

    const injectedTime = performance.now();

    let loadedTime = 0.0;

    let readyTime = 0.0;

    let startTime = 0.0;

    let endTime = 0.0;

    let readyCallback;

    let resultCallback;

    function checkLibReady() {
      if (lib.isAvailable()) {
        readyTime = performance.now();
        libReady = true;
        invokeReadyCallback();
        return;
      }
      if (checkReadyCount++ > 5) {
        errorCode = ERROR['TRANSLATION_TIMEOUT'];
        invokeReadyCallback();
        return;
      }
      setTimeout(checkLibReady, 100);
    }

    function onTranslateProgress(progress, opt_finished, opt_error) {
      finished = opt_finished;

      if (typeof opt_error === 'boolean' && opt_error) {

        errorCode = ERROR['TRANSLATION_ERROR'];

        lib.restore();
        invokeResultCallback();
      } else if (typeof opt_error === 'number' && opt_error !== 0) {
        errorCode = TRANSLATE_ERROR_TO_ERROR_CODE_MAP[opt_error];
        lib.restore();
        invokeResultCallback();
      }

      if (finished) {
        endTime = performance.now();
        invokeResultCallback();
      }
    }

    function invokeReadyCallback() {
      if (readyCallback) {
        readyCallback();
        readyCallback = null;
      }
    }

    function invokeResultCallback() {
      if (resultCallback) {
        resultCallback();
        resultCallback = null;
      }
    }

    window.addEventListener('pagehide', function(event) {
      if (libReady && event.persisted) {
        lib.restore();
      }
    });

    return {

      set readyCallback(callback) {
        if (!readyCallback) {
          readyCallback = callback;
        }
      },

      set resultCallback(callback) {
        if (!resultCallback) {
          resultCallback = callback;
        }
      },

      get libReady() {
        return libReady;
      },

      get finished() {
        return finished;
      },

      get error() {
        return errorCode !== ERROR['NONE'];
      },

      get errorCode() {
        return errorCode;
      },

      get sourceLang() {
        if (!libReady || !finished || errorCode !== ERROR['NONE']) {
          return '';
        }
        if (!lib.getDetectedLanguage) {
          return 'und';
        }
        return lib.getDetectedLanguage();
      },

      get loadTime() {
        if (loadedTime === 0) {
          return 0;
        }
        return loadedTime - injectedTime;
      },

      get readyTime() {
        if (!libReady) {
          return 0;
        }
        return readyTime - injectedTime;
      },

      get translationTime() {
        if (!finished) {
          return 0;
        }
        return endTime - startTime;
      },

      translate(sourceLang, targetLang) {
        finished = false;
        errorCode = ERROR['NONE'];
        if (!libReady) {
          return false;
        }
        startTime = performance.now();
        try {
          lib.translatePage(sourceLang, targetLang, onTranslateProgress);
        } catch (err) {
          console.error('Translate: ' + err);
          errorCode = ERROR['UNEXPECTED_SCRIPT_ERROR'];
          invokeResultCallback();
          return false;
        }
        return true;
      },

      revert() {
        lib.restore();
      },

      onTranslateElementError(error) {
        errorCode = ERROR['UNEXPECTED_SCRIPT_ERROR'];
        invokeReadyCallback();
      },

      onTranslateElementLoad() {
        loadedTime = performance.now();
        try {
          lib = google.translate.TranslateService({

            'key': translateApiKey,
            'serverParams': serverParams,
            'timeInfo': gtTimeInfo,
            'useSecureConnection': true,
          });
          translateApiKey = undefined;
          serverParams = undefined;
          gtTimeInfo = undefined;
        } catch (err) {
          errorCode = ERROR['INITIALIZATION_ERROR'];
          translateApiKey = undefined;
          serverParams = undefined;
          gtTimeInfo = undefined;
          invokeReadyCallback();
          return;
        }

        checkLibReady();
      },

      onLoadCSS(url) {
        const element = document.createElement('link');
        element.type = 'text/css';
        element.rel = 'stylesheet';
        element.charset = 'UTF-8';
        element.href = url;
        document.head.appendChild(element);
      },

      onLoadJavascript(url) {

        if (!url.startsWith(securityOrigin)) {
          console.error('Translate: ' + url + ' is not allowed to load.');
          errorCode = ERROR['BAD_ORIGIN'];
          return;
        }

        const xhr = new XMLHttpRequest();
        xhr.open('GET', url, true);
        xhr.onreadystatechange = function() {
          if (this.readyState !== this.DONE) {
            return;
          }
          if (this.status !== 200) {
            errorCode = ERROR['SCRIPT_LOAD_ERROR'];
            return;
          }

          new Function(this.responseText).call(window);
        };
        xhr.send();
      },
    };
  })();
}





// components/translate/ios/browser/tsc/components/translate/ios/browser/resources/translate_ios.js
if (typeof _injected_translate_ios === 'undefined') {
    var _injected_translate_ios = true;
  
  function installCallbacks() {
      cr.googleTranslate.readyCallback = function () {
          sendWebKitMessage('TranslateMessage', {
              'command': 'ready',
              'errorCode': cr.googleTranslate.errorCode,
              'loadTime': cr.googleTranslate.loadTime,
              'readyTime': cr.googleTranslate.readyTime,
          });
      };

      cr.googleTranslate.resultCallback = function () {
          sendWebKitMessage('TranslateMessage', {
              'command': 'status',
              'errorCode': cr.googleTranslate.errorCode,
              'pageSourceLanguage': cr.googleTranslate.sourceLang,
              'translationTime': cr.googleTranslate.translationTime,
          });
      };
  }
  
  function startTranslation(sourceLanguage, targetLanguage) {
      cr.googleTranslate.translate(sourceLanguage, targetLanguage);
  }
  
  function revertTranslation() {
      try {
          cr.googleTranslate.revert();
      }
      catch {

      }
  }

  gCrWeb.translate = {
      installCallbacks,
      startTranslation,
      revertTranslation,
  };
}
  
  
// components/translate/core/browser/translate_script.cc;l=149
try {
    __gCrWeb.translate.installCallbacks();
} catch (error) {
  cr.googleTranslate.onTranslateElementError(error);
}


// brave-core/components/translate/core/browser/resources/brave_translate.js

try {
  const useGoogleTranslateEndpoint = false;
  const braveTranslateStaticPath = '/static/v1/';
  // securityOrigin is predefined by translate_script.cc.
  const securityOriginHost = new URL(securityOrigin).host;

  // A method to rewrite URL in the scripts:
  // 1. change the domain to translate.brave.com;
  // 2. adjust static paths to use braveTranslateStaticPath.
  const rewriteUrl = (url)=>{
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
              new_url.pathname = new_url.pathname.replace('/translate_static/', braveTranslateStaticPath);
          }
          new_url.host = securityOriginHost;
          return new_url.toString();
      } catch {
          return url;
      }
  }
  ;

  const emptySvgDataUrl = 'data:image/svg+xml;base64,' + btoa('<svg xmlns="http://www.w3.org/2000/svg"/>');

  // Make replacements in loading .js files.
  function processJavascript(text) {
      // Replace gen204 telemetry requests with loading an empty svg.
      text = text.replaceAll('"//"+po+"/gen204?"+Bo(b)', '"' + emptySvgDataUrl + '"');

      // Used in the injected elements, that are currently not visible. Replace it
      // to hide the loading error in devtools (because of CSP).
      text = text.replaceAll('https://www.gstatic.com/images/branding/product/1x/translate_24dp.png', emptySvgDataUrl);
      return text;
  }

  // Make replacements in loading .css files.
  function processCSS(text) {
      // Used in the injected elements, that are currently not visible. Replace it
      // to hide the loading error in devtools (because of CSP).
      text = text.replaceAll('//www.gstatic.com/images/branding/product/2x/translate_24dp.png', emptySvgDataUrl);
      return text;
  }

  // Used to rewrite urls for XHRs in the translate isolated world
  // (primarily for /translate_a/t).
  if (window.__firefox__.$<brave_translate_script>.useNativeNetworking) {
    const methodProperty = Symbol('method')
    const urlProperty = Symbol('url')
    const userProperty = Symbol('user')
    const passwordProperty = Symbol('password')
    const requestHeadersProperty = Symbol('requestHeaders')
    
    XMLHttpRequest.prototype.getResponseHeader = function(headerName) {
      return this[requestHeadersProperty][headerName];
    };
    
    XMLHttpRequest.prototype.getAllResponseHeaders = function() {
      return this[requestHeadersProperty];
    };
    
    XMLHttpRequest.prototype.realSetRequestHeader = XMLHttpRequest.prototype.setRequestHeader;
    XMLHttpRequest.prototype.setRequestHeader = function(header, value) {
      if (!this[requestHeadersProperty]) {
        this[requestHeadersProperty] = {};
      }
      
      this[requestHeadersProperty][header] = value;
      this.realSetRequestHeader(header, value);
    };
    
    XMLHttpRequest.prototype.realOverrideMimeType = XMLHttpRequest.prototype.overrideMimeType;
    XMLHttpRequest.prototype.overrideMimeType = function(mimeType) {
      this.realOverrideMimeType(mimeType);
    }
    
    XMLHttpRequest.prototype.realOpen = XMLHttpRequest.prototype.open;
    XMLHttpRequest.prototype.open = function(method, url, isAsync=true, user='', password='') {
      if (isAsync !== undefined && !isAsync) {
        return this.realOpen(method, rewriteUrl(url), isAsync, user, password);
      }

      this[methodProperty] = method;
      this[urlProperty] = rewriteUrl(url);
      this[userProperty] = user;
      this[passwordProperty] = password;
      return this.realOpen(method, rewriteUrl(url), isAsync, user, password);
    };

    XMLHttpRequest.prototype.realSend = XMLHttpRequest.prototype.send;
    XMLHttpRequest.prototype.send = function(body) {
      if (this[urlProperty] === undefined) {
        return this.realSend(body);
      }
      
      try {
        var t = window.webkit;
        delete window["webkit"];
        
        window.webkit.messageHandlers["TranslateMessage"].postMessage({
          "command": "request",
          "method": this[methodProperty] ?? "GET",
          "url": this[urlProperty],
          "user": this[userProperty] ?? "",
          "password": this[passwordProperty] ?? "",
          "headers": this[requestHeadersProperty] ?? {},
          "body": body ?? ""
        }).then((result) => {
          
          Object.defineProperties(this, {
            readyState: { value: XMLHttpRequest.DONE }  // DONE (4)
          });
          
          if (result.value) {
            Object.defineProperties(this, {
              status: { value: result.value.statusCode }
            });
            
            Object.defineProperties(this, {
              statusText: { value: "OK" }
            });
            
            Object.defineProperties(this, {
              responseType: { value: result.value.responseType }
            });
            
            Object.defineProperty(this, 'response', { writable: true });
            Object.defineProperty(this, 'responseText', { writable: true });
            this.responseText = result.value.response;
            
            switch (result.value.responseType) {
              case "arraybuffer": this.response = new ArrayBuffer(result.value.response);
              case "json": this.response = JSON.parse(result.value.response);
              case "text": this.response = result.value.response;
              case "": this.response = result.value.response;
            }
          }
          
          this.dispatchEvent(new ProgressEvent('loadstart'));
          this.dispatchEvent(new ProgressEvent(result.error ? 'error' : 'load'));
          this.dispatchEvent(new ProgressEvent('readystatechange'));
          this.dispatchEvent(new ProgressEvent('loadend'));
        });
        
        window.webkit = t
      } catch (e) {
        return this.realSend(body);
      }
    };
  } else {
    if (typeof XMLHttpRequest.prototype.realOpen === 'undefined') {
      XMLHttpRequest.prototype.realOpen = XMLHttpRequest.prototype.open;
      XMLHttpRequest.prototype.open = function(method, url, async=true, user='', password='') {
        this.realOpen(method, rewriteUrl(url), async, user, password);
      }
    }
  }
  ;
  // An overridden version of onLoadJavascript from translate.js, that fetches
  // and evaluates secondary scripts (i.e. main.js).
  // The differences:
  // 1. change url via rewriteUrl();
  // 2. process the loaded code via processJavascript().
  cr.googleTranslate.onLoadJavascript = function(url) {
      const xhr = new XMLHttpRequest();
      xhr.open('GET', rewriteUrl(url), true);
      xhr.onreadystatechange = function() {
          if (this.readyState !== this.DONE) {
              return;
          }
          if (this.status !== 200) {
              errorCode = ERROR['SCRIPT_LOAD_ERROR'];
              return;
          }

          // nosemgrep
          new Function(processJavascript(this.responseText)).call(window);
      }
      ;
      xhr.send();
  }
  ;

  // The styles to hide root elements that are injected by the scripts in the DOM.
  // Currently they are always invisible. The styles are added in case of changes
  // in future versions.
  const braveExtraStyles = `.goog-te-spinner-pos, #goog-gt-tt {display: none;}`

  // An overridden version of onLoadCSS from translate.js.
  // The differences:
  // 1. change url via rewriteUrl();
  // 2. process the loaded styles via processCSS().
  // 3. Add braveExtraStyles in the end.
  cr.googleTranslate.onLoadCSS = function(url) {
      const xhr = new XMLHttpRequest();
      xhr.open('GET', rewriteUrl(url), true);
      xhr.onreadystatechange = function() {
          if (this.readyState !== this.DONE || this.status !== 200) {
              return;
          }

          const element = document.createElement('style');
          element.type = 'text/css';
          element.charset = 'UTF-8';
          element.innerText = processCSS(this.responseText) + braveExtraStyles;
          document.head.appendChild(element);
      }
      ;
      xhr.send();
  };
} catch(error) {
  cr.googleTranslate.onTranslateElementError(error);
}


// Brave Translate

try {
    var oldWebkit = window.webkit;
    delete window['webkit'];
    window.webkit.messageHandlers["$<message_handler>"].postMessage({
      "command": "load_brave_translate_script"
    }).then((script) => {
      try {
        new Function(script).call(this);
      } catch (error) {
        cr.googleTranslate.onTranslateElementError(error);
      }
    });
    window.webkit = oldWebkit;
} catch (error) {
  console.error(error);
}
