window.__firefox__.includeOnce("PlaylistFolderSharingScript", function($) {
  let sendMessage = $(function(pageUrl) {
    $.postNativeMessage('$<message_handler>', {
      "securityToken": SECURITY_TOKEN,
      "pageUrl": pageUrl
    });
  });
  
  if (!window.brave) {
    window.brave = {};
  }
  
  if (!window.brave.playlist) {
    window.brave.playlist = {};
    window.brave.playlist.open = $(function(pageUrl) {
      sendMessage(pageUrl);
    });
  }
});
