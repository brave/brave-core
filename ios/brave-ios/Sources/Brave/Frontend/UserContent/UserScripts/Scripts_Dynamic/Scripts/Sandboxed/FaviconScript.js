(function() {
  function getFavicons() {
    var favicons = [];
    delete favicons.toJSON;  // Never inherit Array.prototype.toJSON.
    var links = document.getElementsByTagName('link');
    var linkCount = links.length;
    for (var i = 0; i < linkCount; ++i) {
      if (links[i].rel) {
        var rel = links[i].rel.toLowerCase();
        if (rel == 'alternate icon' || rel == 'shortcut icon' ||
            rel == 'icon' || rel == 'apple-touch-icon' || rel == 'apple-touch-icon-precomposed') {
          var favicon = {rel: links[i].rel.toLowerCase(), href: links[i].href};
          if (links[i].sizes && links[i].sizes.value) {
            favicon.sizes = links[i].sizes.value;
          }
          favicons.push(favicon);
        } else if (rel && links[i].href && (rel.startsWith("shortcut"))) {
          var href = links[i].href;
          if (href.endsWith(".ico") || href.endsWith(".png") ||
              href.endsWith(".jpg") || href.endsWith(".jpeg") ||
              href.endsWith(".bmp") || href.endsWith(".webp") ||
              href.endsWith(".svg")) {
            var favicon = {rel: links[i].rel.toLowerCase(), href: links[i].href};
            if (links[i].sizes && links[i].sizes.value) {
              favicon.sizes = links[i].sizes.value;
            }
            favicons.push(favicon);
          }
        }
      }
    }

    // DEBUGGING! DO NOT REMOVE!
    /*
    if (!favicons.length) {
      favicons.push({rel: "icon", href: "/favicon.ico"});
      favicons.push({rel: "icon", sizes: "32x32", href: "/favicon_32x32.ico"});
      favicons.push({rel: "icon", sizes: "48x48", href: "/favicon_48x48.ico"});
      favicons.push({rel: "icon", sizes: "96x96", href: "/favicon_96x96.ico"});
      favicons.push({rel: "icon", sizes: "144x144", href: "/favicon_144x144.ico"});
      favicons.push({rel: "apple-touch-icon-precomposed", sizes: "180x180", href: "/apple-touch-icon-180x180-precomposed.png"});
      favicons.push({rel: "apple-touch-icon-precomposed", sizes: "167x167", href: "/apple-touch-icon-167x167-precomposed.png"});
      favicons.push({rel: "apple-touch-icon-precomposed", sizes: "152x152", href: "/apple-touch-icon-152x152-precomposed.png"});
      favicons.push({rel: "apple-touch-icon-precomposed", sizes: "144x144", href: "/apple-touch-icon-144x144-precomposed.png"});
      favicons.push({rel: "apple-touch-icon-precomposed", sizes: "120x120", href: "/apple-touch-icon-120x120-precomposed.png"});
      favicons.push({rel: "apple-touch-icon-precomposed", sizes: "114x114", href: "/apple-touch-icon-114x114-precomposed.png"});
      favicons.push({rel: "apple-touch-icon-precomposed", sizes: "72x72", href: "/apple-touch-icon-72x72-precomposed.png"});
      favicons.push({rel: "apple-touch-icon-precomposed", sizes: "57x57", href: "/apple-touch-icon-57x57-precomposed.png"});
      favicons.push({rel: "apple-touch-icon", sizes: "180x180", href: "/apple-touch-icon-180x180.png"});
      favicons.push({rel: "apple-touch-icon", sizes: "167x167", href: "/apple-touch-icon-167x167.png"});
      favicons.push({rel: "apple-touch-icon", sizes: "152x152", href: "/apple-touch-icon-152x152.png"});
      favicons.push({rel: "apple-touch-icon", sizes: "144x144", href: "/apple-touch-icon-144x144.png"});
      favicons.push({rel: "apple-touch-icon", sizes: "120x120", href: "/apple-touch-icon-120x120.png"});
      favicons.push({rel: "apple-touch-icon", sizes: "114x114", href: "/apple-touch-icon-114x114.png"});
      favicons.push({rel: "apple-touch-icon", sizes: "72x72", href: "/apple-touch-icon-72x72.png"});
      favicons.push({rel: "apple-touch-icon", sizes: "57x57", href: "/apple-touch-icon-57x57.png"});
    }
     */

    return favicons;
  }

  sendFaviconUrls = function() {
    webkit.messageHandlers.$<message_handler>.postMessage(getFavicons());
  };

  window.addEventListener('hashchange', function(evt) {
    sendFaviconUrls();
  });

  sendFaviconUrls();
}());
