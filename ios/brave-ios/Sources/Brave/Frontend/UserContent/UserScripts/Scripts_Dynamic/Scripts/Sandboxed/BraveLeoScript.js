// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

window.__firefox__.includeOnce('BraveLeoScript', function($) {

  const kRolesToSkip = [
    "audio", "banner", "button", "complementary",
    "contentinfo", "footer", "img", "label", "navigation",
    "textbox", "combobox", "listbox", "checkbox", "radiobutton",
    "slider", "spinbutton", "searchbox"
  ];
  
  const kTagsToSkip = [
    "AUDIO", "HEADER", "BUTTON", "ASIDE",
    "FOOTER", "IMG", "PICTURE", "LABEL", "NAV",
    "INPUT", "BUTTON", "SEARCH", "STYLE"
  ];
  
  // Walk the node tree to find <main> and <article> tags
  // Returns the child tags of <main> and <article>
  function getRootNodes() {
    // Could use TreeWalker but it doesn't let us filter nested nodes.
    var result = [];
    var queue = [document.querySelector('html')];
    while(queue.length != 0) {
      const node = queue.pop();
      if ((node.role == "main" || node.nodeName == "MAIN") ||
          (node.role == "article" || node.nodeName == "ARTICLE")) {
        result.push(node);
        continue;
      }

      for (const child of node.childNodes) {
        queue.push(child);
      }
    }
    return result;
  }
  
  // Walk the `root` node tree and find all tags we're interested in
  // Filters out all the `TagsToSkip` and returns all the child tags of `root`
  function getContentNodes(root) {
    var result = [];
    var queue = [root];
    while(queue.length != 0) {
      const node = queue.pop();
      if (node.role && kRolesToSkip.includes(node.role)) {
        continue;
      }

      if (kTagsToSkip.includes(node.tagName)) {
        continue;
      }

      //result.push(node);
  
      for (const child of node.childNodes) {
        queue.push(child);
      }
    }
    return result;
  }
  
  // Iterate the <main> and <article> tags
  // Filter out the TagsToSkip
  // Filter further for only Text Nodes
  // Join all the text nodes using " " (space)
  // Return the processed text
  function getTextNodes() {
    const rootNodes = getRootNodes();
    var contentNodes = [];

    for (const node of rootNodes) {
      contentNodes.push(...getContentNodes(node));
    }
    
    var textNodes = [];
    while(contentNodes.length != 0) {
      const node = contentNodes.pop();
      
      if (node.nodeType == Node.TEXT_NODE) {
        textNodes.push(node.wholeText);  // node.data
      }
      
      for (const child of node.childNodes) {
        contentNodes.push(child);
      }
    }

    return textNodes.join(" ");
  }

  Object.defineProperty(window.__firefox__, '$<getMainArticle>', {
      enumerable: false,
      configurable: false,
      writable: false,
      value:
      function(token) {
        if (token != SECURITY_TOKEN) {
          return null;
        }
        
        const mainArticleText = getTextNodes();
        return mainArticleText.length != 0 ? mainArticleText : document.body.innerText;
      }
  });
});
