const securityOriginHost = new URL(securityOrigin).host;
const redirectToSecurityOrigin = (url) => {
  let new_url = new URL(url);
  if (useGoogleTranslateEndpoint && new_url.pathname === '/translate_a/t') {
    new_url.host = 'translate.googleapis.com';

    // Remove API key
    new_url.searchParams.set('key', '');

    return new_url.toString();
  }
  new_url.host = securityOriginHost;
  return new_url.toString();
};

const emptySvgDataUrl = 'data:image/svg+xml;base64,' +
  btoa('<svg xmlns="http://www.w3.org/2000/svg"/>');

function processJavascript(text) {
  text = text.replaceAll('"//"+po+"/gen204?"+Bo(b)',
    '"' + emptySvgDataUrl + '"');
  text = text.replaceAll(
    "https://www.gstatic.com/images/branding/product/1x/translate_24dp.png",
    emptySvgDataUrl);
  return text;
}

function processCSS(text) {
  text = text.replaceAll('//www.gstatic.com/images/branding/product/2x/translate_24dp.png',
    emptySvgDataUrl);
  return text;
}

if (typeof XMLHttpRequest.prototype.realOpen === 'undefined') {
  XMLHttpRequest.prototype.realOpen = XMLHttpRequest.prototype.open;
  XMLHttpRequest.prototype.open = function (method, url, async = true,
    user = "", password = "") {
    this.realOpen(method, redirectToSecurityOrigin(url), async, user,
      password);
  }
};

// Keep sync with original onLoadJavascript in chromium translate.js
cr.googleTranslate.onLoadJavascript = function (url) {
  const xhr = new XMLHttpRequest();
  xhr.open('GET', redirectToSecurityOrigin(url), true);
  xhr.onreadystatechange = function () {
    if (this.readyState !== this.DONE) {
      return;
    }
    if (this.status !== 200) {
      errorCode = ERROR['SCRIPT_LOAD_ERROR'];
      return;
    }

    new Function(processJavascript(this.responseText)).call(window);
  };
  xhr.send();
};

const extraStyles = `.goog-te-spinner-pos, #goog-gt-tt {display: none;}`
cr.googleTranslate.onLoadCSS = function (url) {
  const xhr = new XMLHttpRequest();
  xhr.open('GET', redirectToSecurityOrigin(url), true);
  xhr.onreadystatechange = function () {
    if (this.readyState !== this.DONE || this.status !== 200) {
      return;
    }

    const element = document.createElement('style');
    element.type = 'text/css';
    element.charset = 'UTF-8';
    element.innerText = processCSS(this.responseText) + extraStyles;
    document.head.appendChild(element);
  };
  xhr.send();
};
