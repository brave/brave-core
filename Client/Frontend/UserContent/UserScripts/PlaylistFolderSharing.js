window.__firefox__.includeOnce("PlaylistFolderSharing", function() {
  function sendMessage(pageUrl) {
      if (window.webkit.messageHandlers.$<handler>) {
          window.webkit.messageHandlers.$<handler>.postMessage({
            "securitytoken": "$<security_token>",
            "pageUrl": pageUrl
          });
      }
  }
  
  sendMessage.toString = function() {
    return "function() {\n\t[native code]\n}";
  }
  
  if (!window.brave) {
    window.brave = {};
  }
  
  if (!window.brave.playlist) {
    window.brave.playlist = {};
    window.brave.playlist.open = function(pageUrl) {
      sendMessage(pageUrl);
    };
  }
});
