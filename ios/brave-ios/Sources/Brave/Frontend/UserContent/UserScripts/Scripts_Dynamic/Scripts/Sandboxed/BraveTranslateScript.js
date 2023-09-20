
window.__firefox__.includeOnce("BraveTranslate", function($) {
  function isVisible(element) {
    if (!element) {
      return false;
    }
    
    var style = window.getComputedStyle(element);
    return  style.width !== "0" &&
            style.height !== "0" &&
            style.opacity !== "0" &&
            style.display!=='none' &&
            style.visibility!== 'hidden';
  }
  
  function isScrolledIntoVisibleViewport(element, acceptsPartiallyVisible) {
    if (!element) {
      return false;
    }
    
    var rect = element.getBoundingClientRect();
    var elementTop = rect.top;
    var elementBottom = rect.bottom;

    if (acceptsPartiallyVisible) {
      return elementTop < window.innerHeight && elementBottom >= 0;
    }
    
    return (elementTop >= 0) && (elementBottom <= window.innerHeight);
  }
  
  function nestedChildNodes(element) {
    var children = [].slice.call(element.childNodes), found = 0;
    while (children.length > found) {
      children = children.concat([].slice.call(children[found].childNodes));
      ++found;
    }
    
    // Filter for "Text" elements that is not a JS element
    return children.filter((element) => element.constructor.name == "Text" && 
                           element.parentElement.constructor.name != "HTMLScriptElement")
  }
  
  function nestedVisibleChildNodes(element) {
    return nestedChildNodes(element)
              .filter((element) => element.parentNode.getClientRects().length > 0) // Filter for elements that have hidden parents
              .filter((element) => isScrolledIntoVisibleViewport(element.parentElement, true)) // Filter for elements that are visible to the viewport
              .filter((element) => isVisible(element.parentElement)); // Filter for elements hidden by CSS
  }
  
  function getClientRects(element) {
    var range = document.createRange();
    range.selectNodeContents(element);
    return range.getClientRects();
  }
  
  function throttle(callback, limit) {
      var waiting = false;
      return function () {
          if (!waiting) {
              callback.apply(this, arguments);
              waiting = true;
              setTimeout(function () {
                  waiting = false;
              }, limit);
          }
      }
  }
  
  var didAddScrollListener = false;
  function addScrollListener() {
    window.addEventListener("scroll", function(e) {
      (throttle(function() {
        let nodes = nestedVisibleChildNodes(document.body);
        for (const node of nodes) {
          if (node.translationStatus && node.translationStatus == 1) {
            continue;
          }
          
          node.translationStatus = 1;
          if (String(node.data || '').trim().length === 0) {
            continue;
          }
          
          webkit.messageHandlers.$<message_handler>.postMessage({"text": node.data, "encoded": encodeURIComponent(node.data)}).then((text) => {
            if (text) {
              node.replaceData(0, node.length, decodeURIComponent(text));
            } else {
              node.translationStatus = 0;
            }
          });
        }
      }, 200))();
    });
  }
  
  /*class Stack {
    constructor() {
      this.nodes = [];
    }
    
    push(e) {
      this.nodes.push(e);
    }
    
    pop() {
      if (this.items.length == 0) {
        return undefined;
      }
      return this.nodes.pop();
    }
    
    peek() {
      return this.nodes[this.nodes.length - 1];
    }
    
    isEmpty() {
      return this.nodes.length == 0;
    }
  }
  
  let mutationObserver = new MutationObserver(function(mutations, observer) {
    if (this.g && this.A) {
      for (const i = 0; i < mutations.length; ++i) {
        if (mutations[i].target && mutations[i].target.className &&
            typeof mutations[i].target.className === "string" &&
            (mutations[i].target.className.indexOf("translate") >= 0 || mutations[i].target.className.indexOf("goog-") == 0)) {
          return;
        }
      }
      
      this.fa.add((0, _.y)(this.Fk, this))
    }
  })
  
  mutationObserver.observe(document.body, { attributes: true, childList: true, characterData: true, subtree: true });
  
  function isObjectOrFunction(node) {
    let type = typeof node;
    return (type == "object" && node != null) || (type == "function");
  }
  
  function isElementNode(node) {
    return isObjectOrFunction(a) && node.nodeType == Node.ELEMENT_NODE;
  }
  
//  let a = [].concat(document.documentElement);
//  for (const i = 0; i < a.length; ++i) {
//    if (isElementNode(a[i])) {
//      if (a[i].classList) {
//        a[i].classList.add("translated-ltr");
//      } else if (!a.hasClass("translated-ltr")) {
//        a.setClassAttribute("translated-ltr");
//      }
//    }
//  }
  
  function getNodeComputedStyle(node) {
    let document = (node.nodeType == Node.DOCUMENT_NODE ? node : node.ownerDocument || node.document);
    if (document.defaultView && document.defaultView.getComputedStyle) {
      return document.defaultView.getComputedStyle(node);
    }
    return node.currentStyle;
  }
  
  function isNodeVisible(node) {
    if (node.nodeType == Node.TEXT_NODE) {
      return true;
    }
    
    if (node.nodeType != Node.ELEMENT_NODE) {
      return false;
    }
    
    if (node.offsetWidth && node.offsetHeight) {
      return true;
    }
    
    let style = getNodeComputedStyle(node); // CSSStyleDeclaration
    if (!!style && style.display != "none" && style.visibility != "hidden") {
      return true;
    }
    return false;
  }
  
  //Called on each node
  function isNodeTranslated(node) {
    if (!node) {
      return false;
    }
    
    let uw = "_gt_4f851m3boqt";
    let vw = "_gtn_uioixuvw7b";
    
    if (node.nodeType != Node.TEXT_NODE || !_.E) {
      return uw in node && !!node[uw];
    }
    
    if (!node.parentNode) {
      return true;
    }
    
    if (!(vw in node.parentNode)) {
      return false;
    }
    
    var b = node.parentNode[vw];
    if (!b || !b[node.nodeValue]) {
      return false;
    }
    
    b = b[node.nodeValue];
    for (const i = 0; i < b.length; ++i) {
      if (b[i] == node) {
        return true;
      }
    }
    return false;
  }
  
  let ax = function(node) {
    if (node.nodeType == Node.TEXT_NODE) {
      return true;
    }
    
    if (node.nodeType != Node.ELEMENT_NODE) {
      return false;
    }
    
    let tags = ["A", "ABBR", "ACRONYM", "B", "BASEFONT", "BDO", "BIG", "CITE", "DFN", "EM", "FONT", "I", "INPUT", "NOBR", 
                "LABEL", "Q", "S", "SMALL", "SPAN", "STRIKE", "STRONG", "SUB", "SUP", "TEXTAREA", "TT", "U", "VAR"];
    
    let style = getNodeComputedStyle(node); // CSSStyleDeclaration
    return tags.includes(node.tagName) && (!style || !style.display || style.display == "inline");
  }
  
  let observer = new IntersectionObserver(function(entries, observer) {
    for (const i = 0; i < entries.length; ++i) {
      if (entries[i].intersectionRatio >= 1) {
        console.log(entries[i]);
        debugger;
        //this.l();
        break;
      }
    }
  }, {
    root: null,
    rootMargin: "200px",
    threshold: [0],
  });
  
  
  class Node {
    constructor(element, b) {
      this._value = { done: true, value: undefined }
      
      this.h = false;
      this.node = null;
      this.g = 0;
      this.cg = false;
      this.j = !b;
      
      if (element) {
        this.update(element, 1);
      }
      
      this.depth = element.nodeType === Node.TEXT_NODE ? 0 : 1;
    }
    
    update(element, c, depth) {
      this.node = element;
      if (this.node) {
        if (typeof c === "number") {
          this.g = c;
        } else if (this.node.nodeType != Node.ELEMENT_NODE) {
          this.g = 0;
        } else if (this.h) {
          this.g = -1;
        } else {
          this.g = 1;
        }
      }
      
      if (typeof depth === "number") {
        this.depth = depth;
      }
    }
    
    next() {
      if (this.bg) {
        if (!this.node || this.j && this.depth == 0) {
          return this._value;
        }
        
        var node = this.node;
        var b = this.h ? -1 : 1;
        if (this.g == b) {
          var child = this.h ? node.lastChild : node.firstChild;
          if (child) {
            this.update(child);
          } else {
            this.update(node, -b);
          }
        } else {
          var sibling = this.h ? node.previousSibling : node.nextSibling;
          if (sibling) {
            this.update(sibling);
          } else {
            this.update(node.parentNode, -b);
          }
        }
        
        this.depth += this.g * (this.h ? -1 : 1);
      } else {
        this.bg = true;
      }
      
      node = this.node;
      if (node) {
        return { value: node, done: false };
      }
      return this._value;
    }
  }
  
  class Item {
    constructor(root) {
      this.j = null;
      this.h = [];
      this.done = false;
      
      for (this.leaf = new Node(root, undefined); root = root.parentNode;) {
        this.at(root, true);
      }
      
      this.h.push(false);
      this.h.reverse();
      for (var i = 1; i < this.h.length; ++i) {
        if (this.h[i] == null) {
          this.h[i] = this.h[i - 1];
        }
      }
    }
    
    at(element, c) {
      c = c === undefined ? false : c;
      var tag = (element.style && element.style.whiteSpace || "").substring(0, 3);
      if (tag === "pre" || !tag && element.tagName === "PRE") {
        this.h.push(true);
      } else if (tag && tag !== "pre") {
        this.h.push(false);
      } else if (c) {
        this.h.push(null);
      } else {
        this.h.push(this.h[this.h.length - 1]);
      }
    }
    
    peek() {
      return !!this.h[this.h.length - 1];
    }
    
    node() {
      return this.leaf.node
    }
    
    depth() {
      return this.leaf.depth;
    }
    
    next() {
      try {
        if (this.j && this.j.length > 0 && this.leaf.g === -1) {
          --this.j.length;
        }
        
        if (this.leaf.g === -1) {
          --this.h.length;
        }
        
        if (this.j && this.j.length > 0 && this.leaf.g !== 1 && !this.leaf.node.nextSibling) {
          this.leaf.update(this.j[this.j.length - 1], -1, this.leaf.depth - 1);
        }
        else {
          if (this.leaf.next().done) {
            this.done = true;
            return
          }
          
          if (this.j && this.leaf.g === 1) {
            this.j.push(this.leaf.node);
          }
        }
        
        if (this.leaf.g === 1 && this.leaf.bg) {
          this.at(this.leaf.node);
        }
      } catch(error) {
        console.error(error);
        this.done = true;
      }
    }
  }
  
  let hasClass = function(node, cls) {
    return node.classList ? node.classList.contains(cls) : false;
  }
  
  fx = function(a, node) {
      let shouldTranslate = !isNodeTranslated(node) && a.o[a.o.length - 1];
      a.o.push(shouldTranslate);
  }
  
  dt = function(a) {
      var b = a.h ? -1 : 1;
      a.g == b && (a.g = -1 * b,
      a.depth += a.g * (a.h ? -1 : 1))
  }
  
  let item = new Item(document.documentElement);
  while (!item.done) {
    item.next();
    
    let node = item.node();
    var d = item.g.g;
    if (d == -1) {
      item.o.pop();
      ax(node) || (b.continuous = !1);
      break;
    }
    
    fx(item, node);
    d = 1 == d;
    
    let buttonTypes = ["button", "submit"];
    var e = !!node && ((node.nodeType == Node.TEXT_NODE && typeof node.nodeValue === "string") ||
                       (node.tagName == "TITLE" && typeof node.value == "string") ||
                       (node.tagName == "TEXTAREA" && hasClass(node, "translate")) ||
                       (node.tagName == "INPUT" && (buttonTypes[node.type] || hasClass(node, "translate"))));
    
    if ((e || d) && !isNodeVisible(node)) {
      //Yw(this, a, hx(this), true)
      let c = a.o[a.o.length - 1];
      //a.F.push({ root: b, Vk: c === undefined || c })
      dt(item.g),;
      item.o.pop();
    }
    
    if (d && hx(this) && (this.J && cx(this, a, "title")), !!a.h[a.h.length - 1]) {
      
    }
  }*/
  
  /*
  
  var e = !!a && (a.nodeType == Node.TEXT_NODE && "string" === typeof a.nodeValue ||
                  "TITLE" == a.tagName && "string" === typeof a.value ||
                  "TEXTAREA" == a.tagName && au(a, "translate") ||
                  "INPUT" == a.tagName && (Xs[a.type] || au(a, "translate")));
                  if ((e || d) && this.G && !cx(a))
                      return Uw(this, a, dx(this), !0),
                      $s(this.g.g),
                      $w(this),
                      this.h;*/
  
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
        if (!didAddScrollListener) {
          didAddScrollListener = true;
          addScrollListener();
          
          (function() {
            let nodes = nestedVisibleChildNodes(document.body);
            for (const node of nodes) {
              if (node.translationStatus && node.translationStatus == 1) {
                continue;
              }
              
              node.translationStatus = 1;
              if (String(node.data || '').trim().length === 0) {
                continue;
              }
              
              webkit.messageHandlers.$<message_handler>.postMessage({"text": node.data, "encoded": encodeURIComponent(node.data)}).then((text) => {
                if (text) {
                  node.replaceData(0, node.length, decodeURIComponent(text));
                } else {
                  node.translationStatus = 0;
                }
              });
            }
          })();
        }
      })
    })
  });

  document.addEventListener('readystatechange', $(function(){
    let eventHandler = $(function(e) {
      if (e.target.readyState === "complete") {
        $(function() {
//          webkit.messageHandlers.$<message_handler>.postMessage("test");
        })();
      }
    });
  
    return eventHandler;
  })());
});
