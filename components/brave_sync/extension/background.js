'use strict';

console.log("in chrome.runtime.onInstalled 001");

var loaded = false;
function dynamicallyLoadScript(url) {
  var script = document.createElement("script");
  script.src = url;
  document.head.appendChild(script);
}

chrome.runtime.onStartup.addListener(function() {
  console.log("in chrome.runtime.onStartup");
})

chrome.runtime.onInstalled.addListener(function() {
  console.log("in chrome.runtime.onInstalled 002");
  var arg1 = 142;
  chrome.braveSync.backgroundPageToBrowser(arg1, function(callback_arg1) {
    console.log("in chrome.runtime.onInstalled 002-4 backgroundPageToBrowser callback callback_arg1=", callback_arg1);
  });
  console.log("in chrome.runtime.onInstalled 002-5");

  chrome.braveSync.onBrowserToBackgroundPage.addListener(function(arg1) {
    console.log("in chrome.braveSync.onBrowserToBackgroundPage browser-native initiated call", arg1);
    if (!loaded) {
      var sync_bundle_url = chrome.runtime.getURL('bundle.js');
      dynamicallyLoadScript(sync_bundle_url);
      loaded = true;
    }
  });
});

console.log("in chrome.runtime.onInstalled 003");
var theVar1 = 42;
