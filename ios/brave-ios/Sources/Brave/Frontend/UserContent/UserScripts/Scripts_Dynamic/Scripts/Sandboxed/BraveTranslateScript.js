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



// go/mss-setup#7-load-the-js-or-css-from-your-initial-page
if (!window['_DumpException']) {
    const _DumpException = window['_DumpException'] || function(error) {
        throw error;
    };
    window['_DumpException'] = _DumpException;
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







// BRAVE TRANSLATE https://translate.brave.com/static/v1/js/element/main.js

try {
  "use strict";
  this.default_tr = this.default_tr || {};
  (function(_) {
      var window = this;
      try {
          var ba, ea, ja, qa, xa, Aa, Ea, Ha, Ia, Ja, Ma, Na, Oa, Pa, Qa, Ra, Ua, Va, Za;
          _.aa = function(a, b) {
              if (Error.captureStackTrace)
                  Error.captureStackTrace(this, _.aa);
              else {
                  var c = Error().stack;
                  c && (this.stack = c)
              }
              a && (this.message = String(a));
              void 0 !== b && (this.cause = b)
          }
          ;
          ba = function(a) {
              _.p.setTimeout(function() {
                  throw a;
              }, 0)
          }
          ;
          _.ca = function(a) {
              a && "function" == typeof a.O && a.O()
          }
          ;
          ea = function(a) {
              for (var b = 0, c = arguments.length; b < c; ++b) {
                  var d = arguments[b];
                  _.da(d) ? ea.apply(null, d) : _.ca(d)
              }
          }
          ;
          ja = function() {
              !_.fa && _.ha && _.ia();
              return _.fa
          }
          ;
          _.ia = function() {
              _.fa = (0,
              _.ha)();
              ka.forEach(function(a) {
                  a(_.fa)
              });
              ka = []
          }
          ;
          _.ma = function(a) {
              _.fa && la(a)
          }
          ;
          _.oa = function() {
              _.fa && na(_.fa)
          }
          ;
          qa = function(a, b) {
              b.hasOwnProperty("displayName") || (b.displayName = a);
              b[pa] = a
          }
          ;
          _.ra = function(a) {
              return a[a.length - 1]
          }
          ;
          _.ta = function(a, b) {
              return 0 <= sa(a, b)
          }
          ;
          _.ua = function(a, b) {
              _.ta(a, b) || a.push(b)
          }
          ;
          _.va = function(a, b) {
              b = sa(a, b);
              var c;
              (c = 0 <= b) && Array.prototype.splice.call(a, b, 1);
              return c
          }
          ;
          _.wa = function(a) {
              var b = a.length;
              if (0 < b) {
                  for (var c = Array(b), d = 0; d < b; d++)
                      c[d] = a[d];
                  return c
              }
              return []
          }
          ;
          xa = function(a, b) {
              for (var c = 1; c < arguments.length; c++) {
                  var d = arguments[c];
                  if (_.da(d)) {
                      var e = a.length || 0
                        , f = d.length || 0;
                      a.length = e + f;
                      for (var g = 0; g < f; g++)
                          a[e + g] = d[g]
                  } else
                      a.push(d)
              }
          }
          ;
          Aa = function(a, b) {
              b = b || a;
              for (var c = 0, d = 0, e = {}; d < a.length; ) {
                  var f = a[d++]
                    , g = _.ya(f) ? "o" + _.za(f) : (typeof f).charAt(0) + f;
                  Object.prototype.hasOwnProperty.call(e, g) || (e[g] = !0,
                  b[c++] = f)
              }
              b.length = c
          }
          ;
          _.Ba = function() {
              var a = _.p.navigator;
              return a && (a = a.userAgent) ? a : ""
          }
          ;
          _.t = function(a) {
              return -1 != _.Ba().indexOf(a)
          }
          ;
          _.Ca = function() {
              return _.t("iPhone") && !_.t("iPod") && !_.t("iPad")
          }
          ;
          _.Da = function() {
              return _.Ca() || _.t("iPad") || _.t("iPod")
          }
          ;
          Ea = function(a, b) {
              for (var c in a)
                  if (b.call(void 0, a[c], c, a))
                      return !0;
              return !1
          }
          ;
          _.Fa = function(a) {
              var b = [], c = 0, d;
              for (d in a)
                  b[c++] = a[d];
              return b
          }
          ;
          Ha = function(a, b) {
              for (var c, d, e = 1; e < arguments.length; e++) {
                  d = arguments[e];
                  for (c in d)
                      a[c] = d[c];
                  for (var f = 0; f < Ga.length; f++)
                      c = Ga[f],
                      Object.prototype.hasOwnProperty.call(d, c) && (a[c] = d[c])
              }
          }
          ;
          Ia = function(a) {
              var b = arguments.length;
              if (1 == b && Array.isArray(arguments[0]))
                  return Ia.apply(null, arguments[0]);
              for (var c = {}, d = 0; d < b; d++)
                  c[arguments[d]] = !0;
              return c
          }
          ;
          Ja = function() {}
          ;
          _.La = function(a, b) {
              a.src = _.Ka(b);
              var c, d;
              (c = (b = null == (d = (c = (a.ownerDocument && a.ownerDocument.defaultView || window).document).querySelector) ? void 0 : d.call(c, "script[nonce]")) ? b.nonce || b.getAttribute("nonce") || "" : "") && a.setAttribute("nonce", c)
          }
          ;
          Ma = function(a) {
              var b = 0;
              return function() {
                  return b < a.length ? {
                      done: !1,
                      value: a[b++]
                  } : {
                      done: !0
                  }
              }
          }
          ;
          Na = "function" == typeof Object.defineProperties ? Object.defineProperty : function(a, b, c) {
              if (a == Array.prototype || a == Object.prototype)
                  return a;
              a[b] = c.value;
              return a
          }
          ;
          Oa = function(a) {
              a = ["object" == typeof globalThis && globalThis, a, "object" == typeof window && window, "object" == typeof self && self, "object" == typeof global && global];
              for (var b = 0; b < a.length; ++b) {
                  var c = a[b];
                  if (c && c.Math == Math)
                      return c
              }
              throw Error("a");
          }
          ;
          Pa = Oa(this);
          Qa = function(a, b) {
              if (b)
                  a: {
                      var c = Pa;
                      a = a.split(".");
                      for (var d = 0; d < a.length - 1; d++) {
                          var e = a[d];
                          if (!(e in c))
                              break a;
                          c = c[e]
                      }
                      a = a[a.length - 1];
                      d = c[a];
                      b = b(d);
                      b != d && null != b && Na(c, a, {
                          configurable: !0,
                          writable: !0,
                          value: b
                      })
                  }
          }
          ;
          Qa("Symbol", function(a) {
              if (a)
                  return a;
              var b = function(f, g) {
                  this.g = f;
                  Na(this, "description", {
                      configurable: !0,
                      writable: !0,
                      value: g
                  })
              };
              b.prototype.toString = function() {
                  return this.g
              }
              ;
              var c = "jscomp_symbol_" + (1E9 * Math.random() >>> 0) + "_"
                , d = 0
                , e = function(f) {
                  if (this instanceof e)
                      throw new TypeError("b");
                  return new b(c + (f || "") + "_" + d++,f)
              };
              return e
          });
          Qa("Symbol.iterator", function(a) {
              if (a)
                  return a;
              a = Symbol("c");
              for (var b = "Array Int8Array Uint8Array Uint8ClampedArray Int16Array Uint16Array Int32Array Uint32Array Float32Array Float64Array".split(" "), c = 0; c < b.length; c++) {
                  var d = Pa[b[c]];
                  "function" === typeof d && "function" != typeof d.prototype[a] && Na(d.prototype, a, {
                      configurable: !0,
                      writable: !0,
                      value: function() {
                          return Ra(Ma(this))
                      }
                  })
              }
              return a
          });
          Ra = function(a) {
              a = {
                  next: a
              };
              a[Symbol.iterator] = function() {
                  return this
              }
              ;
              return a
          }
          ;
          _.Sa = function(a) {
              var b = "undefined" != typeof Symbol && Symbol.iterator && a[Symbol.iterator];
              return b ? b.call(a) : {
                  next: Ma(a)
              }
          }
          ;
          _.Ta = function(a) {
              for (var b, c = []; !(b = a.next()).done; )
                  c.push(b.value);
              return c
          }
          ;
          Ua = "function" == typeof Object.create ? Object.create : function(a) {
              var b = function() {};
              b.prototype = a;
              return new b
          }
          ;
          if ("function" == typeof Object.setPrototypeOf)
              Va = Object.setPrototypeOf;
          else {
              var Wa;
              a: {
                  var Xa = {
                      a: !0
                  }
                    , Ya = {};
                  try {
                      Ya.__proto__ = Xa;
                      Wa = Ya.a;
                      break a
                  } catch (a) {}
                  Wa = !1
              }
              Va = Wa ? function(a, b) {
                  a.__proto__ = b;
                  if (a.__proto__ !== b)
                      throw new TypeError("d`" + a);
                  return a
              }
              : null
          }
          Za = Va;
          _.u = function(a, b) {
              a.prototype = Ua(b.prototype);
              a.prototype.constructor = a;
              if (Za)
                  Za(a, b);
              else
                  for (var c in b)
                      if ("prototype" != c)
                          if (Object.defineProperties) {
                              var d = Object.getOwnPropertyDescriptor(b, c);
                              d && Object.defineProperty(a, c, d)
                          } else
                              a[c] = b[c];
              a.G = b.prototype
          }
          ;
          _.$a = function() {
              for (var a = Number(this), b = [], c = a; c < arguments.length; c++)
                  b[c - a] = arguments[c];
              return b
          }
          ;
          Qa("Promise", function(a) {
              function b() {
                  this.g = null
              }
              function c(g) {
                  return g instanceof e ? g : new e(function(k) {
                      k(g)
                  }
                  )
              }
              if (a)
                  return a;
              b.prototype.h = function(g) {
                  if (null == this.g) {
                      this.g = [];
                      var k = this;
                      this.j(function() {
                          k.o()
                      })
                  }
                  this.g.push(g)
              }
              ;
              var d = Pa.setTimeout;
              b.prototype.j = function(g) {
                  d(g, 0)
              }
              ;
              b.prototype.o = function() {
                  for (; this.g && this.g.length; ) {
                      var g = this.g;
                      this.g = [];
                      for (var k = 0; k < g.length; ++k) {
                          var l = g[k];
                          g[k] = null;
                          try {
                              l()
                          } catch (m) {
                              this.l(m)
                          }
                      }
                  }
                  this.g = null
              }
              ;
              b.prototype.l = function(g) {
                  this.j(function() {
                      throw g;
                  })
              }
              ;
              var e = function(g) {
                  this.g = 0;
                  this.j = void 0;
                  this.h = [];
                  this.C = !1;
                  var k = this.l();
                  try {
                      g(k.resolve, k.reject)
                  } catch (l) {
                      k.reject(l)
                  }
              };
              e.prototype.l = function() {
                  function g(m) {
                      return function(n) {
                          l || (l = !0,
                          m.call(k, n))
                      }
                  }
                  var k = this
                    , l = !1;
                  return {
                      resolve: g(this.S),
                      reject: g(this.o)
                  }
              }
              ;
              e.prototype.S = function(g) {
                  if (g === this)
                      this.o(new TypeError("g"));
                  else if (g instanceof e)
                      this.ma(g);
                  else {
                      a: switch (typeof g) {
                      case "object":
                          var k = null != g;
                          break a;
                      case "function":
                          k = !0;
                          break a;
                      default:
                          k = !1
                      }
                      k ? this.K(g) : this.s(g)
                  }
              }
              ;
              e.prototype.K = function(g) {
                  var k = void 0;
                  try {
                      k = g.then
                  } catch (l) {
                      this.o(l);
                      return
                  }
                  "function" == typeof k ? this.W(k, g) : this.s(g)
              }
              ;
              e.prototype.o = function(g) {
                  this.B(2, g)
              }
              ;
              e.prototype.s = function(g) {
                  this.B(1, g)
              }
              ;
              e.prototype.B = function(g, k) {
                  if (0 != this.g)
                      throw Error("h`" + g + "`" + k + "`" + this.g);
                  this.g = g;
                  this.j = k;
                  2 === this.g && this.U();
                  this.J()
              }
              ;
              e.prototype.U = function() {
                  var g = this;
                  d(function() {
                      if (g.N()) {
                          var k = Pa.console;
                          "undefined" !== typeof k && k.error(g.j)
                      }
                  }, 1)
              }
              ;
              e.prototype.N = function() {
                  if (this.C)
                      return !1;
                  var g = Pa.CustomEvent
                    , k = Pa.Event
                    , l = Pa.dispatchEvent;
                  if ("undefined" === typeof l)
                      return !0;
                  "function" === typeof g ? g = new g("unhandledrejection",{
                      cancelable: !0
                  }) : "function" === typeof k ? g = new k("unhandledrejection",{
                      cancelable: !0
                  }) : (g = Pa.document.createEvent("CustomEvent"),
                  g.initCustomEvent("unhandledrejection", !1, !0, g));
                  g.promise = this;
                  g.reason = this.j;
                  return l(g)
              }
              ;
              e.prototype.J = function() {
                  if (null != this.h) {
                      for (var g = 0; g < this.h.length; ++g)
                          f.h(this.h[g]);
                      this.h = null
                  }
              }
              ;
              var f = new b;
              e.prototype.ma = function(g) {
                  var k = this.l();
                  g.Qd(k.resolve, k.reject)
              }
              ;
              e.prototype.W = function(g, k) {
                  var l = this.l();
                  try {
                      g.call(k, l.resolve, l.reject)
                  } catch (m) {
                      l.reject(m)
                  }
              }
              ;
              e.prototype.then = function(g, k) {
                  function l(r, A) {
                      return "function" == typeof r ? function(G) {
                          try {
                              m(r(G))
                          } catch (Q) {
                              n(Q)
                          }
                      }
                      : A
                  }
                  var m, n, q = new e(function(r, A) {
                      m = r;
                      n = A
                  }
                  );
                  this.Qd(l(g, m), l(k, n));
                  return q
              }
              ;
              e.prototype.catch = function(g) {
                  return this.then(void 0, g)
              }
              ;
              e.prototype.Qd = function(g, k) {
                  function l() {
                      switch (m.g) {
                      case 1:
                          g(m.j);
                          break;
                      case 2:
                          k(m.j);
                          break;
                      default:
                          throw Error("i`" + m.g);
                      }
                  }
                  var m = this;
                  null == this.h ? f.h(l) : this.h.push(l);
                  this.C = !0
              }
              ;
              e.resolve = c;
              e.reject = function(g) {
                  return new e(function(k, l) {
                      l(g)
                  }
                  )
              }
              ;
              e.race = function(g) {
                  return new e(function(k, l) {
                      for (var m = _.Sa(g), n = m.next(); !n.done; n = m.next())
                          c(n.value).Qd(k, l)
                  }
                  )
              }
              ;
              e.all = function(g) {
                  var k = _.Sa(g)
                    , l = k.next();
                  return l.done ? c([]) : new e(function(m, n) {
                      function q(G) {
                          return function(Q) {
                              r[G] = Q;
                              A--;
                              0 == A && m(r)
                          }
                      }
                      var r = []
                        , A = 0;
                      do
                          r.push(void 0),
                          A++,
                          c(l.value).Qd(q(r.length - 1), n),
                          l = k.next();
                      while (!l.done)
                  }
                  )
              }
              ;
              return e
          });
          var ab = function(a, b, c) {
              if (null == a)
                  throw new TypeError("j`" + c);
              if (b instanceof RegExp)
                  throw new TypeError("k`" + c);
              return a + ""
          };
          Qa("String.prototype.startsWith", function(a) {
              return a ? a : function(b, c) {
                  var d = ab(this, b, "startsWith")
                    , e = d.length
                    , f = b.length;
                  c = Math.max(0, Math.min(c | 0, d.length));
                  for (var g = 0; g < f && c < e; )
                      if (d[c++] != b[g++])
                          return !1;
                  return g >= f
              }
          });
          var bb = function(a, b) {
              return Object.prototype.hasOwnProperty.call(a, b)
          };
          Qa("WeakMap", function(a) {
              function b() {}
              function c(l) {
                  var m = typeof l;
                  return "object" === m && null !== l || "function" === m
              }
              function d(l) {
                  if (!bb(l, f)) {
                      var m = new b;
                      Na(l, f, {
                          value: m
                      })
                  }
              }
              function e(l) {
                  var m = Object[l];
                  m && (Object[l] = function(n) {
                      if (n instanceof b)
                          return n;
                      Object.isExtensible(n) && d(n);
                      return m(n)
                  }
                  )
              }
              if (function() {
                  if (!a || !Object.seal)
                      return !1;
                  try {
                      var l = Object.seal({})
                        , m = Object.seal({})
                        , n = new a([[l, 2], [m, 3]]);
                      if (2 != n.get(l) || 3 != n.get(m))
                          return !1;
                      n.delete(l);
                      n.set(m, 4);
                      return !n.has(l) && 4 == n.get(m)
                  } catch (q) {
                      return !1
                  }
              }())
                  return a;
              var f = "$jscomp_hidden_" + Math.random();
              e("freeze");
              e("preventExtensions");
              e("seal");
              var g = 0
                , k = function(l) {
                  this.g = (g += Math.random() + 1).toString();
                  if (l) {
                      l = _.Sa(l);
                      for (var m; !(m = l.next()).done; )
                          m = m.value,
                          this.set(m[0], m[1])
                  }
              };
              k.prototype.set = function(l, m) {
                  if (!c(l))
                      throw Error("l");
                  d(l);
                  if (!bb(l, f))
                      throw Error("m`" + l);
                  l[f][this.g] = m;
                  return this
              }
              ;
              k.prototype.get = function(l) {
                  return c(l) && bb(l, f) ? l[f][this.g] : void 0
              }
              ;
              k.prototype.has = function(l) {
                  return c(l) && bb(l, f) && bb(l[f], this.g)
              }
              ;
              k.prototype.delete = function(l) {
                  return c(l) && bb(l, f) && bb(l[f], this.g) ? delete l[f][this.g] : !1
              }
              ;
              return k
          });
          Qa("Map", function(a) {
              if (function() {
                  if (!a || "function" != typeof a || !a.prototype.entries || "function" != typeof Object.seal)
                      return !1;
                  try {
                      var k = Object.seal({
                          x: 4
                      })
                        , l = new a(_.Sa([[k, "s"]]));
                      if ("s" != l.get(k) || 1 != l.size || l.get({
                          x: 4
                      }) || l.set({
                          x: 4
                      }, "t") != l || 2 != l.size)
                          return !1;
                      var m = l.entries()
                        , n = m.next();
                      if (n.done || n.value[0] != k || "s" != n.value[1])
                          return !1;
                      n = m.next();
                      return n.done || 4 != n.value[0].x || "t" != n.value[1] || !m.next().done ? !1 : !0
                  } catch (q) {
                      return !1
                  }
              }())
                  return a;
              var b = new WeakMap
                , c = function(k) {
                  this.h = {};
                  this.g = f();
                  this.size = 0;
                  if (k) {
                      k = _.Sa(k);
                      for (var l; !(l = k.next()).done; )
                          l = l.value,
                          this.set(l[0], l[1])
                  }
              };
              c.prototype.set = function(k, l) {
                  k = 0 === k ? 0 : k;
                  var m = d(this, k);
                  m.list || (m.list = this.h[m.id] = []);
                  m.Ra ? m.Ra.value = l : (m.Ra = {
                      next: this.g,
                      Mb: this.g.Mb,
                      head: this.g,
                      key: k,
                      value: l
                  },
                  m.list.push(m.Ra),
                  this.g.Mb.next = m.Ra,
                  this.g.Mb = m.Ra,
                  this.size++);
                  return this
              }
              ;
              c.prototype.delete = function(k) {
                  k = d(this, k);
                  return k.Ra && k.list ? (k.list.splice(k.index, 1),
                  k.list.length || delete this.h[k.id],
                  k.Ra.Mb.next = k.Ra.next,
                  k.Ra.next.Mb = k.Ra.Mb,
                  k.Ra.head = null,
                  this.size--,
                  !0) : !1
              }
              ;
              c.prototype.clear = function() {
                  this.h = {};
                  this.g = this.g.Mb = f();
                  this.size = 0
              }
              ;
              c.prototype.has = function(k) {
                  return !!d(this, k).Ra
              }
              ;
              c.prototype.get = function(k) {
                  return (k = d(this, k).Ra) && k.value
              }
              ;
              c.prototype.entries = function() {
                  return e(this, function(k) {
                      return [k.key, k.value]
                  })
              }
              ;
              c.prototype.keys = function() {
                  return e(this, function(k) {
                      return k.key
                  })
              }
              ;
              c.prototype.values = function() {
                  return e(this, function(k) {
                      return k.value
                  })
              }
              ;
              c.prototype.forEach = function(k, l) {
                  for (var m = this.entries(), n; !(n = m.next()).done; )
                      n = n.value,
                      k.call(l, n[1], n[0], this)
              }
              ;
              c.prototype[Symbol.iterator] = c.prototype.entries;
              var d = function(k, l) {
                  var m = l && typeof l;
                  "object" == m || "function" == m ? b.has(l) ? m = b.get(l) : (m = "" + ++g,
                  b.set(l, m)) : m = "p_" + l;
                  var n = k.h[m];
                  if (n && bb(k.h, m))
                      for (k = 0; k < n.length; k++) {
                          var q = n[k];
                          if (l !== l && q.key !== q.key || l === q.key)
                              return {
                                  id: m,
                                  list: n,
                                  index: k,
                                  Ra: q
                              }
                      }
                  return {
                      id: m,
                      list: n,
                      index: -1,
                      Ra: void 0
                  }
              }
                , e = function(k, l) {
                  var m = k.g;
                  return Ra(function() {
                      if (m) {
                          for (; m.head != k.g; )
                              m = m.Mb;
                          for (; m.next != m.head; )
                              return m = m.next,
                              {
                                  done: !1,
                                  value: l(m)
                              };
                          m = null
                      }
                      return {
                          done: !0,
                          value: void 0
                      }
                  })
              }
                , f = function() {
                  var k = {};
                  return k.Mb = k.next = k.head = k
              }
                , g = 0;
              return c
          });
          Qa("Array.prototype.find", function(a) {
              return a ? a : function(b, c) {
                  a: {
                      var d = this;
                      d instanceof String && (d = String(d));
                      for (var e = d.length, f = 0; f < e; f++) {
                          var g = d[f];
                          if (b.call(c, g, f, d)) {
                              b = g;
                              break a
                          }
                      }
                      b = void 0
                  }
                  return b
              }
          });
          Qa("String.prototype.endsWith", function(a) {
              return a ? a : function(b, c) {
                  var d = ab(this, b, "endsWith");
                  void 0 === c && (c = d.length);
                  c = Math.max(0, Math.min(c | 0, d.length));
                  for (var e = b.length; 0 < e && 0 < c; )
                      if (d[--c] != b[--e])
                          return !1;
                  return 0 >= e
              }
          });
          var cb = function(a, b) {
              a instanceof String && (a += "");
              var c = 0
                , d = !1
                , e = {
                  next: function() {
                      if (!d && c < a.length) {
                          var f = c++;
                          return {
                              value: b(f, a[f]),
                              done: !1
                          }
                      }
                      d = !0;
                      return {
                          done: !0,
                          value: void 0
                      }
                  }
              };
              e[Symbol.iterator] = function() {
                  return e
              }
              ;
              return e
          };
          Qa("Array.prototype.entries", function(a) {
              return a ? a : function() {
                  return cb(this, function(b, c) {
                      return [b, c]
                  })
              }
          });
          Qa("Array.prototype.keys", function(a) {
              return a ? a : function() {
                  return cb(this, function(b) {
                      return b
                  })
              }
          });
          Qa("Array.from", function(a) {
              return a ? a : function(b, c, d) {
                  c = null != c ? c : function(k) {
                      return k
                  }
                  ;
                  var e = []
                    , f = "undefined" != typeof Symbol && Symbol.iterator && b[Symbol.iterator];
                  if ("function" == typeof f) {
                      b = f.call(b);
                      for (var g = 0; !(f = b.next()).done; )
                          e.push(c.call(d, f.value, g++))
                  } else
                      for (f = b.length,
                      g = 0; g < f; g++)
                          e.push(c.call(d, b[g], g));
                  return e
              }
          });
          Qa("Array.prototype.values", function(a) {
              return a ? a : function() {
                  return cb(this, function(b, c) {
                      return c
                  })
              }
          });
          Qa("Set", function(a) {
              if (function() {
                  if (!a || "function" != typeof a || !a.prototype.entries || "function" != typeof Object.seal)
                      return !1;
                  try {
                      var c = Object.seal({
                          x: 4
                      })
                        , d = new a(_.Sa([c]));
                      if (!d.has(c) || 1 != d.size || d.add(c) != d || 1 != d.size || d.add({
                          x: 4
                      }) != d || 2 != d.size)
                          return !1;
                      var e = d.entries()
                        , f = e.next();
                      if (f.done || f.value[0] != c || f.value[1] != c)
                          return !1;
                      f = e.next();
                      return f.done || f.value[0] == c || 4 != f.value[0].x || f.value[1] != f.value[0] ? !1 : e.next().done
                  } catch (g) {
                      return !1
                  }
              }())
                  return a;
              var b = function(c) {
                  this.g = new Map;
                  if (c) {
                      c = _.Sa(c);
                      for (var d; !(d = c.next()).done; )
                          this.add(d.value)
                  }
                  this.size = this.g.size
              };
              b.prototype.add = function(c) {
                  c = 0 === c ? 0 : c;
                  this.g.set(c, c);
                  this.size = this.g.size;
                  return this
              }
              ;
              b.prototype.delete = function(c) {
                  c = this.g.delete(c);
                  this.size = this.g.size;
                  return c
              }
              ;
              b.prototype.clear = function() {
                  this.g.clear();
                  this.size = 0
              }
              ;
              b.prototype.has = function(c) {
                  return this.g.has(c)
              }
              ;
              b.prototype.entries = function() {
                  return this.g.entries()
              }
              ;
              b.prototype.values = function() {
                  return this.g.values()
              }
              ;
              b.prototype.keys = b.prototype.values;
              b.prototype[Symbol.iterator] = b.prototype.values;
              b.prototype.forEach = function(c, d) {
                  var e = this;
                  this.g.forEach(function(f) {
                      return c.call(d, f, f, e)
                  })
              }
              ;
              return b
          });
          var db = "function" == typeof Object.assign ? Object.assign : function(a, b) {
              for (var c = 1; c < arguments.length; c++) {
                  var d = arguments[c];
                  if (d)
                      for (var e in d)
                          bb(d, e) && (a[e] = d[e])
              }
              return a
          }
          ;
          Qa("Object.assign", function(a) {
              return a || db
          });
          Qa("String.prototype.replaceAll", function(a) {
              return a ? a : function(b, c) {
                  if (b instanceof RegExp && !b.global)
                      throw new TypeError("n");
                  return b instanceof RegExp ? this.replace(b, c) : this.replace(new RegExp(String(b).replace(/([-()\[\]{}+?*.$\^|,:#<!\\])/g, "\\$1").replace(/\x08/g, "\\x08"),"g"), c)
              }
          });
          Qa("Number.isNaN", function(a) {
              return a ? a : function(b) {
                  return "number" === typeof b && isNaN(b)
              }
          });
          Qa("Object.is", function(a) {
              return a ? a : function(b, c) {
                  return b === c ? 0 !== b || 1 / b === 1 / c : b !== b && c !== c
              }
          });
          Qa("Array.prototype.includes", function(a) {
              return a ? a : function(b, c) {
                  var d = this;
                  d instanceof String && (d = String(d));
                  var e = d.length;
                  c = c || 0;
                  for (0 > c && (c = Math.max(c + e, 0)); c < e; c++) {
                      var f = d[c];
                      if (f === b || Object.is(f, b))
                          return !0
                  }
                  return !1
              }
          });
          Qa("String.prototype.includes", function(a) {
              return a ? a : function(b, c) {
                  return -1 !== ab(this, b, "includes").indexOf(b, c || 0)
              }
          });
          Qa("Object.entries", function(a) {
              return a ? a : function(b) {
                  var c = [], d;
                  for (d in b)
                      bb(b, d) && c.push([d, b[d]]);
                  return c
              }
          });
          _._DumpException = window._DumpException || function(a) {
              throw a;
          }
          ;
          window._DumpException = _._DumpException;
          /*

Copyright The Closure Library Authors.
SPDX-License-Identifier: Apache-2.0
*/
          var eb, fb, hb, gb, kb, lb, mb, nb, sb;
          eb = eb || {};
          _.p = this || self;
          fb = /^[a-zA-Z_$][a-zA-Z0-9._$]*$/;
          hb = function(a) {
              if ("string" !== typeof a || !a || -1 == a.search(fb))
                  throw Error("o");
              if (!gb || "goog" != gb.type)
                  throw Error("p`" + a);
              if (gb.Vj)
                  throw Error("q");
              gb.Vj = a
          }
          ;
          hb.get = function() {
              return null
          }
          ;
          gb = null;
          _.ib = function(a, b) {
              a = a.split(".");
              b = b || _.p;
              for (var c = 0; c < a.length; c++)
                  if (b = b[a[c]],
                  null == b)
                      return null;
              return b
          }
          ;
          _.jb = function(a) {
              var b = typeof a;
              return "object" != b ? b : a ? Array.isArray(a) ? "array" : b : "null"
          }
          ;
          _.da = function(a) {
              var b = _.jb(a);
              return "array" == b || "object" == b && "number" == typeof a.length
          }
          ;
          _.ya = function(a) {
              var b = typeof a;
              return "object" == b && null != a || "function" == b
          }
          ;
          _.za = function(a) {
              return Object.prototype.hasOwnProperty.call(a, kb) && a[kb] || (a[kb] = ++lb)
          }
          ;
          kb = "closure_uid_" + (1E9 * Math.random() >>> 0);
          lb = 0;
          mb = function(a, b, c) {
              return a.call.apply(a.bind, arguments)
          }
          ;
          nb = function(a, b, c) {
              if (!a)
                  throw Error();
              if (2 < arguments.length) {
                  var d = Array.prototype.slice.call(arguments, 2);
                  return function() {
                      var e = Array.prototype.slice.call(arguments);
                      Array.prototype.unshift.apply(e, d);
                      return a.apply(b, e)
                  }
              }
              return function() {
                  return a.apply(b, arguments)
              }
          }
          ;
          _.v = function(a, b, c) {
              Function.prototype.bind && -1 != Function.prototype.bind.toString().indexOf("native code") ? _.v = mb : _.v = nb;
              return _.v.apply(null, arguments)
          }
          ;
          _.ob = function(a, b) {
              var c = Array.prototype.slice.call(arguments, 1);
              return function() {
                  var d = c.slice();
                  d.push.apply(d, arguments);
                  return a.apply(this, d)
              }
          }
          ;
          _.qb = function() {
              return Date.now()
          }
          ;
          _.rb = function(a, b) {
              a = a.split(".");
              var c = _.p;
              a[0]in c || "undefined" == typeof c.execScript || c.execScript("var " + a[0]);
              for (var d; a.length && (d = a.shift()); )
                  a.length || void 0 === b ? c[d] && c[d] !== Object.prototype[d] ? c = c[d] : c = c[d] = {} : c[d] = b
          }
          ;
          _.w = function(a, b) {
              function c() {}
              c.prototype = b.prototype;
              a.G = b.prototype;
              a.prototype = new c;
              a.prototype.constructor = a;
              a.Pl = function(d, e, f) {
                  for (var g = Array(arguments.length - 2), k = 2; k < arguments.length; k++)
                      g[k - 2] = arguments[k];
                  return b.prototype[e].apply(d, g)
              }
          }
          ;
          sb = function(a) {
              return a
          }
          ;
          _.w(_.aa, Error);
          _.aa.prototype.name = "CustomError";
          var tb;
          _.x = function() {
              this.La = this.La;
              this.ma = this.ma
          }
          ;
          _.x.prototype.La = !1;
          _.x.prototype.zb = function() {
              return this.La
          }
          ;
          _.x.prototype.O = function() {
              this.La || (this.La = !0,
              this.L())
          }
          ;
          _.x.prototype.L = function() {
              if (this.ma)
                  for (; this.ma.length; )
                      this.ma.shift()()
          }
          ;
          var vb;
          _.ub = function() {}
          ;
          vb = function(a) {
              return function() {
                  throw Error(a);
              }
          }
          ;
          _.wb = function(a) {
              var b = !1, c;
              return function() {
                  b || (c = a(),
                  b = !0);
                  return c
              }
          }
          ;
          var xb, yb = function() {
              if (void 0 === xb) {
                  var a = null
                    , b = _.p.trustedTypes;
                  if (b && b.createPolicy) {
                      try {
                          a = b.createPolicy("goog#html", {
                              createHTML: sb,
                              createScript: sb,
                              createScriptURL: sb
                          })
                      } catch (c) {
                          _.p.console && _.p.console.error(c.message)
                      }
                      xb = a
                  } else
                      xb = a
              }
              return xb
          };
          var zb = {}
            , Ab = function(a, b) {
              this.g = b === zb ? a : "";
              this.tb = !0
          };
          Ab.prototype.toString = function() {
              return this.g.toString()
          }
          ;
          Ab.prototype.Xa = function() {
              return this.g.toString()
          }
          ;
          _.Bb = function(a) {
              return a instanceof Ab && a.constructor === Ab ? a.g : "type_error:SafeScript"
          }
          ;
          _.Cb = function(a) {
              var b = yb();
              a = b ? b.createScript(a) : a;
              return new Ab(a,zb)
          }
          ;
          _.Db = RegExp("^(ar|ckb|dv|he|iw|fa|nqo|ps|sd|ug|ur|yi|.*[-_](Adlm|Arab|Hebr|Nkoo|Rohg|Thaa))(?!.*[-_](Latn|Cyrl)($|-|_))($|-|_)", "i");
          var Eb;
          _.Fb = function(a, b) {
              this.g = b === Eb ? a : ""
          }
          ;
          _.Fb.prototype.toString = function() {
              return this.g + ""
          }
          ;
          _.Fb.prototype.tb = !0;
          _.Fb.prototype.Xa = function() {
              return this.g.toString()
          }
          ;
          _.Ka = function(a) {
              return a instanceof _.Fb && a.constructor === _.Fb ? a.g : "type_error:TrustedResourceUrl"
          }
          ;
          _.Gb = RegExp("^((https:)?//[0-9a-z.:[\\]-]+/|/[^/\\\\]|[^:/\\\\%]+/|[^:/\\\\%]*[?#]|about:blank#)", "i");
          Eb = {};
          _.Hb = function(a) {
              var b = yb();
              a = b ? b.createScriptURL(a) : a;
              return new _.Fb(a,Eb)
          }
          ;
          hb = hb || {};
          var Ib = function() {
              _.x.call(this)
          };
          _.w(Ib, _.x);
          Ib.prototype.initialize = function() {}
          ;
          var Jb = function(a, b) {
              this.g = a;
              this.h = b
          };
          Jb.prototype.j = function(a) {
              this.g && (this.g.call(this.h || null, a),
              this.g = this.h = null)
          }
          ;
          Jb.prototype.abort = function() {
              this.h = this.g = null
          }
          ;
          var Kb = function(a, b) {
              _.x.call(this);
              this.s = a;
              this.o = b;
              this.j = [];
              this.h = [];
              this.l = []
          };
          _.w(Kb, _.x);
          Kb.prototype.C = Ib;
          Kb.prototype.g = null;
          Kb.prototype.Dc = function() {
              return this.s
          }
          ;
          Kb.prototype.mb = function() {
              return this.o
          }
          ;
          var Lb = function(a, b) {
              a.h.push(new Jb(b))
          };
          Kb.prototype.onLoad = function(a) {
              var b = new this.C;
              b.initialize(a());
              this.g = b;
              b = (b = !!Mb(this.l, a())) || !!Mb(this.j, a());
              b || (this.h.length = 0);
              return b
          }
          ;
          Kb.prototype.Of = function(a) {
              (a = Mb(this.h, a)) && _.p.setTimeout(vb("Module errback failures: " + a), 0);
              this.l.length = 0;
              this.j.length = 0
          }
          ;
          var Mb = function(a, b) {
              for (var c = [], d = 0; d < a.length; d++)
                  try {
                      a[d].j(b)
                  } catch (e) {
                      ba(e),
                      c.push(e)
                  }
              a.length = 0;
              return c.length ? c : null
          };
          Kb.prototype.L = function() {
              Kb.G.L.call(this);
              _.ca(this.g)
          }
          ;
          var Nb = function() {
              this.B = this.La = null
          };
          _.h = Nb.prototype;
          _.h.Kh = function() {}
          ;
          _.h.bg = function() {}
          ;
          _.h.Ih = function() {
              throw Error("u");
          }
          ;
          _.h.Vg = function() {
              return this.La
          }
          ;
          _.h.cg = function(a) {
              this.La = a
          }
          ;
          _.h.pg = function() {
              return !1
          }
          ;
          _.h.ph = function() {
              return !1
          }
          ;
          var ka;
          _.fa = null;
          _.ha = null;
          ka = [];
          var y = function(a, b) {
              var c = c || [];
              this.qk = a;
              this.Uj = b || null;
              this.ff = [];
              this.ff = this.ff.concat(c)
          };
          y.prototype.toString = function() {
              return this.qk
          }
          ;
          y.prototype.Dc = function() {
              return this.ff
          }
          ;
          new y("rJmJrc","rJmJrc");
          var Ob = new y("n73qwf","n73qwf");
          var pa = Symbol("w");
          var sa, Rb;
          sa = Array.prototype.indexOf ? function(a, b) {
              return Array.prototype.indexOf.call(a, b, void 0)
          }
          : function(a, b) {
              if ("string" === typeof a)
                  return "string" !== typeof b || 1 != b.length ? -1 : a.indexOf(b, 0);
              for (var c = 0; c < a.length; c++)
                  if (c in a && a[c] === b)
                      return c;
              return -1
          }
          ;
          _.Pb = Array.prototype.forEach ? function(a, b) {
              Array.prototype.forEach.call(a, b, void 0)
          }
          : function(a, b) {
              for (var c = a.length, d = "string" === typeof a ? a.split("") : a, e = 0; e < c; e++)
                  e in d && b.call(void 0, d[e], e, a)
          }
          ;
          _.Qb = Array.prototype.map ? function(a, b) {
              return Array.prototype.map.call(a, b, void 0)
          }
          : function(a, b) {
              for (var c = a.length, d = Array(c), e = "string" === typeof a ? a.split("") : a, f = 0; f < c; f++)
                  f in e && (d[f] = b.call(void 0, e[f], f, a));
              return d
          }
          ;
          Rb = Array.prototype.reduce ? function(a, b, c) {
              Array.prototype.reduce.call(a, b, c)
          }
          : function(a, b, c) {
              var d = c;
              (0,
              _.Pb)(a, function(e, f) {
                  d = b.call(void 0, d, e, f, a)
              })
          }
          ;
          _.Sb = Array.prototype.some ? function(a, b) {
              return Array.prototype.some.call(a, b, void 0)
          }
          : function(a, b) {
              for (var c = a.length, d = "string" === typeof a ? a.split("") : a, e = 0; e < c; e++)
                  if (e in d && b.call(void 0, d[e], e, a))
                      return !0;
              return !1
          }
          ;
          _.Tb = Array.prototype.every ? function(a, b) {
              return Array.prototype.every.call(a, b, void 0)
          }
          : function(a, b) {
              for (var c = a.length, d = "string" === typeof a ? a.split("") : a, e = 0; e < c; e++)
                  if (e in d && !b.call(void 0, d[e], e, a))
                      return !1;
              return !0
          }
          ;
          var Wb;
          _.Ub = function(a, b) {
              var c = a.length - b.length;
              return 0 <= c && a.indexOf(b, c) == c
          }
          ;
          _.Vb = String.prototype.trim ? function(a) {
              return a.trim()
          }
          : function(a) {
              return /^[\s\xa0]*([\s\S]*?)[\s\xa0]*$/.exec(a)[1]
          }
          ;
          _.Xb = function(a, b) {
              var c = 0;
              a = (0,
              _.Vb)(String(a)).split(".");
              b = (0,
              _.Vb)(String(b)).split(".");
              for (var d = Math.max(a.length, b.length), e = 0; 0 == c && e < d; e++) {
                  var f = a[e] || ""
                    , g = b[e] || "";
                  do {
                      f = /(\d*)(\D*)(.*)/.exec(f) || ["", "", "", ""];
                      g = /(\d*)(\D*)(.*)/.exec(g) || ["", "", "", ""];
                      if (0 == f[0].length && 0 == g[0].length)
                          break;
                      c = Wb(0 == f[1].length ? 0 : parseInt(f[1], 10), 0 == g[1].length ? 0 : parseInt(g[1], 10)) || Wb(0 == f[2].length, 0 == g[2].length) || Wb(f[2], g[2]);
                      f = f[3];
                      g = g[3]
                  } while (0 == c)
              }
              return c
          }
          ;
          Wb = function(a, b) {
              return a < b ? -1 : a > b ? 1 : 0
          }
          ;
          var Yb = function(a) {
              Yb[" "](a);
              return a
          };
          Yb[" "] = function() {}
          ;
          _.Zb = function(a, b) {
              try {
                  return Yb(a[b]),
                  !0
              } catch (c) {}
              return !1
          }
          ;
          _.$b = function(a, b, c) {
              return Object.prototype.hasOwnProperty.call(a, b) ? a[b] : a[b] = c(b)
          }
          ;
          var mc, nc, sc, uc;
          _.ac = _.t("Opera");
          _.z = _.t("Trident") || _.t("MSIE");
          _.bc = _.t("Edge");
          _.cc = _.bc || _.z;
          _.B = _.t("Gecko") && !(-1 != _.Ba().toLowerCase().indexOf("webkit") && !_.t("Edge")) && !(_.t("Trident") || _.t("MSIE")) && !_.t("Edge");
          _.C = -1 != _.Ba().toLowerCase().indexOf("webkit") && !_.t("Edge");
          _.dc = _.C && _.t("Mobile");
          _.ec = _.t("Macintosh");
          _.fc = _.t("Windows");
          _.gc = _.t("Android");
          _.hc = _.Ca();
          _.ic = _.t("iPad");
          _.jc = _.t("iPod");
          _.lc = _.Da();
          mc = function() {
              var a = _.p.document;
              return a ? a.documentMode : void 0
          }
          ;
          a: {
              var oc = ""
                , pc = function() {
                  var a = _.Ba();
                  if (_.B)
                      return /rv:([^\);]+)(\)|;)/.exec(a);
                  if (_.bc)
                      return /Edge\/([\d\.]+)/.exec(a);
                  if (_.z)
                      return /\b(?:MSIE|rv)[: ]([^\);]+)(\)|;)/.exec(a);
                  if (_.C)
                      return /WebKit\/(\S+)/.exec(a);
                  if (_.ac)
                      return /(?:Version)[ \/]?(\S+)/.exec(a)
              }();
              pc && (oc = pc ? pc[1] : "");
              if (_.z) {
                  var qc = mc();
                  if (null != qc && qc > parseFloat(oc)) {
                      nc = String(qc);
                      break a
                  }
              }
              nc = oc
          }
          _.rc = nc;
          sc = {};
          _.tc = function(a) {
              return _.$b(sc, a, function() {
                  return 0 <= _.Xb(_.rc, a)
              })
          }
          ;
          if (_.p.document && _.z) {
              var vc = mc();
              uc = vc ? vc : parseInt(_.rc, 10) || void 0
          } else
              uc = void 0;
          _.wc = uc;
          _.xc = _.z || _.C;
          var Ga;
          Ga = "constructor hasOwnProperty isPrototypeOf propertyIsEnumerable toLocaleString toString valueOf".split(" ");
          _.yc = function(a, b, c) {
              for (var d in a)
                  b.call(c, a[d], d, a)
          }
          ;
          var zc;
          _.Ac = function(a, b) {
              this.g = b === zc ? a : ""
          }
          ;
          _.Ac.prototype.toString = function() {
              return this.g.toString()
          }
          ;
          _.Ac.prototype.tb = !0;
          _.Ac.prototype.Xa = function() {
              return this.g.toString()
          }
          ;
          zc = {};
          _.Bc = function(a) {
              return new _.Ac(a,zc)
          }
          ;
          _.Cc = _.Bc("about:invalid#zClosurez");
          _.Dc = {};
          _.Ec = function(a, b) {
              this.g = b === _.Dc ? a : "";
              this.tb = !0
          }
          ;
          _.Ec.prototype.Xa = function() {
              return this.g
          }
          ;
          _.Ec.prototype.toString = function() {
              return this.g.toString()
          }
          ;
          _.Fc = new _.Ec("",_.Dc);
          _.Gc = RegExp("^[-,.\"'%_!#/ a-zA-Z0-9\\[\\]]+$");
          _.Hc = RegExp("\\b(url\\([ \t\n]*)('[ -&(-\\[\\]-~]*'|\"[ !#-\\[\\]-~]*\"|[!#-&*-\\[\\]-~]*)([ \t\n]*\\))", "g");
          _.Ic = RegExp("\\b(calc|cubic-bezier|fit-content|hsl|hsla|linear-gradient|matrix|minmax|radial-gradient|repeat|rgb|rgba|(rotate|scale|translate)(X|Y|Z|3d)?|var)\\([-+*/0-9a-zA-Z.%#\\[\\], ]+\\)", "g");
          var Jc;
          Jc = {};
          _.Kc = function(a, b) {
              this.g = b === Jc ? a : "";
              this.tb = !0
          }
          ;
          _.Kc.prototype.Xa = function() {
              return this.g.toString()
          }
          ;
          _.Kc.prototype.toString = function() {
              return this.g.toString()
          }
          ;
          _.Lc = function(a) {
              return a instanceof _.Kc && a.constructor === _.Kc ? a.g : "type_error:SafeHtml"
          }
          ;
          _.Mc = function(a) {
              var b = yb();
              a = b ? b.createHTML(a) : a;
              return new _.Kc(a,Jc)
          }
          ;
          _.Nc = _.Mc("<!DOCTYPE html>");
          _.Oc = new _.Kc(_.p.trustedTypes && _.p.trustedTypes.emptyHTML || "",Jc);
          _.Pc = _.Mc("<br>");
          _.Qc = _.wb(function() {
              var a = document.createElement("div")
                , b = document.createElement("div");
              b.appendChild(document.createElement("div"));
              a.appendChild(b);
              b = a.firstChild.firstChild;
              a.innerHTML = _.Lc(_.Oc);
              return !b.parentElement
          });
          _.Rc = function(a, b) {
              this.width = a;
              this.height = b
          }
          ;
          _.Sc = function(a, b) {
              return a == b ? !0 : a && b ? a.width == b.width && a.height == b.height : !1
          }
          ;
          _.h = _.Rc.prototype;
          _.h.aspectRatio = function() {
              return this.width / this.height
          }
          ;
          _.h.pc = function() {
              return !(this.width * this.height)
          }
          ;
          _.h.ceil = function() {
              this.width = Math.ceil(this.width);
              this.height = Math.ceil(this.height);
              return this
          }
          ;
          _.h.floor = function() {
              this.width = Math.floor(this.width);
              this.height = Math.floor(this.height);
              return this
          }
          ;
          _.h.round = function() {
              this.width = Math.round(this.width);
              this.height = Math.round(this.height);
              return this
          }
          ;
          _.Tc = function(a) {
              return encodeURIComponent(String(a))
          }
          ;
          _.Uc = function(a) {
              return decodeURIComponent(a.replace(/\+/g, " "))
          }
          ;
          _.Vc = function() {
              return Math.floor(2147483648 * Math.random()).toString(36) + Math.abs(Math.floor(2147483648 * Math.random()) ^ _.qb()).toString(36)
          }
          ;
          var Zc, Yc, ld, md;
          _.E = function(a) {
              return a ? new _.Wc(_.D(a)) : tb || (tb = new _.Wc)
          }
          ;
          _.Xc = function(a, b) {
              return "string" === typeof b ? a.getElementById(b) : b
          }
          ;
          Zc = function(a, b) {
              _.yc(b, function(c, d) {
                  c && "object" == typeof c && c.tb && (c = c.Xa());
                  "style" == d ? a.style.cssText = c : "class" == d ? a.className = c : "for" == d ? a.htmlFor = c : Yc.hasOwnProperty(d) ? a.setAttribute(Yc[d], c) : 0 == d.lastIndexOf("aria-", 0) || 0 == d.lastIndexOf("data-", 0) ? a.setAttribute(d, c) : a[d] = c
              })
          }
          ;
          Yc = {
              cellpadding: "cellPadding",
              cellspacing: "cellSpacing",
              colspan: "colSpan",
              frameborder: "frameBorder",
              height: "height",
              maxlength: "maxLength",
              nonce: "nonce",
              role: "role",
              rowspan: "rowSpan",
              type: "type",
              usemap: "useMap",
              valign: "vAlign",
              width: "width"
          };
          _.ad = function(a) {
              a = a.document;
              a = _.$c(a) ? a.documentElement : a.body;
              return new _.Rc(a.clientWidth,a.clientHeight)
          }
          ;
          _.bd = function(a) {
              return a ? a.parentWindow || a.defaultView : window
          }
          ;
          _.ed = function(a, b) {
              var c = b[1]
                , d = _.cd(a, String(b[0]));
              c && ("string" === typeof c ? d.className = c : Array.isArray(c) ? d.className = c.join(" ") : Zc(d, c));
              2 < b.length && _.dd(a, d, b, 2);
              return d
          }
          ;
          _.dd = function(a, b, c, d) {
              function e(k) {
                  k && b.appendChild("string" === typeof k ? a.createTextNode(k) : k)
              }
              for (; d < c.length; d++) {
                  var f = c[d];
                  if (!_.da(f) || _.ya(f) && 0 < f.nodeType)
                      e(f);
                  else {
                      a: {
                          if (f && "number" == typeof f.length) {
                              if (_.ya(f)) {
                                  var g = "function" == typeof f.item || "string" == typeof f.item;
                                  break a
                              }
                              if ("function" === typeof f) {
                                  g = "function" == typeof f.item;
                                  break a
                              }
                          }
                          g = !1
                      }
                      _.Pb(g ? _.wa(f) : f, e)
                  }
              }
          }
          ;
          _.cd = function(a, b) {
              b = String(b);
              "application/xhtml+xml" === a.contentType && (b = b.toLowerCase());
              return a.createElement(b)
          }
          ;
          _.$c = function(a) {
              return "CSS1Compat" == a.compatMode
          }
          ;
          _.fd = function(a) {
              for (var b; b = a.firstChild; )
                  a.removeChild(b)
          }
          ;
          _.gd = function(a) {
              return void 0 != a.children ? a.children : Array.prototype.filter.call(a.childNodes, function(b) {
                  return 1 == b.nodeType
              })
          }
          ;
          _.id = function(a) {
              return void 0 !== a.firstElementChild ? a.firstElementChild : _.hd(a.firstChild, !0)
          }
          ;
          _.hd = function(a, b) {
              for (; a && 1 != a.nodeType; )
                  a = b ? a.nextSibling : a.previousSibling;
              return a
          }
          ;
          _.jd = function(a, b) {
              if (!a || !b)
                  return !1;
              if (a.contains && 1 == b.nodeType)
                  return a == b || a.contains(b);
              if ("undefined" != typeof a.compareDocumentPosition)
                  return a == b || !!(a.compareDocumentPosition(b) & 16);
              for (; b && a != b; )
                  b = b.parentNode;
              return b == a
          }
          ;
          _.D = function(a) {
              return 9 == a.nodeType ? a : a.ownerDocument || a.document
          }
          ;
          _.kd = function(a, b) {
              if ("textContent"in a)
                  a.textContent = b;
              else if (3 == a.nodeType)
                  a.data = String(b);
              else if (a.firstChild && 3 == a.firstChild.nodeType) {
                  for (; a.lastChild != a.firstChild; )
                      a.removeChild(a.lastChild);
                  a.firstChild.data = String(b)
              } else
                  _.fd(a),
                  a.appendChild(_.D(a).createTextNode(String(b)))
          }
          ;
          ld = {
              SCRIPT: 1,
              STYLE: 1,
              HEAD: 1,
              IFRAME: 1,
              OBJECT: 1
          };
          md = {
              IMG: " ",
              BR: "\n"
          };
          _.od = function(a) {
              var b = [];
              _.nd(a, b, !0);
              a = b.join("");
              a = a.replace(/ \xAD /g, " ").replace(/\xAD/g, "");
              a = a.replace(/\u200B/g, "");
              a = a.replace(/ +/g, " ");
              " " != a && (a = a.replace(/^\s*/, ""));
              return a
          }
          ;
          _.nd = function(a, b, c) {
              if (!(a.nodeName in ld))
                  if (3 == a.nodeType)
                      c ? b.push(String(a.nodeValue).replace(/(\r\n|\r|\n)/g, "")) : b.push(a.nodeValue);
                  else if (a.nodeName in md)
                      b.push(md[a.nodeName]);
                  else
                      for (a = a.firstChild; a; )
                          _.nd(a, b, c),
                          a = a.nextSibling
          }
          ;
          _.Wc = function(a) {
              this.V = a || _.p.document || document
          }
          ;
          _.Wc.prototype.A = function(a) {
              return _.Xc(this.V, a)
          }
          ;
          _.Wc.prototype.M = function(a, b, c) {
              return _.ed(this.V, arguments)
          }
          ;
          _.pd = function(a) {
              a = a.V;
              return a.parentWindow || a.defaultView
          }
          ;
          _.h = _.Wc.prototype;
          _.h.appendChild = function(a, b) {
              a.appendChild(b)
          }
          ;
          _.h.rd = _.fd;
          _.h.Si = _.gd;
          _.h.Ug = _.id;
          _.h.contains = _.jd;
          _.h.qb = _.kd;
          _.h.qg = _.od;
          var qd = function() {
              this.id = "b"
          };
          qd.prototype.toString = function() {
              return this.id
          }
          ;
          _.sd = function(a, b) {
              this.type = a instanceof qd ? String(a) : a;
              this.currentTarget = this.target = b;
              this.defaultPrevented = this.h = !1
          }
          ;
          _.sd.prototype.stopPropagation = function() {
              this.h = !0
          }
          ;
          _.sd.prototype.preventDefault = function() {
              this.defaultPrevented = !0
          }
          ;
          var td = function() {
              if (!_.p.addEventListener || !Object.defineProperty)
                  return !1;
              var a = !1
                , b = Object.defineProperty({}, "passive", {
                  get: function() {
                      a = !0
                  }
              });
              try {
                  _.p.addEventListener("test", function() {}, b),
                  _.p.removeEventListener("test", function() {}, b)
              } catch (c) {}
              return a
          }();
          _.vd = function(a, b) {
              _.sd.call(this, a ? a.type : "");
              this.relatedTarget = this.currentTarget = this.target = null;
              this.button = this.screenY = this.screenX = this.clientY = this.clientX = this.offsetY = this.offsetX = 0;
              this.key = "";
              this.charCode = this.keyCode = 0;
              this.metaKey = this.shiftKey = this.altKey = this.ctrlKey = !1;
              this.state = null;
              this.j = !1;
              this.pointerId = 0;
              this.pointerType = "";
              this.g = null;
              if (a) {
                  var c = this.type = a.type
                    , d = a.changedTouches && a.changedTouches.length ? a.changedTouches[0] : null;
                  this.target = a.target || a.srcElement;
                  this.currentTarget = b;
                  (b = a.relatedTarget) ? _.B && (_.Zb(b, "nodeName") || (b = null)) : "mouseover" == c ? b = a.fromElement : "mouseout" == c && (b = a.toElement);
                  this.relatedTarget = b;
                  d ? (this.clientX = void 0 !== d.clientX ? d.clientX : d.pageX,
                  this.clientY = void 0 !== d.clientY ? d.clientY : d.pageY,
                  this.screenX = d.screenX || 0,
                  this.screenY = d.screenY || 0) : (this.offsetX = _.C || void 0 !== a.offsetX ? a.offsetX : a.layerX,
                  this.offsetY = _.C || void 0 !== a.offsetY ? a.offsetY : a.layerY,
                  this.clientX = void 0 !== a.clientX ? a.clientX : a.pageX,
                  this.clientY = void 0 !== a.clientY ? a.clientY : a.pageY,
                  this.screenX = a.screenX || 0,
                  this.screenY = a.screenY || 0);
                  this.button = a.button;
                  this.keyCode = a.keyCode || 0;
                  this.key = a.key || "";
                  this.charCode = a.charCode || ("keypress" == c ? a.keyCode : 0);
                  this.ctrlKey = a.ctrlKey;
                  this.altKey = a.altKey;
                  this.shiftKey = a.shiftKey;
                  this.metaKey = a.metaKey;
                  this.j = _.ec ? a.metaKey : a.ctrlKey;
                  this.pointerId = a.pointerId || 0;
                  this.pointerType = "string" === typeof a.pointerType ? a.pointerType : ud[a.pointerType] || "";
                  this.state = a.state;
                  this.g = a;
                  a.defaultPrevented && _.vd.G.preventDefault.call(this)
              }
          }
          ;
          _.w(_.vd, _.sd);
          var ud = {
              2: "touch",
              3: "pen",
              4: "mouse"
          };
          _.vd.prototype.stopPropagation = function() {
              _.vd.G.stopPropagation.call(this);
              this.g.stopPropagation ? this.g.stopPropagation() : this.g.cancelBubble = !0
          }
          ;
          _.vd.prototype.preventDefault = function() {
              _.vd.G.preventDefault.call(this);
              var a = this.g;
              a.preventDefault ? a.preventDefault() : a.returnValue = !1
          }
          ;
          var wd;
          wd = "closure_listenable_" + (1E6 * Math.random() | 0);
          _.xd = function(a) {
              return !(!a || !a[wd])
          }
          ;
          var yd = 0;
          var zd = function(a, b, c, d, e) {
              this.listener = a;
              this.proxy = null;
              this.src = b;
              this.type = c;
              this.capture = !!d;
              this.ge = e;
              this.key = ++yd;
              this.Oc = this.Pd = !1
          }
            , Ad = function(a) {
              a.Oc = !0;
              a.listener = null;
              a.proxy = null;
              a.src = null;
              a.ge = null
          };
          var Bd = function(a) {
              this.src = a;
              this.g = {};
              this.h = 0
          }, Dd;
          Bd.prototype.add = function(a, b, c, d, e) {
              var f = a.toString();
              a = this.g[f];
              a || (a = this.g[f] = [],
              this.h++);
              var g = Cd(a, b, d, e);
              -1 < g ? (b = a[g],
              c || (b.Pd = !1)) : (b = new zd(b,this.src,f,!!d,e),
              b.Pd = c,
              a.push(b));
              return b
          }
          ;
          Bd.prototype.remove = function(a, b, c, d) {
              a = a.toString();
              if (!(a in this.g))
                  return !1;
              var e = this.g[a];
              b = Cd(e, b, c, d);
              return -1 < b ? (Ad(e[b]),
              Array.prototype.splice.call(e, b, 1),
              0 == e.length && (delete this.g[a],
              this.h--),
              !0) : !1
          }
          ;
          Dd = function(a, b) {
              var c = b.type;
              if (!(c in a.g))
                  return !1;
              var d = _.va(a.g[c], b);
              d && (Ad(b),
              0 == a.g[c].length && (delete a.g[c],
              a.h--));
              return d
          }
          ;
          _.Ed = function(a, b) {
              b = b && b.toString();
              var c = 0, d;
              for (d in a.g)
                  if (!b || d == b) {
                      for (var e = a.g[d], f = 0; f < e.length; f++)
                          ++c,
                          Ad(e[f]);
                      delete a.g[d];
                      a.h--
                  }
              return c
          }
          ;
          Bd.prototype.md = function(a, b, c, d) {
              a = this.g[a.toString()];
              var e = -1;
              a && (e = Cd(a, b, c, d));
              return -1 < e ? a[e] : null
          }
          ;
          Bd.prototype.hasListener = function(a, b) {
              var c = void 0 !== a
                , d = c ? a.toString() : ""
                , e = void 0 !== b;
              return Ea(this.g, function(f) {
                  for (var g = 0; g < f.length; ++g)
                      if (!(c && f[g].type != d || e && f[g].capture != b))
                          return !0;
                  return !1
              })
          }
          ;
          var Cd = function(a, b, c, d) {
              for (var e = 0; e < a.length; ++e) {
                  var f = a[e];
                  if (!f.Oc && f.listener == b && f.capture == !!c && f.ge == d)
                      return e
              }
              return -1
          };
          var Fd, Gd, Hd, Kd, Md, Nd, Od, Sd, Jd;
          Fd = "closure_lm_" + (1E6 * Math.random() | 0);
          Gd = {};
          Hd = 0;
          _.F = function(a, b, c, d, e) {
              if (d && d.once)
                  return _.Id(a, b, c, d, e);
              if (Array.isArray(b)) {
                  for (var f = 0; f < b.length; f++)
                      _.F(a, b[f], c, d, e);
                  return null
              }
              c = Jd(c);
              return _.xd(a) ? a.D(b, c, _.ya(d) ? !!d.capture : !!d, e) : Kd(a, b, c, !1, d, e)
          }
          ;
          Kd = function(a, b, c, d, e, f) {
              if (!b)
                  throw Error("B");
              var g = _.ya(e) ? !!e.capture : !!e
                , k = _.Ld(a);
              k || (a[Fd] = k = new Bd(a));
              c = k.add(b, c, d, g, f);
              if (c.proxy)
                  return c;
              d = Md();
              c.proxy = d;
              d.src = a;
              d.listener = c;
              if (a.addEventListener)
                  td || (e = g),
                  void 0 === e && (e = !1),
                  a.addEventListener(b.toString(), d, e);
              else if (a.attachEvent)
                  a.attachEvent(Nd(b.toString()), d);
              else if (a.addListener && a.removeListener)
                  a.addListener(d);
              else
                  throw Error("C");
              Hd++;
              return c
          }
          ;
          Md = function() {
              var a = Od
                , b = function(c) {
                  return a.call(b.src, b.listener, c)
              };
              return b
          }
          ;
          _.Id = function(a, b, c, d, e) {
              if (Array.isArray(b)) {
                  for (var f = 0; f < b.length; f++)
                      _.Id(a, b[f], c, d, e);
                  return null
              }
              c = Jd(c);
              return _.xd(a) ? a.Wb(b, c, _.ya(d) ? !!d.capture : !!d, e) : Kd(a, b, c, !0, d, e)
          }
          ;
          _.Pd = function(a, b, c, d, e) {
              if (Array.isArray(b))
                  for (var f = 0; f < b.length; f++)
                      _.Pd(a, b[f], c, d, e);
              else
                  d = _.ya(d) ? !!d.capture : !!d,
                  c = Jd(c),
                  _.xd(a) ? a.ra(b, c, d, e) : a && (a = _.Ld(a)) && (b = a.md(b, c, d, e)) && _.Qd(b)
          }
          ;
          _.Qd = function(a) {
              if ("number" === typeof a || !a || a.Oc)
                  return !1;
              var b = a.src;
              if (_.xd(b))
                  return Dd(b.lb, a);
              var c = a.type
                , d = a.proxy;
              b.removeEventListener ? b.removeEventListener(c, d, a.capture) : b.detachEvent ? b.detachEvent(Nd(c), d) : b.addListener && b.removeListener && b.removeListener(d);
              Hd--;
              (c = _.Ld(b)) ? (Dd(c, a),
              0 == c.h && (c.src = null,
              b[Fd] = null)) : Ad(a);
              return !0
          }
          ;
          Nd = function(a) {
              return a in Gd ? Gd[a] : Gd[a] = "on" + a
          }
          ;
          _.Rd = function(a, b) {
              var c = a.listener
                , d = a.ge || a.src;
              a.Pd && _.Qd(a);
              return c.call(d, b)
          }
          ;
          Od = function(a, b) {
              return a.Oc ? !0 : _.Rd(a, new _.vd(b,this))
          }
          ;
          _.Ld = function(a) {
              a = a[Fd];
              return a instanceof Bd ? a : null
          }
          ;
          Sd = "__closure_events_fn_" + (1E9 * Math.random() >>> 0);
          Jd = function(a) {
              if ("function" === typeof a)
                  return a;
              a[Sd] || (a[Sd] = function(b) {
                  return a.handleEvent(b)
              }
              );
              return a[Sd]
          }
          ;
          _.H = function() {
              _.x.call(this);
              this.lb = new Bd(this);
              this.Fi = this;
              this.Rf = null
          }
          ;
          _.w(_.H, _.x);
          _.H.prototype[wd] = !0;
          _.h = _.H.prototype;
          _.h.Yd = function() {
              return this.Rf
          }
          ;
          _.h.te = function(a) {
              this.Rf = a
          }
          ;
          _.h.addEventListener = function(a, b, c, d) {
              _.F(this, a, b, c, d)
          }
          ;
          _.h.removeEventListener = function(a, b, c, d) {
              _.Pd(this, a, b, c, d)
          }
          ;
          _.h.dispatchEvent = function(a) {
              var b, c = this.Yd();
              if (c)
                  for (b = []; c; c = c.Yd())
                      b.push(c);
              c = this.Fi;
              var d = a.type || a;
              if ("string" === typeof a)
                  a = new _.sd(a,c);
              else if (a instanceof _.sd)
                  a.target = a.target || c;
              else {
                  var e = a;
                  a = new _.sd(d,c);
                  Ha(a, e)
              }
              e = !0;
              if (b)
                  for (var f = b.length - 1; !a.h && 0 <= f; f--) {
                      var g = a.currentTarget = b[f];
                      e = _.Td(g, d, !0, a) && e
                  }
              a.h || (g = a.currentTarget = c,
              e = _.Td(g, d, !0, a) && e,
              a.h || (e = _.Td(g, d, !1, a) && e));
              if (b)
                  for (f = 0; !a.h && f < b.length; f++)
                      g = a.currentTarget = b[f],
                      e = _.Td(g, d, !1, a) && e;
              return e
          }
          ;
          _.h.L = function() {
              _.H.G.L.call(this);
              this.lb && _.Ed(this.lb);
              this.Rf = null
          }
          ;
          _.h.D = function(a, b, c, d) {
              return this.lb.add(String(a), b, !1, c, d)
          }
          ;
          _.h.Wb = function(a, b, c, d) {
              return this.lb.add(String(a), b, !0, c, d)
          }
          ;
          _.h.ra = function(a, b, c, d) {
              return this.lb.remove(String(a), b, c, d)
          }
          ;
          _.Td = function(a, b, c, d) {
              b = a.lb.g[String(b)];
              if (!b)
                  return !0;
              b = b.concat();
              for (var e = !0, f = 0; f < b.length; ++f) {
                  var g = b[f];
                  if (g && !g.Oc && g.capture == c) {
                      var k = g.listener
                        , l = g.ge || g.src;
                      g.Pd && Dd(a.lb, g);
                      e = !1 !== k.call(l, d) && e
                  }
              }
              return e && !d.defaultPrevented
          }
          ;
          _.H.prototype.md = function(a, b, c, d) {
              return this.lb.md(String(a), b, c, d)
          }
          ;
          _.H.prototype.hasListener = function(a, b) {
              return this.lb.hasListener(void 0 !== a ? String(a) : void 0, b)
          }
          ;
          var Ud = function(a) {
              _.H.call(this);
              this.g = a || window;
              this.h = _.F(this.g, "resize", this.l, !1, this);
              this.j = _.ad(this.g || window)
          };
          _.w(Ud, _.H);
          Ud.prototype.L = function() {
              Ud.G.L.call(this);
              this.h && (_.Qd(this.h),
              this.h = null);
              this.j = this.g = null
          }
          ;
          Ud.prototype.l = function() {
              var a = _.ad(this.g || window);
              _.Sc(a, this.j) || (this.j = a,
              this.dispatchEvent("resize"))
          }
          ;
          var Vd = function(a) {
              _.H.call(this);
              this.j = a ? _.pd(a) : window;
              this.o = 1.5 <= this.j.devicePixelRatio ? 2 : 1;
              this.h = (0,
              _.v)(this.s, this);
              this.l = null;
              (this.g = this.j.matchMedia ? this.j.matchMedia("(min-resolution: 1.5dppx), (-webkit-min-device-pixel-ratio: 1.5)") : null) && "function" !== typeof this.g.addListener && "function" !== typeof this.g.addEventListener && (this.g = null)
          };
          _.w(Vd, _.H);
          Vd.prototype.start = function() {
              var a = this;
              this.g && ("function" === typeof this.g.addEventListener ? (this.g.addEventListener("change", this.h),
              this.l = function() {
                  a.g.removeEventListener("change", a.h)
              }
              ) : (this.g.addListener(this.h),
              this.l = function() {
                  a.g.removeListener(a.h)
              }
              ))
          }
          ;
          Vd.prototype.s = function() {
              var a = 1.5 <= this.j.devicePixelRatio ? 2 : 1;
              this.o != a && (this.o = a,
              this.dispatchEvent("a"))
          }
          ;
          Vd.prototype.L = function() {
              this.l && this.l();
              Vd.G.L.call(this)
          }
          ;
          var Wd = function(a, b) {
              _.x.call(this);
              this.o = a;
              if (b) {
                  if (this.l)
                      throw Error("D");
                  this.l = b;
                  this.g = _.E(b);
                  this.h = new Ud(_.bd(b));
                  this.h.te(this.o.h());
                  this.j = new Vd(this.g);
                  this.j.start()
              }
          };
          _.w(Wd, _.x);
          Wd.prototype.L = function() {
              this.g = this.l = null;
              this.h && (this.h.O(),
              this.h = null);
              _.ca(this.j);
              this.j = null
          }
          ;
          qa(Ob, Wd);
          var Xd = function(a, b) {
              this.l = a;
              this.j = b;
              this.h = 0;
              this.g = null
          };
          Xd.prototype.get = function() {
              if (0 < this.h) {
                  this.h--;
                  var a = this.g;
                  this.g = a.next;
                  a.next = null
              } else
                  a = this.l();
              return a
          }
          ;
          var Yd = function(a, b) {
              a.j(b);
              100 > a.h && (a.h++,
              b.next = a.g,
              a.g = b)
          };
          var Zd, $d = function() {
              var a = _.p.MessageChannel;
              "undefined" === typeof a && "undefined" !== typeof window && window.postMessage && window.addEventListener && !_.t("Presto") && (a = function() {
                  var e = _.cd(document, "IFRAME");
                  e.style.display = "none";
                  document.documentElement.appendChild(e);
                  var f = e.contentWindow;
                  e = f.document;
                  e.open();
                  e.close();
                  var g = "callImmediate" + Math.random()
                    , k = "file:" == f.location.protocol ? "*" : f.location.protocol + "//" + f.location.host;
                  e = (0,
                  _.v)(function(l) {
                      if (("*" == k || l.origin == k) && l.data == g)
                          this.port1.onmessage()
                  }, this);
                  f.addEventListener("message", e, !1);
                  this.port1 = {};
                  this.port2 = {
                      postMessage: function() {
                          f.postMessage(g, k)
                      }
                  }
              }
              );
              if ("undefined" !== typeof a && !_.t("Trident") && !_.t("MSIE")) {
                  var b = new a
                    , c = {}
                    , d = c;
                  b.port1.onmessage = function() {
                      if (void 0 !== c.next) {
                          c = c.next;
                          var e = c.Ig;
                          c.Ig = null;
                          e()
                      }
                  }
                  ;
                  return function(e) {
                      d.next = {
                          Ig: e
                      };
                      d = d.next;
                      b.port2.postMessage(0)
                  }
              }
              return function(e) {
                  _.p.setTimeout(e, 0)
              }
          };
          var ae = function() {
              this.h = this.g = null
          };
          ae.prototype.add = function(a, b) {
              var c = be.get();
              c.set(a, b);
              this.h ? this.h.next = c : this.g = c;
              this.h = c
          }
          ;
          ae.prototype.remove = function() {
              var a = null;
              this.g && (a = this.g,
              this.g = this.g.next,
              this.g || (this.h = null),
              a.next = null);
              return a
          }
          ;
          var be = new Xd(function() {
              return new ce
          }
          ,function(a) {
              return a.reset()
          }
          )
            , ce = function() {
              this.next = this.g = this.h = null
          };
          ce.prototype.set = function(a, b) {
              this.h = a;
              this.g = b;
              this.next = null
          }
          ;
          ce.prototype.reset = function() {
              this.next = this.g = this.h = null
          }
          ;
          var de, ee = !1, fe = new ae, he = function(a, b) {
              de || ge();
              ee || (de(),
              ee = !0);
              fe.add(a, b)
          }, ge = function() {
              if (_.p.Promise && _.p.Promise.resolve) {
                  var a = _.p.Promise.resolve(void 0);
                  de = function() {
                      a.then(ie)
                  }
              } else
                  de = function() {
                      var b = ie;
                      "function" !== typeof _.p.setImmediate || _.p.Window && _.p.Window.prototype && !_.t("Edge") && _.p.Window.prototype.setImmediate == _.p.setImmediate ? (Zd || (Zd = $d()),
                      Zd(b)) : _.p.setImmediate(b)
                  }
          }, ie = function() {
              for (var a; a = fe.remove(); ) {
                  try {
                      a.h.call(a.g)
                  } catch (b) {
                      ba(b)
                  }
                  Yd(be, a)
              }
              ee = !1
          };
          var je = function(a) {
              if (!a)
                  return !1;
              try {
                  return !!a.$goog_Thenable
              } catch (b) {
                  return !1
              }
          };
          var me, re, ve, we, ue, se;
          _.le = function(a, b) {
              this.g = 0;
              this.C = void 0;
              this.l = this.h = this.j = null;
              this.o = this.s = !1;
              if (a != _.ub)
                  try {
                      var c = this;
                      a.call(b, function(d) {
                          _.ke(c, 2, d)
                      }, function(d) {
                          _.ke(c, 3, d)
                      })
                  } catch (d) {
                      _.ke(this, 3, d)
                  }
          }
          ;
          me = function() {
              this.next = this.j = this.h = this.l = this.g = null;
              this.o = !1
          }
          ;
          me.prototype.reset = function() {
              this.j = this.h = this.l = this.g = null;
              this.o = !1
          }
          ;
          var ne = new Xd(function() {
              return new me
          }
          ,function(a) {
              a.reset()
          }
          )
            , oe = function(a, b, c) {
              var d = ne.get();
              d.l = a;
              d.h = b;
              d.j = c;
              return d
          };
          _.le.prototype.then = function(a, b, c) {
              return _.pe(this, "function" === typeof a ? a : null, "function" === typeof b ? b : null, c)
          }
          ;
          _.le.prototype.$goog_Thenable = !0;
          _.le.prototype.cancel = function(a) {
              if (0 == this.g) {
                  var b = new qe(a);
                  he(function() {
                      re(this, b)
                  }, this)
              }
          }
          ;
          re = function(a, b) {
              if (0 == a.g)
                  if (a.j) {
                      var c = a.j;
                      if (c.h) {
                          for (var d = 0, e = null, f = null, g = c.h; g && (g.o || (d++,
                          g.g == a && (e = g),
                          !(e && 1 < d))); g = g.next)
                              e || (f = g);
                          e && (0 == c.g && 1 == d ? re(c, b) : (f ? (d = f,
                          d.next == c.l && (c.l = d),
                          d.next = d.next.next) : se(c),
                          te(c, e, 3, b)))
                      }
                      a.j = null
                  } else
                      _.ke(a, 3, b)
          }
          ;
          ve = function(a, b) {
              a.h || 2 != a.g && 3 != a.g || ue(a);
              a.l ? a.l.next = b : a.h = b;
              a.l = b
          }
          ;
          _.pe = function(a, b, c, d) {
              var e = oe(null, null, null);
              e.g = new _.le(function(f, g) {
                  e.l = b ? function(k) {
                      try {
                          var l = b.call(d, k);
                          f(l)
                      } catch (m) {
                          g(m)
                      }
                  }
                  : f;
                  e.h = c ? function(k) {
                      try {
                          var l = c.call(d, k);
                          void 0 === l && k instanceof qe ? g(k) : f(l)
                      } catch (m) {
                          g(m)
                      }
                  }
                  : g
              }
              );
              e.g.j = a;
              ve(a, e);
              return e.g
          }
          ;
          _.le.prototype.J = function(a) {
              this.g = 0;
              _.ke(this, 2, a)
          }
          ;
          _.le.prototype.N = function(a) {
              this.g = 0;
              _.ke(this, 3, a)
          }
          ;
          _.ke = function(a, b, c) {
              if (0 == a.g) {
                  a === c && (b = 3,
                  c = new TypeError("E"));
                  a.g = 1;
                  a: {
                      var d = c
                        , e = a.J
                        , f = a.N;
                      if (d instanceof _.le) {
                          ve(d, oe(e || _.ub, f || null, a));
                          var g = !0
                      } else if (je(d))
                          d.then(e, f, a),
                          g = !0;
                      else {
                          if (_.ya(d))
                              try {
                                  var k = d.then;
                                  if ("function" === typeof k) {
                                      we(d, k, e, f, a);
                                      g = !0;
                                      break a
                                  }
                              } catch (l) {
                                  f.call(a, l);
                                  g = !0;
                                  break a
                              }
                          g = !1
                      }
                  }
                  g || (a.C = c,
                  a.g = b,
                  a.j = null,
                  ue(a),
                  3 != b || c instanceof qe || xe(a, c))
              }
          }
          ;
          we = function(a, b, c, d, e) {
              var f = !1
                , g = function(l) {
                  f || (f = !0,
                  c.call(e, l))
              }
                , k = function(l) {
                  f || (f = !0,
                  d.call(e, l))
              };
              try {
                  b.call(a, g, k)
              } catch (l) {
                  k(l)
              }
          }
          ;
          ue = function(a) {
              a.s || (a.s = !0,
              he(a.B, a))
          }
          ;
          se = function(a) {
              var b = null;
              a.h && (b = a.h,
              a.h = b.next,
              b.next = null);
              a.h || (a.l = null);
              return b
          }
          ;
          _.le.prototype.B = function() {
              for (var a; a = se(this); )
                  te(this, a, this.g, this.C);
              this.s = !1
          }
          ;
          var te = function(a, b, c, d) {
              if (3 == c && b.h && !b.o)
                  for (; a && a.o; a = a.j)
                      a.o = !1;
              if (b.g)
                  b.g.j = null,
                  ye(b, c, d);
              else
                  try {
                      b.o ? b.l.call(b.j) : ye(b, c, d)
                  } catch (e) {
                      ze.call(null, e)
                  }
              Yd(ne, b)
          }
            , ye = function(a, b, c) {
              2 == b ? a.l.call(a.j, c) : a.h && a.h.call(a.j, c)
          }
            , xe = function(a, b) {
              a.o = !0;
              he(function() {
                  a.o && ze.call(null, b)
              })
          }
            , ze = ba
            , qe = function(a) {
              _.aa.call(this, a)
          };
          _.w(qe, _.aa);
          qe.prototype.name = "cancel";
          /*

Copyright 2005, 2007 Bob Ippolito. All Rights Reserved.
Copyright The Closure Library Authors.
SPDX-License-Identifier: MIT
*/
          var Ae = function(a, b) {
              this.s = [];
              this.U = a;
              this.S = b || null;
              this.l = this.g = !1;
              this.j = void 0;
              this.N = this.ma = this.B = !1;
              this.C = 0;
              this.h = null;
              this.o = 0
          };
          _.w(Ae, Ja);
          Ae.prototype.cancel = function(a) {
              if (this.g)
                  this.j instanceof Ae && this.j.cancel();
              else {
                  if (this.h) {
                      var b = this.h;
                      delete this.h;
                      a ? b.cancel(a) : (b.o--,
                      0 >= b.o && b.cancel())
                  }
                  this.U ? this.U.call(this.S, this) : this.N = !0;
                  this.g || this.J(new Be(this))
              }
          }
          ;
          Ae.prototype.K = function(a, b) {
              this.B = !1;
              Ce(this, a, b)
          }
          ;
          var Ce = function(a, b, c) {
              a.g = !0;
              a.j = c;
              a.l = !b;
              De(a)
          }
            , Fe = function(a) {
              if (a.g) {
                  if (!a.N)
                      throw new Ee(a);
                  a.N = !1
              }
          };
          Ae.prototype.callback = function(a) {
              Fe(this);
              Ce(this, !0, a)
          }
          ;
          Ae.prototype.J = function(a) {
              Fe(this);
              Ce(this, !1, a)
          }
          ;
          var He = function(a, b, c) {
              Ge(a, b, null, c)
          }
            , Ie = function(a, b, c) {
              Ge(a, null, b, c)
          }
            , Ge = function(a, b, c, d) {
              a.s.push([b, c, d]);
              a.g && De(a)
          };
          Ae.prototype.then = function(a, b, c) {
              var d, e, f = new _.le(function(g, k) {
                  e = g;
                  d = k
              }
              );
              Ge(this, e, function(g) {
                  g instanceof Be ? f.cancel() : d(g);
                  return Je
              }, this);
              return f.then(a, b, c)
          }
          ;
          Ae.prototype.$goog_Thenable = !0;
          var Ke = function(a, b) {
              b instanceof Ae ? He(a, (0,
              _.v)(b.W, b)) : He(a, function() {
                  return b
              })
          };
          Ae.prototype.W = function(a) {
              var b = new Ae;
              Ge(this, b.callback, b.J, b);
              a && (b.h = this,
              this.o++);
              return b
          }
          ;
          var Le = function(a) {
              return _.Sb(a.s, function(b) {
                  return "function" === typeof b[1]
              })
          }
            , Je = {}
            , De = function(a) {
              if (a.C && a.g && Le(a)) {
                  var b = a.C
                    , c = Me[b];
                  c && (_.p.clearTimeout(c.g),
                  delete Me[b]);
                  a.C = 0
              }
              a.h && (a.h.o--,
              delete a.h);
              b = a.j;
              for (var d = c = !1; a.s.length && !a.B; ) {
                  var e = a.s.shift()
                    , f = e[0]
                    , g = e[1];
                  e = e[2];
                  if (f = a.l ? g : f)
                      try {
                          var k = f.call(e || a.S, b);
                          k === Je && (k = void 0);
                          void 0 !== k && (a.l = a.l && (k == b || k instanceof Error),
                          a.j = b = k);
                          if (je(b) || "function" === typeof _.p.Promise && b instanceof _.p.Promise)
                              d = !0,
                              a.B = !0
                      } catch (l) {
                          b = l,
                          a.l = !0,
                          Le(a) || (c = !0)
                      }
              }
              a.j = b;
              d && (k = (0,
              _.v)(a.K, a, !0),
              d = (0,
              _.v)(a.K, a, !1),
              b instanceof Ae ? (Ge(b, k, d),
              b.ma = !0) : b.then(k, d));
              c && (b = new Ne(b),
              Me[b.g] = b,
              a.C = b.g)
          }
            , Ee = function() {
              _.aa.call(this)
          };
          _.w(Ee, _.aa);
          Ee.prototype.message = "Deferred has already fired";
          Ee.prototype.name = "AlreadyCalledError";
          var Be = function() {
              _.aa.call(this)
          };
          _.w(Be, _.aa);
          Be.prototype.message = "Deferred was canceled";
          Be.prototype.name = "CanceledError";
          var Ne = function(a) {
              this.g = _.p.setTimeout((0,
              _.v)(this.j, this), 0);
              this.h = a
          };
          Ne.prototype.j = function() {
              delete Me[this.g];
              throw this.h;
          }
          ;
          var Me = {};
          var Oe = function(a, b) {
              this.type = a;
              this.status = b
          };
          Oe.prototype.toString = function() {
              return Pe(this) + " (" + (void 0 != this.status ? this.status : "?") + ")"
          }
          ;
          var Pe = function(a) {
              switch (a.type) {
              case Oe.g.Dg:
                  return "Unauthorized";
              case Oe.g.ng:
                  return "Consecutive load failures";
              case Oe.g.TIMEOUT:
                  return "Timed out";
              case Oe.g.yg:
                  return "Out of date module id";
              case Oe.g.Ce:
                  return "Init error";
              default:
                  return "Unknown failure type " + a.type
              }
          };
          hb.bb = Oe;
          hb.bb.g = {
              Dg: 0,
              ng: 1,
              TIMEOUT: 2,
              yg: 3,
              Ce: 4
          };
          var Qe = function() {
              Nb.call(this);
              this.g = {};
              this.j = [];
              this.l = [];
              this.S = [];
              this.h = [];
              this.C = [];
              this.s = {};
              this.ma = {};
              this.o = this.J = new Kb([],"");
              this.W = null;
              this.K = new Ae;
              this.U = !1;
              this.N = 0;
              this.X = this.ba = this.Z = !1
          };
          _.w(Qe, Nb);
          var Re = function(a, b) {
              _.aa.call(this, "Error loading " + a + ": " + b)
          };
          _.w(Re, _.aa);
          _.h = Qe.prototype;
          _.h.Kh = function(a) {
              this.U = a
          }
          ;
          _.h.bg = function(a, b) {
              if (!(this instanceof Qe))
                  this.bg(a, b);
              else if ("string" === typeof a) {
                  a = a.split("/");
                  for (var c = [], d = 0; d < a.length; d++) {
                      var e = a[d].split(":")
                        , f = e[0];
                      if (e[1]) {
                          e = e[1].split(",");
                          for (var g = 0; g < e.length; g++)
                              e[g] = c[parseInt(e[g], 36)]
                      } else
                          e = [];
                      c.push(f);
                      this.g[f] ? (f = this.g[f].Dc(),
                      f != e && f.splice.apply(f, [0, f.length].concat(e instanceof Array ? e : _.Ta(_.Sa(e))))) : this.g[f] = new Kb(e,f)
                  }
                  b && b.length ? (xa(this.j, b),
                  this.W = _.ra(b)) : this.K.g || this.K.callback();
                  Se(this)
              }
          }
          ;
          _.h.Ih = function(a, b) {
              if (this.s[a]) {
                  delete this.s[a][b];
                  for (var c in this.s[a])
                      return;
                  delete this.s[a]
              }
          }
          ;
          _.h.cg = function(a) {
              Qe.G.cg.call(this, a);
              Se(this)
          }
          ;
          _.h.pg = function() {
              return 0 < this.j.length
          }
          ;
          _.h.ph = function() {
              return 0 < this.C.length
          }
          ;
          var Ue = function(a) {
              var b = a.pg();
              b != a.Z && (Te(a, b ? "active" : "idle"),
              a.Z = b);
              b = a.ph();
              b != a.ba && (Te(a, b ? "userActive" : "userIdle"),
              a.ba = b)
          }
            , Xe = function(a, b, c) {
              var d = [];
              Aa(b, d);
              b = [];
              for (var e = {}, f = 0; f < d.length; f++) {
                  var g = d[f]
                    , k = a.g[g];
                  if (!k)
                      throw Error("F`" + g);
                  var l = new Ae;
                  e[g] = l;
                  k.g ? l.callback(a.La) : (Ve(a, g, k, !!c, l),
                  We(a, g) || b.push(g))
              }
              0 < b.length && (0 === a.j.length ? a.T(b) : (a.h.push(b),
              Ue(a)));
              return e
          }
            , Ve = function(a, b, c, d, e) {
              c.j.push(new Jb(e.callback,e));
              Lb(c, function(f) {
                  e.J(new Re(b,f))
              });
              We(a, b) ? d && (_.ta(a.C, b) || a.C.push(b),
              Ue(a)) : d && (_.ta(a.C, b) || a.C.push(b))
          };
          Qe.prototype.T = function(a, b, c) {
              var d = this;
              b || (this.N = 0);
              var e = Ye(this, a);
              this.j = e;
              this.l = this.U ? a : _.wa(e);
              Ue(this);
              if (0 !== e.length) {
                  this.S.push.apply(this.S, e);
                  if (0 < Object.keys(this.s).length && !this.B.S)
                      throw Error("G");
                  a = (0,
                  _.v)(this.B.N, this.B, _.wa(e), this.g, {
                      Pi: this.s,
                      Ri: !!c,
                      Of: function(f) {
                          var g = d.l;
                          f = null != f ? f : void 0;
                          d.N++;
                          d.l = g;
                          e.forEach(_.ob(_.va, d.S), d);
                          401 == f ? (Ze(d, new hb.bb(hb.bb.g.Dg,f)),
                          d.h.length = 0) : 410 == f ? ($e(d, new hb.bb(hb.bb.g.yg,f)),
                          bf(d)) : 3 <= d.N ? ($e(d, new hb.bb(hb.bb.g.ng,f)),
                          bf(d)) : d.T(d.l, !0, 8001 == f)
                      },
                      ak: (0,
                      _.v)(this.fa, this)
                  });
                  (b = 5E3 * Math.pow(this.N, 2)) ? _.p.setTimeout(a, b) : a()
              }
          }
          ;
          var Ye = function(a, b) {
              b = b.filter(function(e) {
                  return a.g[e].g ? (_.p.setTimeout(function() {
                      return Error("H`" + e)
                  }, 0),
                  !1) : !0
              });
              for (var c = [], d = 0; d < b.length; d++)
                  c = c.concat(cf(a, b[d]));
              Aa(c);
              return !a.U && 1 < c.length ? (b = c.shift(),
              a.h = c.map(function(e) {
                  return [e]
              }).concat(a.h),
              [b]) : c
          }
            , cf = function(a, b) {
              var c = Ia(a.S)
                , d = [];
              c[b] || d.push(b);
              b = [b];
              for (var e = 0; e < b.length; e++)
                  for (var f = a.g[b[e]].Dc(), g = f.length - 1; 0 <= g; g--) {
                      var k = f[g];
                      a.g[k].g || c[k] || (d.push(k),
                      b.push(k))
                  }
              d.reverse();
              Aa(d);
              return d
          }
            , Se = function(a) {
              a.o == a.J && (a.o = null,
              a.J.onLoad((0,
              _.v)(a.Vg, a)) && Ze(a, new hb.bb(hb.bb.g.Ce)),
              Ue(a))
          }
            , na = function(a) {
              if (a.o) {
                  var b = a.o.mb()
                    , c = [];
                  if (a.s[b]) {
                      for (var d = _.Sa(Object.keys(a.s[b])), e = d.next(); !e.done; e = d.next()) {
                          e = e.value;
                          var f = a.g[e];
                          f && !f.g && (a.Ih(b, e),
                          c.push(e))
                      }
                      Xe(a, c)
                  }
                  a.zb() || (a.g[b].onLoad((0,
                  _.v)(a.Vg, a)) && Ze(a, new hb.bb(hb.bb.g.Ce)),
                  _.va(a.C, b),
                  _.va(a.j, b),
                  0 === a.j.length && bf(a),
                  a.W && b == a.W && (a.K.g || a.K.callback()),
                  Ue(a),
                  a.o = null)
              }
          }
            , We = function(a, b) {
              if (_.ta(a.j, b))
                  return !0;
              for (var c = 0; c < a.h.length; c++)
                  if (_.ta(a.h[c], b))
                      return !0;
              return !1
          };
          Qe.prototype.load = function(a, b) {
              return Xe(this, [a], b)[a]
          }
          ;
          var la = function(a) {
              var b = _.fa;
              b.o && "synthetic_module_overhead" === b.o.mb() && (na(b),
              delete b.g.synthetic_module_overhead);
              b.g[a] && df(b, b.g[a].Dc() || [], function(c) {
                  c.g = new Ib;
                  _.va(b.j, c.mb())
              }, function(c) {
                  return !c.g
              });
              b.o = b.g[a]
          };
          Qe.prototype.fa = function() {
              $e(this, new hb.bb(hb.bb.g.TIMEOUT));
              bf(this)
          }
          ;
          var $e = function(a, b) {
              1 < a.l.length ? a.h = a.l.map(function(c) {
                  return [c]
              }).concat(a.h) : Ze(a, b)
          }
            , Ze = function(a, b) {
              var c = a.l;
              a.j.length = 0;
              for (var d = [], e = 0; e < a.h.length; e++) {
                  var f = a.h[e].filter(function(l) {
                      var m = cf(this, l);
                      return _.Sb(c, function(n) {
                          return _.ta(m, n)
                      })
                  }, a);
                  xa(d, f)
              }
              for (e = 0; e < c.length; e++)
                  _.ua(d, c[e]);
              for (e = 0; e < d.length; e++) {
                  for (f = 0; f < a.h.length; f++)
                      _.va(a.h[f], d[e]);
                  _.va(a.C, d[e])
              }
              var g = a.ma.error;
              if (g)
                  for (e = 0; e < g.length; e++) {
                      var k = g[e];
                      for (f = 0; f < d.length; f++)
                          k("error", d[f], b)
                  }
              for (e = 0; e < c.length; e++)
                  a.g[c[e]] && a.g[c[e]].Of(b);
              a.l.length = 0;
              Ue(a)
          }
            , bf = function(a) {
              for (; a.h.length; ) {
                  var b = a.h.shift().filter(function(c) {
                      return !this.g[c].g
                  }, a);
                  if (0 < b.length) {
                      a.T(b);
                      return
                  }
              }
              Ue(a)
          }
            , Te = function(a, b) {
              a = a.ma[b];
              for (var c = 0; a && c < a.length; c++)
                  a[c](b)
          }
            , df = function(a, b, c, d, e) {
              d = void 0 === d ? function() {
                  return !0
              }
              : d;
              e = void 0 === e ? {} : e;
              b = _.Sa(b);
              for (var f = b.next(); !f.done; f = b.next()) {
                  f = f.value;
                  var g = a.g[f];
                  !e[f] && d(g) && (e[f] = !0,
                  df(a, g.Dc() || [], c, d, e),
                  c(g))
              }
          };
          Qe.prototype.O = function() {
              ea(_.Fa(this.g), this.J);
              this.g = {};
              this.j = [];
              this.l = [];
              this.C = [];
              this.h = [];
              this.ma = {};
              this.X = !0
          }
          ;
          Qe.prototype.zb = function() {
              return this.X
          }
          ;
          _.ha = function() {
              return new Qe
          }
          ;
          var ef = function(a, b) {
              this.g = a[_.p.Symbol.iterator]();
              this.h = b
          };
          ef.prototype[Symbol.iterator] = function() {
              return this
          }
          ;
          ef.prototype.next = function() {
              var a = this.g.next();
              return {
                  value: a.done ? void 0 : this.h.call(void 0, a.value),
                  done: a.done
              }
          }
          ;
          var ff = function(a, b) {
              return new ef(a,b)
          };
          _.gf = function() {}
          ;
          _.gf.prototype.next = function() {
              return _.hf
          }
          ;
          _.hf = {
              done: !0,
              value: void 0
          };
          _.gf.prototype.Ja = function() {
              return this
          }
          ;
          var mf = function(a) {
              if (a instanceof jf || a instanceof kf || a instanceof lf)
                  return a;
              if ("function" == typeof a.next)
                  return new jf(function() {
                      return a
                  }
                  );
              if ("function" == typeof a[Symbol.iterator])
                  return new jf(function() {
                      return a[Symbol.iterator]()
                  }
                  );
              if ("function" == typeof a.Ja)
                  return new jf(function() {
                      return a.Ja()
                  }
                  );
              throw Error("J");
          }
            , jf = function(a) {
              this.g = a
          };
          jf.prototype.Ja = function() {
              return new kf(this.g())
          }
          ;
          jf.prototype[Symbol.iterator] = function() {
              return new lf(this.g())
          }
          ;
          jf.prototype.h = function() {
              return new lf(this.g())
          }
          ;
          var kf = function(a) {
              this.g = a
          };
          _.u(kf, _.gf);
          kf.prototype.next = function() {
              return this.g.next()
          }
          ;
          kf.prototype[Symbol.iterator] = function() {
              return new lf(this.g)
          }
          ;
          kf.prototype.h = function() {
              return new lf(this.g)
          }
          ;
          var lf = function(a) {
              jf.call(this, function() {
                  return a
              });
              this.j = a
          };
          _.u(lf, jf);
          lf.prototype.next = function() {
              return this.j.next()
          }
          ;
          _.nf = function(a, b) {
              this.h = {};
              this.g = [];
              this.j = this.size = 0;
              var c = arguments.length;
              if (1 < c) {
                  if (c % 2)
                      throw Error("y");
                  for (var d = 0; d < c; d += 2)
                      this.set(arguments[d], arguments[d + 1])
              } else if (a)
                  if (a instanceof _.nf)
                      for (c = a.Vb(),
                      d = 0; d < c.length; d++)
                          this.set(c[d], a.get(c[d]));
                  else
                      for (d in a)
                          this.set(d, a[d])
          }
          ;
          _.nf.prototype.nb = function() {
              of(this);
              for (var a = [], b = 0; b < this.g.length; b++)
                  a.push(this.h[this.g[b]]);
              return a
          }
          ;
          _.nf.prototype.Vb = function() {
              of(this);
              return this.g.concat()
          }
          ;
          _.pf = function(a, b) {
              return a.has(b)
          }
          ;
          _.nf.prototype.has = function(a) {
              return qf(this.h, a)
          }
          ;
          _.nf.prototype.pc = function() {
              return 0 == this.size
          }
          ;
          _.nf.prototype.remove = function(a) {
              qf(this.h, a) ? (delete this.h[a],
              --this.size,
              this.j++,
              this.g.length > 2 * this.size && of(this),
              a = !0) : a = !1;
              return a
          }
          ;
          var of = function(a) {
              if (a.size != a.g.length) {
                  for (var b = 0, c = 0; b < a.g.length; ) {
                      var d = a.g[b];
                      qf(a.h, d) && (a.g[c++] = d);
                      b++
                  }
                  a.g.length = c
              }
              if (a.size != a.g.length) {
                  var e = {};
                  for (c = b = 0; b < a.g.length; )
                      d = a.g[b],
                      qf(e, d) || (a.g[c++] = d,
                      e[d] = 1),
                      b++;
                  a.g.length = c
              }
          };
          _.h = _.nf.prototype;
          _.h.get = function(a, b) {
              return qf(this.h, a) ? this.h[a] : b
          }
          ;
          _.h.set = function(a, b) {
              qf(this.h, a) || (this.size += 1,
              this.g.push(a),
              this.j++);
              this.h[a] = b
          }
          ;
          _.h.forEach = function(a, b) {
              for (var c = this.Vb(), d = 0; d < c.length; d++) {
                  var e = c[d]
                    , f = this.get(e);
                  a.call(b, f, e, this)
              }
          }
          ;
          _.h.keys = function() {
              return mf(this.Ja(!0)).h()
          }
          ;
          _.h.values = function() {
              return mf(this.Ja(!1)).h()
          }
          ;
          _.h.entries = function() {
              var a = this;
              return ff(this.keys(), function(b) {
                  return [b, a.get(b)]
              })
          }
          ;
          _.h.Ja = function(a) {
              of(this);
              var b = 0
                , c = this.j
                , d = this
                , e = new _.gf;
              e.next = function() {
                  if (c != d.j)
                      throw Error("K");
                  if (b >= d.g.length)
                      return _.hf;
                  var f = d.g[b++];
                  return {
                      value: a ? f : d.h[f],
                      done: !1
                  }
              }
              ;
              return e
          }
          ;
          var qf = function(a, b) {
              return Object.prototype.hasOwnProperty.call(a, b)
          };
          var sf;
          _.rf = function() {
              this.g = new _.nf;
              this.size = 0
          }
          ;
          sf = function(a) {
              var b = typeof a;
              return "object" == b && a || "function" == b ? "o" + _.za(a) : b.charAt(0) + a
          }
          ;
          _.h = _.rf.prototype;
          _.h.add = function(a) {
              this.g.set(sf(a), a);
              this.size = this.g.size
          }
          ;
          _.h.remove = function(a) {
              a = this.g.remove(sf(a));
              this.size = this.g.size;
              return a
          }
          ;
          _.h.pc = function() {
              return 0 === this.g.size
          }
          ;
          _.h.has = function(a) {
              return _.pf(this.g, sf(a))
          }
          ;
          _.h.contains = function(a) {
              return _.pf(this.g, sf(a))
          }
          ;
          _.h.nb = function() {
              return this.g.nb()
          }
          ;
          _.h.values = function() {
              return this.g.values()
          }
          ;
          _.h.Ja = function() {
              return this.g.Ja(!1)
          }
          ;
          _.rf.prototype[Symbol.iterator] = function() {
              return this.values()
          }
          ;
          var tf = []
            , uf = function(a) {
              function b(d) {
                  d && Rb(d, function(e, f) {
                      e[f.id] = !0;
                      return e
                  }, c.kk)
              }
              var c = {
                  kk: {},
                  index: tf.length,
                  Ul: a
              };
              b(a.g);
              b(a.j);
              tf.push(c);
              a.g && _.Pb(a.g, function(d) {
                  var e = d.id;
                  e instanceof y && d.module && (e.Uj = d.module)
              })
          };
          var vf = new y("MpJwZc","MpJwZc");
          var wf = new y("UUJqVe","UUJqVe");
          new y("Wt6vjf","Wt6vjf");
          new y("byfTOb","byfTOb");
          new y("LEikZe","LEikZe");
          new y("lsjVmc","lsjVmc");
          new y("pVbxBc");
          new y("tdUkaf");
          new y("fJuxOc");
          new y("ZtVrH");
          new y("WSziFf");
          new y("ZmXAm");
          new y("BWETze");
          new y("UBSgGf");
          new y("zZa4xc");
          new y("o1bZcd");
          new y("WwG67d");
          new y("z72MOc");
          new y("JccZRe");
          new y("amY3Td");
          new y("ABma3e");
          new y("GHAeAc","GHAeAc");
          new y("gSshPb");
          new y("klpyYe");
          new y("OPbIxb");
          new y("pg9hFd");
          new y("yu4DA");
          new y("vk3Wc");
          new y("IykvEf");
          new y("J5K1Ad");
          new y("IW8Usd");
          new y("IaqD3e");
          new y("jbDgG");
          new y("b8xKu");
          new y("d0RAGb");
          new y("AzG0ke");
          new y("J4QWB");
          new y("TuDsZ");
          new y("hdXIif");
          new y("mITR5c");
          new y("DFElXb");
          new y("NGntwf");
          new y("Bgf0ib");
          new y("Xpw1of");
          new y("v5BQle");
          new y("ofuapc");
          new y("FENZqe");
          new y("tLnxq");
          uf({
              g: [{
                  id: Ob,
                  Hi: Wd,
                  multiple: !0
              }]
          });
          var xf = {};
          var yf = new qd
            , zf = function(a, b) {
              _.sd.call(this, a, b);
              this.node = b
          };
          _.u(zf, _.sd);
          /*

SPDX-License-Identifier: Apache-2.0
*/
          var Df;
          _.Af = RegExp("^(?:([^:/?#.]+):)?(?://(?:([^\\\\/?#]*)@)?([^\\\\/?#]*?)(?::([0-9]+))?(?=[\\\\/?#]|$))?([^?#]+)?(?:\\?([^#]*))?(?:#([\\s\\S]*))?$");
          _.Bf = function(a) {
              return a ? decodeURI(a) : a
          }
          ;
          _.Cf = function(a, b) {
              if (a) {
                  a = a.split("&");
                  for (var c = 0; c < a.length; c++) {
                      var d = a[c].indexOf("=")
                        , e = null;
                      if (0 <= d) {
                          var f = a[c].substring(0, d);
                          e = a[c].substring(d + 1)
                      } else
                          f = a[c];
                      b(f, e ? _.Uc(e) : "")
                  }
              }
          }
          ;
          Df = function(a, b, c) {
              if (Array.isArray(b))
                  for (var d = 0; d < b.length; d++)
                      Df(a, String(b[d]), c);
              else
                  null != b && c.push(a + ("" === b ? "" : "=" + _.Tc(b)))
          }
          ;
          /*
Copyright The Closure Library Authors.
SPDX-License-Identifier: Apache-2.0
*/
          _.Ef = function(a, b) {
              b || _.E();
              this.j = a || null
          }
          ;
          _.Ef.prototype.ja = function(a) {
              a = a({}, this.j ? this.j.g() : {});
              this.h(null, "function" == typeof _.Ff && a instanceof _.Ff ? a.Hb : null)
          }
          ;
          _.Ef.prototype.h = function() {}
          ;
          var Gf = function(a) {
              this.h = a;
              this.j = this.h.g(wf)
          };
          Gf.prototype.g = function() {
              this.h.zb() || (this.j = this.h.g(wf));
              return this.j ? this.j.g() : {}
          }
          ;
          var Hf = function(a) {
              var b = new Gf(a);
              _.Ef.call(this, b, a.get(Ob).g);
              this.l = new _.H;
              this.o = b
          };
          _.u(Hf, _.Ef);
          Hf.prototype.g = function() {
              return this.o.g()
          }
          ;
          Hf.prototype.h = function(a, b) {
              _.Ef.prototype.h.call(this, a, b);
              this.l.dispatchEvent(new zf(yf,a,b))
          }
          ;
          qa(vf, Hf);
          uf({
              g: [{
                  id: vf,
                  Hi: Hf,
                  multiple: !0
              }]
          });
          var If = function(a, b) {
              this.defaultValue = a;
              this.type = b;
              this.value = a
          };
          If.prototype.get = function() {
              return this.value
          }
          ;
          If.prototype.set = function(a) {
              this.value = a
          }
          ;
          var Jf = function(a) {
              If.call(this, a, "b")
          };
          _.u(Jf, If);
          Jf.prototype.get = function() {
              return this.value
          }
          ;
          var Kf = function() {
              this.g = {};
              this.j = "";
              this.h = {}
          };
          Kf.prototype.toString = function() {
              var a = this.j + Lf(this);
              var b = this.h;
              var c = [], d;
              for (d in b)
                  Df(d, b[d], c);
              b = c.join("&");
              c = "";
              "" != b && (c = "?" + b);
              return a + c
          }
          ;
          var Lf = function(a) {
              var b = []
                , c = (0,
              _.v)(function(d) {
                  void 0 !== this.g[d] && b.push(d + "=" + this.g[d])
              }, a);
              "1" == Mf(a, "md") ? (c("md"),
              c("k"),
              c("ck"),
              c("am"),
              c("rs"),
              c("gssmodulesetproto")) : (c("sdch"),
              c("k"),
              c("ck"),
              c("am"),
              c("rt"),
              "d"in a.g || Nf(a, "d", "0"),
              c("d"),
              c("exm"),
              c("excm"),
              c("esmo"),
              (a.g.excm || a.g.exm) && b.push("ed=1"),
              c("im"),
              c("dg"),
              c("sm"),
              "1" == Mf(a, "br") && c("br"),
              "" !== Of(a) && c("wt"),
              c("gssmodulesetproto"),
              c("rs"),
              c("ee"),
              c("cb"),
              c("m"));
              return b.join("/")
          }
            , Mf = function(a, b) {
              return a.g[b] ? a.g[b] : null
          }
            , Nf = function(a, b, c) {
              c ? a.g[b] = c : delete a.g[b]
          }
            , Of = function(a) {
              switch (Mf(a, "wt")) {
              case "0":
                  return "0";
              case "1":
                  return "1";
              case "2":
                  return "2";
              default:
                  return ""
              }
          }
            , Rf = function(a) {
              var b = void 0 === b ? !0 : b;
              var c = Pf(a)
                , d = new Kf
                , e = c.match(_.Af)[5];
              _.yc(Qf, function(g) {
                  var k = e.match("/" + g + "=([^/]+)");
                  k && Nf(d, g, k[1])
              });
              var f = -1 != a.indexOf("_/ss/") ? "_/ss/" : "_/js/";
              d.j = a.substr(0, a.indexOf(f) + f.length);
              if (!b)
                  return d;
              (a = c.match(_.Af)[6] || null) && _.Cf(a, function(g, k) {
                  d.h[g] = k
              });
              return d
          }
            , Pf = function(a) {
              return a.startsWith("https://uberproxy-pen-redirect.corp.google.com/uberproxy/pen?url=") ? a.substr(65) : a
          }
            , Qf = {
              vl: "k",
              Pk: "ck",
              il: "m",
              Yk: "exm",
              Wk: "excm",
              Zk: "esmo",
              Gk: "am",
              ul: "rt",
              dl: "d",
              Xk: "ed",
              Dl: "sv",
              Qk: "deob",
              Nk: "cb",
              Al: "rs",
              wl: "sdch",
              el: "im",
              Rk: "dg",
              Vk: "br",
              Ml: "wt",
              al: "ee",
              Cl: "sm",
              gl: "md",
              bl: "gssmodulesetproto"
          };
          _.I = function(a) {
              _.x.call(this);
              this.h = a;
              this.g = {}
          }
          ;
          _.w(_.I, _.x);
          var Sf = [];
          _.I.prototype.D = function(a, b, c, d) {
              return Tf(this, a, b, c, d)
          }
          ;
          var Tf = function(a, b, c, d, e, f) {
              Array.isArray(c) || (c && (Sf[0] = c.toString()),
              c = Sf);
              for (var g = 0; g < c.length; g++) {
                  var k = _.F(b, c[g], d || a.handleEvent, e || !1, f || a.h || a);
                  if (!k)
                      break;
                  a.g[k.key] = k
              }
              return a
          };
          _.I.prototype.Wb = function(a, b, c, d) {
              return Uf(this, a, b, c, d)
          }
          ;
          var Uf = function(a, b, c, d, e, f) {
              if (Array.isArray(c))
                  for (var g = 0; g < c.length; g++)
                      Uf(a, b, c[g], d, e, f);
              else {
                  b = _.Id(b, c, d || a.handleEvent, e, f || a.h || a);
                  if (!b)
                      return a;
                  a.g[b.key] = b
              }
              return a
          };
          _.I.prototype.ra = function(a, b, c, d, e) {
              if (Array.isArray(b))
                  for (var f = 0; f < b.length; f++)
                      this.ra(a, b[f], c, d, e);
              else
                  c = c || this.handleEvent,
                  d = _.ya(d) ? !!d.capture : !!d,
                  e = e || this.h || this,
                  c = Jd(c),
                  d = !!d,
                  b = _.xd(a) ? a.md(b, c, d, e) : a ? (a = _.Ld(a)) ? a.md(b, c, d, e) : null : null,
                  b && (_.Qd(b),
                  delete this.g[b.key]);
              return this
          }
          ;
          _.Vf = function(a) {
              _.yc(a.g, function(b, c) {
                  this.g.hasOwnProperty(c) && _.Qd(b)
              }, a);
              a.g = {}
          }
          ;
          _.I.prototype.L = function() {
              _.I.G.L.call(this);
              _.Vf(this)
          }
          ;
          _.I.prototype.handleEvent = function() {
              throw Error("T");
          }
          ;
          var Wf = function() {};
          Wf.prototype.h = null;
          var Xf = function(a) {
              return a.h || (a.h = a.l())
          };
          var Yf, Zf = function() {};
          _.w(Zf, Wf);
          Zf.prototype.g = function() {
              var a = $f(this);
              return a ? new ActiveXObject(a) : new XMLHttpRequest
          }
          ;
          Zf.prototype.l = function() {
              var a = {};
              $f(this) && (a[0] = !0,
              a[1] = !0);
              return a
          }
          ;
          var $f = function(a) {
              if (!a.j && "undefined" == typeof XMLHttpRequest && "undefined" != typeof ActiveXObject) {
                  for (var b = ["MSXML2.XMLHTTP.6.0", "MSXML2.XMLHTTP.3.0", "MSXML2.XMLHTTP", "Microsoft.XMLHTTP"], c = 0; c < b.length; c++) {
                      var d = b[c];
                      try {
                          return new ActiveXObject(d),
                          a.j = d
                      } catch (e) {}
                  }
                  throw Error("U");
              }
              return a.j
          };
          Yf = new Zf;
          var ag = function() {};
          _.w(ag, Wf);
          ag.prototype.g = function() {
              var a = new XMLHttpRequest;
              if ("withCredentials"in a)
                  return a;
              if ("undefined" != typeof XDomainRequest)
                  return new bg;
              throw Error("V");
          }
          ;
          ag.prototype.l = function() {
              return {}
          }
          ;
          var bg = function() {
              this.g = new XDomainRequest;
              this.readyState = 0;
              this.onreadystatechange = null;
              this.responseType = this.responseText = "";
              this.status = -1;
              this.statusText = "";
              this.g.onload = (0,
              _.v)(this.bi, this);
              this.g.onerror = (0,
              _.v)(this.rg, this);
              this.g.onprogress = (0,
              _.v)(this.pj, this);
              this.g.ontimeout = (0,
              _.v)(this.uj, this)
          };
          _.h = bg.prototype;
          _.h.open = function(a, b, c) {
              if (null != c && !c)
                  throw Error("W");
              this.g.open(a, b)
          }
          ;
          _.h.send = function(a) {
              if (a)
                  if ("string" == typeof a)
                      this.g.send(a);
                  else
                      throw Error("X");
              else
                  this.g.send()
          }
          ;
          _.h.abort = function() {
              this.g.abort()
          }
          ;
          _.h.setRequestHeader = function() {}
          ;
          _.h.getResponseHeader = function(a) {
              return "content-type" == a.toLowerCase() ? this.g.contentType : ""
          }
          ;
          _.h.bi = function() {
              this.status = 200;
              this.responseText = this.g.responseText;
              cg(this, 4)
          }
          ;
          _.h.rg = function() {
              this.status = 500;
              this.responseText = "";
              cg(this, 4)
          }
          ;
          _.h.uj = function() {
              this.rg()
          }
          ;
          _.h.pj = function() {
              this.status = 200;
              cg(this, 1)
          }
          ;
          var cg = function(a, b) {
              a.readyState = b;
              if (a.onreadystatechange)
                  a.onreadystatechange()
          };
          bg.prototype.getAllResponseHeaders = function() {
              return "content-type: " + this.g.contentType
          }
          ;
          _.dg = function(a, b, c) {
              if ("function" === typeof a)
                  c && (a = (0,
                  _.v)(a, c));
              else if (a && "function" == typeof a.handleEvent)
                  a = (0,
                  _.v)(a.handleEvent, a);
              else
                  throw Error("Y");
              return 2147483647 < Number(b) ? -1 : _.p.setTimeout(a, b || 0)
          }
          ;
          _.eg = function(a) {
              _.p.clearTimeout(a)
          }
          ;
          var gg, hg, og, ng, kg;
          _.fg = function(a) {
              _.H.call(this);
              this.headers = new Map;
              this.N = a || null;
              this.h = !1;
              this.J = this.g = null;
              this.o = "";
              this.l = 0;
              this.j = this.S = this.C = this.K = !1;
              this.s = 0;
              this.B = null;
              this.X = "";
              this.U = this.W = !1
          }
          ;
          _.w(_.fg, _.H);
          gg = /^https?$/i;
          hg = ["POST", "PUT"];
          _.ig = [];
          _.fg.prototype.ba = function() {
              this.O();
              _.va(_.ig, this)
          }
          ;
          _.fg.prototype.send = function(a, b, c, d) {
              if (this.g)
                  throw Error("Z`" + this.o + "`" + a);
              b = b ? b.toUpperCase() : "GET";
              this.o = a;
              this.l = 0;
              this.K = !1;
              this.h = !0;
              this.g = this.N ? this.N.g() : Yf.g();
              this.J = this.N ? Xf(this.N) : Xf(Yf);
              this.g.onreadystatechange = (0,
              _.v)(this.T, this);
              try {
                  this.S = !0,
                  this.g.open(b, String(a), !0),
                  this.S = !1
              } catch (g) {
                  jg(this);
                  return
              }
              a = c || "";
              c = new Map(this.headers);
              if (d)
                  if (Object.getPrototypeOf(d) === Object.prototype)
                      for (var e in d)
                          c.set(e, d[e]);
                  else if ("function" === typeof d.keys && "function" === typeof d.get) {
                      e = _.Sa(d.keys());
                      for (var f = e.next(); !f.done; f = e.next())
                          f = f.value,
                          c.set(f, d.get(f))
                  } else
                      throw Error("$`" + String(d));
              d = Array.from(c.keys()).find(function(g) {
                  return "content-type" == g.toLowerCase()
              });
              e = _.p.FormData && a instanceof _.p.FormData;
              !_.ta(hg, b) || d || e || c.set("Content-Type", "application/x-www-form-urlencoded;charset=utf-8");
              b = _.Sa(c);
              for (d = b.next(); !d.done; d = b.next())
                  c = _.Sa(d.value),
                  d = c.next().value,
                  c = c.next().value,
                  this.g.setRequestHeader(d, c);
              this.X && (this.g.responseType = this.X);
              "withCredentials"in this.g && this.g.withCredentials !== this.W && (this.g.withCredentials = this.W);
              try {
                  kg(this),
                  0 < this.s && ((this.U = lg(this.g)) ? (this.g.timeout = this.s,
                  this.g.ontimeout = (0,
                  _.v)(this.Z, this)) : this.B = _.dg(this.Z, this.s, this)),
                  this.C = !0,
                  this.g.send(a),
                  this.C = !1
              } catch (g) {
                  jg(this)
              }
          }
          ;
          var lg = function(a) {
              return _.z && _.tc(9) && "number" === typeof a.timeout && void 0 !== a.ontimeout
          };
          _.fg.prototype.Z = function() {
              "undefined" != typeof eb && this.g && (this.l = 8,
              this.dispatchEvent("timeout"),
              this.abort(8))
          }
          ;
          var jg = function(a) {
              a.h = !1;
              a.g && (a.j = !0,
              a.g.abort(),
              a.j = !1);
              a.l = 5;
              mg(a);
              ng(a)
          }
            , mg = function(a) {
              a.K || (a.K = !0,
              a.dispatchEvent("complete"),
              a.dispatchEvent("error"))
          };
          _.fg.prototype.abort = function(a) {
              this.g && this.h && (this.h = !1,
              this.j = !0,
              this.g.abort(),
              this.j = !1,
              this.l = a || 7,
              this.dispatchEvent("complete"),
              this.dispatchEvent("abort"),
              ng(this))
          }
          ;
          _.fg.prototype.L = function() {
              this.g && (this.h && (this.h = !1,
              this.j = !0,
              this.g.abort(),
              this.j = !1),
              ng(this, !0));
              _.fg.G.L.call(this)
          }
          ;
          _.fg.prototype.T = function() {
              this.zb() || (this.S || this.C || this.j ? og(this) : this.fa())
          }
          ;
          _.fg.prototype.fa = function() {
              og(this)
          }
          ;
          og = function(a) {
              if (a.h && "undefined" != typeof eb && (!a.J[1] || 4 != (a.g ? a.g.readyState : 0) || 2 != _.pg(a)))
                  if (a.C && 4 == (a.g ? a.g.readyState : 0))
                      _.dg(a.T, 0, a);
                  else if (a.dispatchEvent("readystatechange"),
                  4 == (a.g ? a.g.readyState : 0)) {
                      a.h = !1;
                      try {
                          _.qg(a) ? (a.dispatchEvent("complete"),
                          a.dispatchEvent("success")) : (a.l = 6,
                          mg(a))
                      } finally {
                          ng(a)
                      }
                  }
          }
          ;
          ng = function(a, b) {
              if (a.g) {
                  kg(a);
                  var c = a.g
                    , d = a.J[0] ? function() {}
                  : null;
                  a.g = null;
                  a.J = null;
                  b || a.dispatchEvent("ready");
                  try {
                      c.onreadystatechange = d
                  } catch (e) {}
              }
          }
          ;
          kg = function(a) {
              a.g && a.U && (a.g.ontimeout = null);
              a.B && (_.eg(a.B),
              a.B = null)
          }
          ;
          _.qg = function(a) {
              var b = _.pg(a);
              a: switch (b) {
              case 200:
              case 201:
              case 202:
              case 204:
              case 206:
              case 304:
              case 1223:
                  var c = !0;
                  break a;
              default:
                  c = !1
              }
              if (!c) {
                  if (b = 0 === b)
                      a = String(a.o).match(_.Af)[1] || null,
                      !a && _.p.self && _.p.self.location && (a = _.p.self.location.protocol.slice(0, -1)),
                      b = !gg.test(a ? a.toLowerCase() : "");
                  c = b
              }
              return c
          }
          ;
          _.pg = function(a) {
              try {
                  return 2 < (a.g ? a.g.readyState : 0) ? a.g.status : -1
              } catch (b) {
                  return -1
              }
          }
          ;
          _.rg = function(a) {
              try {
                  return a.g ? a.g.responseText : ""
              } catch (b) {
                  return ""
              }
          }
          ;
          var tg = function(a) {
              _.x.call(this);
              this.J = a;
              this.s = Rf(a);
              this.j = this.l = null;
              this.S = !0;
              this.H = new _.I(this);
              this.K = [];
              this.o = new Set;
              this.g = [];
              this.U = new sg;
              this.h = [];
              this.C = !1;
              a = (0,
              _.v)(this.B, this);
              xf.version = a
          };
          _.u(tg, _.x);
          var ug = function(a, b) {
              a.g.length && Ke(b, a.g[a.g.length - 1]);
              a.g.push(b);
              He(b, function() {
                  _.va(this.g, b)
              }, a)
          };
          tg.prototype.N = function(a, b, c) {
              var d = void 0 === c ? {} : c;
              c = d.Ri;
              var e = d.Of
                , f = d.ak;
              a = vg(this, a, b, d.Pi, c);
              wg(this, a, e, f, c)
          }
          ;
          var vg = function(a, b, c, d, e) {
              d = void 0 === d ? {} : d;
              var f = [];
              xg(a, b, c, d, void 0 === e ? !1 : e, function(g) {
                  f.push(g.mb())
              });
              return f
          }
            , xg = function(a, b, c, d, e, f, g) {
              g = void 0 === g ? {} : g;
              b = _.Sa(b);
              for (var k = b.next(); !k.done; k = b.next()) {
                  var l = k.value;
                  k = c[l];
                  !e && (a.o.has(l) || k.g) || g[l] || (g[l] = !0,
                  l = d[l] ? Object.keys(d[l]) : [],
                  xg(a, k.Dc().concat(l), c, d, e, f, g),
                  f(k))
              }
          }
            , wg = function(a, b, c, d, e) {
              e = void 0 === e ? !1 : e;
              var f = []
                , g = new Ae;
              b = [b];
              for (var k = function(q, r) {
                  for (var A = [], G = 0, Q = Math.floor(q.length / r) + 1, rd = 0; rd < r; rd++) {
                      var kc = (rd + 1) * Q;
                      A.push(q.slice(G, kc));
                      G = kc
                  }
                  return A
              }, l = b.shift(); l; ) {
                  var m = yg(a, l, !!e, !0);
                  if (2E3 >= m.length) {
                      if (l = zg(a, l, e))
                          f.push(l),
                          Ke(g, l.g)
                  } else
                      b = k(l, Math.ceil(m.length / 2E3)).concat(b);
                  l = b.shift()
              }
              var n = new Ae;
              ug(a, n);
              He(n, function() {
                  return Ag(a, f, c, d)
              });
              Ie(n, function() {
                  var q = new Bg;
                  q.j = !0;
                  q.h = -1;
                  Ag(this, [q], c, d)
              }, a);
              He(g, function() {
                  return n.callback()
              });
              g.callback()
          }
            , zg = function(a, b, c) {
              var d = yg(a, b, !(void 0 === c || !c));
              a.K.push(d);
              b = _.Sa(b);
              for (c = b.next(); !c.done; c = b.next())
                  a.o.add(c.value);
              if (a.C)
                  a = _.cd(document, "SCRIPT"),
                  _.La(a, _.Hb(d)),
                  a.type = "text/javascript",
                  a.async = !1,
                  document.body.appendChild(a);
              else {
                  var e = new Bg
                    , f = new _.fg(0 < a.h.length ? new ag : void 0);
                  a.H.D(f, "success", (0,
                  _.v)(e.C, e, f));
                  a.H.D(f, "error", (0,
                  _.v)(e.s, e, f));
                  a.H.D(f, "timeout", (0,
                  _.v)(e.B, e));
                  Tf(a.H, f, "ready", f.O, !1, f);
                  f.s = 3E4;
                  Cg(a.U, function() {
                      f.send(d);
                      return e.g
                  });
                  return e
              }
              return null
          }
            , Ag = function(a, b, c, d) {
              for (var e = !1, f, g = !1, k = 0; k < b.length; k++) {
                  var l = b[k];
                  if (!f && l.j) {
                      e = !0;
                      f = l.h;
                      break
                  } else
                      l.l && (g = !0)
              }
              var m = _.wa(a.g);
              (e || g) && -1 != f && (a.g.length = 0);
              if (e)
                  c && c(f);
              else if (g)
                  d && d();
              else
                  for (k = 0; k < b.length; k++)
                      l = b[k],
                      Dg(l.o, l.ac) || c && c(8001);
              (e || g) && -1 != f && _.Pb(m, function(n) {
                  n.cancel()
              })
          };
          tg.prototype.L = function() {
              this.H.O();
              delete xf.version;
              _.x.prototype.L.call(this)
          }
          ;
          tg.prototype.B = function() {
              return Mf(this.s, "k")
          }
          ;
          var yg = function(a, b, c, d) {
              d = void 0 === d ? !1 : d;
              var e = _.Bf(a.J.match(_.Af)[3] || null);
              if (0 < a.h.length && !_.ta(a.h, e) && null != e && window.location.hostname != e)
                  throw Error("ca`" + e);
              e = Rf(a.s.toString());
              delete e.g.m;
              delete e.g.exm;
              delete e.g.ed;
              Nf(e, "m", b.join(","));
              a.l && (Nf(e, "ck", a.l),
              a.j && Nf(e, "rs", a.j));
              Nf(e, "d", "0");
              c && (a = _.Vc(),
              e.h.zx = a);
              a = e.toString();
              if (d && 0 == a.lastIndexOf("/", 0)) {
                  e = document.location.href.match(_.Af);
                  d = e[1];
                  b = e[2];
                  c = e[3];
                  e = e[4];
                  var f = "";
                  d && (f += d + ":");
                  c && (f += "//",
                  b && (f += b + "@"),
                  f += c,
                  e && (f += ":" + e));
                  a = f + a
              }
              return a
          }
            , Dg = function(a, b) {
              var c = "";
              if (1 < a.length && "\n" === a.charAt(a.length - 1)) {
                  var d = a.lastIndexOf("\n", a.length - 2);
                  0 <= d && (c = a.substring(d + 1, a.length - 1))
              }
              if (_.Ub(c, "Google Inc.") || 0 == c.lastIndexOf("//# sourceMappingURL=", 0))
                  try {
                      c = window;
                      var e = _.Cb(a + "\r\n//# sourceURL=" + b)
                        , f = _.Bb(e);
                      c.eval(f) === f && c.eval(f.toString())
                  } catch (g) {
                      return !1
                  }
              else
                  return !1;
              return !0
          }
            , Eg = function(a) {
              var b = _.Bf(a.match(_.Af)[5] || null) || "";
              b = _.Bf(Pf(b).match(_.Af)[5] || null);
              return null !== b && b.match("(/_/js/)|(/_/ss/)") && b.match("/k=") ? a : null
          }
            , Bg = function() {
              this.g = new Ae;
              this.ac = this.o = "";
              this.j = !1;
              this.h = 0;
              this.l = !1
          };
          Bg.prototype.C = function(a) {
              this.o = _.rg(a);
              this.ac = String(a.o);
              this.g.callback()
          }
          ;
          Bg.prototype.s = function(a) {
              this.j = !0;
              this.h = _.pg(a);
              this.g.callback()
          }
          ;
          Bg.prototype.B = function() {
              this.l = !0;
              this.g.callback()
          }
          ;
          var sg = function() {
              this.g = 0;
              this.h = []
          }
            , Cg = function(a, b) {
              a.h.push(b);
              Fg(a)
          }
            , Fg = function(a) {
              for (; 5 > a.g && a.h.length; )
                  Gg(a, a.h.shift())
          }
            , Gg = function(a, b) {
              a.g++;
              He(b(), function() {
                  this.g--;
                  Fg(this)
              }, a)
          };
          var Hg = new Jf(!1)
            , Ig = document.location.href;
          uf({
              h: {
                  dml: Hg
              },
              initialize: function(a) {
                  var b = Hg.get()
                    , c = ""
                    , d = "";
                  window && window._F_cssRowKey && (c = window._F_cssRowKey,
                  window._F_combinedSignature && (d = window._F_combinedSignature));
                  if (c && "function" !== typeof window._F_installCss)
                      throw Error("aa");
                  var e, f = _.p._F_jsUrl;
                  f && (e = Eg(f));
                  !e && (f = document.getElementById("base-js")) && (e = f.src ? f.src : f.getAttribute("href"),
                  e = Eg(e));
                  e || (e = Eg(Ig));
                  e || (e = document.getElementsByTagName("script"),
                  e = Eg(e[e.length - 1].src));
                  if (!e)
                      throw Error("ba");
                  e = new tg(e);
                  c && (e.l = c);
                  d && (e.j = d);
                  e.C = b;
                  b = ja();
                  b.B = e;
                  b.Kh(!0);
                  b = ja();
                  b.cg(a);
                  a.j(b)
              }
          });

          _._ModuleManager_initialize = function(a, b) {
              if (!_.fa) {
                  if (!_.ha)
                      return;
                  _.ia()
              }
              _.fa.bg(a, b)
          }
          ;

          _._ModuleManager_initialize('b/sy0/el_conf:1/sy2/sy4/sy3:4/sy1:1,3,5/el_main:6/el_sect:6/ajaxproxy/website_error/navigationui:5/_stam:3,4/n73qwf/MpJwZc', ['sy0', 'el_conf']);

      } catch (e) {
          _._DumpException(e)
      }
      try {/*

Copyright The Closure Library Authors.
SPDX-License-Identifier: Apache-2.0
*/

      } catch (e) {
          _._DumpException(e)
      }
      try {
          _.ma("el_conf");

          var Jg, J;
          _._exportVersion = function(a) {
              _.rb("google.translate.v", a)
          }
          ;
          _._getCallbackFunction = function(a) {
              return _.ib(a)
          }
          ;
          _._exportMessages = function() {
              _.rb("google.translate.m", J)
          }
          ;
          Jg = function(a) {
              var b = document.getElementsByTagName("head")[0];
              b || (b = document.body.parentNode.appendChild(document.createElement("head")));
              b.appendChild(a)
          }
          ;
          _._loadJs = function(a) {
              var b = _.cd(document, "SCRIPT");
              b.type = "text/javascript";
              b.charset = "UTF-8";
              _.La(b, _.Hb(a));
              Jg(b)
          }
          ;
          _._loadCss = function(a) {
              var b = document.createElement("link");
              b.type = "text/css";
              b.rel = "stylesheet";
              b.charset = "UTF-8";
              b.href = a;
              Jg(b)
          }
          ;
          _._isNS = function(a) {
              a = a.split(".");
              for (var b = window, c = 0; c < a.length; ++c)
                  if (!(b = b[a[c]]))
                      return !1;
              return !0
          }
          ;
          _._setupNS = function(a) {
              a = a.split(".");
              for (var b = window, c = 0; c < a.length; ++c)
                  b.hasOwnProperty ? b.hasOwnProperty(a[c]) ? b = b[a[c]] : b = b[a[c]] = {} : b = b[a[c]] || (b[a[c]] = {});
              return b
          }
          ;
          J = {};
          MSG_TRANSLATE = "Translate";
          J[0] = MSG_TRANSLATE;
          MSG_CANCEL = "Cancel";
          J[1] = MSG_CANCEL;
          MSG_CLOSE = "Close";
          J[2] = MSG_CLOSE;
          MSGFUNC_PAGE_TRANSLATED_TO = function(a) {
              return "Google has automatically translated this page to: " + a
          }
          ;
          J[3] = MSGFUNC_PAGE_TRANSLATED_TO;
          MSGFUNC_TRANSLATED_TO = function(a) {
              return "Translated to: " + a
          }
          ;
          J[4] = MSGFUNC_TRANSLATED_TO;
          MSG_GENERAL_ERROR = "Error: The server could not complete your request. Try again later.";
          J[5] = MSG_GENERAL_ERROR;
          MSG_LEARN_MORE = "Learn more";
          J[6] = MSG_LEARN_MORE;
          MSGFUNC_POWERED_BY = function(a) {
              return "Powered by " + a
          }
          ;
          J[7] = MSGFUNC_POWERED_BY;
          MSG_TRANSLATE_PRODUCT_NAME = "Translate";
          J[8] = MSG_TRANSLATE_PRODUCT_NAME;
          MSG_TRANSLATION_IN_PROGRESS = "Translation in progress";
          J[9] = MSG_TRANSLATION_IN_PROGRESS;
          MSGFUNC_TRANSLATE_PAGE_TO = function(a) {
              return "Translate this page to: " + a + " using Google Translate?"
          }
          ;
          J[10] = MSGFUNC_TRANSLATE_PAGE_TO;
          MSGFUNC_VIEW_PAGE_IN = function(a) {
              return "View this page in: " + a
          }
          ;
          J[11] = MSGFUNC_VIEW_PAGE_IN;
          MSG_RESTORE = "Show original";
          J[12] = MSG_RESTORE;
          MSG_SSL_INFO_LOCAL_FILE = "The content of this local file will be sent to Google for translation using a secure connection.";
          J[13] = MSG_SSL_INFO_LOCAL_FILE;
          MSG_SSL_INFO_SECURE_PAGE = "The content of this secure page will be sent to Google for translation using a secure connection.";
          J[14] = MSG_SSL_INFO_SECURE_PAGE;
          MSG_SSL_INFO_INTRANET_PAGE = "The content of this intranet page will be sent to Google for translation using a secure connection.";
          J[15] = MSG_SSL_INFO_INTRANET_PAGE;
          MSG_SELECT_LANGUAGE = "Select Language";
          J[16] = MSG_SELECT_LANGUAGE;
          MSGFUNC_TURN_OFF_TRANSLATION = function(a) {
              return "Turn off " + a + " translation"
          }
          ;
          J[17] = MSGFUNC_TURN_OFF_TRANSLATION;
          MSGFUNC_TURN_OFF_FOR = function(a) {
              return "Turn off for: " + a
          }
          ;
          J[18] = MSGFUNC_TURN_OFF_FOR;
          MSG_ALWAYS_HIDE_AUTO_POPUP_BANNER = "Always hide";
          J[19] = MSG_ALWAYS_HIDE_AUTO_POPUP_BANNER;
          MSG_ORIGINAL_TEXT = "Original text:";
          J[20] = MSG_ORIGINAL_TEXT;
          MSG_FILL_SUGGESTION = "Contribute a better translation";
          J[21] = MSG_FILL_SUGGESTION;
          MSG_SUBMIT_SUGGESTION = "Contribute";
          J[22] = MSG_SUBMIT_SUGGESTION;
          MSG_SHOW_TRANSLATE_ALL = "Translate all";
          J[23] = MSG_SHOW_TRANSLATE_ALL;
          MSG_SHOW_RESTORE_ALL = "Restore all";
          J[24] = MSG_SHOW_RESTORE_ALL;
          MSG_SHOW_CANCEL_ALL = "Cancel all";
          J[25] = MSG_SHOW_CANCEL_ALL;
          MSG_TRANSLATE_TO_MY_LANGUAGE = "Translate sections to my language";
          J[26] = MSG_TRANSLATE_TO_MY_LANGUAGE;
          MSGFUNC_TRANSLATE_EVERYTHING_TO = function(a) {
              return "Translate everything to " + a
          }
          ;
          J[27] = MSGFUNC_TRANSLATE_EVERYTHING_TO;
          MSG_SHOW_ORIGINAL_LANGUAGES = "Show original languages";
          J[28] = MSG_SHOW_ORIGINAL_LANGUAGES;
          MSG_OPTIONS = "Options";
          J[29] = MSG_OPTIONS;
          MSG_TURN_OFF_TRANSLATION_FOR_THIS_SITE = "Turn off translation for this site";
          J[30] = MSG_TURN_OFF_TRANSLATION_FOR_THIS_SITE;
          J[31] = null;
          MSG_ALT_SUGGESTION = "Show alternative translations";
          J[32] = MSG_ALT_SUGGESTION;
          MSG_ALT_ACTIVITY_HELPER_TEXT = "Click on words above to get alternative translations";
          J[33] = MSG_ALT_ACTIVITY_HELPER_TEXT;
          MSG_USE_ALTERNATIVES = "Use";
          J[34] = MSG_USE_ALTERNATIVES;
          MSG_DRAG_TIP = "Drag with shift key to reorder";
          J[35] = MSG_DRAG_TIP;
          MSG_CLICK_FOR_ALT = "Click for alternative translations";
          J[36] = MSG_CLICK_FOR_ALT;
          MSG_DRAG_INSTUCTIONS = "Hold down the shift key, click, and drag the words above to reorder.";
          J[37] = MSG_DRAG_INSTUCTIONS;
          MSG_SUGGESTION_SUBMITTED = "Thank you for contributing your translation suggestion to Google Translate.";
          J[38] = MSG_SUGGESTION_SUBMITTED;
          MSG_MANAGE_TRANSLATION_FOR_THIS_SITE = "Manage translation for this site";
          J[39] = MSG_MANAGE_TRANSLATION_FOR_THIS_SITE;
          MSG_ALT_AND_CONTRIBUTE_ACTIVITY_HELPER_TEXT = "Click a word for alternative translations, or double-click to edit directly";
          J[40] = MSG_ALT_AND_CONTRIBUTE_ACTIVITY_HELPER_TEXT;
          MSG_ORIGINAL_TEXT_NO_COLON = "Original text";
          J[41] = MSG_ORIGINAL_TEXT_NO_COLON;
          J[42] = "Translate";
          J[43] = "Translate";
          J[44] = "Your correction has been submitted.";
          MSG_LANGUAGE_UNSUPPORTED = "Error: The language of the webpage is not supported.";
          J[45] = MSG_LANGUAGE_UNSUPPORTED;
          MSG_LANGUAGE_TRANSLATE_WIDGET = "Language Translate Widget";
          J[46] = MSG_LANGUAGE_TRANSLATE_WIDGET;
          _.rb("_exportVersion", _._exportVersion);
          _.rb("_getCallbackFunction", _._getCallbackFunction);
          _.rb("_exportMessages", _._exportMessages);
          _.rb("_loadJs", _._loadJs);
          _.rb("_loadCss", _._loadCss);
          _.rb("_isNS", _._isNS);
          _.rb("_setupNS", _._setupNS);
          window.addEventListener && "undefined" == typeof document.readyState && window.addEventListener("DOMContentLoaded", function() {
              document.readyState = "complete"
          }, !1);

          _.oa();

      } catch (e) {
          _._DumpException(e)
      }
  }).call(this, this.default_tr);
  
  
  // Google Inc.

  //# sourceURL=/_/translate_http/_/js/k=translate_http.tr.en_US.oOC1Oa7Rttc.O/d=1/rs=AN8SPfrSx_NPtaSO9FSK_pNCvaEYOQSpFQ/m=el_conf
  // Configure Constants
  (function() {
      let gtConstEvalStartTime = new Date();
      if (_isNS('google.translate.Element')) {
          return
      }

      (function() {
          const c = _setupNS('google.translate._const');

          c._cest = gtConstEvalStartTime;
          gtConstEvalStartTime = undefined;
          // hide this eval start time constant
          c._cl = 'en-US';
          c._cuc = 'cr.googleTranslate.onTranslateElementLoad';
          c._cac = '';
          c._cam = 'lib';
          c._ctkk = '459921.2020309657';
          const h = 'translate.googleapis.com';
          const s = 'https' + '://';
          c._pah = h;
          c._pas = s;
          const b = s + 'translate.googleapis.com';
          const staticPath = '/translate_static/';
          c._pci = b + staticPath + 'img/te_ctrl3.gif';
          c._pmi = b + staticPath + 'img/mini_google.png';
          c._pbi = b + staticPath + 'img/te_bk.gif';
          c._pli = b + staticPath + 'img/loading.gif';
          c._ps = b + staticPath + 'css\/translateelement.css';
          c._plla = 'translate-pa.googleapis.com' + '\/v1\/supportedLanguages';
          c._puh = 'translate.google.com';
          c._cnal = {};
          c._cjlc = _getCallbackFunction('cr.googleTranslate.onLoadJavascript');
          _getCallbackFunction('cr.googleTranslate.onLoadCSS')(c._ps);
          c._cjlc('https:\/\/translate.googleapis.com\/translate_static\/js\/element\/main.js');
          _exportMessages();
          _exportVersion('TE_20220615');
      }
      )();
  })();
} catch(error) {
  cr.googleTranslate.onTranslateElementError(error);
}
