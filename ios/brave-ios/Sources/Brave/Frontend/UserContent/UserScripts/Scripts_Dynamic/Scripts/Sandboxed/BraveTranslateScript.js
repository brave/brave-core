window.__firefox__.includeOnce("BraveTranslate", function($) {
  function getAllTextNodes() {
    let element = document.body
    var children = [].slice.call(element.childNodes), found = 0;
    while (children.length > found) {
      children = children.concat([].slice.call(children[found].childNodes));
      ++found;
    }

    // Filter for "Text" elements that is not a JS element
    return children.filter((element) => element.constructor.name == "Text" &&
                           element.parentElement.constructor.name != "HTMLScriptElement")
//
//    var walker = document.createTreeWalker(document.body, NodeFilter.SHOW_TEXT)
//    var textNodes = []
//    while(node = walker.nextNode()) {
//      textNodes.push(node)
//    }
//    return textNodes
  }

  Object.defineProperty(window.__firefox__, "$<brave_translate_script>", {
    enumerable: false,
    configurable: false,
    writable: false,
    value: Object.freeze({
      "getPageSource": $(function() {
        return encodeURIComponent(document.documentElement.outerHTML);
      }),
      "getPageLanguage": $(function() {
        return document.documentElement.lang;
      }),
      "getRawPageSource": $(function() {
        return document.documentElement.outerText;
      }),
      "start": $(function() {
        var nodes = getAllTextNodes().filter((node) => {
          if (node.translationStatus && node.translationStatus == 1) {
            return false;
          }
          return node.data && node.data.trim().length > 0;
        })
        webkit.messageHandlers.$<message_handler>.postMessage({"text": nodes.map((node) => node.data.trim()) }).then((translatedBatch) => {
          if (translatedBatch.length > 0) {
            for (var i = 0; i < translatedBatch.length; i++) {
              nodes[i].replaceData(0, nodes[i].length, translatedBatch[i])
            }
          } else {
            //              node.translationStatus = 0;
          }
        })
//        while(nodes.length) {
//          let batchedNodes = nodes.splice(0, 50)
//          node.translationStatus = 1
//          webkit.messageHandlers.$<message_handler>.postMessage({"text": batchedNodes.map((node) => node.data.trim()) }).then((translatedBatch) => {
//            if (translatedBatch.length > 0) {
//              for (i = 0; i < translatedBatch.length; i++) {
//                batchedNodes.replaceData(0, node.length, translatedBatch[i])
//              }
//            } else {
////              node.translationStatus = 0;
//            }
//          })
//        }
      })
    })
  });
});
