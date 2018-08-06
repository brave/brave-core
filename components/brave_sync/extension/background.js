'use strict';

console.log("in chrome.runtime.onInstalled 001");

function dynamicallyLoadScript(url) {
  var script = document.createElement("script");
  script.src = url;
  document.head.appendChild(script);
}

chrome.runtime.onInstalled.addListener(function() {
  console.log("in chrome.runtime.onInstalled 002");
  var arg1 = 142;
  chrome.braveSync.backgroundPageToBrowser(arg1, function(callback_arg1) {
    console.log("in chrome.runtime.onInstalled 002-4 backgroundPageToBrowser callback callback_arg1=", callback_arg1);
  });
  console.log("in chrome.runtime.onInstalled 002-5");

  chrome.braveSync.onBrowserToBackgroundPage.addListener(function(arg1) {
    console.log("in chrome.braveSync.onBrowserToBackgroundPage browser-native initiated call", arg1);
  });

});

console.log("in chrome.runtime.onInstalled 003");
var theVar1 = 42;
