// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

window.__firefox__.includeOnce("AutoplayBlocking", function($) {
  // Remove autoplay parameter from iframe URLs.
  // e.g This stops YouTube embed from auto playing even when autoplay=1
  function removeAutoplayFromURL(url) {
    if (!url || typeof url !== 'string') {
      return url;
    }

    try {
      const urlObj = new URL(url);
      // Remove autoplay parameter
      urlObj.searchParams.delete('autoplay');
      // Also remove it from the hash if present
      if (urlObj.hash) {
        const hashParams = new URLSearchParams(urlObj.hash.substring(1));
        hashParams.delete('autoplay');
        urlObj.hash = hashParams.toString() ? '#' + hashParams.toString() : '';
      }
      return urlObj.toString();
    } catch (e) {
      // If URL parsing fails, try simple string replacement
      return url.replace(/[?&]autoplay=1/gi, '')
        .replace(/autoplay=1[&]?/gi, '');
    }
  }

  // Process iframe elements to remove autoplay
  function sanitizeIframe(iframe) {
    if (!iframe || iframe.tagName !== 'IFRAME') {
      return;
    }

    // Check if it's a YouTube iframe
    const src = iframe.getAttribute('src') || iframe.src || '';
    // We are focused on YouTube for now.
    // If and when we encounter other embeds where iOS's
    // mediaTypesRequiringUserActionForPlayback = .all then we will handle
    // them separately here.
    const isYouTube = src.includes('youtube.com/embed') ||
                      src.includes('youtu.be') ||
                      src.includes('youtube-nocookie.com/embed') ||
                      src.includes('youtube.com/v/') ||
                      src.includes('youtube-nocookie.com/v/');

    if (src && isYouTube) {
      const newSrc = removeAutoplayFromURL(src);
      if (newSrc !== src) {
        // Remove src first to prevent loading, then set new src
        iframe.removeAttribute('src');
        iframe.src = '';
        // Use setTimeout to ensure the change takes effect
        setTimeout($(function() {
          iframe.setAttribute('src', newSrc);
          iframe.src = newSrc;
        }), 0);
      }
    }
  }

  // Sanitize video elements to prevent autoplay
  function sanitizeVideo(video) {
    if (!video || video.tagName !== 'VIDEO') {
      return;
    }

    // Remove autoplay attribute
    if (video.hasAttribute('autoplay')) {
      video.removeAttribute('autoplay');
    }
  }

  // Sanitize all existing iframes and videos
  function sanitizeExistingElements() {
    document.querySelectorAll('iframe').forEach(sanitizeIframe);
    document.querySelectorAll('video').forEach(sanitizeVideo);
  }

  // Observe and catch dynamically added elements
  const observer = new MutationObserver($(function(mutations) {
    mutations.forEach($(function(mutation) {
      mutation.addedNodes.forEach($(function(node) {
        if (node.nodeType === Node.ELEMENT_NODE) {
          if (node.tagName === 'IFRAME') {
            sanitizeIframe(node);
          } else if (node.tagName === 'VIDEO') {
            sanitizeVideo(node);
          } else {
            // Check for iframes and videos within the added node
            if (node.querySelectorAll) {
              node.querySelectorAll('iframe').forEach(sanitizeIframe);
              node.querySelectorAll('video').forEach(sanitizeVideo);
            }
          }
        }
      }));
    }));
  }));

  // Observe the entire document
  observer.observe(document, {
    childList: true,
    subtree: true,
    attributes: true,
    attributeFilter: ['src']
  });

  // Sanitize existing elements on load
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', $(function() {
      sanitizeExistingElements();
    }));
  } else {
    sanitizeExistingElements();
  }

  // Intercept iframe src attribute changes via setAttribute
  const originalSetAttribute = Element.prototype.setAttribute;
  Element.prototype.setAttribute = $(function(name, value) {
    if (name === 'src' &&
        this.tagName === 'IFRAME' &&
        typeof value === 'string') {
      const newValue = removeAutoplayFromURL(value);
      originalSetAttribute.call(this, name, newValue);
      sanitizeIframe(this);
    } else {
      originalSetAttribute.call(this, name, value);
    }
  });

  // Intercept direct property assignment to src
  const iframeProto = HTMLIFrameElement.prototype;
  const originalSrcDescriptor = Object.getOwnPropertyDescriptor(iframeProto,
                                                                'src');
  if (originalSrcDescriptor) {
    Object.defineProperty(iframeProto, 'src', {
      get: $(function() {
        return originalSrcDescriptor.get.call(this);
      }),
      set: $(function(value) {
        if (typeof value === 'string') {
          const newValue = removeAutoplayFromURL(value);
          originalSrcDescriptor.set.call(this, newValue);
          sanitizeIframe(this);
        } else {
          originalSrcDescriptor.set.call(this, value);
        }
      }),
      enumerable: originalSrcDescriptor.enumerable,
      configurable: originalSrcDescriptor.configurable
    });
  }
});

