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
  text = text.replaceAll('//www.gstatic.com/images/branding/product/2x/translate_24dp.png',
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
