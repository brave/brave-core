/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

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


try {
  // components/translate/core/browser/resources/translate.js
  $<brave_core_translate_translate_js>
  
  // components/translate/core/browser/translate_script.cc;l=149
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
          let newURL = new URL(url);
          if (newURL.pathname === '/translate_a/t') {
              // useGoogleTranslateEndpoint is predefined by translate_script.cc.
              // It's used only for local testing to disable the redirection of
              // translation requests.
              if (useGoogleTranslateEndpoint) {
                  // Remove API key
                  newURL.searchParams.set('key', '');

                  // Leave the domain unchanged (translate.googleapis.com).
                  return newURL.toString();
              }
          } else {
              // braveTranslateStaticPath is predefined by translate_script.cc.
              newURL.pathname = newURL.pathname.replace('/translate_static/', braveTranslateStaticPath);
          }
          newURL.host = securityOriginHost;
          return newURL.toString();
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
