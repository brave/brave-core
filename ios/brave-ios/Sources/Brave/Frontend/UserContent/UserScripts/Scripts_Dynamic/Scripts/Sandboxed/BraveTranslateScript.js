window.__firefox__ = {};

Object.defineProperty(window.__firefox__, "$<brave_translate_script>", {
  enumerable: false,
  configurable: false,
  writable: false,
  value: {
    "useNativeNetworking": true,
    "getPageSource": (function() {
      return encodeURIComponent(document.documentElement.outerHTML);
    }),
    "getPageLanguage": (function() {
      return document.documentElement.lang;
    }),
    "getRawPageSource": (function() {
      return document.documentElement.outerText;
    }),
  }
});


//gCrWeb

if (typeof _injected_gcrweb === 'undefined') {
    var _injected_gcrweb = true;
    var gcrweb = function(r) {
        if (!window.__gCrWeb) {
            window.__gCrWeb = {}
        }
        const n = window.__gCrWeb;
        r.gCrWeb = n;
        return r
    }({});
}

//utils.js?
if (typeof _injected_common === 'undefined') {
    var _injected_common = true;
    (function() {
        __gCrWeb.common = {};
        __gCrWeb["common"] = __gCrWeb.common;
        __gCrWeb.common.JSONSafeObject = function e() {};
        __gCrWeb.common.JSONSafeObject.prototype["toJSON"] = null;
        __gCrWeb.common.JSONStringify = JSON.stringify;
        __gCrWeb.stringify = function(e) {
            if (e === null)
                return "null";
            if (e === undefined)
                return "undefined";
            if (typeof e.toJSON == "function") {
                var n = e.toJSON;
                e.toJSON = undefined;
                var t = __gCrWeb.common.JSONStringify(e);
                e.toJSON = n;
                return t
            }
            return __gCrWeb.common.JSONStringify(e)
        };
        __gCrWeb.common.isTextField = function(e) {
            if (!e) {
                return false
            }
            if (e.type === "hidden") {
                return false
            }
            return e.type === "text" || e.type === "email" || e.type === "password" || e.type === "search" || e.type === "tel" || e.type === "url" || e.type === "number"
        };
        __gCrWeb.common.trim = function(e) {
            return e.replace(/^\s+|\s+$/g, "")
        };
        __gCrWeb.common.removeQueryAndReferenceFromURL = function(e) {
            var n = new URL(e);
            const t = e => typeof e !== "string";
            if (t(n.origin) || t(n.protocol) || t(n.pathname)) {
                return ""
            }
            return (n.origin !== "null" ? n.origin : n.protocol) + n.pathname
        };
        __gCrWeb.common.sendWebKitMessage = function(e, n) {
            try {
                var t = window.webkit;
                delete window["webkit"];
                window.webkit.messageHandlers[e].postMessage(n);
                window.webkit = t
            } catch (e) {}
        }
    })();
}


//Frame?
if (typeof _injected_message === 'undefined') {
    var _injected_message = true;
    (function() {
        if (!window.__gCrWeb) {
            window.__gCrWeb = {}
        }
        const e = window.__gCrWeb;
        function n(e, n) {
            try {
                var t = window.webkit;
                delete window["webkit"];
                window.webkit.messageHandlers[e].postMessage(n);
                window.webkit = t
            } catch (e) {}
        }
        function t() {
            if (!e.hasOwnProperty("frameId")) {
                e.frameId = r()
            }
            return e.frameId
        }
        function r() {
            const e = new Uint32Array(4);
            window.crypto.getRandomValues(e);
            let n = "";
            for (const t of e) {
                n += t.toString(16).padStart(8, "0")
            }
            return n
        }
        function o() {
            n("FrameBecameAvailable", {
                crwFrameId: t()
            })
        }
        function i() {
            o();
            const e = window.frames.length;
            for (let n = 0; n < e; n++) {
                const e = window.frames[n];
                if (!e) {
                    continue
                }
                e.postMessage({
                    type: "org.chromium.registerForFrameMessaging"
                }, "*")
            }
        }
        e.message = {
            getFrameId: t,
            getExistingFrames: i
        }
    })();
}

// Navigation
if (typeof _injected_navigation === 'undefined') {
    var _injected_navigation = true;
    (function() {
        if (!window.__gCrWeb) {
            window.__gCrWeb = {}
        }
        const e = window.__gCrWeb;
        function t(e, t) {
            try {
                var s = window.webkit;
                delete window["webkit"];
                window.webkit.messageHandlers[e].postMessage(t);
                window.webkit = s
            } catch (e) {}
        }
        class s {
            name = "DataCloneError";
            code = 25;
            message = "Cyclic structures are not supported."
        }
        class a {
            queuedMessages = [];
            sendQueuedMessages()
            {
                while (this.queuedMessages.length > 0) {
                    try {
                        t("NavigationEventMessage", this.queuedMessages[0]);
                        this.queuedMessages.shift()
                    } catch (e) {
                        break
                    }
                }
            }
            queueNavigationEventMessage(e)
            {
                this.queuedMessages.push(e);
                this.sendQueuedMessages()
            }
        }
        const i = new a;
        const n = JSON.stringify;
        const o = window.history.pushState;
        const d = window.history.replaceState;
        History.prototype.pushState = function(t, a, d) {
            i.queueNavigationEventMessage({
                command: "willChangeState",
                frame_id: e.message.getFrameId()
            });
            let r = "";
            try {
                if (typeof t != "undefined") {
                    r = n(t)
                }
            } catch (e) {
                throw new s
            }
            d = d || window.location.href;
            o.call(history, t, a, d);
            i.queueNavigationEventMessage({
                command: "didPushState",
                stateObject: r,
                baseUrl: document.baseURI,
                pageUrl: d.toString(),
                frame_id: e.message.getFrameId()
            })
        };
        History.prototype.replaceState = function(t, a, o) {
            i.queueNavigationEventMessage({
                command: "willChangeState",
                frame_id: e.message.getFrameId()
            });
            let r = "";
            try {
                if (typeof t != "undefined") {
                    r = n(t)
                }
            } catch (e) {
                throw new s
            }
            o = o || window.location.href;
            d.call(history, t, a, o);
            i.queueNavigationEventMessage({
                command: "didReplaceState",
                stateObject: r,
                baseUrl: document.baseURI,
                pageUrl: o.toString(),
                frame_id: e.message.getFrameId()
            })
        }
    })();
}

// Navigation Hash
(function() {
    if (!window.__gCrWeb) {
        window.__gCrWeb = {}
    }
    const e = window.__gCrWeb;
    function n(e, n) {
        try {
            var w = window.webkit;
            delete window["webkit"];
            window.webkit.messageHandlers[e].postMessage(n);
            window.webkit = w
        } catch (e) {}
    }
    window.addEventListener("hashchange", (() => {
        n("NavigationEventMessage", {
            command: "hashchange",
            frame_id: e.message.getFrameId()
        })
    }))
})();

// Text Fragments
if (typeof _injected_text_fragments === 'undefined') {
    var _injected_text_fragments = true;
    (function() {
        const e = ["ADDRESS", "ARTICLE", "ASIDE", "BLOCKQUOTE", "BR", "DETAILS", "DIALOG", "DD", "DIV", "DL", "DT", "FIELDSET", "FIGCAPTION", "FIGURE", "FOOTER", "FORM", "H1", "H2", "H3", "H4", "H5", "H6", "HEADER", "HGROUP", "HR", "LI", "MAIN", "NAV", "OL", "P", "PRE", "SECTION", "TABLE", "UL", "TR", "TH", "TD", "COLGROUP", "COL", "CAPTION", "THEAD", "TBODY", "TFOOT"];
        const t = /[\t-\r -#%-\*,-\/:;\?@\[-\]_\{\}\x85\xA0\xA1\xA7\xAB\xB6\xB7\xBB\xBF\u037E\u0387\u055A-\u055F\u0589\u058A\u05BE\u05C0\u05C3\u05C6\u05F3\u05F4\u0609\u060A\u060C\u060D\u061B\u061E\u061F\u066A-\u066D\u06D4\u0700-\u070D\u07F7-\u07F9\u0830-\u083E\u085E\u0964\u0965\u0970\u0AF0\u0DF4\u0E4F\u0E5A\u0E5B\u0F04-\u0F12\u0F14\u0F3A-\u0F3D\u0F85\u0FD0-\u0FD4\u0FD9\u0FDA\u104A-\u104F\u10FB\u1360-\u1368\u1400\u166D\u166E\u1680\u169B\u169C\u16EB-\u16ED\u1735\u1736\u17D4-\u17D6\u17D8-\u17DA\u1800-\u180A\u1944\u1945\u1A1E\u1A1F\u1AA0-\u1AA6\u1AA8-\u1AAD\u1B5A-\u1B60\u1BFC-\u1BFF\u1C3B-\u1C3F\u1C7E\u1C7F\u1CC0-\u1CC7\u1CD3\u2000-\u200A\u2010-\u2029\u202F-\u2043\u2045-\u2051\u2053-\u205F\u207D\u207E\u208D\u208E\u2308-\u230B\u2329\u232A\u2768-\u2775\u27C5\u27C6\u27E6-\u27EF\u2983-\u2998\u29D8-\u29DB\u29FC\u29FD\u2CF9-\u2CFC\u2CFE\u2CFF\u2D70\u2E00-\u2E2E\u2E30-\u2E44\u3000-\u3003\u3008-\u3011\u3014-\u301F\u3030\u303D\u30A0\u30FB\uA4FE\uA4FF\uA60D-\uA60F\uA673\uA67E\uA6F2-\uA6F7\uA874-\uA877\uA8CE\uA8CF\uA8F8-\uA8FA\uA8FC\uA92E\uA92F\uA95F\uA9C1-\uA9CD\uA9DE\uA9DF\uAA5C-\uAA5F\uAADE\uAADF\uAAF0\uAAF1\uABEB\uFD3E\uFD3F\uFE10-\uFE19\uFE30-\uFE52\uFE54-\uFE61\uFE63\uFE68\uFE6A\uFE6B\uFF01-\uFF03\uFF05-\uFF0A\uFF0C-\uFF0F\uFF1A\uFF1B\uFF1F\uFF20\uFF3B-\uFF3D\uFF3F\uFF5B\uFF5D\uFF5F-\uFF65]|\uD800[\uDD00-\uDD02\uDF9F\uDFD0]|\uD801\uDD6F|\uD802[\uDC57\uDD1F\uDD3F\uDE50-\uDE58\uDE7F\uDEF0-\uDEF6\uDF39-\uDF3F\uDF99-\uDF9C]|\uD804[\uDC47-\uDC4D\uDCBB\uDCBC\uDCBE-\uDCC1\uDD40-\uDD43\uDD74\uDD75\uDDC5-\uDDC9\uDDCD\uDDDB\uDDDD-\uDDDF\uDE38-\uDE3D\uDEA9]|\uD805[\uDC4B-\uDC4F\uDC5B\uDC5D\uDCC6\uDDC1-\uDDD7\uDE41-\uDE43\uDE60-\uDE6C\uDF3C-\uDF3E]|\uD807[\uDC41-\uDC45\uDC70\uDC71]|\uD809[\uDC70-\uDC74]|\uD81A[\uDE6E\uDE6F\uDEF5\uDF37-\uDF3B\uDF44]|\uD82F\uDC9F|\uD836[\uDE87-\uDE8B]|\uD83A[\uDD5E\uDD5F]/u;
        const n = "text-fragments-polyfill-target-text";
        const u = (e, t=document) => {
            const n = [];
            const u = t.createRange();
            u.selectNodeContents(t.body);
            while (!u.collapsed && n.length < 2) {
                let r;
                if (e.prefix) {
                    const n = g(e.prefix, u);
                    if (n == null) {
                        break
                    }
                    i(u, n.startContainer, n.startOffset);
                    const o = t.createRange();
                    o.setStart(n.endContainer, n.endOffset);
                    o.setEnd(u.endContainer, u.endOffset);
                    a(o);
                    if (o.collapsed) {
                        break
                    }
                    r = g(e.textStart, o);
                    if (r == null) {
                        break
                    }
                    if (r.compareBoundaryPoints(Range.START_TO_START, o) !== 0) {
                        continue
                    }
                } else {
                    r = g(e.textStart, u);
                    if (r == null) {
                        break
                    }
                    i(u, r.startContainer, r.startOffset)
                }
                if (e.textEnd) {
                    const a = t.createRange();
                    a.setStart(r.endContainer, r.endOffset);
                    a.setEnd(u.endContainer, u.endOffset);
                    let c = false;
                    while (!a.collapsed && n.length < 2) {
                        const l = g(e.textEnd, a);
                        if (l == null) {
                            break
                        }
                        i(a, l.startContainer, l.startOffset);
                        r.setEnd(l.endContainer, l.endOffset);
                        if (e.suffix) {
                            const i = s(e.suffix, r, u, t);
                            if (i === o.NO_SUFFIX_MATCH) {
                                break
                            } else if (i === o.SUFFIX_MATCH) {
                                c = true;
                                n.push(r.cloneRange());
                                continue
                            } else if (i === o.MISPLACED_SUFFIX) {
                                continue
                            }
                        } else {
                            c = true;
                            n.push(r.cloneRange())
                        }
                    }
                    if (!c) {
                        break
                    }
                } else if (e.suffix) {
                    const a = s(e.suffix, r, u, t);
                    if (a === o.NO_SUFFIX_MATCH) {
                        break
                    } else if (a === o.SUFFIX_MATCH) {
                        n.push(r.cloneRange());
                        i(u, u.startContainer, u.startOffset);
                        continue
                    } else if (a === o.MISPLACED_SUFFIX) {
                        continue
                    }
                } else {
                    n.push(r.cloneRange())
                }
            }
            return n
        };
        const r = (e, t=document) => {
            for (const n of e) {
                const e = t.createRange();
                e.selectNodeContents(n);
                const u = e.extractContents();
                const r = n.parentNode;
                r.insertBefore(u, n);
                r.removeChild(n)
            }
        };
        const o = {
            NO_SUFFIX_MATCH: 0,
            SUFFIX_MATCH: 1,
            MISPLACED_SUFFIX: 2
        };
        const s = (e, t, n, u) => {
            const r = u.createRange();
            r.setStart(t.endContainer, t.endOffset);
            r.setEnd(n.endContainer, n.endOffset);
            a(r);
            const s = g(e, r);
            if (s == null) {
                return o.NO_SUFFIX_MATCH
            }
            if (s.compareBoundaryPoints(Range.START_TO_START, r) !== 0) {
                return o.MISPLACED_SUFFIX
            }
            return o.SUFFIX_MATCH
        };
        const i = (e, t, n) => {
            try {
                e.setStart(t, n + 1)
            } catch (n) {
                e.setStartAfter(t)
            }
        };
        const a = e => {
            const t = c(e);
            let n = t.nextNode();
            while (!e.collapsed && n != null) {
                if (n !== e.startContainer) {
                    e.setStart(n, 0)
                }
                if (n.textContent.length > e.startOffset) {
                    const t = n.textContent[e.startOffset];
                    if (!t.match(/\s/)) {
                        return
                    }
                }
                try {
                    e.setStart(n, e.startOffset + 1)
                } catch (u) {
                    n = t.nextNode();
                    if (n == null) {
                        e.collapse()
                    } else {
                        e.setStart(n, 0)
                    }
                }
            }
        };
        const c = e => {
            const t = document.createTreeWalker(e.commonAncestorContainer, NodeFilter.SHOW_TEXT | NodeFilter.SHOW_ELEMENT, (t => D(t, e)));
            return t
        };
        const l = (t, u=document) => {
            if (t.startContainer.nodeType != Node.TEXT_NODE || t.endContainer.nodeType != Node.TEXT_NODE)
                return [];
            if (t.startContainer === t.endContainer) {
                const e = u.createElement("mark");
                e.setAttribute("class", n);
                t.surroundContents(e);
                return [e]
            }
            const r = t.startContainer;
            const o = t.cloneRange();
            o.setEndAfter(r);
            const s = t.endContainer;
            const i = t.cloneRange();
            i.setStartBefore(s);
            const a = [];
            t.setStartAfter(r);
            t.setEndBefore(s);
            const c = u.createTreeWalker(t.commonAncestorContainer, NodeFilter.SHOW_ELEMENT | NodeFilter.SHOW_TEXT, {
                acceptNode: function(n) {
                    if (!t.intersectsNode(n))
                        return NodeFilter.FILTER_REJECT;
                    if (e.includes(n.tagName) || n.nodeType === Node.TEXT_NODE)
                        return NodeFilter.FILTER_ACCEPT;
                    return NodeFilter.FILTER_SKIP
                }
            });
            let l = c.nextNode();
            while (l) {
                if (l.nodeType === Node.TEXT_NODE) {
                    const e = u.createElement("mark");
                    e.setAttribute("class", n);
                    l.parentNode.insertBefore(e, l);
                    e.appendChild(l);
                    a.push(e)
                }
                l = c.nextNode()
            }
            const F = u.createElement("mark");
            F.setAttribute("class", n);
            o.surroundContents(F);
            const d = u.createElement("mark");
            d.setAttribute("class", n);
            i.surroundContents(d);
            return [F, ...a, d]
        };
        const F = e => {
            const t = {
                behavior: "auto",
                block: "center",
                inline: "nearest"
            };
            e.scrollIntoView(t)
        };
        const d = e => {
            let t = e;
            while (t != null && !(t instanceof HTMLElement))
                t = t.parentNode;
            if (t != null) {
                const e = window.getComputedStyle(t);
                if (e.visibility === "hidden" || e.display === "none" || e.height === 0 || e.width === 0 || e.opacity === 0) {
                    return false
                }
            }
            return true
        };
        const f = (e, t) => {
            if (t != null && !t.intersectsNode(e))
                return NodeFilter.FILTER_REJECT;
            return d(e) ? NodeFilter.FILTER_ACCEPT : NodeFilter.FILTER_REJECT
        };
        const D = (e, t) => {
            if (t != null && !t.intersectsNode(e))
                return NodeFilter.FILTER_REJECT;
            if (!d(e)) {
                return NodeFilter.FILTER_REJECT
            }
            return e.nodeType === Node.TEXT_NODE ? NodeFilter.FILTER_ACCEPT : NodeFilter.FILTER_SKIP
        };
        const E = (t, n) => {
            const u = [];
            let r = [];
            const o = Array.from(A(t, (e => f(e, n))));
            for (const t of o) {
                if (t.nodeType === Node.TEXT_NODE) {
                    r.push(t)
                } else if (t instanceof HTMLElement && e.includes(t.tagName) && r.length > 0) {
                    u.push(r);
                    r = []
                }
            }
            if (r.length > 0)
                u.push(r);
            return u
        };
        const C = (e, t, n) => {
            let u = "";
            if (e.length === 1) {
                u = e[0].textContent.substring(t, n)
            } else {
                u = e[0].textContent.substring(t) + e.slice(1, -1).reduce(((e, t) => e + t.textContent), "") + e.slice(-1)[0].textContent.substring(0, n)
            }
            return u.replace(/[\t\n\r ]+/g, " ")
        };
        function* A(e, t) {
            const n = document.createTreeWalker(e, NodeFilter.SHOW_ELEMENT | NodeFilter.SHOW_TEXT, {
                acceptNode: t
            });
            const u = new Set;
            while (x(n, u) !== null) {
                yield n.currentNode
            }
        }
        const g = (e, t) => {
            const n = E(t.commonAncestorContainer, t);
            const u = S();
            for (const r of n) {
                const n = T(e, t, r, u);
                if (n !== undefined)
                    return n
            }
            return undefined
        };
        const T = (e, t, n, u) => {
            if (!e || !t || !(n || []).length)
                return undefined;
            const r = N(C(n, 0, undefined));
            const o = N(e);
            let s = n[0] === t.startNode ? t.startOffset : 0;
            let i;
            let a;
            while (s < r.length) {
                const e = r.indexOf(o, s);
                if (e === -1)
                    return undefined;
                if (h(r, e, o.length, u)) {
                    i = m(e, n, false);
                    a = m(e + o.length, n, true)
                }
                if (i != null && a != null) {
                    const e = new Range;
                    e.setStart(i.node, i.offset);
                    e.setEnd(a.node, a.offset);
                    if (t.compareBoundaryPoints(Range.START_TO_START, e) <= 0 && t.compareBoundaryPoints(Range.END_TO_END, e) >= 0) {
                        return e
                    }
                }
                s = e + 1
            }
            return undefined
        };
        const m = (e, t, n) => {
            let u = 0;
            let r;
            for (let o = 0; o < t.length; o++) {
                const s = t[o];
                if (!r)
                    r = N(s.data);
                let i = u + r.length;
                if (n)
                    i += 1;
                if (i > e) {
                    const t = e - u;
                    let o = Math.min(e - u, s.data.length);
                    const i = n ? r.substring(0, t) : r.substring(t);
                    let a = n ? N(s.data.substring(0, o)) : N(s.data.substring(o));
                    const c = (n ? -1 : 1) * (i.length > a.length ? -1 : 1);
                    while (o >= 0 && o <= s.data.length) {
                        if (a.length === i.length) {
                            return {
                                node: s,
                                offset: o
                            }
                        }
                        o += c;
                        a = n ? N(s.data.substring(0, o)) : N(s.data.substring(o))
                    }
                }
                u += r.length;
                if (o + 1 < t.length) {
                    const e = N(t[o + 1].data);
                    if (r.slice(-1) === " " && e.slice(0, 1) === " ") {
                        u -= 1
                    }
                    r = e
                }
            }
            return undefined
        };
        const h = (e, n, u, r) => {
            if (n < 0 || n >= e.length || u <= 0 || n + u > e.length) {
                return false
            }
            if (r) {
                const t = r.segment(e);
                const o = t.containing(n);
                if (!o)
                    return false;
                if (o.isWordLike && o.index != n)
                    return false;
                const s = n + u;
                const i = t.containing(s);
                if (i && i.isWordLike && i.index != s)
                    return false
            } else {
                if (e[n].match(t)) {
                    ++n;
                    --u;
                    if (!u) {
                        return false
                    }
                }
                if (e[n + u - 1].match(t)) {
                    --u;
                    if (!u) {
                        return false
                    }
                }
                if (n !== 0 && !e[n - 1].match(t))
                    return false;
                if (n + u !== e.length && !e[n + u].match(t))
                    return false
            }
            return true
        };
        const N = e => (e || "").normalize("NFKD").replace(/\s+/g, " ").replace(/[\u0300-\u036f]/g, "").toLowerCase();
        const S = () => {
            if (Intl.Segmenter) {
                let e = document.documentElement.lang;
                if (!e) {
                    e = navigator.languages
                }
                return new Intl.Segmenter(e, {
                    granularity: "word"
                })
            }
            return undefined
        };
        const x = (e, t) => {
            if (!t.has(e.currentNode)) {
                const t = e.firstChild();
                if (t !== null) {
                    return t
                }
            }
            const n = e.nextSibling();
            if (n !== null) {
                return n
            }
            const u = e.parentNode();
            if (u !== null) {
                t.add(u)
            }
            return u
        };
        if (typeof goog !== "undefined") {
            goog.declareModuleId("googleChromeLabs.textFragmentPolyfill.textFragmentUtils")
        }
        const O = () => {
            const e = document.getElementsByTagName("style");
            if (!e)
                return;
            for (const t of e) {
                const e = t.innerHTML;
                const u = e.match(/(\w*)::target-text\s*{\s*((.|\n)*?)\s*}/g);
                if (!u)
                    continue;
                const r = u.join("\n");
                const o = document.createTextNode(r.replaceAll("::target-text", ` .${n}`));
                t.appendChild(o)
            }
        };
        const p = ({backgroundColor: e, color: t}) => {
            const u = document.getElementsByTagName("style");
            const r = `.${n} {\n    background-color: ${e};\n    color: ${t};\n  }\n  \n  .${n} a, a .${n} {\n    text-decoration: underline;\n  }\n  `;
            if (u.length === 0) {
                document.head.insertAdjacentHTML("beforeend", `<style type="text/css">${r}</style>`)
            } else {
                O();
                const e = document.createTextNode(r);
                u[0].insertBefore(e, u[0].firstChild)
            }
        };
        if (!window.__gCrWeb) {
            window.__gCrWeb = {}
        }
        const _ = window.__gCrWeb;
        function B(e, t) {
            try {
                var n = window.webkit;
                delete window["webkit"];
                window.webkit.messageHandlers[e].postMessage(t);
                window.webkit = n
            } catch (e) {}
        }
        let R;
        let I;
        function L(e, t, n, u) {
            if(R?.length)
                return;
            let r = null;
            if (n && u) {
                r = {
                    backgroundColor: `#${n}`,
                    color: `#${u}`
                }
            }
            if (document.readyState === "complete" || document.readyState === "interactive") {
                b(e, t, r);
                return
            }
            document.addEventListener("DOMContentLoaded", (() => {
                b(e, t, r)
            }))
        }
        function y(e) {
            if (R) {
                r(R);
                R = null
            }
            document.removeEventListener("click", w, true);
            if (e) {
                try {
                    history.replaceState(history.state, "", e)
                } catch (e) {}
            }
        }
        function b(e, t, n) {
            R = [];
            let r = 0;
            if (n)
                p(n);
            for (const t of e) {
                const e = u(t).filter((e => !!e));
                if (Array.isArray(e)) {
                    if (e.length >= 1) {
                        ++r;
                        let t = l(e[0]);
                        if (Array.isArray(t)) {
                            R.push(...t)
                        }
                    }
                }
            }
            if (t && R.length > 0) {
                I = e;
                F(R[0])
            }
            document.addEventListener("click", w, true);
            for (let e of R) {
                e.addEventListener("click", H.bind(e), true)
            }
            B("textFragments", {
                command: "textFragments.processingComplete",
                result: {
                    successCount: r,
                    fragmentsCount: e.length
                }
            })
        }
        function w() {
            B("textFragments", {
                command: "textFragments.onClick"
            })
        }
        function H(e) {
            if (!(e.currentTarget instanceof HTMLElement)) {
                return
            }
            const t = e.currentTarget;
            let n = t.parentNode;
            while (n != null) {
                if (n instanceof HTMLElement && n.tagName == "A") {
                    return
                }
                n = n.parentNode
            }
            B("textFragments", {
                command: "textFragments.onClickWithSender",
                rect: k(t),
                text: `"${t.innerText}"`,
                fragments: I
            })
        }
        function k(e) {
            const t = e.getClientRects()[0];
            return {
                x:t?.x,
                y:t?.y,
                width:t?.width,
                height:t?.height
            }
        }
        _.textFragments = {
            handleTextFragments: L,
            removeHighlights: y
        }
    })();
}

// Annotations
if (typeof _injected_annotations === 'undefined') {
    var _injected_annotations = true;
    (function() {
        const t = new Set(["A", "APP", "APPLET", "AREA", "AUDIO", "BUTTON", "CANVAS", "CHROME_ANNOTATION", "EMBED", "FORM", "FRAME", "FRAMESET", "HEAD", "IFRAME", "IMG", "INPUT", "KEYGEN", "LABEL", "MAP", "NOSCRIPT", "OBJECT", "OPTGROUP", "OPTION", "PROGRESS", "SCRIPT", "SELECT", "STYLE", "TEXTAREA", "VIDEO"]);
        const e = new Set(["A", "LABEL"]);
        const n = 300;
        if (!window.__gCrWeb) {
            window.__gCrWeb = {}
        }
        const o = window.__gCrWeb;
        function i(t, e) {
            try {
                var n = window.webkit;
                delete window["webkit"];
                window.webkit.messageHandlers[t].postMessage(e);
                window.webkit = n
            } catch (t) {}
        }
        class a {
            index;
            left;
            right;
            text;
            type;
            annotationText;
            data;
            constructor(t, e, n, o, i, a, s)
            {
                this.index = t;
                this.left = e;
                this.right = n;
                this.text = o;
                this.type = i;
                this.annotationText = a;
                this.data = s
            }
        }
        class s {
            original;
            replacements;
            constructor(t, e)
            {
                this.original = t;
                this.replacements = e
            }
        }
        class r {
            node;
            index;
            constructor(t, e)
            {
                this.node = t;
                this.index = e
            }
        }
        class c {
            initialEvent;
            hasMutations = false;
            mutationObserver;
            mutationExtendId = 0;
            constructor(t)
            {
                this.initialEvent = t;
                this.mutationObserver = new MutationObserver((t => {
                    for (let e of t) {
                        if (e.target.contains(this.initialEvent.target)) {
                            this.hasMutations = true;
                            this.stopObserving();
                            break
                        }
                    }
                }));
                this.mutationObserver.observe(document, {
                    attributes: false,
                    childList: true,
                    subtree: true
                })
            }
            hasPreventativeActivity(t)
            {
                return t !== this.initialEvent || t.defaultPrevented || this.hasMutations
            }
            extendObservation(t)
            {
                if (this.mutationExtendId) {
                    clearTimeout(this.mutationExtendId)
                }
                this.mutationExtendId = setTimeout(t, n)
            }
            stopObserving()
            {
                if (this.mutationExtendId) {
                    clearTimeout(this.mutationExtendId)
                }
                this.mutationExtendId = 0;
                this.mutationObserver?.disconnect()
            }
        }
        function l() {
            const t = document.getElementsByTagName("meta");
            for (let e = 0; e < t.length; e++) {
                if (t[e].getAttribute("name") === "chrome" && t[e].getAttribute("content") === "nointentdetection") {
                    return true
                }
            }
            return false
        }
        function u() {
            const t = document.getElementsByTagName("meta");
            let e = new Set;
            for (const n of t) {
                if (n.getAttribute("name") !== "format-detection")
                    continue;
                let t = n.getAttribute("content");
                if (!t)
                    continue;
                let o = t.toLowerCase().matchAll(/([a-z]+)\s*=\s*([a-z]+)/g);
                if (!o)
                    continue;
                for (let t of o) {
                    if (t && t[2] === "no" && t[1]) {
                        e.add(t[1])
                    }
                }
            }
            return e
        }
        function d() {
            const t = document.getElementsByTagName("meta");
            for (let e = 0; e < t.length; e++) {
                if (t[e].getAttribute("name") === "google" && t[e].getAttribute("content") === "notranslate") {
                    return true
                }
            }
            return false
        }
        function f(t) {
            const e = document.getElementsByTagName("meta");
            for (let n of e) {
                if (n.httpEquiv.toLowerCase() === t) {
                    return n.content
                }
            }
            return ""
        }
        const h = "#000";
        const m = "rgba(20,111,225,0.25)";
        const g = "border-bottom-width: 1px; " + "border-bottom-style: dotted; " + "background-color: transparent";
        const b = "border-bottom-width: 1px; " + "border-bottom-style: solid; " + "background-color: transparent";
        const E = "blue";
        let p = [];
        let N;
        function T(t, e) {
            if (p.length) {
                w()
            }
            let n = u();
            i("annotations", {
                command: "annotations.extractedText",
                text: O(t),
                seqId: e,
                metadata: {
                    hasNoIntentDetection: l(),
                    hasNoTranslate: d(),
                    htmlLang: document.documentElement.lang,
                    httpContentLanguage: f("content-language"),
                    wkNoTelephone: n.has("telephone"),
                    wkNoEmail: n.has("email"),
                    wkNoAddress: n.has("address"),
                    wkNoDate: n.has("date")
                }
            })
        }
        function x(t) {
            if (p.length || !t.length)
                return;
            let n = 0;
            p = [];
            document.addEventListener("click", L.bind(document));
            document.addEventListener("click", L.bind(document), true);
            t = P(t);
            let o = 0;
            C(((i, s, r) => {
                if (!i.parentNode || r === "\n")
                    return true;
                while (o < t.length) {
                    const e = t[o];
                    if (!e || e.end > s) {
                        break
                    }
                    n++;
                    o++
                }
                const c = r.length;
                let l = [];
                while (o < t.length) {
                    const e = t[o];
                    if (!e) {
                        break
                    }
                    const i = e.start;
                    const u = e.end;
                    if (s < u && s + c > i) {
                        const t = Math.max(0, i - s);
                        const d = Math.min(c, u - s);
                        const f = r.substring(t, d);
                        const h = Math.max(0, s - i);
                        const m = Math.min(u - i, s + c - i);
                        const g = e.text.substring(h, m);
                        if (f != g) {
                            n++;
                            o++;
                            continue
                        }
                        l.push(new a(o, t, d, f, e.type, e.text, e.data));
                        if (u <= s + c) {
                            o++;
                            continue
                        }
                    }
                    break
                }
                let u = i.parentNode;
                while (u) {
                    if (u instanceof HTMLElement && e.has(u.tagName)) {
                        l = [];
                        n++;
                        break
                    }
                    u = u.parentNode
                }
                B(i, l, r);
                return o < t.length
            }));
            n += t.length - o;
            i("annotations", {
                command: "annotations.decoratingComplete",
                successes: t.length - n,
                failures: n,
                annotations: t.length,
                cancelled: []
            })
        }
        function w() {
            for (let t of p) {
                const e = t.replacements;
                const n = e[0].parentNode;
                if (!n)
                    return;
                n.insertBefore(t.original, e[0]);
                for (let t of e) {
                    n.removeChild(t)
                }
            }
            p = []
        }
        function A(t) {
            var e = [];
            for (let s of p) {
                const r = s.replacements;
                const c = r[0].parentNode;
                if (!c)
                    return;
                var n = false;
                var o = false;
                for (let e of r) {
                    if (!(e instanceof HTMLElement)) {
                        continue
                    }
                    var i = e;
                    var a = i.getAttribute("data-type");
                    if (a === t) {
                        n = true
                    } else {
                        o = true
                    }
                }
                if (!n) {
                    e.push(s);
                    continue
                }
                if (!o) {
                    c.insertBefore(s.original, r[0]);
                    for (let t of r) {
                        c.removeChild(t)
                    }
                    continue
                }
                let l = [];
                for (let e of r) {
                    if (!(e instanceof HTMLElement)) {
                        l.push(e);
                        continue
                    }
                    var i = e;
                    var a = i.getAttribute("data-type");
                    if (a !== t) {
                        l.push(e);
                        continue
                    }
                    let n = document.createTextNode(i.textContent ?? "");
                    c.replaceChild(n, i);
                    l.push(n)
                }
                s.replacements = l;
                e.push(s)
            }
            p = e
        }
        function v() {
            for (let t of p) {
                for (let e of t.replacements) {
                    if (!(e instanceof HTMLElement)) {
                        continue
                    }
                    e.style.color = "";
                    e.style.background = ""
                }
            }
        }
        function y(e, n, o=true, i=true) {
            const a = [e];
            let s = 0;
            let r = true;
            while (a.length > 0) {
                let e = a.pop();
                if (!e) {
                    break
                }
                if (e.nodeType === Node.ELEMENT_NODE) {
                    if (t.has(e.nodeName)) {
                        continue
                    }
                    if (e instanceof Element && e.getAttribute("contenteditable")) {
                        continue
                    }
                    if (e.nodeName === "BR") {
                        if (r)
                            continue;
                        if (!n(e, s, "\n"))
                            break;
                        r = true;
                        s += 1;
                        continue
                    }
                    const c = window.getComputedStyle(e);
                    if (i && (c.display === "none" || c.visibility === "hidden")) {
                        continue
                    }
                    if (e.nodeName.toUpperCase() !== "BODY" && c.display !== "inline" && !r) {
                        if (!n(e, s, "\n"))
                            break;
                        r = true;
                        s += 1
                    }
                    if (o) {
                        const t = e;
                        if (t.shadowRoot && t.shadowRoot != e) {
                            a.push(t.shadowRoot);
                            continue
                        }
                    }
                }
                if (e.hasChildNodes()) {
                    for (let t = e.childNodes.length - 1; t >= 0; t--) {
                        a.push(e.childNodes[t])
                    }
                } else if (e.nodeType === Node.TEXT_NODE && e.textContent) {
                    const t = e.textContent.trim() === "";
                    if (t && r)
                        continue;
                    if (!n(e, s, e.textContent))
                        break;
                    r = t;
                    s += e.textContent.length
                }
            }
        }
        function C(t) {
            for (let e of N) {
                const n = e.node.deref();
                if (!n)
                    continue;
                const o = n.nodeType === Node.ELEMENT_NODE ? "\n" : n.textContent;
                if (o && !t(n, e.index, o))
                    break
            }
        }
        function O(t) {
            const e = [];
            N = [];
            y(document.body, (function(n, o, i) {
                N.push(new r(new WeakRef(n), o));
                if (o + i.length > t) {
                    e.push(i.substring(0, t - o))
                } else {
                    e.push(i)
                }
                return o + i.length < t
            }));
            return "".concat(...e)
        }
        let M;
        function k() {
            M?.stopObserving();
            M = null
        }
        function L(t) {
            const e = t.target;
            if (e instanceof HTMLElement && e.tagName === "CHROME_ANNOTATION") {
                if (t.eventPhase === Event.CAPTURING_PHASE) {
                    k();
                    M = new c(t)
                } else if (M) {
                    if (!M.hasPreventativeActivity(t)) {
                        M.extendObservation((() => {
                            if (M) {
                                R(e);
                                I(e, M.hasMutations)
                            }
                        }))
                    } else {
                        I(e, M.hasMutations)
                    }
                }
            } else {
                k()
            }
        }
        function I(t, e) {
            i("annotations", {
                command: "annotations.onClick",
                cancel: e,
                data: t.dataset["data"],
                rect: _(t),
                text: t.dataset["annotation"]
            });
            k()
        }
        function R(t) {
            for (let e of p) {
                for (let n of e.replacements) {
                    if (!(n instanceof HTMLElement)) {
                        continue
                    }
                    if (n.tagName === "CHROME_ANNOTATION" && n.dataset["index"] === t.dataset["index"]) {
                        n.style.color = h;
                        n.style.backgroundColor = m
                    }
                }
            }
        }
        function P(t) {
            t.sort(((t, e) => t.start - e.start));
            let e = undefined;
            return t.filter((t => {
                if (e && e.start < t.end && e.end > t.start) {
                    return false
                }
                e = t;
                return true
            }))
        }
        function B(t, e, n) {
            const o = t.parentNode;
            if (e.length <= 0 || !o) {
                return
            }
            let i = E;
            if (o instanceof Element) {
                i = window.getComputedStyle(o).color || i
            }
            let a = 0;
            const r = [];
            for (let t of e) {
                if (t.left > a) {
                    r.push(document.createTextNode(n.substring(a, t.left)))
                }
                const e = document.createElement("chrome_annotation");
                e.setAttribute("data-index", "" + t.index);
                e.setAttribute("data-data", t.data);
                e.setAttribute("data-annotation", t.annotationText);
                e.setAttribute("data-type", t.type);
                e.setAttribute("role", "link");
                e.textContent = t.text;
                if (t.type == "PHONE_NUMBER" || t.type == "EMAIL") {
                    e.style.cssText = b
                } else {
                    e.style.cssText = g
                }
                e.style.borderBottomColor = i;
                r.push(e);
                a = t.right
            }
            if (a < n.length) {
                r.push(document.createTextNode(n.substring(a, n.length)))
            }
            for (let e of r) {
                o.insertBefore(e, t)
            }
            o.removeChild(t);
            p.push(new s(t, r))
        }
        function _(t) {
            const e = t.getClientRects()[0];
            if (!e) {
                return {}
            }
            return {
                x: e.x,
                y: e.y,
                width: e.width,
                height: e.height
            }
        }
        o.annotations = {
            extractText: T,
            decorateAnnotations: x,
            removeDecorations: w,
            removeDecorationsWithType: A,
            removeHighlight: v
        }
    })();
}

// Frame Setup

if (typeof _injected_setup_frame === 'undefined') {
    var _injected_setup_frame = true;
    (function() {
        if (!window.__gCrWeb) {
            window.__gCrWeb = {}
        }
        const e = window.__gCrWeb;
        function n(e, n) {
            try {
                var t = window.webkit;
                delete window["webkit"];
                window.webkit.messageHandlers[e].postMessage(n);
                window.webkit = t
            } catch (e) {}
        }
        function t() {
            if (!e.hasOwnProperty("frameId")) {
                e.frameId = r()
            }
            return e.frameId
        }
        function r() {
            const e = new Uint32Array(4);
            window.crypto.getRandomValues(e);
            let n = "";
            for (const t of e) {
                n += t.toString(16).padStart(8, "0")
            }
            return n
        }
        function o() {
            n("FrameBecameAvailable", {
                crwFrameId: t()
            })
        }
        o()
    })();
}

// Frame Registration

(function() {
    if (!window.__gCrWeb) {
        window.__gCrWeb = {}
    }
    const e = window.__gCrWeb;
    function n(e, n) {
        try {
            var t = window.webkit;
            delete window["webkit"];
            window.webkit.messageHandlers[e].postMessage(n);
            window.webkit = t
        } catch (e) {}
    }
    function t() {
        if (!e.hasOwnProperty("frameId")) {
            e.frameId = r()
        }
        return e.frameId
    }
    function r() {
        const e = new Uint32Array(4);
        window.crypto.getRandomValues(e);
        let n = "";
        for (const t of e) {
            n += t.toString(16).padStart(8, "0")
        }
        return n
    }
    window.addEventListener("unload", (function() {
        n("FrameBecameUnavailable", t())
    }));
    window.addEventListener("message", (function(n) {
        const t = n.data;
        if (!t || typeof t !== "object") {
            return
        }
        if (t.hasOwnProperty("type") && t.type == "org.chromium.registerForFrameMessaging") {
            e.message.getExistingFrames()
        }
    }))
})();

// Link To Text

if (typeof _injected_link_to_text === 'undefined') {
    var _injected_link_to_text = true;
    (function() {
        if (!window.__gCrWeb) {
            window.__gCrWeb = {}
        }
        const e = window.__gCrWeb;
        const t = ["ADDRESS", "ARTICLE", "ASIDE", "BLOCKQUOTE", "BR", "DETAILS", "DIALOG", "DD", "DIV", "DL", "DT", "FIELDSET", "FIGCAPTION", "FIGURE", "FOOTER", "FORM", "H1", "H2", "H3", "H4", "H5", "H6", "HEADER", "HGROUP", "HR", "LI", "MAIN", "NAV", "OL", "P", "PRE", "SECTION", "TABLE", "UL", "TR", "TH", "TD", "COLGROUP", "COL", "CAPTION", "THEAD", "TBODY", "TFOOT"];
        const s = /[\t-\r -#%-\*,-\/:;\?@\[-\]_\{\}\x85\xA0\xA1\xA7\xAB\xB6\xB7\xBB\xBF\u037E\u0387\u055A-\u055F\u0589\u058A\u05BE\u05C0\u05C3\u05C6\u05F3\u05F4\u0609\u060A\u060C\u060D\u061B\u061E\u061F\u066A-\u066D\u06D4\u0700-\u070D\u07F7-\u07F9\u0830-\u083E\u085E\u0964\u0965\u0970\u0AF0\u0DF4\u0E4F\u0E5A\u0E5B\u0F04-\u0F12\u0F14\u0F3A-\u0F3D\u0F85\u0FD0-\u0FD4\u0FD9\u0FDA\u104A-\u104F\u10FB\u1360-\u1368\u1400\u166D\u166E\u1680\u169B\u169C\u16EB-\u16ED\u1735\u1736\u17D4-\u17D6\u17D8-\u17DA\u1800-\u180A\u1944\u1945\u1A1E\u1A1F\u1AA0-\u1AA6\u1AA8-\u1AAD\u1B5A-\u1B60\u1BFC-\u1BFF\u1C3B-\u1C3F\u1C7E\u1C7F\u1CC0-\u1CC7\u1CD3\u2000-\u200A\u2010-\u2029\u202F-\u2043\u2045-\u2051\u2053-\u205F\u207D\u207E\u208D\u208E\u2308-\u230B\u2329\u232A\u2768-\u2775\u27C5\u27C6\u27E6-\u27EF\u2983-\u2998\u29D8-\u29DB\u29FC\u29FD\u2CF9-\u2CFC\u2CFE\u2CFF\u2D70\u2E00-\u2E2E\u2E30-\u2E44\u3000-\u3003\u3008-\u3011\u3014-\u301F\u3030\u303D\u30A0\u30FB\uA4FE\uA4FF\uA60D-\uA60F\uA673\uA67E\uA6F2-\uA6F7\uA874-\uA877\uA8CE\uA8CF\uA8F8-\uA8FA\uA8FC\uA92E\uA92F\uA95F\uA9C1-\uA9CD\uA9DE\uA9DF\uAA5C-\uAA5F\uAADE\uAADF\uAAF0\uAAF1\uABEB\uFD3E\uFD3F\uFE10-\uFE19\uFE30-\uFE52\uFE54-\uFE61\uFE63\uFE68\uFE6A\uFE6B\uFF01-\uFF03\uFF05-\uFF0A\uFF0C-\uFF0F\uFF1A\uFF1B\uFF1F\uFF20\uFF3B-\uFF3D\uFF3F\uFF5B\uFF5D\uFF5F-\uFF65]|\uD800[\uDD00-\uDD02\uDF9F\uDFD0]|\uD801\uDD6F|\uD802[\uDC57\uDD1F\uDD3F\uDE50-\uDE58\uDE7F\uDEF0-\uDEF6\uDF39-\uDF3F\uDF99-\uDF9C]|\uD804[\uDC47-\uDC4D\uDCBB\uDCBC\uDCBE-\uDCC1\uDD40-\uDD43\uDD74\uDD75\uDDC5-\uDDC9\uDDCD\uDDDB\uDDDD-\uDDDF\uDE38-\uDE3D\uDEA9]|\uD805[\uDC4B-\uDC4F\uDC5B\uDC5D\uDCC6\uDDC1-\uDDD7\uDE41-\uDE43\uDE60-\uDE6C\uDF3C-\uDF3E]|\uD807[\uDC41-\uDC45\uDC70\uDC71]|\uD809[\uDC70-\uDC74]|\uD81A[\uDE6E\uDE6F\uDEF5\uDF37-\uDF3B\uDF44]|\uD82F\uDC9F|\uD836[\uDE87-\uDE8B]|\uD83A[\uDD5E\uDD5F]/u;
        const n = /[^\t-\r -#%-\*,-\/:;\?@\[-\]_\{\}\x85\xA0\xA1\xA7\xAB\xB6\xB7\xBB\xBF\u037E\u0387\u055A-\u055F\u0589\u058A\u05BE\u05C0\u05C3\u05C6\u05F3\u05F4\u0609\u060A\u060C\u060D\u061B\u061E\u061F\u066A-\u066D\u06D4\u0700-\u070D\u07F7-\u07F9\u0830-\u083E\u085E\u0964\u0965\u0970\u0AF0\u0DF4\u0E4F\u0E5A\u0E5B\u0F04-\u0F12\u0F14\u0F3A-\u0F3D\u0F85\u0FD0-\u0FD4\u0FD9\u0FDA\u104A-\u104F\u10FB\u1360-\u1368\u1400\u166D\u166E\u1680\u169B\u169C\u16EB-\u16ED\u1735\u1736\u17D4-\u17D6\u17D8-\u17DA\u1800-\u180A\u1944\u1945\u1A1E\u1A1F\u1AA0-\u1AA6\u1AA8-\u1AAD\u1B5A-\u1B60\u1BFC-\u1BFF\u1C3B-\u1C3F\u1C7E\u1C7F\u1CC0-\u1CC7\u1CD3\u2000-\u200A\u2010-\u2029\u202F-\u2043\u2045-\u2051\u2053-\u205F\u207D\u207E\u208D\u208E\u2308-\u230B\u2329\u232A\u2768-\u2775\u27C5\u27C6\u27E6-\u27EF\u2983-\u2998\u29D8-\u29DB\u29FC\u29FD\u2CF9-\u2CFC\u2CFE\u2CFF\u2D70\u2E00-\u2E2E\u2E30-\u2E44\u3000-\u3003\u3008-\u3011\u3014-\u301F\u3030\u303D\u30A0\u30FB\uA4FE\uA4FF\uA60D-\uA60F\uA673\uA67E\uA6F2-\uA6F7\uA874-\uA877\uA8CE\uA8CF\uA8F8-\uA8FA\uA8FC\uA92E\uA92F\uA95F\uA9C1-\uA9CD\uA9DE\uA9DF\uAA5C-\uAA5F\uAADE\uAADF\uAAF0\uAAF1\uABEB\uFD3E\uFD3F\uFE10-\uFE19\uFE30-\uFE52\uFE54-\uFE61\uFE63\uFE68\uFE6A\uFE6B\uFF01-\uFF03\uFF05-\uFF0A\uFF0C-\uFF0F\uFF1A\uFF1B\uFF1F\uFF20\uFF3B-\uFF3D\uFF3F\uFF5B\uFF5D\uFF5F-\uFF65]|\uD800[\uDD00-\uDD02\uDF9F\uDFD0]|\uD801\uDD6F|\uD802[\uDC57\uDD1F\uDD3F\uDE50-\uDE58\uDE7F\uDEF0-\uDEF6\uDF39-\uDF3F\uDF99-\uDF9C]|\uD804[\uDC47-\uDC4D\uDCBB\uDCBC\uDCBE-\uDCC1\uDD40-\uDD43\uDD74\uDD75\uDDC5-\uDDC9\uDDCD\uDDDB\uDDDD-\uDDDF\uDE38-\uDE3D\uDEA9]|\uD805[\uDC4B-\uDC4F\uDC5B\uDC5D\uDCC6\uDDC1-\uDDD7\uDE41-\uDE43\uDE60-\uDE6C\uDF3C-\uDF3E]|\uD807[\uDC41-\uDC45\uDC70\uDC71]|\uD809[\uDC70-\uDC74]|\uD81A[\uDE6E\uDE6F\uDEF5\uDF37-\uDF3B\uDF44]|\uD82F\uDC9F|\uD836[\uDE87-\uDE8B]|\uD83A[\uDD5E\uDD5F]/u;
        const u = (e, t=document) => {
            const s = [];
            const n = t.createRange();
            n.selectNodeContents(t.body);
            while (!n.collapsed && s.length < 2) {
                let u;
                if (e.prefix) {
                    const s = F(e.prefix, n);
                    if (s == null) {
                        break
                    }
                    a(n, s.startContainer, s.startOffset);
                    const r = t.createRange();
                    r.setStart(s.endContainer, s.endOffset);
                    r.setEnd(n.endContainer, n.endOffset);
                    f(r);
                    if (r.collapsed) {
                        break
                    }
                    u = F(e.textStart, r);
                    if (u == null) {
                        break
                    }
                    if (u.compareBoundaryPoints(Range.START_TO_START, r) !== 0) {
                        continue
                    }
                } else {
                    u = F(e.textStart, n);
                    if (u == null) {
                        break
                    }
                    a(n, u.startContainer, u.startOffset)
                }
                if (e.textEnd) {
                    const f = t.createRange();
                    f.setStart(u.endContainer, u.endOffset);
                    f.setEnd(n.endContainer, n.endOffset);
                    let o = false;
                    while (!f.collapsed && s.length < 2) {
                        const c = F(e.textEnd, f);
                        if (c == null) {
                            break
                        }
                        a(f, c.startContainer, c.startOffset);
                        u.setEnd(c.endContainer, c.endOffset);
                        if (e.suffix) {
                            const a = i(e.suffix, u, n, t);
                            if (a === r.NO_SUFFIX_MATCH) {
                                break
                            } else if (a === r.SUFFIX_MATCH) {
                                o = true;
                                s.push(u.cloneRange());
                                continue
                            } else if (a === r.MISPLACED_SUFFIX) {
                                continue
                            }
                        } else {
                            o = true;
                            s.push(u.cloneRange())
                        }
                    }
                    if (!o) {
                        break
                    }
                } else if (e.suffix) {
                    const f = i(e.suffix, u, n, t);
                    if (f === r.NO_SUFFIX_MATCH) {
                        break
                    } else if (f === r.SUFFIX_MATCH) {
                        s.push(u.cloneRange());
                        a(n, n.startContainer, n.startOffset);
                        continue
                    } else if (f === r.MISPLACED_SUFFIX) {
                        continue
                    }
                } else {
                    s.push(u.cloneRange())
                }
            }
            return s
        };
        const r = {
            NO_SUFFIX_MATCH: 0,
            SUFFIX_MATCH: 1,
            MISPLACED_SUFFIX: 2
        };
        const i = (e, t, s, n) => {
            const u = n.createRange();
            u.setStart(t.endContainer, t.endOffset);
            u.setEnd(s.endContainer, s.endOffset);
            f(u);
            const i = F(e, u);
            if (i == null) {
                return r.NO_SUFFIX_MATCH
            }
            if (i.compareBoundaryPoints(Range.START_TO_START, u) !== 0) {
                return r.MISPLACED_SUFFIX
            }
            return r.SUFFIX_MATCH
        };
        const a = (e, t, s) => {
            try {
                e.setStart(t, s + 1)
            } catch (s) {
                e.setStartAfter(t)
            }
        };
        const f = e => {
            const t = o(e);
            let s = t.nextNode();
            while (!e.collapsed && s != null) {
                if (s !== e.startContainer) {
                    e.setStart(s, 0)
                }
                if (s.textContent.length > e.startOffset) {
                    const t = s.textContent[e.startOffset];
                    if (!t.match(/\s/)) {
                        return
                    }
                }
                try {
                    e.setStart(s, e.startOffset + 1)
                } catch (n) {
                    s = t.nextNode();
                    if (s == null) {
                        e.collapse()
                    } else {
                        e.setStart(s, 0)
                    }
                }
            }
        };
        const o = e => {
            const t = document.createTreeWalker(e.commonAncestorContainer, NodeFilter.SHOW_TEXT | NodeFilter.SHOW_ELEMENT, (t => d(t, e)));
            return t
        };
        const c = e => {
            let t = e;
            while (t != null && !(t instanceof HTMLElement))
                t = t.parentNode;
            if (t != null) {
                const e = window.getComputedStyle(t);
                if (e.visibility === "hidden" || e.display === "none" || e.height === 0 || e.width === 0 || e.opacity === 0) {
                    return false
                }
            }
            return true
        };
        const h = (e, t) => {
            if (t != null && !t.intersectsNode(e))
                return NodeFilter.FILTER_REJECT;
            return c(e) ? NodeFilter.FILTER_ACCEPT : NodeFilter.FILTER_REJECT
        };
        const d = (e, t) => {
            if (t != null && !t.intersectsNode(e))
                return NodeFilter.FILTER_REJECT;
            if (!c(e)) {
                return NodeFilter.FILTER_REJECT
            }
            return e.nodeType === Node.TEXT_NODE ? NodeFilter.FILTER_ACCEPT : NodeFilter.FILTER_SKIP
        };
        const l = (e, s) => {
            const n = [];
            let u = [];
            const r = Array.from(S(e, (e => h(e, s))));
            for (const e of r) {
                if (e.nodeType === Node.TEXT_NODE) {
                    u.push(e)
                } else if (e instanceof HTMLElement && t.includes(e.tagName) && u.length > 0) {
                    n.push(u);
                    u = []
                }
            }
            if (u.length > 0)
                n.push(u);
            return n
        };
        const D = (e, t, s) => {
            let n = "";
            if (e.length === 1) {
                n = e[0].textContent.substring(t, s)
            } else {
                n = e[0].textContent.substring(t) + e.slice(1, -1).reduce(((e, t) => e + t.textContent), "") + e.slice(-1)[0].textContent.substring(0, s)
            }
            return n.replace(/[\t\n\r ]+/g, " ")
        };
        function* S(e, t) {
            const s = document.createTreeWalker(e, NodeFilter.SHOW_ELEMENT | NodeFilter.SHOW_TEXT, {
                acceptNode: t
            });
            const n = new Set;
            while (N(s, n) !== null) {
                yield s.currentNode
            }
        }
        const F = (e, t) => {
            const s = l(t.commonAncestorContainer, t);
            const n = O();
            for (const u of s) {
                const s = E(e, t, u, n);
                if (s !== undefined)
                    return s
            }
            return undefined
        };
        const E = (e, t, s, n) => {
            if (!e || !t || !(s || []).length)
                return undefined;
            const u = C(D(s, 0, undefined));
            const r = C(e);
            let i = s[0] === t.startNode ? t.startOffset : 0;
            let a;
            let f;
            while (i < u.length) {
                const e = u.indexOf(r, i);
                if (e === -1)
                    return undefined;
                if (A(u, e, r.length, n)) {
                    a = g(e, s, false);
                    f = g(e + r.length, s, true)
                }
                if (a != null && f != null) {
                    const e = new Range;
                    e.setStart(a.node, a.offset);
                    e.setEnd(f.node, f.offset);
                    if (t.compareBoundaryPoints(Range.START_TO_START, e) <= 0 && t.compareBoundaryPoints(Range.END_TO_END, e) >= 0) {
                        return e
                    }
                }
                i = e + 1
            }
            return undefined
        };
        const g = (e, t, s) => {
            let n = 0;
            let u;
            for (let r = 0; r < t.length; r++) {
                const i = t[r];
                if (!u)
                    u = C(i.data);
                let a = n + u.length;
                if (s)
                    a += 1;
                if (a > e) {
                    const t = e - n;
                    let r = Math.min(e - n, i.data.length);
                    const a = s ? u.substring(0, t) : u.substring(t);
                    let f = s ? C(i.data.substring(0, r)) : C(i.data.substring(r));
                    const o = (s ? -1 : 1) * (a.length > f.length ? -1 : 1);
                    while (r >= 0 && r <= i.data.length) {
                        if (f.length === a.length) {
                            return {
                                node: i,
                                offset: r
                            }
                        }
                        r += o;
                        f = s ? C(i.data.substring(0, r)) : C(i.data.substring(r))
                    }
                }
                n += u.length;
                if (r + 1 < t.length) {
                    const e = C(t[r + 1].data);
                    if (u.slice(-1) === " " && e.slice(0, 1) === " ") {
                        n -= 1
                    }
                    u = e
                }
            }
            return undefined
        };
        const A = (e, t, n, u) => {
            if (t < 0 || t >= e.length || n <= 0 || t + n > e.length) {
                return false
            }
            if (u) {
                const s = u.segment(e);
                const r = s.containing(t);
                if (!r)
                    return false;
                if (r.isWordLike && r.index != t)
                    return false;
                const i = t + n;
                const a = s.containing(i);
                if (a && a.isWordLike && a.index != i)
                    return false
            } else {
                if (e[t].match(s)) {
                    ++t;
                    --n;
                    if (!n) {
                        return false
                    }
                }
                if (e[t + n - 1].match(s)) {
                    --n;
                    if (!n) {
                        return false
                    }
                }
                if (t !== 0 && !e[t - 1].match(s))
                    return false;
                if (t + n !== e.length && !e[t + n].match(s))
                    return false
            }
            return true
        };
        const C = e => (e || "").normalize("NFKD").replace(/\s+/g, " ").replace(/[\u0300-\u036f]/g, "").toLowerCase();
        const O = () => {
            if (Intl.Segmenter) {
                let e = document.documentElement.lang;
                if (!e) {
                    e = navigator.languages
                }
                return new Intl.Segmenter(e, {
                    granularity: "word"
                })
            }
            return undefined
        };
        const N = (e, t) => {
            if (!t.has(e.currentNode)) {
                const t = e.firstChild();
                if (t !== null) {
                    return t
                }
            }
            const s = e.nextSibling();
            if (s !== null) {
                return s
            }
            const n = e.parentNode();
            if (n !== null) {
                t.add(n)
            }
            return n
        };
        const x = (e, t) => {
            if (!t.has(e.currentNode)) {
                const t = e.lastChild();
                if (t !== null) {
                    return t
                }
            }
            const s = e.previousSibling();
            if (s !== null) {
                return s
            }
            const n = e.parentNode();
            if (n !== null) {
                t.add(n)
            }
            return n
        };
        const T = {
            BLOCK_ELEMENTS: t,
            BOUNDARY_CHARS: s,
            NON_BOUNDARY_CHARS: n,
            acceptNodeIfVisibleInRange: h,
            normalizeString: C,
            makeNewSegmenter: O,
            forwardTraverse: N,
            backwardTraverse: x,
            makeTextNodeWalker: o,
            isNodeVisible: c
        };
        if (typeof goog !== "undefined") {
            goog.declareModuleId("googleChromeLabs.textFragmentPolyfill.textFragmentUtils")
        }
        const p = 300;
        const m = 20;
        const w = 1;
        const _ = 3;
        const B = 1;
        let R = 500;
        let b;
        const k = {
            SUCCESS: 0,
            INVALID_SELECTION: 1,
            AMBIGUOUS: 2,
            TIMEOUT: 3,
            EXECUTION_FAILED: 4
        };
        const I = (e, t=Date.now()) => {
            try {
                return L(e, t)
            } catch (e) {
                if (e.isTimeout) {
                    return {
                        status: k.TIMEOUT
                    }
                } else {
                    return {
                        status: k.EXECUTION_FAILED
                    }
                }
            }
        };
        const L = (e, t) => {
            P(t);
            let s;
            try {
                s = e.getRangeAt(0)
            } catch {
                return {
                    status: k.INVALID_SELECTION
                }
            }
            Q(s);
            se(s);
            const n = s.cloneRange();
            Z(s);
            if (s.collapsed) {
                return {
                    status: k.INVALID_SELECTION
                }
            }
            let u;
            if (Y(s)) {
                const e = T.normalizeString(s.toString());
                const t = {
                    textStart: e
                };
                if (e.length >= m && W(t)) {
                    return {
                        status: k.SUCCESS,
                        fragment: t
                    }
                }
                u = (new y).setExactTextMatch(e)
            } else {
                const e = H(s);
                const t = U(s);
                if (e && t) {
                    u = (new y).setStartAndEndSearchSpace(e, t)
                } else {
                    u = (new y).setSharedSearchSpace(s.toString().trim())
                }
            }
            const r = document.createRange();
            r.selectNodeContents(document.body);
            const i = r.cloneRange();
            r.setEnd(n.startContainer, n.startOffset);
            i.setStart(n.endContainer, n.endOffset);
            const a = U(r);
            const f = H(i);
            if (a || f) {
                u.setPrefixAndSuffixSearchSpace(a, f)
            }
            u.useSegmenter(T.makeNewSegmenter());
            let o = false;
            do {
                M();
                o = u.embiggen();
                const e = u.tryToMakeUniqueFragment();
                if (e != null) {
                    return {
                        status: k.SUCCESS,
                        fragment: e
                    }
                }
            } while (o);
            return {
                status: k.AMBIGUOUS
            }
        };
        const M = () => {
            const e = Date.now() - b;
            if (e > R) {
                const t = new Error(`Fragment generation timed out after ${e} ms.`);
                t.isTimeout = true;
                throw t
            }
        };
        const P = e => {
            b = e
        };
        const H = e => {
            let t = V(e);
            const s = $(t, e.endContainer);
            if (!s) {
                return undefined
            }
            const n = new Set;
            if (e.startContainer.nodeType === Node.ELEMENT_NODE && e.startOffset === e.startContainer.childNodes.length) {
                n.add(e.startContainer)
            }
            const u = t;
            const r = new X(e, true);
            const i = e.cloneRange();
            while (!i.collapsed && t != null) {
                M();
                if (t.contains(u)) {
                    i.setStartAfter(t)
                } else {
                    i.setStartBefore(t)
                }
                r.appendNode(t);
                if (r.textInBlock !== null) {
                    return r.textInBlock
                }
                t = T.forwardTraverse(s, n)
            }
            return undefined
        };
        const U = e => {
            let t = G(e);
            const s = $(t, e.startContainer);
            if (!s) {
                return undefined
            }
            const n = new Set;
            if (e.endContainer.nodeType === Node.ELEMENT_NODE && e.endOffset === 0) {
                n.add(e.endContainer)
            }
            const u = t;
            const r = new X(e, false);
            const i = e.cloneRange();
            while (!i.collapsed && t != null) {
                M();
                if (t.contains(u)) {
                    i.setEnd(t, 0)
                } else {
                    i.setEndAfter(t)
                }
                r.appendNode(t);
                if (r.textInBlock !== null) {
                    return r.textInBlock
                }
                t = T.backwardTraverse(s, n)
            }
            return undefined
        };
        const y = class {
            constructor()
            {
                this.Mode = {
                    ALL_PARTS: 1,
                    SHARED_START_AND_END: 2,
                    CONTEXT_ONLY: 3
                };
                this.startOffset = null;
                this.endOffset = null;
                this.prefixOffset = null;
                this.suffixOffset = null;
                this.prefixSearchSpace = "";
                this.backwardsPrefixSearchSpace = "";
                this.suffixSearchSpace = "";
                this.numIterations = 0
            }
            tryToMakeUniqueFragment()
            {
                let e;
                if (this.mode === this.Mode.CONTEXT_ONLY) {
                    e = {
                        textStart: this.exactTextMatch
                    }
                } else {
                    e = {
                        textStart: this.getStartSearchSpace().substring(0, this.startOffset).trim(),
                        textEnd: this.getEndSearchSpace().substring(this.endOffset).trim()
                    }
                }
                if (this.prefixOffset != null) {
                    const t = this.getPrefixSearchSpace().substring(this.prefixOffset).trim();
                    if (t) {
                        e.prefix = t
                    }
                }
                if (this.suffixOffset != null) {
                    const t = this.getSuffixSearchSpace().substring(0, this.suffixOffset).trim();
                    if (t) {
                        e.suffix = t
                    }
                }
                return W(e) ? e : undefined
            }
            embiggen()
            {
                let e = true;
                if (this.mode === this.Mode.SHARED_START_AND_END) {
                    if (this.startOffset >= this.endOffset) {
                        e = false
                    }
                } else if (this.mode === this.Mode.ALL_PARTS) {
                    if (this.startOffset === this.getStartSearchSpace().length && this.backwardsEndOffset() === this.getEndSearchSpace().length) {
                        e = false
                    }
                } else if (this.mode === this.Mode.CONTEXT_ONLY) {
                    e = false
                }
                if (e) {
                    const e = this.getNumberOfRangeWordsToAdd();
                    if (this.startOffset < this.getStartSearchSpace().length) {
                        let t = 0;
                        if (this.getStartSegments() != null) {
                            while (t < e && this.startOffset < this.getStartSearchSpace().length) {
                                this.startOffset = this.getNextOffsetForwards(this.getStartSegments(), this.startOffset, this.getStartSearchSpace());
                                t++
                            }
                        } else {
                            let s = this.startOffset;
                            do {
                                M();
                                const e = this.getStartSearchSpace().substring(this.startOffset + 1).search(T.BOUNDARY_CHARS);
                                if (e === -1) {
                                    this.startOffset = this.getStartSearchSpace().length
                                } else {
                                    this.startOffset = this.startOffset + 1 + e
                                }
                                if (this.getStartSearchSpace().substring(s, this.startOffset).search(T.NON_BOUNDARY_CHARS) !== -1) {
                                    s = this.startOffset;
                                    t++
                                }
                            } while (this.startOffset < this.getStartSearchSpace().length && t < e)
                        }
                        if (this.mode === this.Mode.SHARED_START_AND_END) {
                            this.startOffset = Math.min(this.startOffset, this.endOffset)
                        }
                    }
                    if (this.backwardsEndOffset() < this.getEndSearchSpace().length) {
                        let t = 0;
                        if (this.getEndSegments() != null) {
                            while (t < e && this.endOffset > 0) {
                                this.endOffset = this.getNextOffsetBackwards(this.getEndSegments(), this.endOffset);
                                t++
                            }
                        } else {
                            let s = this.backwardsEndOffset();
                            do {
                                M();
                                const e = this.getBackwardsEndSearchSpace().substring(this.backwardsEndOffset() + 1).search(T.BOUNDARY_CHARS);
                                if (e === -1) {
                                    this.setBackwardsEndOffset(this.getEndSearchSpace().length)
                                } else {
                                    this.setBackwardsEndOffset(this.backwardsEndOffset() + 1 + e)
                                }
                                if (this.getBackwardsEndSearchSpace().substring(s, this.backwardsEndOffset()).search(T.NON_BOUNDARY_CHARS) !== -1) {
                                    s = this.backwardsEndOffset();
                                    t++
                                }
                            } while (this.backwardsEndOffset() < this.getEndSearchSpace().length && t < e)
                        }
                        if (this.mode === this.Mode.SHARED_START_AND_END) {
                            this.endOffset = Math.max(this.startOffset, this.endOffset)
                        }
                    }
                }
                let t = false;
                if (!e || this.startOffset + this.backwardsEndOffset() < m || this.numIterations >= w) {
                    if (this.backwardsPrefixOffset() != null && this.backwardsPrefixOffset() !== this.getPrefixSearchSpace().length || this.suffixOffset != null && this.suffixOffset !== this.getSuffixSearchSpace().length) {
                        t = true
                    }
                }
                if (t) {
                    const e = this.getNumberOfContextWordsToAdd();
                    if (this.backwardsPrefixOffset() < this.getPrefixSearchSpace().length) {
                        let t = 0;
                        if (this.getPrefixSegments() != null) {
                            while (t < e && this.prefixOffset > 0) {
                                this.prefixOffset = this.getNextOffsetBackwards(this.getPrefixSegments(), this.prefixOffset);
                                t++
                            }
                        } else {
                            let s = this.backwardsPrefixOffset();
                            do {
                                M();
                                const e = this.getBackwardsPrefixSearchSpace().substring(this.backwardsPrefixOffset() + 1).search(T.BOUNDARY_CHARS);
                                if (e === -1) {
                                    this.setBackwardsPrefixOffset(this.getBackwardsPrefixSearchSpace().length)
                                } else {
                                    this.setBackwardsPrefixOffset(this.backwardsPrefixOffset() + 1 + e)
                                }
                                if (this.getBackwardsPrefixSearchSpace().substring(s, this.backwardsPrefixOffset()).search(T.NON_BOUNDARY_CHARS) !== -1) {
                                    s = this.backwardsPrefixOffset();
                                    t++
                                }
                            } while (this.backwardsPrefixOffset() < this.getPrefixSearchSpace().length && t < e)
                        }
                    }
                    if (this.suffixOffset < this.getSuffixSearchSpace().length) {
                        let t = 0;
                        if (this.getSuffixSegments() != null) {
                            while (t < e && this.suffixOffset < this.getSuffixSearchSpace().length) {
                                this.suffixOffset = this.getNextOffsetForwards(this.getSuffixSegments(), this.suffixOffset, this.suffixOffset);
                                t++
                            }
                        } else {
                            let s = this.suffixOffset;
                            do {
                                M();
                                const e = this.getSuffixSearchSpace().substring(this.suffixOffset + 1).search(T.BOUNDARY_CHARS);
                                if (e === -1) {
                                    this.suffixOffset = this.getSuffixSearchSpace().length
                                } else {
                                    this.suffixOffset = this.suffixOffset + 1 + e
                                }
                                if (this.getSuffixSearchSpace().substring(s, this.suffixOffset).search(T.NON_BOUNDARY_CHARS) !== -1) {
                                    s = this.suffixOffset;
                                    t++
                                }
                            } while (this.suffixOffset < this.getSuffixSearchSpace().length && t < e)
                        }
                    }
                }
                this.numIterations++;
                return e || t
            }
            setStartAndEndSearchSpace(e, t)
            {
                this.startSearchSpace = e;
                this.endSearchSpace = t;
                this.backwardsEndSearchSpace = v(t);
                this.startOffset = 0;
                this.endOffset = t.length;
                this.mode = this.Mode.ALL_PARTS;
                return this
            }
            setSharedSearchSpace(e)
            {
                this.sharedSearchSpace = e;
                this.backwardsSharedSearchSpace = v(e);
                this.startOffset = 0;
                this.endOffset = e.length;
                this.mode = this.Mode.SHARED_START_AND_END;
                return this
            }
            setExactTextMatch(e)
            {
                this.exactTextMatch = e;
                this.mode = this.Mode.CONTEXT_ONLY;
                return this
            }
            setPrefixAndSuffixSearchSpace(e, t)
            {
                if (e) {
                    this.prefixSearchSpace = e;
                    this.backwardsPrefixSearchSpace = v(e);
                    this.prefixOffset = e.length
                }
                if (t) {
                    this.suffixSearchSpace = t;
                    this.suffixOffset = 0
                }
                return this
            }
            useSegmenter(e)
            {
                if (e == null) {
                    return this
                }
                if (this.mode === this.Mode.ALL_PARTS) {
                    this.startSegments = e.segment(this.startSearchSpace);
                    this.endSegments = e.segment(this.endSearchSpace)
                } else if (this.mode === this.Mode.SHARED_START_AND_END) {
                    this.sharedSegments = e.segment(this.sharedSearchSpace)
                }
                if (this.prefixSearchSpace) {
                    this.prefixSegments = e.segment(this.prefixSearchSpace)
                }
                if (this.suffixSearchSpace) {
                    this.suffixSegments = e.segment(this.suffixSearchSpace)
                }
                return this
            }
            getNumberOfContextWordsToAdd()
            {
                return this.backwardsPrefixOffset() === 0 && this.suffixOffset === 0 ? _ : B
            }
            getNumberOfRangeWordsToAdd()
            {
                return this.startOffset === 0 && this.backwardsEndOffset() === 0 ? _ : B
            }
            getNextOffsetForwards(e, t, s)
            {
                let n = e.containing(t);
                while (n != null) {
                    M();
                    const t = n.index + n.segment.length;
                    if (n.isWordLike) {
                        return t
                    }
                    n = e.containing(t)
                }
                return s.length
            }
            getNextOffsetBackwards(e, t)
            {
                let s = e.containing(t);
                if (!s || t == s.index) {
                    s = e.containing(t - 1)
                }
                while (s != null) {
                    M();
                    if (s.isWordLike) {
                        return s.index
                    }
                    s = e.containing(s.index - 1)
                }
                return 0
            }
            getStartSearchSpace()
            {
                return this.mode === this.Mode.SHARED_START_AND_END ? this.sharedSearchSpace : this.startSearchSpace
            }
            getStartSegments()
            {
                return this.mode === this.Mode.SHARED_START_AND_END ? this.sharedSegments : this.startSegments
            }
            getEndSearchSpace()
            {
                return this.mode === this.Mode.SHARED_START_AND_END ? this.sharedSearchSpace : this.endSearchSpace
            }
            getEndSegments()
            {
                return this.mode === this.Mode.SHARED_START_AND_END ? this.sharedSegments : this.endSegments
            }
            getBackwardsEndSearchSpace()
            {
                return this.mode === this.Mode.SHARED_START_AND_END ? this.backwardsSharedSearchSpace : this.backwardsEndSearchSpace
            }
            getPrefixSearchSpace()
            {
                return this.prefixSearchSpace
            }
            getPrefixSegments()
            {
                return this.prefixSegments
            }
            getBackwardsPrefixSearchSpace()
            {
                return this.backwardsPrefixSearchSpace
            }
            getSuffixSearchSpace()
            {
                return this.suffixSearchSpace
            }
            getSuffixSegments()
            {
                return this.suffixSegments
            }
            backwardsEndOffset()
            {
                return this.getEndSearchSpace().length - this.endOffset
            }
            setBackwardsEndOffset(e)
            {
                this.endOffset = this.getEndSearchSpace().length - e
            }
            backwardsPrefixOffset()
            {
                if (this.prefixOffset == null)
                    return null;
                return this.getPrefixSearchSpace().length - this.prefixOffset
            }
            setBackwardsPrefixOffset(e)
            {
                if (this.prefixOffset == null)
                    return;
                this.prefixOffset = this.getPrefixSearchSpace().length - e
            }
        }
        ;
        const X = class {
            constructor(e, t)
            {
                this.searchRange = e;
                this.isForwardTraversal = t;
                this.textFound = false;
                this.textNodes = [];
                this.textInBlock = null
            }
            appendNode(e)
            {
                if (this.textInBlock !== null) {
                    return
                }
                if (ne(e)) {
                    if (this.textFound) {
                        if (!this.isForwardTraversal) {
                            this.textNodes.reverse()
                        }
                        this.textInBlock = this.textNodes.map((e => e.textContent)).join("").trim()
                    } else {
                        this.textNodes = []
                    }
                    return
                }
                if (!ue(e))
                    return;
                const t = this.getNodeIntersectionWithRange(e);
                this.textFound = this.textFound || t.textContent.trim() !== "";
                this.textNodes.push(t)
            }
            getNodeIntersectionWithRange(e)
            {
                let t = null;
                let s = null;
                if (e === this.searchRange.startContainer && this.searchRange.startOffset !== 0) {
                    t = this.searchRange.startOffset
                }
                if (e === this.searchRange.endContainer && this.searchRange.endOffset !== e.textContent.length) {
                    s = this.searchRange.endOffset
                }
                if (t !== null || s !== null) {
                    return {
                        textContent: e.textContent.substring(t ?? 0, s ?? e.textContent.length)
                    }
                }
                return e
            }
        }
        ;
        const W = e => u(e).length === 1;
        const v = e => [...e || ""].reverse().join("");
        const Y = e => {
            if (e.toString().length > p)
                return false;
            return !q(e)
        };
        const V = e => {
            let t = e.startContainer;
            if (t.nodeType == Node.ELEMENT_NODE && e.startOffset < t.childNodes.length) {
                t = t.childNodes[e.startOffset]
            }
            return t
        };
        const G = e => {
            let t = e.endContainer;
            if (t.nodeType == Node.ELEMENT_NODE && e.endOffset > 0) {
                t = t.childNodes[e.endOffset - 1]
            }
            return t
        };
        const K = e => {
            const t = V(e);
            if (ue(t) && T.isNodeVisible(t)) {
                return t
            }
            const s = T.makeTextNodeWalker(e);
            s.currentNode = t;
            return s.nextNode()
        };
        const J = e => {
            const t = G(e);
            if (ue(t) && T.isNodeVisible(t)) {
                return t
            }
            const s = T.makeTextNodeWalker(e);
            s.currentNode = t;
            return T.backwardTraverse(s, new Set)
        };
        const q = e => {
            const t = e.cloneRange();
            let s = V(t);
            const n = $(s);
            if (!n) {
                return false
            }
            const u = new Set;
            while (!t.collapsed && s != null) {
                if (ne(s))
                    return true;
                if (s != null)
                    t.setStartAfter(s);
                s = T.forwardTraverse(n, u);
                M()
            }
            return false
        };
        const z = (e, t) => {
            if (e.nodeType !== Node.TEXT_NODE)
                return -1;
            const s = t != null ? t : e.data.length;
            if (s < e.data.length && T.BOUNDARY_CHARS.test(e.data[s]))
                return s;
            const n = e.data.substring(0, s);
            const u = v(n).search(T.BOUNDARY_CHARS);
            if (u !== -1) {
                return s - u
            }
            return -1
        };
        const j = (e, t) => {
            if (e.nodeType !== Node.TEXT_NODE)
                return -1;
            const s = t != null ? t : 0;
            if (s < e.data.length && s > 0 && T.BOUNDARY_CHARS.test(e.data[s - 1])) {
                return s
            }
            const n = e.data.substring(s);
            const u = n.search(T.BOUNDARY_CHARS);
            if (u !== -1) {
                return s + u
            }
            return -1
        };
        const $ = (e, t) => {
            if (!e) {
                return undefined
            }
            let s = e;
            const n = t != null ? t : e;
            while (!s.contains(n) || !ne(s)) {
                if (s.parentNode) {
                    s = s.parentNode
                }
            }
            const u = document.createTreeWalker(s, NodeFilter.SHOW_ELEMENT | NodeFilter.SHOW_TEXT, (e => T.acceptNodeIfVisibleInRange(e)));
            u.currentNode = e;
            return u
        };
        const Q = e => {
            const t = T.makeNewSegmenter();
            if (t) {
                const s = V(e);
                if (s !== e.startContainer) {
                    e.setStartBefore(s)
                }
                ee(t, false, e)
            } else {
                const t = z(e.startContainer, e.startOffset);
                if (t !== -1) {
                    e.setStart(e.startContainer, t);
                    return
                }
                if (ne(e.startContainer) && e.startOffset === 0) {
                    return
                }
                const s = $(e.startContainer);
                if (!s) {
                    return
                }
                const n = new Set;
                let u = T.backwardTraverse(s, n);
                while (u != null) {
                    const t = z(u);
                    if (t !== -1) {
                        e.setStart(u, t);
                        return
                    }
                    if (ne(u)) {
                        if (u.contains(e.startContainer)) {
                            e.setStart(u, 0)
                        } else {
                            e.setStartAfter(u)
                        }
                        return
                    }
                    u = T.backwardTraverse(s, n);
                    e.collapse()
                }
            }
        };
        const Z = e => {
            const t = K(e);
            if (t == null) {
                e.collapse();
                return
            }
            const s = V(e);
            if (s !== t) {
                e.setStart(t, 0)
            }
            const n = G(e);
            const u = J(e);
            if (n !== u) {
                e.setEnd(u, u.textContent.length)
            }
        };
        const ee = (e, t, s) => {
            const n = t ? {
                node: s.endContainer,
                offset: s.endOffset
            } : {
                node: s.startContainer,
                offset: s.startOffset
            };
            const u = te(n.node);
            const r = u.preNodes.reduce(((e, t) => e.concat(t.textContent)), "");
            const i = u.innerNodes.reduce(((e, t) => e.concat(t.textContent)), "");
            let a = r.length;
            if (n.node.nodeType === Node.TEXT_NODE) {
                a += n.offset
            } else if (t) {
                a += i.length
            }
            const f = u.postNodes.reduce(((e, t) => e.concat(t.textContent)), "");
            const o = [...u.preNodes, ...u.innerNodes, ...u.postNodes];
            if (o.length == 0) {
                return
            }
            const c = r.concat(i, f);
            const h = e.segment(c);
            const d = h.containing(a);
            if (!d) {
                if (t) {
                    s.setEndAfter(o[o.length - 1])
                } else {
                    s.setEndBefore(o[0])
                }
                return
            }
            if (!d.isWordLike) {
                return
            }
            if (a === d.index || a === d.index + d.segment.length) {
                return
            }
            const l = t ? d.index + d.segment.length : d.index;
            let D = 0;
            for (const e of o) {
                if (D <= l && l < D + e.textContent.length) {
                    const n = l - D;
                    if (t) {
                        if (n >= e.textContent.length) {
                            s.setEndAfter(e)
                        } else {
                            s.setEnd(e, n)
                        }
                    } else {
                        if (n >= e.textContent.length) {
                            s.setStartAfter(e)
                        } else {
                            s.setStart(e, n)
                        }
                    }
                    return
                }
                D += e.textContent.length
            }
            if (t) {
                s.setEndAfter(o[o.length - 1])
            } else {
                s.setStartBefore(o[0])
            }
        };
        const te = e => {
            const t = [];
            const s = $(e);
            if (!s) {
                return
            }
            const n = new Set;
            let u = T.backwardTraverse(s, n);
            while (u != null && !ne(u)) {
                M();
                if (u.nodeType === Node.TEXT_NODE) {
                    t.push(u)
                }
                u = T.backwardTraverse(s, n)
            }
            t.reverse();
            const r = [];
            if (e.nodeType === Node.TEXT_NODE) {
                r.push(e)
            } else {
                const t = document.createTreeWalker(e, NodeFilter.SHOW_ELEMENT | NodeFilter.SHOW_TEXT, (e => T.acceptNodeIfVisibleInRange(e)));
                t.currentNode = e;
                let s = t.nextNode();
                while (s != null) {
                    M();
                    if (s.nodeType === Node.TEXT_NODE) {
                        r.push(s)
                    }
                    s = t.nextNode()
                }
            }
            const i = [];
            const a = $(e);
            if (!a) {
                return
            }
            const f = new Set([e]);
            let o = T.forwardTraverse(a, f);
            while (o != null && !ne(o)) {
                M();
                if (o.nodeType === Node.TEXT_NODE) {
                    i.push(o)
                }
                o = T.forwardTraverse(a, f)
            }
            return {
                preNodes: t,
                innerNodes: r,
                postNodes: i
            }
        };
        const se = e => {
            const t = T.makeNewSegmenter();
            if (t) {
                const s = G(e);
                if (s !== e.endContainer) {
                    e.setEndAfter(s)
                }
                ee(t, true, e)
            } else {
                let t = e.endOffset;
                let s = e.endContainer;
                if (s.nodeType === Node.ELEMENT_NODE) {
                    if (e.endOffset < s.childNodes.length) {
                        s = s.childNodes[e.endOffset]
                    }
                }
                const n = $(s);
                if (!n) {
                    return
                }
                const u = new Set([s]);
                while (s != null) {
                    M();
                    const r = j(s, t);
                    t = null;
                    if (r !== -1) {
                        e.setEnd(s, r);
                        return
                    }
                    if (ne(s)) {
                        if (s.contains(e.endContainer)) {
                            e.setEnd(s, s.childNodes.length)
                        } else {
                            e.setEndBefore(s)
                        }
                        return
                    }
                    s = T.forwardTraverse(n, u)
                }
                e.collapse()
            }
        };
        const ne = e => e.nodeType === Node.ELEMENT_NODE && (T.BLOCK_ELEMENTS.includes(e.tagName) || e.tagName === "HTML" || e.tagName === "BODY");
        const ue = e => e.nodeType === Node.TEXT_NODE;
        if (typeof goog !== "undefined") {
            goog.declareModuleId("googleChromeLabs.textFragmentPolyfill.fragmentGenerationUtils")
        }
        function re() {
            const e = window.getSelection();
            let t = {
                x: 0,
                y: 0,
                width: 0,
                height: 0
            };
            if (e && e.rangeCount) {
                const s = e.getRangeAt(0).getClientRects()[0];
                if (s) {
                    t.x = s.x;
                    t.y = s.y;
                    t.width = s.width;
                    t.height = s.height
                }
            }
            const s = `"${e?.toString()}"`;
            const n = document.querySelector("link[rel='canonical']");
            const u = I(e);
            return {
                status: u.status,
                fragment: u.fragment,
                selectedText: s,
                selectionRect: t,
                canonicalUrl: n && n.getAttribute("href")
            }
        }
        e.linkToText = {
            getLinkToText: re
        }
    })();
}


// LANGUAGE DETECTION

if (typeof _injected_language_detection === 'undefined') {
    var _injected_language_detection = true;
    (function() {
        if (!window.__gCrWeb) {
            window.__gCrWeb = {}
        }
        const e = window.__gCrWeb;
        function t(e, t) {
            try {
                var n = window.webkit;
                delete window["webkit"];
                window.webkit.messageHandlers[e].postMessage(t);
                window.webkit = n
            } catch (e) {}
        }
        let n;
        let o = 0;
        function a() {
            for (const e of document.getElementsByTagName("meta")) {
                if (e.name === "google") {
                    if (e.content === "notranslate" || e.getAttribute("value") === "notranslate") {
                        return true
                    }
                }
            }
            return false
        }
        function i(e) {
            for (const t of document.getElementsByTagName("meta")) {
                if (t.httpEquiv.toLowerCase() === e) {
                    return t.content
                }
            }
            return ""
        }
        const r = new Set(["EMBED", "NOSCRIPT", "OBJECT", "SCRIPT", "STYLE"]);
        function s(e, t) {
            if (!e || t <= 0) {
                return ""
            }
            let n = "";
            if (e.nodeType === Node.ELEMENT_NODE && e instanceof Element) {
                if (r.has(e.nodeName)) {
                    return ""
                }
                if (e.nodeName === "BR") {
                    return "\n"
                }
                const t = window.getComputedStyle(e);
                if (t.display === "none" || t.visibility === "hidden") {
                    return ""
                }
                if (e.nodeName.toUpperCase() !== "BODY" && t.display !== "inline") {
                    n = "\n"
                }
            }
            if (e.hasChildNodes()) {
                for (const o of e.childNodes) {
                    n += s(o, t - n.length);
                    if (n.length >= t) {
                        break
                    }
                }
            } else if (e.nodeType === Node.TEXT_NODE && e.textContent) {
                n += e.textContent.substring(0, t - n.length)
            }
            return n
        }
        function d() {
            const r = 65535;
            o += 1;
            n = s(document.body, r);
            const d = i("content-language");
            const u = {
                hasNoTranslate: false,
                htmlLang: document.documentElement.lang,
                httpContentLanguage: d,
                frameId: e.message.getFrameId()
            };
            if (a()) {
                u["hasNoTranslate"] = true
            }
            t("LanguageDetectionTextCaptured", u)
        }
        function u() {
            const e = n;
            o -= 1;
            if (o === 0) {
                n = null
            }
            return e
        }
        e.languageDetection = {
            detectLanguage: d,
            retrieveBufferedTextContent: u
        }
    })();
}

// TRANSLATE

var translateApiKey = '$<brave_translate_api_key>';
var gtTimeInfo = {
    'fetchStart': Date.now(),
    'fetchEnd': Date.now() + 1
};
var serverParams = '';
var securityOrigin = 'https://translate.brave.com/';

// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This code is used in conjunction with the Google Translate Element script.
// It is executed in an isolated world of a page to translate it from one
// language to another.
// It should be included in the page before the Translate Element script.

// eslint-disable-next-line no-var
var cr = cr || {};

/**
 * An object to provide functions to interact with the Translate library.
 * @type {object}
 */
cr.googleTranslate = (function() {
  /**
   * The Translate Element library's instance.
   * @type {object}
   */
  let lib;

  /**
   * A flag representing if the Translate Element library is initialized.
   * @type {boolean}
   */
  let libReady = false;

  /**
   * Error definitions for |errorCode|. See chrome/common/translate_errors.h
   * to modify the definition.
   * @const
   */
  const ERROR = {
    'NONE': 0,
    'INITIALIZATION_ERROR': 2,
    'UNSUPPORTED_LANGUAGE': 4,
    'TRANSLATION_ERROR': 6,
    'TRANSLATION_TIMEOUT': 7,
    'UNEXPECTED_SCRIPT_ERROR': 8,
    'BAD_ORIGIN': 9,
    'SCRIPT_LOAD_ERROR': 10,
  };

  /**
   * Error code map from te.dom.DomTranslator.Error to |errorCode|.
   * See also go/dom_translator.js in google3.
   * @const
   */
  const TRANSLATE_ERROR_TO_ERROR_CODE_MAP = {
    0: ERROR['NONE'],
    1: ERROR['TRANSLATION_ERROR'],
    2: ERROR['UNSUPPORTED_LANGUAGE'],
  };

  /**
   * An error code happened in translate.js and the Translate Element library.
   */
  let errorCode = ERROR['NONE'];

  /**
   * A flag representing if the Translate Element has finished a translation.
   * @type {boolean}
   */
  let finished = false;

  /**
   * Counts how many times the checkLibReady function is called. The function
   * is called in every 100 msec and counted up to 6.
   * @type {number}
   */
  let checkReadyCount = 0;

  /**
   * Time in msec when this script is injected.
   * @type {number}
   */
  const injectedTime = performance.now();

  /**
   * Time in msec when the Translate Element library is loaded completely.
   * @type {number}
   */
  let loadedTime = 0.0;

  /**
   * Time in msec when the Translate Element library is initialized and ready
   * for performing translation.
   * @type {number}
   */
  let readyTime = 0.0;

  /**
   * Time in msec when the Translate Element library starts a translation.
   * @type {number}
   */
  let startTime = 0.0;

  /**
   * Time in msec when the Translate Element library ends a translation.
   * @type {number}
   */
  let endTime = 0.0;

  /**
   * Callback invoked when Translate Element's ready state is known.
   * Will only be invoked once to indicate successful or failed initialization.
   * In the failure case, errorCode() and error() will indicate the reason.
   * Only used on iOS.
   * @type {function}
   */
  let readyCallback;

  /**
   * Callback invoked when Translate Element's translation result is known.
   * Will only be invoked once to indicate successful or failed translation.
   * In the failure case, errorCode() and error() will indicate the reason.
   * Only used on iOS.
   * @type {function}
   */
  let resultCallback;

  function checkLibReady() {
    if (lib.isAvailable()) {
      readyTime = performance.now();
      libReady = true;
      invokeReadyCallback();
      return;
    }
    if (checkReadyCount++ > 5) {
      errorCode = ERROR['TRANSLATION_TIMEOUT'];
      invokeReadyCallback();
      return;
    }
    setTimeout(checkLibReady, 100);
  }

  function onTranslateProgress(progress, opt_finished, opt_error) {
    finished = opt_finished;
    // opt_error can be 'undefined'.
    if (typeof opt_error === 'boolean' && opt_error) {
      // TODO(toyoshim): Remove boolean case once a server is updated.
      errorCode = ERROR['TRANSLATION_ERROR'];
      // We failed to translate, restore so the page is in a consistent state.
      lib.restore();
      invokeResultCallback();
    } else if (typeof opt_error === 'number' && opt_error !== 0) {
      errorCode = TRANSLATE_ERROR_TO_ERROR_CODE_MAP[opt_error];
      lib.restore();
      invokeResultCallback();
    }
    // Translate works differently depending on the prescence of the native
    // IntersectionObserver APIs.
    // If it is available, translate will occur incrementally as the user
    // scrolls elements into view, and this method will be called continuously
    // with |opt_finished| always set as true.
    // On the other hand, if it is unavailable, the entire page will be
    // translated at once in a piece meal manner, and this method may still be
    // called several times, though only the last call will have |opt_finished|
    // set as true.
    if (finished) {
      endTime = performance.now();
      invokeResultCallback();
    }
  }

  function invokeReadyCallback() {
    if (readyCallback) {
      readyCallback();
      readyCallback = null;
    }
  }

  function invokeResultCallback() {
    if (resultCallback) {
      resultCallback();
      resultCallback = null;
    }
  }

  window.addEventListener('pagehide', function(event) {
    if (libReady && event.persisted) {
      lib.restore();
    }
  });

  // Public API.
  return {
    /**
     * Setter for readyCallback. No op if already set.
     * @param {function} callback The function to be invoked.
     */
    set readyCallback(callback) {
      if (!readyCallback) {
        readyCallback = callback;
      }
    },

    /**
     * Setter for resultCallback. No op if already set.
     * @param {function} callback The function to be invoked.
     */
    set resultCallback(callback) {
      if (!resultCallback) {
        resultCallback = callback;
      }
    },

    /**
     * Whether the library is ready.
     * The translate function should only be called when |libReady| is true.
     * @type {boolean}
     */
    get libReady() {
      return libReady;
    },

    /**
     * Whether the current translate has finished successfully.
     * @type {boolean}
     */
    get finished() {
      return finished;
    },

    /**
     * Whether an error occured initializing the library of translating the
     * page.
     * @type {boolean}
     */
    get error() {
      return errorCode !== ERROR['NONE'];
    },

    /**
     * Returns a number to represent error type.
     * @type {number}
     */
    get errorCode() {
      return errorCode;
    },

    /**
     * The language the page translated was in. Is valid only after the page
     * has been successfully translated and the original language specified to
     * the translate function was 'auto'. Is empty otherwise.
     * Some versions of Element library don't provide |getDetectedLanguage|
     * function. In that case, this function returns 'und'.
     * @type {boolean}
     */
    get sourceLang() {
      if (!libReady || !finished || errorCode !== ERROR['NONE']) {
        return '';
      }
      if (!lib.getDetectedLanguage) {
        return 'und';
      }  // Defined as translate::kUnknownLanguageCode in C++.
      return lib.getDetectedLanguage();
    },

    /**
     * Time in msec from this script being injected to all server side scripts
     * being loaded.
     * @type {number}
     */
    get loadTime() {
      if (loadedTime === 0) {
        return 0;
      }
      return loadedTime - injectedTime;
    },

    /**
     * Time in msec from this script being injected to the Translate Element
     * library being ready.
     * @type {number}
     */
    get readyTime() {
      if (!libReady) {
        return 0;
      }
      return readyTime - injectedTime;
    },

    /**
     * Time in msec to perform translation.
     * @type {number}
     */
    get translationTime() {
      if (!finished) {
        return 0;
      }
      return endTime - startTime;
    },

    /**
     * Translate the page contents.  Note that the translation is asynchronous.
     * You need to regularly check the state of |finished| and |errorCode| to
     * know if the translation finished or if there was an error.
     * @param {string} sourceLang The language the page is in.
     * @param {string} targetLang The language the page should be translated to.
     * @return {boolean} False if the translate library was not ready, in which
     *                   case the translation is not started.  True otherwise.
     */
    translate(sourceLang, targetLang) {
      finished = false;
      errorCode = ERROR['NONE'];
      if (!libReady) {
        return false;
      }
      startTime = performance.now();
      try {
        lib.translatePage(sourceLang, targetLang, onTranslateProgress);
      } catch (err) {
        console.error('Translate: ' + err);
        errorCode = ERROR['UNEXPECTED_SCRIPT_ERROR'];
        invokeResultCallback();
        return false;
      }
      return true;
    },

    /**
     * Reverts the page contents to its original value, effectively reverting
     * any performed translation.  Does nothing if the page was not translated.
     */
    revert() {
      lib.restore();
    },

    /**
     * Called when an error is caught while executing script fetched in
     * translate_script.cc.
     */
    onTranslateElementError(error) {
      errorCode = ERROR['UNEXPECTED_SCRIPT_ERROR'];
      invokeReadyCallback();
    },

    /**
     * Entry point called by the Translate Element once it has been injected in
     * the page.
     */
    onTranslateElementLoad() {
      loadedTime = performance.now();
      try {
        lib = google.translate.TranslateService({
          // translateApiKey is predefined by translate_script.cc.
          'key': translateApiKey,
          'serverParams': serverParams,
          'timeInfo': gtTimeInfo,
          'useSecureConnection': true,
        });
        translateApiKey = undefined;
        serverParams = undefined;
        gtTimeInfo = undefined;
      } catch (err) {
        errorCode = ERROR['INITIALIZATION_ERROR'];
        translateApiKey = undefined;
        serverParams = undefined;
        gtTimeInfo = undefined;
        invokeReadyCallback();
        return;
      }
      // The TranslateService is not available immediately as it needs to start
      // Flash.  Let's wait until it is ready.
      checkLibReady();
    },

    /**
     * Entry point called by the Translate Element when it want to load an
     * external CSS resource into the page.
     * @param {string} url URL of an external CSS resource to load.
     */
    onLoadCSS(url) {
      const element = document.createElement('link');
      element.type = 'text/css';
      element.rel = 'stylesheet';
      element.charset = 'UTF-8';
      element.href = url;
      document.head.appendChild(element);
    },

    /**
     * Entry point called by the Translate Element when it want to load and run
     * an external JavaScript on the page.
     * @param {string} url URL of an external JavaScript to load.
     */
    onLoadJavascript(url) {
      // securityOrigin is predefined by translate_script.cc.
      if (!url.startsWith(securityOrigin)) {
        console.error('Translate: ' + url + ' is not allowed to load.');
        errorCode = ERROR['BAD_ORIGIN'];
        return;
      }

      const xhr = new XMLHttpRequest();
      xhr.open('GET', url, true);
      xhr.onreadystatechange = function() {
        if (this.readyState !== this.DONE) {
          return;
        }
        if (this.status !== 200) {
          errorCode = ERROR['SCRIPT_LOAD_ERROR'];
          return;
        }
        // Execute translate script using an anonymous function on the window,
        // this prevents issues with the code being inside of the scope of the
        // XHR request.
        new Function(this.responseText).call(window);
      };
      xhr.send();
    },
  };
})();

// TRANSLATE_IOS

if (typeof _injected_translate_ios === 'undefined') {
    var _injected_translate_ios = true;
    (function() {
        if (!window.__gCrWeb) {
            window.__gCrWeb = {}
        }
        const e = window.__gCrWeb;
        function a(e, a) {
            try {
                var r = window.webkit;
                delete window["webkit"];
                window.webkit.messageHandlers[e].postMessage(a);
                window.webkit = r
            } catch (e) {}
        }
        function r() {
            cr.googleTranslate.readyCallback = function() {
                a("TranslateMessage", {
                    command: "ready",
                    errorCode: cr.googleTranslate.errorCode,
                    loadTime: cr.googleTranslate.loadTime,
                    readyTime: cr.googleTranslate.readyTime
                })
            };
            cr.googleTranslate.resultCallback = function() {
                a("TranslateMessage", {
                    command: "status",
                    errorCode: cr.googleTranslate.errorCode,
                    pageSourceLanguage: cr.googleTranslate.sourceLang,
                    translationTime: cr.googleTranslate.translationTime
                })
            }
        }
        function o(e, a) {
            cr.googleTranslate.translate(e, a)
        }
        function n() {
            try {
                cr.googleTranslate.revert()
            } catch {}
        }
        e.translate = {
            installCallbacks: r,
            startTranslation: o,
            revertTranslation: n
        }
    })();
}
  
  
// TRANSLATE_SCRIPT.cc
try {
    __gCrWeb.translate.installCallbacks();
} catch (error) {
  cr.googleTranslate.onTranslateElementError(error);
}



// go/mss-setup#7-load-the-js-or-css-from-your-initial-page
if (!window['_DumpException']) {
    const _DumpException = window['_DumpException'] || function(e) {
        throw e;
    };
    window['_DumpException'] = _DumpException;
}






// BRAVE TRANSLATE HOOKS???
try {
  const useGoogleTranslateEndpoint = false;
  const braveTranslateStaticPath = '/static/v1/';
  // securityOrigin is predefined by translate_script.cc.
  const securityOriginHost = new URL(securityOrigin).host;

  // A method to rewrite URL in the scripts:
  // 1. change the domain to translate.brave.com;
  // 2. adjust static paths to use braveTranslateStaticPath.
  const rewriteUrl = (url)=>{
      try {
          let new_url = new URL(url);
          if (new_url.pathname === '/translate_a/t') {
              // useGoogleTranslateEndpoint is predefined by translate_script.cc.
              // It's used only for local testing to disable the redirection of
              // translation requests.
              if (useGoogleTranslateEndpoint) {
                  // Remove API key
                  new_url.searchParams.set('key', '');

                  // Leave the domain unchanged (translate.googleapis.com).
                  return new_url.toString();
              }
          } else {
              // braveTranslateStaticPath is predefined by translate_script.cc.
              new_url.pathname = new_url.pathname.replace('/translate_static/', braveTranslateStaticPath);
          }
          new_url.host = securityOriginHost;
          return new_url.toString();
      } catch {
          return url;
      }
  }
  ;

  const emptySvgDataUrl = 'data:image/svg+xml;base64,' + btoa('<svg xmlns="http://www.w3.org/2000/svg"/>');

  // Make replacements in loading .js files.
  function processJavascript(text) {
      // Replace gen204 telemetry requests with loading an empty svg.
      text = text.replaceAll('"//"+po+"/gen204?"+Bo(b)', '"' + emptySvgDataUrl + '"');

      // Used in the injected elements, that are currently not visible. Replace it
      // to hide the loading error in devtools (because of CSP).
      text = text.replaceAll('https://www.gstatic.com/images/branding/product/1x/translate_24dp.png', emptySvgDataUrl);
      return text;
  }

  // Make replacements in loading .css files.
  function processCSS(text) {
      // Used in the injected elements, that are currently not visible. Replace it
      // to hide the loading error in devtools (because of CSP).
      text = text.replaceAll('//www.gstatic.com/images/branding/product/2x/translate_24dp.png', emptySvgDataUrl);
      return text;
  }

  // Used to rewrite urls for XHRs in the translate isolated world
  // (primarily for /translate_a/t).
  if (window.__firefox__.$<brave_translate_script>.useNativeNetworking) {
    const methodProperty = Symbol('method')
    const urlProperty = Symbol('url')
    const userProperty = Symbol('user')
    const passwordProperty = Symbol('password')
    const requestHeadersProperty = Symbol('requestHeaders')
    
    XMLHttpRequest.prototype.getResponseHeader = function(headerName) {
      return this[requestHeadersProperty][headerName];
    };
    
    XMLHttpRequest.prototype.getAllResponseHeaders = function() {
      return this[requestHeadersProperty];
    };
    
    XMLHttpRequest.prototype.realSetRequestHeader = XMLHttpRequest.prototype.setRequestHeader;
    XMLHttpRequest.prototype.setRequestHeader = function(header, value) {
      if (!this[requestHeadersProperty]) {
        this[requestHeadersProperty] = {};
      }
      
      this[requestHeadersProperty][header] = value;
      this.realSetRequestHeader(header, value);
    };
    
    XMLHttpRequest.prototype.realOverrideMimeType = XMLHttpRequest.prototype.overrideMimeType;
    XMLHttpRequest.prototype.overrideMimeType = function(mimeType) {
      this.realOverrideMimeType(mimeType);
    }
    
    XMLHttpRequest.prototype.realOpen = XMLHttpRequest.prototype.open;
    XMLHttpRequest.prototype.open = function(method, url, isAsync=true, user='', password='') {
      if (isAsync !== undefined && !isAsync) {
        return this.realOpen(method, rewriteUrl(url), isAsync, user, password);
      }

      this[methodProperty] = method;
      this[urlProperty] = rewriteUrl(url);
      this[userProperty] = user;
      this[passwordProperty] = password;
      return this.realOpen(method, rewriteUrl(url), isAsync, user, password);
    };

    XMLHttpRequest.prototype.realSend = XMLHttpRequest.prototype.send;
    XMLHttpRequest.prototype.send = function(body) {
      if (this[urlProperty] === undefined) {
        return this.realSend(body);
      }
      
      try {
        var t = window.webkit;
        delete window["webkit"];
        
        window.webkit.messageHandlers["TranslateMessage"].postMessage({
          "command": "request",
          "method": this[methodProperty] ?? "GET",
          "url": this[urlProperty],
          "user": this[userProperty] ?? "",
          "password": this[passwordProperty] ?? "",
          "headers": this[requestHeadersProperty] ?? {},
          "body": body ?? ""
        }).then((result) => {
          
          Object.defineProperties(this, {
            readyState: { value: XMLHttpRequest.DONE }  // DONE (4)
          });
          
          if (result.value) {
            Object.defineProperties(this, {
              status: { value: result.value.statusCode }
            });
            
            Object.defineProperties(this, {
              statusText: { value: "OK" }
            });
            
            Object.defineProperties(this, {
              responseType: { value: result.value.responseType }
            });
            
            Object.defineProperty(this, 'response', { writable: true });
            Object.defineProperty(this, 'responseText', { writable: true });
            this.responseText = result.value.response;
            
            switch (result.value.responseType) {
              case "arraybuffer": this.response = new ArrayBuffer(result.value.response);
              case "json": this.response = JSON.parse(result.value.response);
              case "text": this.response = result.value.response;
              case "": this.response = result.value.response;
            }
          }
          
          this.dispatchEvent(new ProgressEvent('loadstart'));
          this.dispatchEvent(new ProgressEvent(result.error ? 'error' : 'load'));
          this.dispatchEvent(new ProgressEvent('readystatechange'));
          this.dispatchEvent(new ProgressEvent('loadend'));
        });
        
        window.webkit = t
      } catch (e) {
        return this.realSend(body);
      }
    };
  } else {
    if (typeof XMLHttpRequest.prototype.realOpen === 'undefined') {
      XMLHttpRequest.prototype.realOpen = XMLHttpRequest.prototype.open;
      XMLHttpRequest.prototype.open = function(method, url, async=true, user='', password='') {
        this.realOpen(method, rewriteUrl(url), async, user, password);
      }
    }
  }
  ;
  // An overridden version of onLoadJavascript from translate.js, that fetches
  // and evaluates secondary scripts (i.e. main.js).
  // The differences:
  // 1. change url via rewriteUrl();
  // 2. process the loaded code via processJavascript().
  cr.googleTranslate.onLoadJavascript = function(url) {
      const xhr = new XMLHttpRequest();
      xhr.open('GET', rewriteUrl(url), true);
      xhr.onreadystatechange = function() {
          if (this.readyState !== this.DONE) {
              return;
          }
          if (this.status !== 200) {
              errorCode = ERROR['SCRIPT_LOAD_ERROR'];
              return;
          }

          // nosemgrep
          new Function(processJavascript(this.responseText)).call(window);
      }
      ;
      xhr.send();
  }
  ;

  // The styles to hide root elements that are injected by the scripts in the DOM.
  // Currently they are always invisible. The styles are added in case of changes
  // in future versions.
  const braveExtraStyles = `.goog-te-spinner-pos, #goog-gt-tt {display: none;}`

  // An overridden version of onLoadCSS from translate.js.
  // The differences:
  // 1. change url via rewriteUrl();
  // 2. process the loaded styles via processCSS().
  // 3. Add braveExtraStyles in the end.
  cr.googleTranslate.onLoadCSS = function(url) {
      const xhr = new XMLHttpRequest();
      xhr.open('GET', rewriteUrl(url), true);
      xhr.onreadystatechange = function() {
          if (this.readyState !== this.DONE || this.status !== 200) {
              return;
          }

          const element = document.createElement('style');
          element.type = 'text/css';
          element.charset = 'UTF-8';
          element.innerText = processCSS(this.responseText) + braveExtraStyles;
          document.head.appendChild(element);
      }
      ;
      xhr.send();
  };
} catch(error) {
  cr.googleTranslate.onTranslateElementError(error);
}

// BRAVE TRANSLATE (main.js)???

try {
  "use strict";
  this.default_tr = this.default_tr || {};
  (function(_) {
      var window = this;
      try {
          var ba, ea, ja, qa, xa, Aa, Ea, Ha, Ia, Ja, Ma, Na, Oa, Pa, Qa, Ra, Ua, Va, Za;
          _.aa = function(a, b) {
              if (Error.captureStackTrace)
                  Error.captureStackTrace(this, _.aa);
              else {
                  var c = Error().stack;
                  c && (this.stack = c)
              }
              a && (this.message = String(a));
              void 0 !== b && (this.cause = b)
          }
          ;
          ba = function(a) {
              _.p.setTimeout(function() {
                  throw a;
              }, 0)
          }
          ;
          _.ca = function(a) {
              a && "function" == typeof a.O && a.O()
          }
          ;
          ea = function(a) {
              for (var b = 0, c = arguments.length; b < c; ++b) {
                  var d = arguments[b];
                  _.da(d) ? ea.apply(null, d) : _.ca(d)
              }
          }
          ;
          ja = function() {
              !_.fa && _.ha && _.ia();
              return _.fa
          }
          ;
          _.ia = function() {
              _.fa = (0,
              _.ha)();
              ka.forEach(function(a) {
                  a(_.fa)
              });
              ka = []
          }
          ;
          _.ma = function(a) {
              _.fa && la(a)
          }
          ;
          _.oa = function() {
              _.fa && na(_.fa)
          }
          ;
          qa = function(a, b) {
              b.hasOwnProperty("displayName") || (b.displayName = a);
              b[pa] = a
          }
          ;
          _.ra = function(a) {
              return a[a.length - 1]
          }
          ;
          _.ta = function(a, b) {
              return 0 <= sa(a, b)
          }
          ;
          _.ua = function(a, b) {
              _.ta(a, b) || a.push(b)
          }
          ;
          _.va = function(a, b) {
              b = sa(a, b);
              var c;
              (c = 0 <= b) && Array.prototype.splice.call(a, b, 1);
              return c
          }
          ;
          _.wa = function(a) {
              var b = a.length;
              if (0 < b) {
                  for (var c = Array(b), d = 0; d < b; d++)
                      c[d] = a[d];
                  return c
              }
              return []
          }
          ;
          xa = function(a, b) {
              for (var c = 1; c < arguments.length; c++) {
                  var d = arguments[c];
                  if (_.da(d)) {
                      var e = a.length || 0
                        , f = d.length || 0;
                      a.length = e + f;
                      for (var g = 0; g < f; g++)
                          a[e + g] = d[g]
                  } else
                      a.push(d)
              }
          }
          ;
          Aa = function(a, b) {
              b = b || a;
              for (var c = 0, d = 0, e = {}; d < a.length; ) {
                  var f = a[d++]
                    , g = _.ya(f) ? "o" + _.za(f) : (typeof f).charAt(0) + f;
                  Object.prototype.hasOwnProperty.call(e, g) || (e[g] = !0,
                  b[c++] = f)
              }
              b.length = c
          }
          ;
          _.Ba = function() {
              var a = _.p.navigator;
              return a && (a = a.userAgent) ? a : ""
          }
          ;
          _.t = function(a) {
              return -1 != _.Ba().indexOf(a)
          }
          ;
          _.Ca = function() {
              return _.t("iPhone") && !_.t("iPod") && !_.t("iPad")
          }
          ;
          _.Da = function() {
              return _.Ca() || _.t("iPad") || _.t("iPod")
          }
          ;
          Ea = function(a, b) {
              for (var c in a)
                  if (b.call(void 0, a[c], c, a))
                      return !0;
              return !1
          }
          ;
          _.Fa = function(a) {
              var b = [], c = 0, d;
              for (d in a)
                  b[c++] = a[d];
              return b
          }
          ;
          Ha = function(a, b) {
              for (var c, d, e = 1; e < arguments.length; e++) {
                  d = arguments[e];
                  for (c in d)
                      a[c] = d[c];
                  for (var f = 0; f < Ga.length; f++)
                      c = Ga[f],
                      Object.prototype.hasOwnProperty.call(d, c) && (a[c] = d[c])
              }
          }
          ;
          Ia = function(a) {
              var b = arguments.length;
              if (1 == b && Array.isArray(arguments[0]))
                  return Ia.apply(null, arguments[0]);
              for (var c = {}, d = 0; d < b; d++)
                  c[arguments[d]] = !0;
              return c
          }
          ;
          Ja = function() {}
          ;
          _.La = function(a, b) {
              a.src = _.Ka(b);
              var c, d;
              (c = (b = null == (d = (c = (a.ownerDocument && a.ownerDocument.defaultView || window).document).querySelector) ? void 0 : d.call(c, "script[nonce]")) ? b.nonce || b.getAttribute("nonce") || "" : "") && a.setAttribute("nonce", c)
          }
          ;
          Ma = function(a) {
              var b = 0;
              return function() {
                  return b < a.length ? {
                      done: !1,
                      value: a[b++]
                  } : {
                      done: !0
                  }
              }
          }
          ;
          Na = "function" == typeof Object.defineProperties ? Object.defineProperty : function(a, b, c) {
              if (a == Array.prototype || a == Object.prototype)
                  return a;
              a[b] = c.value;
              return a
          }
          ;
          Oa = function(a) {
              a = ["object" == typeof globalThis && globalThis, a, "object" == typeof window && window, "object" == typeof self && self, "object" == typeof global && global];
              for (var b = 0; b < a.length; ++b) {
                  var c = a[b];
                  if (c && c.Math == Math)
                      return c
              }
              throw Error("a");
          }
          ;
          Pa = Oa(this);
          Qa = function(a, b) {
              if (b)
                  a: {
                      var c = Pa;
                      a = a.split(".");
                      for (var d = 0; d < a.length - 1; d++) {
                          var e = a[d];
                          if (!(e in c))
                              break a;
                          c = c[e]
                      }
                      a = a[a.length - 1];
                      d = c[a];
                      b = b(d);
                      b != d && null != b && Na(c, a, {
                          configurable: !0,
                          writable: !0,
                          value: b
                      })
                  }
          }
          ;
          Qa("Symbol", function(a) {
              if (a)
                  return a;
              var b = function(f, g) {
                  this.g = f;
                  Na(this, "description", {
                      configurable: !0,
                      writable: !0,
                      value: g
                  })
              };
              b.prototype.toString = function() {
                  return this.g
              }
              ;
              var c = "jscomp_symbol_" + (1E9 * Math.random() >>> 0) + "_"
                , d = 0
                , e = function(f) {
                  if (this instanceof e)
                      throw new TypeError("b");
                  return new b(c + (f || "") + "_" + d++,f)
              };
              return e
          });
          Qa("Symbol.iterator", function(a) {
              if (a)
                  return a;
              a = Symbol("c");
              for (var b = "Array Int8Array Uint8Array Uint8ClampedArray Int16Array Uint16Array Int32Array Uint32Array Float32Array Float64Array".split(" "), c = 0; c < b.length; c++) {
                  var d = Pa[b[c]];
                  "function" === typeof d && "function" != typeof d.prototype[a] && Na(d.prototype, a, {
                      configurable: !0,
                      writable: !0,
                      value: function() {
                          return Ra(Ma(this))
                      }
                  })
              }
              return a
          });
          Ra = function(a) {
              a = {
                  next: a
              };
              a[Symbol.iterator] = function() {
                  return this
              }
              ;
              return a
          }
          ;
          _.Sa = function(a) {
              var b = "undefined" != typeof Symbol && Symbol.iterator && a[Symbol.iterator];
              return b ? b.call(a) : {
                  next: Ma(a)
              }
          }
          ;
          _.Ta = function(a) {
              for (var b, c = []; !(b = a.next()).done; )
                  c.push(b.value);
              return c
          }
          ;
          Ua = "function" == typeof Object.create ? Object.create : function(a) {
              var b = function() {};
              b.prototype = a;
              return new b
          }
          ;
          if ("function" == typeof Object.setPrototypeOf)
              Va = Object.setPrototypeOf;
          else {
              var Wa;
              a: {
                  var Xa = {
                      a: !0
                  }
                    , Ya = {};
                  try {
                      Ya.__proto__ = Xa;
                      Wa = Ya.a;
                      break a
                  } catch (a) {}
                  Wa = !1
              }
              Va = Wa ? function(a, b) {
                  a.__proto__ = b;
                  if (a.__proto__ !== b)
                      throw new TypeError("d`" + a);
                  return a
              }
              : null
          }
          Za = Va;
          _.u = function(a, b) {
              a.prototype = Ua(b.prototype);
              a.prototype.constructor = a;
              if (Za)
                  Za(a, b);
              else
                  for (var c in b)
                      if ("prototype" != c)
                          if (Object.defineProperties) {
                              var d = Object.getOwnPropertyDescriptor(b, c);
                              d && Object.defineProperty(a, c, d)
                          } else
                              a[c] = b[c];
              a.G = b.prototype
          }
          ;
          _.$a = function() {
              for (var a = Number(this), b = [], c = a; c < arguments.length; c++)
                  b[c - a] = arguments[c];
              return b
          }
          ;
          Qa("Promise", function(a) {
              function b() {
                  this.g = null
              }
              function c(g) {
                  return g instanceof e ? g : new e(function(k) {
                      k(g)
                  }
                  )
              }
              if (a)
                  return a;
              b.prototype.h = function(g) {
                  if (null == this.g) {
                      this.g = [];
                      var k = this;
                      this.j(function() {
                          k.o()
                      })
                  }
                  this.g.push(g)
              }
              ;
              var d = Pa.setTimeout;
              b.prototype.j = function(g) {
                  d(g, 0)
              }
              ;
              b.prototype.o = function() {
                  for (; this.g && this.g.length; ) {
                      var g = this.g;
                      this.g = [];
                      for (var k = 0; k < g.length; ++k) {
                          var l = g[k];
                          g[k] = null;
                          try {
                              l()
                          } catch (m) {
                              this.l(m)
                          }
                      }
                  }
                  this.g = null
              }
              ;
              b.prototype.l = function(g) {
                  this.j(function() {
                      throw g;
                  })
              }
              ;
              var e = function(g) {
                  this.g = 0;
                  this.j = void 0;
                  this.h = [];
                  this.C = !1;
                  var k = this.l();
                  try {
                      g(k.resolve, k.reject)
                  } catch (l) {
                      k.reject(l)
                  }
              };
              e.prototype.l = function() {
                  function g(m) {
                      return function(n) {
                          l || (l = !0,
                          m.call(k, n))
                      }
                  }
                  var k = this
                    , l = !1;
                  return {
                      resolve: g(this.S),
                      reject: g(this.o)
                  }
              }
              ;
              e.prototype.S = function(g) {
                  if (g === this)
                      this.o(new TypeError("g"));
                  else if (g instanceof e)
                      this.ma(g);
                  else {
                      a: switch (typeof g) {
                      case "object":
                          var k = null != g;
                          break a;
                      case "function":
                          k = !0;
                          break a;
                      default:
                          k = !1
                      }
                      k ? this.K(g) : this.s(g)
                  }
              }
              ;
              e.prototype.K = function(g) {
                  var k = void 0;
                  try {
                      k = g.then
                  } catch (l) {
                      this.o(l);
                      return
                  }
                  "function" == typeof k ? this.W(k, g) : this.s(g)
              }
              ;
              e.prototype.o = function(g) {
                  this.B(2, g)
              }
              ;
              e.prototype.s = function(g) {
                  this.B(1, g)
              }
              ;
              e.prototype.B = function(g, k) {
                  if (0 != this.g)
                      throw Error("h`" + g + "`" + k + "`" + this.g);
                  this.g = g;
                  this.j = k;
                  2 === this.g && this.U();
                  this.J()
              }
              ;
              e.prototype.U = function() {
                  var g = this;
                  d(function() {
                      if (g.N()) {
                          var k = Pa.console;
                          "undefined" !== typeof k && k.error(g.j)
                      }
                  }, 1)
              }
              ;
              e.prototype.N = function() {
                  if (this.C)
                      return !1;
                  var g = Pa.CustomEvent
                    , k = Pa.Event
                    , l = Pa.dispatchEvent;
                  if ("undefined" === typeof l)
                      return !0;
                  "function" === typeof g ? g = new g("unhandledrejection",{
                      cancelable: !0
                  }) : "function" === typeof k ? g = new k("unhandledrejection",{
                      cancelable: !0
                  }) : (g = Pa.document.createEvent("CustomEvent"),
                  g.initCustomEvent("unhandledrejection", !1, !0, g));
                  g.promise = this;
                  g.reason = this.j;
                  return l(g)
              }
              ;
              e.prototype.J = function() {
                  if (null != this.h) {
                      for (var g = 0; g < this.h.length; ++g)
                          f.h(this.h[g]);
                      this.h = null
                  }
              }
              ;
              var f = new b;
              e.prototype.ma = function(g) {
                  var k = this.l();
                  g.Qd(k.resolve, k.reject)
              }
              ;
              e.prototype.W = function(g, k) {
                  var l = this.l();
                  try {
                      g.call(k, l.resolve, l.reject)
                  } catch (m) {
                      l.reject(m)
                  }
              }
              ;
              e.prototype.then = function(g, k) {
                  function l(r, A) {
                      return "function" == typeof r ? function(G) {
                          try {
                              m(r(G))
                          } catch (Q) {
                              n(Q)
                          }
                      }
                      : A
                  }
                  var m, n, q = new e(function(r, A) {
                      m = r;
                      n = A
                  }
                  );
                  this.Qd(l(g, m), l(k, n));
                  return q
              }
              ;
              e.prototype.catch = function(g) {
                  return this.then(void 0, g)
              }
              ;
              e.prototype.Qd = function(g, k) {
                  function l() {
                      switch (m.g) {
                      case 1:
                          g(m.j);
                          break;
                      case 2:
                          k(m.j);
                          break;
                      default:
                          throw Error("i`" + m.g);
                      }
                  }
                  var m = this;
                  null == this.h ? f.h(l) : this.h.push(l);
                  this.C = !0
              }
              ;
              e.resolve = c;
              e.reject = function(g) {
                  return new e(function(k, l) {
                      l(g)
                  }
                  )
              }
              ;
              e.race = function(g) {
                  return new e(function(k, l) {
                      for (var m = _.Sa(g), n = m.next(); !n.done; n = m.next())
                          c(n.value).Qd(k, l)
                  }
                  )
              }
              ;
              e.all = function(g) {
                  var k = _.Sa(g)
                    , l = k.next();
                  return l.done ? c([]) : new e(function(m, n) {
                      function q(G) {
                          return function(Q) {
                              r[G] = Q;
                              A--;
                              0 == A && m(r)
                          }
                      }
                      var r = []
                        , A = 0;
                      do
                          r.push(void 0),
                          A++,
                          c(l.value).Qd(q(r.length - 1), n),
                          l = k.next();
                      while (!l.done)
                  }
                  )
              }
              ;
              return e
          });
          var ab = function(a, b, c) {
              if (null == a)
                  throw new TypeError("j`" + c);
              if (b instanceof RegExp)
                  throw new TypeError("k`" + c);
              return a + ""
          };
          Qa("String.prototype.startsWith", function(a) {
              return a ? a : function(b, c) {
                  var d = ab(this, b, "startsWith")
                    , e = d.length
                    , f = b.length;
                  c = Math.max(0, Math.min(c | 0, d.length));
                  for (var g = 0; g < f && c < e; )
                      if (d[c++] != b[g++])
                          return !1;
                  return g >= f
              }
          });
          var bb = function(a, b) {
              return Object.prototype.hasOwnProperty.call(a, b)
          };
          Qa("WeakMap", function(a) {
              function b() {}
              function c(l) {
                  var m = typeof l;
                  return "object" === m && null !== l || "function" === m
              }
              function d(l) {
                  if (!bb(l, f)) {
                      var m = new b;
                      Na(l, f, {
                          value: m
                      })
                  }
              }
              function e(l) {
                  var m = Object[l];
                  m && (Object[l] = function(n) {
                      if (n instanceof b)
                          return n;
                      Object.isExtensible(n) && d(n);
                      return m(n)
                  }
                  )
              }
              if (function() {
                  if (!a || !Object.seal)
                      return !1;
                  try {
                      var l = Object.seal({})
                        , m = Object.seal({})
                        , n = new a([[l, 2], [m, 3]]);
                      if (2 != n.get(l) || 3 != n.get(m))
                          return !1;
                      n.delete(l);
                      n.set(m, 4);
                      return !n.has(l) && 4 == n.get(m)
                  } catch (q) {
                      return !1
                  }
              }())
                  return a;
              var f = "$jscomp_hidden_" + Math.random();
              e("freeze");
              e("preventExtensions");
              e("seal");
              var g = 0
                , k = function(l) {
                  this.g = (g += Math.random() + 1).toString();
                  if (l) {
                      l = _.Sa(l);
                      for (var m; !(m = l.next()).done; )
                          m = m.value,
                          this.set(m[0], m[1])
                  }
              };
              k.prototype.set = function(l, m) {
                  if (!c(l))
                      throw Error("l");
                  d(l);
                  if (!bb(l, f))
                      throw Error("m`" + l);
                  l[f][this.g] = m;
                  return this
              }
              ;
              k.prototype.get = function(l) {
                  return c(l) && bb(l, f) ? l[f][this.g] : void 0
              }
              ;
              k.prototype.has = function(l) {
                  return c(l) && bb(l, f) && bb(l[f], this.g)
              }
              ;
              k.prototype.delete = function(l) {
                  return c(l) && bb(l, f) && bb(l[f], this.g) ? delete l[f][this.g] : !1
              }
              ;
              return k
          });
          Qa("Map", function(a) {
              if (function() {
                  if (!a || "function" != typeof a || !a.prototype.entries || "function" != typeof Object.seal)
                      return !1;
                  try {
                      var k = Object.seal({
                          x: 4
                      })
                        , l = new a(_.Sa([[k, "s"]]));
                      if ("s" != l.get(k) || 1 != l.size || l.get({
                          x: 4
                      }) || l.set({
                          x: 4
                      }, "t") != l || 2 != l.size)
                          return !1;
                      var m = l.entries()
                        , n = m.next();
                      if (n.done || n.value[0] != k || "s" != n.value[1])
                          return !1;
                      n = m.next();
                      return n.done || 4 != n.value[0].x || "t" != n.value[1] || !m.next().done ? !1 : !0
                  } catch (q) {
                      return !1
                  }
              }())
                  return a;
              var b = new WeakMap
                , c = function(k) {
                  this.h = {};
                  this.g = f();
                  this.size = 0;
                  if (k) {
                      k = _.Sa(k);
                      for (var l; !(l = k.next()).done; )
                          l = l.value,
                          this.set(l[0], l[1])
                  }
              };
              c.prototype.set = function(k, l) {
                  k = 0 === k ? 0 : k;
                  var m = d(this, k);
                  m.list || (m.list = this.h[m.id] = []);
                  m.Ra ? m.Ra.value = l : (m.Ra = {
                      next: this.g,
                      Mb: this.g.Mb,
                      head: this.g,
                      key: k,
                      value: l
                  },
                  m.list.push(m.Ra),
                  this.g.Mb.next = m.Ra,
                  this.g.Mb = m.Ra,
                  this.size++);
                  return this
              }
              ;
              c.prototype.delete = function(k) {
                  k = d(this, k);
                  return k.Ra && k.list ? (k.list.splice(k.index, 1),
                  k.list.length || delete this.h[k.id],
                  k.Ra.Mb.next = k.Ra.next,
                  k.Ra.next.Mb = k.Ra.Mb,
                  k.Ra.head = null,
                  this.size--,
                  !0) : !1
              }
              ;
              c.prototype.clear = function() {
                  this.h = {};
                  this.g = this.g.Mb = f();
                  this.size = 0
              }
              ;
              c.prototype.has = function(k) {
                  return !!d(this, k).Ra
              }
              ;
              c.prototype.get = function(k) {
                  return (k = d(this, k).Ra) && k.value
              }
              ;
              c.prototype.entries = function() {
                  return e(this, function(k) {
                      return [k.key, k.value]
                  })
              }
              ;
              c.prototype.keys = function() {
                  return e(this, function(k) {
                      return k.key
                  })
              }
              ;
              c.prototype.values = function() {
                  return e(this, function(k) {
                      return k.value
                  })
              }
              ;
              c.prototype.forEach = function(k, l) {
                  for (var m = this.entries(), n; !(n = m.next()).done; )
                      n = n.value,
                      k.call(l, n[1], n[0], this)
              }
              ;
              c.prototype[Symbol.iterator] = c.prototype.entries;
              var d = function(k, l) {
                  var m = l && typeof l;
                  "object" == m || "function" == m ? b.has(l) ? m = b.get(l) : (m = "" + ++g,
                  b.set(l, m)) : m = "p_" + l;
                  var n = k.h[m];
                  if (n && bb(k.h, m))
                      for (k = 0; k < n.length; k++) {
                          var q = n[k];
                          if (l !== l && q.key !== q.key || l === q.key)
                              return {
                                  id: m,
                                  list: n,
                                  index: k,
                                  Ra: q
                              }
                      }
                  return {
                      id: m,
                      list: n,
                      index: -1,
                      Ra: void 0
                  }
              }
                , e = function(k, l) {
                  var m = k.g;
                  return Ra(function() {
                      if (m) {
                          for (; m.head != k.g; )
                              m = m.Mb;
                          for (; m.next != m.head; )
                              return m = m.next,
                              {
                                  done: !1,
                                  value: l(m)
                              };
                          m = null
                      }
                      return {
                          done: !0,
                          value: void 0
                      }
                  })
              }
                , f = function() {
                  var k = {};
                  return k.Mb = k.next = k.head = k
              }
                , g = 0;
              return c
          });
          Qa("Array.prototype.find", function(a) {
              return a ? a : function(b, c) {
                  a: {
                      var d = this;
                      d instanceof String && (d = String(d));
                      for (var e = d.length, f = 0; f < e; f++) {
                          var g = d[f];
                          if (b.call(c, g, f, d)) {
                              b = g;
                              break a
                          }
                      }
                      b = void 0
                  }
                  return b
              }
          });
          Qa("String.prototype.endsWith", function(a) {
              return a ? a : function(b, c) {
                  var d = ab(this, b, "endsWith");
                  void 0 === c && (c = d.length);
                  c = Math.max(0, Math.min(c | 0, d.length));
                  for (var e = b.length; 0 < e && 0 < c; )
                      if (d[--c] != b[--e])
                          return !1;
                  return 0 >= e
              }
          });
          var cb = function(a, b) {
              a instanceof String && (a += "");
              var c = 0
                , d = !1
                , e = {
                  next: function() {
                      if (!d && c < a.length) {
                          var f = c++;
                          return {
                              value: b(f, a[f]),
                              done: !1
                          }
                      }
                      d = !0;
                      return {
                          done: !0,
                          value: void 0
                      }
                  }
              };
              e[Symbol.iterator] = function() {
                  return e
              }
              ;
              return e
          };
          Qa("Array.prototype.entries", function(a) {
              return a ? a : function() {
                  return cb(this, function(b, c) {
                      return [b, c]
                  })
              }
          });
          Qa("Array.prototype.keys", function(a) {
              return a ? a : function() {
                  return cb(this, function(b) {
                      return b
                  })
              }
          });
          Qa("Array.from", function(a) {
              return a ? a : function(b, c, d) {
                  c = null != c ? c : function(k) {
                      return k
                  }
                  ;
                  var e = []
                    , f = "undefined" != typeof Symbol && Symbol.iterator && b[Symbol.iterator];
                  if ("function" == typeof f) {
                      b = f.call(b);
                      for (var g = 0; !(f = b.next()).done; )
                          e.push(c.call(d, f.value, g++))
                  } else
                      for (f = b.length,
                      g = 0; g < f; g++)
                          e.push(c.call(d, b[g], g));
                  return e
              }
          });
          Qa("Array.prototype.values", function(a) {
              return a ? a : function() {
                  return cb(this, function(b, c) {
                      return c
                  })
              }
          });
          Qa("Set", function(a) {
              if (function() {
                  if (!a || "function" != typeof a || !a.prototype.entries || "function" != typeof Object.seal)
                      return !1;
                  try {
                      var c = Object.seal({
                          x: 4
                      })
                        , d = new a(_.Sa([c]));
                      if (!d.has(c) || 1 != d.size || d.add(c) != d || 1 != d.size || d.add({
                          x: 4
                      }) != d || 2 != d.size)
                          return !1;
                      var e = d.entries()
                        , f = e.next();
                      if (f.done || f.value[0] != c || f.value[1] != c)
                          return !1;
                      f = e.next();
                      return f.done || f.value[0] == c || 4 != f.value[0].x || f.value[1] != f.value[0] ? !1 : e.next().done
                  } catch (g) {
                      return !1
                  }
              }())
                  return a;
              var b = function(c) {
                  this.g = new Map;
                  if (c) {
                      c = _.Sa(c);
                      for (var d; !(d = c.next()).done; )
                          this.add(d.value)
                  }
                  this.size = this.g.size
              };
              b.prototype.add = function(c) {
                  c = 0 === c ? 0 : c;
                  this.g.set(c, c);
                  this.size = this.g.size;
                  return this
              }
              ;
              b.prototype.delete = function(c) {
                  c = this.g.delete(c);
                  this.size = this.g.size;
                  return c
              }
              ;
              b.prototype.clear = function() {
                  this.g.clear();
                  this.size = 0
              }
              ;
              b.prototype.has = function(c) {
                  return this.g.has(c)
              }
              ;
              b.prototype.entries = function() {
                  return this.g.entries()
              }
              ;
              b.prototype.values = function() {
                  return this.g.values()
              }
              ;
              b.prototype.keys = b.prototype.values;
              b.prototype[Symbol.iterator] = b.prototype.values;
              b.prototype.forEach = function(c, d) {
                  var e = this;
                  this.g.forEach(function(f) {
                      return c.call(d, f, f, e)
                  })
              }
              ;
              return b
          });
          var db = "function" == typeof Object.assign ? Object.assign : function(a, b) {
              for (var c = 1; c < arguments.length; c++) {
                  var d = arguments[c];
                  if (d)
                      for (var e in d)
                          bb(d, e) && (a[e] = d[e])
              }
              return a
          }
          ;
          Qa("Object.assign", function(a) {
              return a || db
          });
          Qa("String.prototype.replaceAll", function(a) {
              return a ? a : function(b, c) {
                  if (b instanceof RegExp && !b.global)
                      throw new TypeError("n");
                  return b instanceof RegExp ? this.replace(b, c) : this.replace(new RegExp(String(b).replace(/([-()\[\]{}+?*.$\^|,:#<!\\])/g, "\\$1").replace(/\x08/g, "\\x08"),"g"), c)
              }
          });
          Qa("Number.isNaN", function(a) {
              return a ? a : function(b) {
                  return "number" === typeof b && isNaN(b)
              }
          });
          Qa("Object.is", function(a) {
              return a ? a : function(b, c) {
                  return b === c ? 0 !== b || 1 / b === 1 / c : b !== b && c !== c
              }
          });
          Qa("Array.prototype.includes", function(a) {
              return a ? a : function(b, c) {
                  var d = this;
                  d instanceof String && (d = String(d));
                  var e = d.length;
                  c = c || 0;
                  for (0 > c && (c = Math.max(c + e, 0)); c < e; c++) {
                      var f = d[c];
                      if (f === b || Object.is(f, b))
                          return !0
                  }
                  return !1
              }
          });
          Qa("String.prototype.includes", function(a) {
              return a ? a : function(b, c) {
                  return -1 !== ab(this, b, "includes").indexOf(b, c || 0)
              }
          });
          Qa("Object.entries", function(a) {
              return a ? a : function(b) {
                  var c = [], d;
                  for (d in b)
                      bb(b, d) && c.push([d, b[d]]);
                  return c
              }
          });
          _._DumpException = window._DumpException || function(a) {
              throw a;
          }
          ;
          window._DumpException = _._DumpException;
          /*

Copyright The Closure Library Authors.
SPDX-License-Identifier: Apache-2.0
*/
          var eb, fb, hb, gb, kb, lb, mb, nb, sb;
          eb = eb || {};
          _.p = this || self;
          fb = /^[a-zA-Z_$][a-zA-Z0-9._$]*$/;
          hb = function(a) {
              if ("string" !== typeof a || !a || -1 == a.search(fb))
                  throw Error("o");
              if (!gb || "goog" != gb.type)
                  throw Error("p`" + a);
              if (gb.Vj)
                  throw Error("q");
              gb.Vj = a
          }
          ;
          hb.get = function() {
              return null
          }
          ;
          gb = null;
          _.ib = function(a, b) {
              a = a.split(".");
              b = b || _.p;
              for (var c = 0; c < a.length; c++)
                  if (b = b[a[c]],
                  null == b)
                      return null;
              return b
          }
          ;
          _.jb = function(a) {
              var b = typeof a;
              return "object" != b ? b : a ? Array.isArray(a) ? "array" : b : "null"
          }
          ;
          _.da = function(a) {
              var b = _.jb(a);
              return "array" == b || "object" == b && "number" == typeof a.length
          }
          ;
          _.ya = function(a) {
              var b = typeof a;
              return "object" == b && null != a || "function" == b
          }
          ;
          _.za = function(a) {
              return Object.prototype.hasOwnProperty.call(a, kb) && a[kb] || (a[kb] = ++lb)
          }
          ;
          kb = "closure_uid_" + (1E9 * Math.random() >>> 0);
          lb = 0;
          mb = function(a, b, c) {
              return a.call.apply(a.bind, arguments)
          }
          ;
          nb = function(a, b, c) {
              if (!a)
                  throw Error();
              if (2 < arguments.length) {
                  var d = Array.prototype.slice.call(arguments, 2);
                  return function() {
                      var e = Array.prototype.slice.call(arguments);
                      Array.prototype.unshift.apply(e, d);
                      return a.apply(b, e)
                  }
              }
              return function() {
                  return a.apply(b, arguments)
              }
          }
          ;
          _.v = function(a, b, c) {
              Function.prototype.bind && -1 != Function.prototype.bind.toString().indexOf("native code") ? _.v = mb : _.v = nb;
              return _.v.apply(null, arguments)
          }
          ;
          _.ob = function(a, b) {
              var c = Array.prototype.slice.call(arguments, 1);
              return function() {
                  var d = c.slice();
                  d.push.apply(d, arguments);
                  return a.apply(this, d)
              }
          }
          ;
          _.qb = function() {
              return Date.now()
          }
          ;
          _.rb = function(a, b) {
              a = a.split(".");
              var c = _.p;
              a[0]in c || "undefined" == typeof c.execScript || c.execScript("var " + a[0]);
              for (var d; a.length && (d = a.shift()); )
                  a.length || void 0 === b ? c[d] && c[d] !== Object.prototype[d] ? c = c[d] : c = c[d] = {} : c[d] = b
          }
          ;
          _.w = function(a, b) {
              function c() {}
              c.prototype = b.prototype;
              a.G = b.prototype;
              a.prototype = new c;
              a.prototype.constructor = a;
              a.Pl = function(d, e, f) {
                  for (var g = Array(arguments.length - 2), k = 2; k < arguments.length; k++)
                      g[k - 2] = arguments[k];
                  return b.prototype[e].apply(d, g)
              }
          }
          ;
          sb = function(a) {
              return a
          }
          ;
          _.w(_.aa, Error);
          _.aa.prototype.name = "CustomError";
          var tb;
          _.x = function() {
              this.La = this.La;
              this.ma = this.ma
          }
          ;
          _.x.prototype.La = !1;
          _.x.prototype.zb = function() {
              return this.La
          }
          ;
          _.x.prototype.O = function() {
              this.La || (this.La = !0,
              this.L())
          }
          ;
          _.x.prototype.L = function() {
              if (this.ma)
                  for (; this.ma.length; )
                      this.ma.shift()()
          }
          ;
          var vb;
          _.ub = function() {}
          ;
          vb = function(a) {
              return function() {
                  throw Error(a);
              }
          }
          ;
          _.wb = function(a) {
              var b = !1, c;
              return function() {
                  b || (c = a(),
                  b = !0);
                  return c
              }
          }
          ;
          var xb, yb = function() {
              if (void 0 === xb) {
                  var a = null
                    , b = _.p.trustedTypes;
                  if (b && b.createPolicy) {
                      try {
                          a = b.createPolicy("goog#html", {
                              createHTML: sb,
                              createScript: sb,
                              createScriptURL: sb
                          })
                      } catch (c) {
                          _.p.console && _.p.console.error(c.message)
                      }
                      xb = a
                  } else
                      xb = a
              }
              return xb
          };
          var zb = {}
            , Ab = function(a, b) {
              this.g = b === zb ? a : "";
              this.tb = !0
          };
          Ab.prototype.toString = function() {
              return this.g.toString()
          }
          ;
          Ab.prototype.Xa = function() {
              return this.g.toString()
          }
          ;
          _.Bb = function(a) {
              return a instanceof Ab && a.constructor === Ab ? a.g : "type_error:SafeScript"
          }
          ;
          _.Cb = function(a) {
              var b = yb();
              a = b ? b.createScript(a) : a;
              return new Ab(a,zb)
          }
          ;
          _.Db = RegExp("^(ar|ckb|dv|he|iw|fa|nqo|ps|sd|ug|ur|yi|.*[-_](Adlm|Arab|Hebr|Nkoo|Rohg|Thaa))(?!.*[-_](Latn|Cyrl)($|-|_))($|-|_)", "i");
          var Eb;
          _.Fb = function(a, b) {
              this.g = b === Eb ? a : ""
          }
          ;
          _.Fb.prototype.toString = function() {
              return this.g + ""
          }
          ;
          _.Fb.prototype.tb = !0;
          _.Fb.prototype.Xa = function() {
              return this.g.toString()
          }
          ;
          _.Ka = function(a) {
              return a instanceof _.Fb && a.constructor === _.Fb ? a.g : "type_error:TrustedResourceUrl"
          }
          ;
          _.Gb = RegExp("^((https:)?//[0-9a-z.:[\\]-]+/|/[^/\\\\]|[^:/\\\\%]+/|[^:/\\\\%]*[?#]|about:blank#)", "i");
          Eb = {};
          _.Hb = function(a) {
              var b = yb();
              a = b ? b.createScriptURL(a) : a;
              return new _.Fb(a,Eb)
          }
          ;
          hb = hb || {};
          var Ib = function() {
              _.x.call(this)
          };
          _.w(Ib, _.x);
          Ib.prototype.initialize = function() {}
          ;
          var Jb = function(a, b) {
              this.g = a;
              this.h = b
          };
          Jb.prototype.j = function(a) {
              this.g && (this.g.call(this.h || null, a),
              this.g = this.h = null)
          }
          ;
          Jb.prototype.abort = function() {
              this.h = this.g = null
          }
          ;
          var Kb = function(a, b) {
              _.x.call(this);
              this.s = a;
              this.o = b;
              this.j = [];
              this.h = [];
              this.l = []
          };
          _.w(Kb, _.x);
          Kb.prototype.C = Ib;
          Kb.prototype.g = null;
          Kb.prototype.Dc = function() {
              return this.s
          }
          ;
          Kb.prototype.mb = function() {
              return this.o
          }
          ;
          var Lb = function(a, b) {
              a.h.push(new Jb(b))
          };
          Kb.prototype.onLoad = function(a) {
              var b = new this.C;
              b.initialize(a());
              this.g = b;
              b = (b = !!Mb(this.l, a())) || !!Mb(this.j, a());
              b || (this.h.length = 0);
              return b
          }
          ;
          Kb.prototype.Of = function(a) {
              (a = Mb(this.h, a)) && _.p.setTimeout(vb("Module errback failures: " + a), 0);
              this.l.length = 0;
              this.j.length = 0
          }
          ;
          var Mb = function(a, b) {
              for (var c = [], d = 0; d < a.length; d++)
                  try {
                      a[d].j(b)
                  } catch (e) {
                      ba(e),
                      c.push(e)
                  }
              a.length = 0;
              return c.length ? c : null
          };
          Kb.prototype.L = function() {
              Kb.G.L.call(this);
              _.ca(this.g)
          }
          ;
          var Nb = function() {
              this.B = this.La = null
          };
          _.h = Nb.prototype;
          _.h.Kh = function() {}
          ;
          _.h.bg = function() {}
          ;
          _.h.Ih = function() {
              throw Error("u");
          }
          ;
          _.h.Vg = function() {
              return this.La
          }
          ;
          _.h.cg = function(a) {
              this.La = a
          }
          ;
          _.h.pg = function() {
              return !1
          }
          ;
          _.h.ph = function() {
              return !1
          }
          ;
          var ka;
          _.fa = null;
          _.ha = null;
          ka = [];
          var y = function(a, b) {
              var c = c || [];
              this.qk = a;
              this.Uj = b || null;
              this.ff = [];
              this.ff = this.ff.concat(c)
          };
          y.prototype.toString = function() {
              return this.qk
          }
          ;
          y.prototype.Dc = function() {
              return this.ff
          }
          ;
          new y("rJmJrc","rJmJrc");
          var Ob = new y("n73qwf","n73qwf");
          var pa = Symbol("w");
          var sa, Rb;
          sa = Array.prototype.indexOf ? function(a, b) {
              return Array.prototype.indexOf.call(a, b, void 0)
          }
          : function(a, b) {
              if ("string" === typeof a)
                  return "string" !== typeof b || 1 != b.length ? -1 : a.indexOf(b, 0);
              for (var c = 0; c < a.length; c++)
                  if (c in a && a[c] === b)
                      return c;
              return -1
          }
          ;
          _.Pb = Array.prototype.forEach ? function(a, b) {
              Array.prototype.forEach.call(a, b, void 0)
          }
          : function(a, b) {
              for (var c = a.length, d = "string" === typeof a ? a.split("") : a, e = 0; e < c; e++)
                  e in d && b.call(void 0, d[e], e, a)
          }
          ;
          _.Qb = Array.prototype.map ? function(a, b) {
              return Array.prototype.map.call(a, b, void 0)
          }
          : function(a, b) {
              for (var c = a.length, d = Array(c), e = "string" === typeof a ? a.split("") : a, f = 0; f < c; f++)
                  f in e && (d[f] = b.call(void 0, e[f], f, a));
              return d
          }
          ;
          Rb = Array.prototype.reduce ? function(a, b, c) {
              Array.prototype.reduce.call(a, b, c)
          }
          : function(a, b, c) {
              var d = c;
              (0,
              _.Pb)(a, function(e, f) {
                  d = b.call(void 0, d, e, f, a)
              })
          }
          ;
          _.Sb = Array.prototype.some ? function(a, b) {
              return Array.prototype.some.call(a, b, void 0)
          }
          : function(a, b) {
              for (var c = a.length, d = "string" === typeof a ? a.split("") : a, e = 0; e < c; e++)
                  if (e in d && b.call(void 0, d[e], e, a))
                      return !0;
              return !1
          }
          ;
          _.Tb = Array.prototype.every ? function(a, b) {
              return Array.prototype.every.call(a, b, void 0)
          }
          : function(a, b) {
              for (var c = a.length, d = "string" === typeof a ? a.split("") : a, e = 0; e < c; e++)
                  if (e in d && !b.call(void 0, d[e], e, a))
                      return !1;
              return !0
          }
          ;
          var Wb;
          _.Ub = function(a, b) {
              var c = a.length - b.length;
              return 0 <= c && a.indexOf(b, c) == c
          }
          ;
          _.Vb = String.prototype.trim ? function(a) {
              return a.trim()
          }
          : function(a) {
              return /^[\s\xa0]*([\s\S]*?)[\s\xa0]*$/.exec(a)[1]
          }
          ;
          _.Xb = function(a, b) {
              var c = 0;
              a = (0,
              _.Vb)(String(a)).split(".");
              b = (0,
              _.Vb)(String(b)).split(".");
              for (var d = Math.max(a.length, b.length), e = 0; 0 == c && e < d; e++) {
                  var f = a[e] || ""
                    , g = b[e] || "";
                  do {
                      f = /(\d*)(\D*)(.*)/.exec(f) || ["", "", "", ""];
                      g = /(\d*)(\D*)(.*)/.exec(g) || ["", "", "", ""];
                      if (0 == f[0].length && 0 == g[0].length)
                          break;
                      c = Wb(0 == f[1].length ? 0 : parseInt(f[1], 10), 0 == g[1].length ? 0 : parseInt(g[1], 10)) || Wb(0 == f[2].length, 0 == g[2].length) || Wb(f[2], g[2]);
                      f = f[3];
                      g = g[3]
                  } while (0 == c)
              }
              return c
          }
          ;
          Wb = function(a, b) {
              return a < b ? -1 : a > b ? 1 : 0
          }
          ;
          var Yb = function(a) {
              Yb[" "](a);
              return a
          };
          Yb[" "] = function() {}
          ;
          _.Zb = function(a, b) {
              try {
                  return Yb(a[b]),
                  !0
              } catch (c) {}
              return !1
          }
          ;
          _.$b = function(a, b, c) {
              return Object.prototype.hasOwnProperty.call(a, b) ? a[b] : a[b] = c(b)
          }
          ;
          var mc, nc, sc, uc;
          _.ac = _.t("Opera");
          _.z = _.t("Trident") || _.t("MSIE");
          _.bc = _.t("Edge");
          _.cc = _.bc || _.z;
          _.B = _.t("Gecko") && !(-1 != _.Ba().toLowerCase().indexOf("webkit") && !_.t("Edge")) && !(_.t("Trident") || _.t("MSIE")) && !_.t("Edge");
          _.C = -1 != _.Ba().toLowerCase().indexOf("webkit") && !_.t("Edge");
          _.dc = _.C && _.t("Mobile");
          _.ec = _.t("Macintosh");
          _.fc = _.t("Windows");
          _.gc = _.t("Android");
          _.hc = _.Ca();
          _.ic = _.t("iPad");
          _.jc = _.t("iPod");
          _.lc = _.Da();
          mc = function() {
              var a = _.p.document;
              return a ? a.documentMode : void 0
          }
          ;
          a: {
              var oc = ""
                , pc = function() {
                  var a = _.Ba();
                  if (_.B)
                      return /rv:([^\);]+)(\)|;)/.exec(a);
                  if (_.bc)
                      return /Edge\/([\d\.]+)/.exec(a);
                  if (_.z)
                      return /\b(?:MSIE|rv)[: ]([^\);]+)(\)|;)/.exec(a);
                  if (_.C)
                      return /WebKit\/(\S+)/.exec(a);
                  if (_.ac)
                      return /(?:Version)[ \/]?(\S+)/.exec(a)
              }();
              pc && (oc = pc ? pc[1] : "");
              if (_.z) {
                  var qc = mc();
                  if (null != qc && qc > parseFloat(oc)) {
                      nc = String(qc);
                      break a
                  }
              }
              nc = oc
          }
          _.rc = nc;
          sc = {};
          _.tc = function(a) {
              return _.$b(sc, a, function() {
                  return 0 <= _.Xb(_.rc, a)
              })
          }
          ;
          if (_.p.document && _.z) {
              var vc = mc();
              uc = vc ? vc : parseInt(_.rc, 10) || void 0
          } else
              uc = void 0;
          _.wc = uc;
          _.xc = _.z || _.C;
          var Ga;
          Ga = "constructor hasOwnProperty isPrototypeOf propertyIsEnumerable toLocaleString toString valueOf".split(" ");
          _.yc = function(a, b, c) {
              for (var d in a)
                  b.call(c, a[d], d, a)
          }
          ;
          var zc;
          _.Ac = function(a, b) {
              this.g = b === zc ? a : ""
          }
          ;
          _.Ac.prototype.toString = function() {
              return this.g.toString()
          }
          ;
          _.Ac.prototype.tb = !0;
          _.Ac.prototype.Xa = function() {
              return this.g.toString()
          }
          ;
          zc = {};
          _.Bc = function(a) {
              return new _.Ac(a,zc)
          }
          ;
          _.Cc = _.Bc("about:invalid#zClosurez");
          _.Dc = {};
          _.Ec = function(a, b) {
              this.g = b === _.Dc ? a : "";
              this.tb = !0
          }
          ;
          _.Ec.prototype.Xa = function() {
              return this.g
          }
          ;
          _.Ec.prototype.toString = function() {
              return this.g.toString()
          }
          ;
          _.Fc = new _.Ec("",_.Dc);
          _.Gc = RegExp("^[-,.\"'%_!#/ a-zA-Z0-9\\[\\]]+$");
          _.Hc = RegExp("\\b(url\\([ \t\n]*)('[ -&(-\\[\\]-~]*'|\"[ !#-\\[\\]-~]*\"|[!#-&*-\\[\\]-~]*)([ \t\n]*\\))", "g");
          _.Ic = RegExp("\\b(calc|cubic-bezier|fit-content|hsl|hsla|linear-gradient|matrix|minmax|radial-gradient|repeat|rgb|rgba|(rotate|scale|translate)(X|Y|Z|3d)?|var)\\([-+*/0-9a-zA-Z.%#\\[\\], ]+\\)", "g");
          var Jc;
          Jc = {};
          _.Kc = function(a, b) {
              this.g = b === Jc ? a : "";
              this.tb = !0
          }
          ;
          _.Kc.prototype.Xa = function() {
              return this.g.toString()
          }
          ;
          _.Kc.prototype.toString = function() {
              return this.g.toString()
          }
          ;
          _.Lc = function(a) {
              return a instanceof _.Kc && a.constructor === _.Kc ? a.g : "type_error:SafeHtml"
          }
          ;
          _.Mc = function(a) {
              var b = yb();
              a = b ? b.createHTML(a) : a;
              return new _.Kc(a,Jc)
          }
          ;
          _.Nc = _.Mc("<!DOCTYPE html>");
          _.Oc = new _.Kc(_.p.trustedTypes && _.p.trustedTypes.emptyHTML || "",Jc);
          _.Pc = _.Mc("<br>");
          _.Qc = _.wb(function() {
              var a = document.createElement("div")
                , b = document.createElement("div");
              b.appendChild(document.createElement("div"));
              a.appendChild(b);
              b = a.firstChild.firstChild;
              a.innerHTML = _.Lc(_.Oc);
              return !b.parentElement
          });
          _.Rc = function(a, b) {
              this.width = a;
              this.height = b
          }
          ;
          _.Sc = function(a, b) {
              return a == b ? !0 : a && b ? a.width == b.width && a.height == b.height : !1
          }
          ;
          _.h = _.Rc.prototype;
          _.h.aspectRatio = function() {
              return this.width / this.height
          }
          ;
          _.h.pc = function() {
              return !(this.width * this.height)
          }
          ;
          _.h.ceil = function() {
              this.width = Math.ceil(this.width);
              this.height = Math.ceil(this.height);
              return this
          }
          ;
          _.h.floor = function() {
              this.width = Math.floor(this.width);
              this.height = Math.floor(this.height);
              return this
          }
          ;
          _.h.round = function() {
              this.width = Math.round(this.width);
              this.height = Math.round(this.height);
              return this
          }
          ;
          _.Tc = function(a) {
              return encodeURIComponent(String(a))
          }
          ;
          _.Uc = function(a) {
              return decodeURIComponent(a.replace(/\+/g, " "))
          }
          ;
          _.Vc = function() {
              return Math.floor(2147483648 * Math.random()).toString(36) + Math.abs(Math.floor(2147483648 * Math.random()) ^ _.qb()).toString(36)
          }
          ;
          var Zc, Yc, ld, md;
          _.E = function(a) {
              return a ? new _.Wc(_.D(a)) : tb || (tb = new _.Wc)
          }
          ;
          _.Xc = function(a, b) {
              return "string" === typeof b ? a.getElementById(b) : b
          }
          ;
          Zc = function(a, b) {
              _.yc(b, function(c, d) {
                  c && "object" == typeof c && c.tb && (c = c.Xa());
                  "style" == d ? a.style.cssText = c : "class" == d ? a.className = c : "for" == d ? a.htmlFor = c : Yc.hasOwnProperty(d) ? a.setAttribute(Yc[d], c) : 0 == d.lastIndexOf("aria-", 0) || 0 == d.lastIndexOf("data-", 0) ? a.setAttribute(d, c) : a[d] = c
              })
          }
          ;
          Yc = {
              cellpadding: "cellPadding",
              cellspacing: "cellSpacing",
              colspan: "colSpan",
              frameborder: "frameBorder",
              height: "height",
              maxlength: "maxLength",
              nonce: "nonce",
              role: "role",
              rowspan: "rowSpan",
              type: "type",
              usemap: "useMap",
              valign: "vAlign",
              width: "width"
          };
          _.ad = function(a) {
              a = a.document;
              a = _.$c(a) ? a.documentElement : a.body;
              return new _.Rc(a.clientWidth,a.clientHeight)
          }
          ;
          _.bd = function(a) {
              return a ? a.parentWindow || a.defaultView : window
          }
          ;
          _.ed = function(a, b) {
              var c = b[1]
                , d = _.cd(a, String(b[0]));
              c && ("string" === typeof c ? d.className = c : Array.isArray(c) ? d.className = c.join(" ") : Zc(d, c));
              2 < b.length && _.dd(a, d, b, 2);
              return d
          }
          ;
          _.dd = function(a, b, c, d) {
              function e(k) {
                  k && b.appendChild("string" === typeof k ? a.createTextNode(k) : k)
              }
              for (; d < c.length; d++) {
                  var f = c[d];
                  if (!_.da(f) || _.ya(f) && 0 < f.nodeType)
                      e(f);
                  else {
                      a: {
                          if (f && "number" == typeof f.length) {
                              if (_.ya(f)) {
                                  var g = "function" == typeof f.item || "string" == typeof f.item;
                                  break a
                              }
                              if ("function" === typeof f) {
                                  g = "function" == typeof f.item;
                                  break a
                              }
                          }
                          g = !1
                      }
                      _.Pb(g ? _.wa(f) : f, e)
                  }
              }
          }
          ;
          _.cd = function(a, b) {
              b = String(b);
              "application/xhtml+xml" === a.contentType && (b = b.toLowerCase());
              return a.createElement(b)
          }
          ;
          _.$c = function(a) {
              return "CSS1Compat" == a.compatMode
          }
          ;
          _.fd = function(a) {
              for (var b; b = a.firstChild; )
                  a.removeChild(b)
          }
          ;
          _.gd = function(a) {
              return void 0 != a.children ? a.children : Array.prototype.filter.call(a.childNodes, function(b) {
                  return 1 == b.nodeType
              })
          }
          ;
          _.id = function(a) {
              return void 0 !== a.firstElementChild ? a.firstElementChild : _.hd(a.firstChild, !0)
          }
          ;
          _.hd = function(a, b) {
              for (; a && 1 != a.nodeType; )
                  a = b ? a.nextSibling : a.previousSibling;
              return a
          }
          ;
          _.jd = function(a, b) {
              if (!a || !b)
                  return !1;
              if (a.contains && 1 == b.nodeType)
                  return a == b || a.contains(b);
              if ("undefined" != typeof a.compareDocumentPosition)
                  return a == b || !!(a.compareDocumentPosition(b) & 16);
              for (; b && a != b; )
                  b = b.parentNode;
              return b == a
          }
          ;
          _.D = function(a) {
              return 9 == a.nodeType ? a : a.ownerDocument || a.document
          }
          ;
          _.kd = function(a, b) {
              if ("textContent"in a)
                  a.textContent = b;
              else if (3 == a.nodeType)
                  a.data = String(b);
              else if (a.firstChild && 3 == a.firstChild.nodeType) {
                  for (; a.lastChild != a.firstChild; )
                      a.removeChild(a.lastChild);
                  a.firstChild.data = String(b)
              } else
                  _.fd(a),
                  a.appendChild(_.D(a).createTextNode(String(b)))
          }
          ;
          ld = {
              SCRIPT: 1,
              STYLE: 1,
              HEAD: 1,
              IFRAME: 1,
              OBJECT: 1
          };
          md = {
              IMG: " ",
              BR: "\n"
          };
          _.od = function(a) {
              var b = [];
              _.nd(a, b, !0);
              a = b.join("");
              a = a.replace(/ \xAD /g, " ").replace(/\xAD/g, "");
              a = a.replace(/\u200B/g, "");
              a = a.replace(/ +/g, " ");
              " " != a && (a = a.replace(/^\s*/, ""));
              return a
          }
          ;
          _.nd = function(a, b, c) {
              if (!(a.nodeName in ld))
                  if (3 == a.nodeType)
                      c ? b.push(String(a.nodeValue).replace(/(\r\n|\r|\n)/g, "")) : b.push(a.nodeValue);
                  else if (a.nodeName in md)
                      b.push(md[a.nodeName]);
                  else
                      for (a = a.firstChild; a; )
                          _.nd(a, b, c),
                          a = a.nextSibling
          }
          ;
          _.Wc = function(a) {
              this.V = a || _.p.document || document
          }
          ;
          _.Wc.prototype.A = function(a) {
              return _.Xc(this.V, a)
          }
          ;
          _.Wc.prototype.M = function(a, b, c) {
              return _.ed(this.V, arguments)
          }
          ;
          _.pd = function(a) {
              a = a.V;
              return a.parentWindow || a.defaultView
          }
          ;
          _.h = _.Wc.prototype;
          _.h.appendChild = function(a, b) {
              a.appendChild(b)
          }
          ;
          _.h.rd = _.fd;
          _.h.Si = _.gd;
          _.h.Ug = _.id;
          _.h.contains = _.jd;
          _.h.qb = _.kd;
          _.h.qg = _.od;
          var qd = function() {
              this.id = "b"
          };
          qd.prototype.toString = function() {
              return this.id
          }
          ;
          _.sd = function(a, b) {
              this.type = a instanceof qd ? String(a) : a;
              this.currentTarget = this.target = b;
              this.defaultPrevented = this.h = !1
          }
          ;
          _.sd.prototype.stopPropagation = function() {
              this.h = !0
          }
          ;
          _.sd.prototype.preventDefault = function() {
              this.defaultPrevented = !0
          }
          ;
          var td = function() {
              if (!_.p.addEventListener || !Object.defineProperty)
                  return !1;
              var a = !1
                , b = Object.defineProperty({}, "passive", {
                  get: function() {
                      a = !0
                  }
              });
              try {
                  _.p.addEventListener("test", function() {}, b),
                  _.p.removeEventListener("test", function() {}, b)
              } catch (c) {}
              return a
          }();
          _.vd = function(a, b) {
              _.sd.call(this, a ? a.type : "");
              this.relatedTarget = this.currentTarget = this.target = null;
              this.button = this.screenY = this.screenX = this.clientY = this.clientX = this.offsetY = this.offsetX = 0;
              this.key = "";
              this.charCode = this.keyCode = 0;
              this.metaKey = this.shiftKey = this.altKey = this.ctrlKey = !1;
              this.state = null;
              this.j = !1;
              this.pointerId = 0;
              this.pointerType = "";
              this.g = null;
              if (a) {
                  var c = this.type = a.type
                    , d = a.changedTouches && a.changedTouches.length ? a.changedTouches[0] : null;
                  this.target = a.target || a.srcElement;
                  this.currentTarget = b;
                  (b = a.relatedTarget) ? _.B && (_.Zb(b, "nodeName") || (b = null)) : "mouseover" == c ? b = a.fromElement : "mouseout" == c && (b = a.toElement);
                  this.relatedTarget = b;
                  d ? (this.clientX = void 0 !== d.clientX ? d.clientX : d.pageX,
                  this.clientY = void 0 !== d.clientY ? d.clientY : d.pageY,
                  this.screenX = d.screenX || 0,
                  this.screenY = d.screenY || 0) : (this.offsetX = _.C || void 0 !== a.offsetX ? a.offsetX : a.layerX,
                  this.offsetY = _.C || void 0 !== a.offsetY ? a.offsetY : a.layerY,
                  this.clientX = void 0 !== a.clientX ? a.clientX : a.pageX,
                  this.clientY = void 0 !== a.clientY ? a.clientY : a.pageY,
                  this.screenX = a.screenX || 0,
                  this.screenY = a.screenY || 0);
                  this.button = a.button;
                  this.keyCode = a.keyCode || 0;
                  this.key = a.key || "";
                  this.charCode = a.charCode || ("keypress" == c ? a.keyCode : 0);
                  this.ctrlKey = a.ctrlKey;
                  this.altKey = a.altKey;
                  this.shiftKey = a.shiftKey;
                  this.metaKey = a.metaKey;
                  this.j = _.ec ? a.metaKey : a.ctrlKey;
                  this.pointerId = a.pointerId || 0;
                  this.pointerType = "string" === typeof a.pointerType ? a.pointerType : ud[a.pointerType] || "";
                  this.state = a.state;
                  this.g = a;
                  a.defaultPrevented && _.vd.G.preventDefault.call(this)
              }
          }
          ;
          _.w(_.vd, _.sd);
          var ud = {
              2: "touch",
              3: "pen",
              4: "mouse"
          };
          _.vd.prototype.stopPropagation = function() {
              _.vd.G.stopPropagation.call(this);
              this.g.stopPropagation ? this.g.stopPropagation() : this.g.cancelBubble = !0
          }
          ;
          _.vd.prototype.preventDefault = function() {
              _.vd.G.preventDefault.call(this);
              var a = this.g;
              a.preventDefault ? a.preventDefault() : a.returnValue = !1
          }
          ;
          var wd;
          wd = "closure_listenable_" + (1E6 * Math.random() | 0);
          _.xd = function(a) {
              return !(!a || !a[wd])
          }
          ;
          var yd = 0;
          var zd = function(a, b, c, d, e) {
              this.listener = a;
              this.proxy = null;
              this.src = b;
              this.type = c;
              this.capture = !!d;
              this.ge = e;
              this.key = ++yd;
              this.Oc = this.Pd = !1
          }
            , Ad = function(a) {
              a.Oc = !0;
              a.listener = null;
              a.proxy = null;
              a.src = null;
              a.ge = null
          };
          var Bd = function(a) {
              this.src = a;
              this.g = {};
              this.h = 0
          }, Dd;
          Bd.prototype.add = function(a, b, c, d, e) {
              var f = a.toString();
              a = this.g[f];
              a || (a = this.g[f] = [],
              this.h++);
              var g = Cd(a, b, d, e);
              -1 < g ? (b = a[g],
              c || (b.Pd = !1)) : (b = new zd(b,this.src,f,!!d,e),
              b.Pd = c,
              a.push(b));
              return b
          }
          ;
          Bd.prototype.remove = function(a, b, c, d) {
              a = a.toString();
              if (!(a in this.g))
                  return !1;
              var e = this.g[a];
              b = Cd(e, b, c, d);
              return -1 < b ? (Ad(e[b]),
              Array.prototype.splice.call(e, b, 1),
              0 == e.length && (delete this.g[a],
              this.h--),
              !0) : !1
          }
          ;
          Dd = function(a, b) {
              var c = b.type;
              if (!(c in a.g))
                  return !1;
              var d = _.va(a.g[c], b);
              d && (Ad(b),
              0 == a.g[c].length && (delete a.g[c],
              a.h--));
              return d
          }
          ;
          _.Ed = function(a, b) {
              b = b && b.toString();
              var c = 0, d;
              for (d in a.g)
                  if (!b || d == b) {
                      for (var e = a.g[d], f = 0; f < e.length; f++)
                          ++c,
                          Ad(e[f]);
                      delete a.g[d];
                      a.h--
                  }
              return c
          }
          ;
          Bd.prototype.md = function(a, b, c, d) {
              a = this.g[a.toString()];
              var e = -1;
              a && (e = Cd(a, b, c, d));
              return -1 < e ? a[e] : null
          }
          ;
          Bd.prototype.hasListener = function(a, b) {
              var c = void 0 !== a
                , d = c ? a.toString() : ""
                , e = void 0 !== b;
              return Ea(this.g, function(f) {
                  for (var g = 0; g < f.length; ++g)
                      if (!(c && f[g].type != d || e && f[g].capture != b))
                          return !0;
                  return !1
              })
          }
          ;
          var Cd = function(a, b, c, d) {
              for (var e = 0; e < a.length; ++e) {
                  var f = a[e];
                  if (!f.Oc && f.listener == b && f.capture == !!c && f.ge == d)
                      return e
              }
              return -1
          };
          var Fd, Gd, Hd, Kd, Md, Nd, Od, Sd, Jd;
          Fd = "closure_lm_" + (1E6 * Math.random() | 0);
          Gd = {};
          Hd = 0;
          _.F = function(a, b, c, d, e) {
              if (d && d.once)
                  return _.Id(a, b, c, d, e);
              if (Array.isArray(b)) {
                  for (var f = 0; f < b.length; f++)
                      _.F(a, b[f], c, d, e);
                  return null
              }
              c = Jd(c);
              return _.xd(a) ? a.D(b, c, _.ya(d) ? !!d.capture : !!d, e) : Kd(a, b, c, !1, d, e)
          }
          ;
          Kd = function(a, b, c, d, e, f) {
              if (!b)
                  throw Error("B");
              var g = _.ya(e) ? !!e.capture : !!e
                , k = _.Ld(a);
              k || (a[Fd] = k = new Bd(a));
              c = k.add(b, c, d, g, f);
              if (c.proxy)
                  return c;
              d = Md();
              c.proxy = d;
              d.src = a;
              d.listener = c;
              if (a.addEventListener)
                  td || (e = g),
                  void 0 === e && (e = !1),
                  a.addEventListener(b.toString(), d, e);
              else if (a.attachEvent)
                  a.attachEvent(Nd(b.toString()), d);
              else if (a.addListener && a.removeListener)
                  a.addListener(d);
              else
                  throw Error("C");
              Hd++;
              return c
          }
          ;
          Md = function() {
              var a = Od
                , b = function(c) {
                  return a.call(b.src, b.listener, c)
              };
              return b
          }
          ;
          _.Id = function(a, b, c, d, e) {
              if (Array.isArray(b)) {
                  for (var f = 0; f < b.length; f++)
                      _.Id(a, b[f], c, d, e);
                  return null
              }
              c = Jd(c);
              return _.xd(a) ? a.Wb(b, c, _.ya(d) ? !!d.capture : !!d, e) : Kd(a, b, c, !0, d, e)
          }
          ;
          _.Pd = function(a, b, c, d, e) {
              if (Array.isArray(b))
                  for (var f = 0; f < b.length; f++)
                      _.Pd(a, b[f], c, d, e);
              else
                  d = _.ya(d) ? !!d.capture : !!d,
                  c = Jd(c),
                  _.xd(a) ? a.ra(b, c, d, e) : a && (a = _.Ld(a)) && (b = a.md(b, c, d, e)) && _.Qd(b)
          }
          ;
          _.Qd = function(a) {
              if ("number" === typeof a || !a || a.Oc)
                  return !1;
              var b = a.src;
              if (_.xd(b))
                  return Dd(b.lb, a);
              var c = a.type
                , d = a.proxy;
              b.removeEventListener ? b.removeEventListener(c, d, a.capture) : b.detachEvent ? b.detachEvent(Nd(c), d) : b.addListener && b.removeListener && b.removeListener(d);
              Hd--;
              (c = _.Ld(b)) ? (Dd(c, a),
              0 == c.h && (c.src = null,
              b[Fd] = null)) : Ad(a);
              return !0
          }
          ;
          Nd = function(a) {
              return a in Gd ? Gd[a] : Gd[a] = "on" + a
          }
          ;
          _.Rd = function(a, b) {
              var c = a.listener
                , d = a.ge || a.src;
              a.Pd && _.Qd(a);
              return c.call(d, b)
          }
          ;
          Od = function(a, b) {
              return a.Oc ? !0 : _.Rd(a, new _.vd(b,this))
          }
          ;
          _.Ld = function(a) {
              a = a[Fd];
              return a instanceof Bd ? a : null
          }
          ;
          Sd = "__closure_events_fn_" + (1E9 * Math.random() >>> 0);
          Jd = function(a) {
              if ("function" === typeof a)
                  return a;
              a[Sd] || (a[Sd] = function(b) {
                  return a.handleEvent(b)
              }
              );
              return a[Sd]
          }
          ;
          _.H = function() {
              _.x.call(this);
              this.lb = new Bd(this);
              this.Fi = this;
              this.Rf = null
          }
          ;
          _.w(_.H, _.x);
          _.H.prototype[wd] = !0;
          _.h = _.H.prototype;
          _.h.Yd = function() {
              return this.Rf
          }
          ;
          _.h.te = function(a) {
              this.Rf = a
          }
          ;
          _.h.addEventListener = function(a, b, c, d) {
              _.F(this, a, b, c, d)
          }
          ;
          _.h.removeEventListener = function(a, b, c, d) {
              _.Pd(this, a, b, c, d)
          }
          ;
          _.h.dispatchEvent = function(a) {
              var b, c = this.Yd();
              if (c)
                  for (b = []; c; c = c.Yd())
                      b.push(c);
              c = this.Fi;
              var d = a.type || a;
              if ("string" === typeof a)
                  a = new _.sd(a,c);
              else if (a instanceof _.sd)
                  a.target = a.target || c;
              else {
                  var e = a;
                  a = new _.sd(d,c);
                  Ha(a, e)
              }
              e = !0;
              if (b)
                  for (var f = b.length - 1; !a.h && 0 <= f; f--) {
                      var g = a.currentTarget = b[f];
                      e = _.Td(g, d, !0, a) && e
                  }
              a.h || (g = a.currentTarget = c,
              e = _.Td(g, d, !0, a) && e,
              a.h || (e = _.Td(g, d, !1, a) && e));
              if (b)
                  for (f = 0; !a.h && f < b.length; f++)
                      g = a.currentTarget = b[f],
                      e = _.Td(g, d, !1, a) && e;
              return e
          }
          ;
          _.h.L = function() {
              _.H.G.L.call(this);
              this.lb && _.Ed(this.lb);
              this.Rf = null
          }
          ;
          _.h.D = function(a, b, c, d) {
              return this.lb.add(String(a), b, !1, c, d)
          }
          ;
          _.h.Wb = function(a, b, c, d) {
              return this.lb.add(String(a), b, !0, c, d)
          }
          ;
          _.h.ra = function(a, b, c, d) {
              return this.lb.remove(String(a), b, c, d)
          }
          ;
          _.Td = function(a, b, c, d) {
              b = a.lb.g[String(b)];
              if (!b)
                  return !0;
              b = b.concat();
              for (var e = !0, f = 0; f < b.length; ++f) {
                  var g = b[f];
                  if (g && !g.Oc && g.capture == c) {
                      var k = g.listener
                        , l = g.ge || g.src;
                      g.Pd && Dd(a.lb, g);
                      e = !1 !== k.call(l, d) && e
                  }
              }
              return e && !d.defaultPrevented
          }
          ;
          _.H.prototype.md = function(a, b, c, d) {
              return this.lb.md(String(a), b, c, d)
          }
          ;
          _.H.prototype.hasListener = function(a, b) {
              return this.lb.hasListener(void 0 !== a ? String(a) : void 0, b)
          }
          ;
          var Ud = function(a) {
              _.H.call(this);
              this.g = a || window;
              this.h = _.F(this.g, "resize", this.l, !1, this);
              this.j = _.ad(this.g || window)
          };
          _.w(Ud, _.H);
          Ud.prototype.L = function() {
              Ud.G.L.call(this);
              this.h && (_.Qd(this.h),
              this.h = null);
              this.j = this.g = null
          }
          ;
          Ud.prototype.l = function() {
              var a = _.ad(this.g || window);
              _.Sc(a, this.j) || (this.j = a,
              this.dispatchEvent("resize"))
          }
          ;
          var Vd = function(a) {
              _.H.call(this);
              this.j = a ? _.pd(a) : window;
              this.o = 1.5 <= this.j.devicePixelRatio ? 2 : 1;
              this.h = (0,
              _.v)(this.s, this);
              this.l = null;
              (this.g = this.j.matchMedia ? this.j.matchMedia("(min-resolution: 1.5dppx), (-webkit-min-device-pixel-ratio: 1.5)") : null) && "function" !== typeof this.g.addListener && "function" !== typeof this.g.addEventListener && (this.g = null)
          };
          _.w(Vd, _.H);
          Vd.prototype.start = function() {
              var a = this;
              this.g && ("function" === typeof this.g.addEventListener ? (this.g.addEventListener("change", this.h),
              this.l = function() {
                  a.g.removeEventListener("change", a.h)
              }
              ) : (this.g.addListener(this.h),
              this.l = function() {
                  a.g.removeListener(a.h)
              }
              ))
          }
          ;
          Vd.prototype.s = function() {
              var a = 1.5 <= this.j.devicePixelRatio ? 2 : 1;
              this.o != a && (this.o = a,
              this.dispatchEvent("a"))
          }
          ;
          Vd.prototype.L = function() {
              this.l && this.l();
              Vd.G.L.call(this)
          }
          ;
          var Wd = function(a, b) {
              _.x.call(this);
              this.o = a;
              if (b) {
                  if (this.l)
                      throw Error("D");
                  this.l = b;
                  this.g = _.E(b);
                  this.h = new Ud(_.bd(b));
                  this.h.te(this.o.h());
                  this.j = new Vd(this.g);
                  this.j.start()
              }
          };
          _.w(Wd, _.x);
          Wd.prototype.L = function() {
              this.g = this.l = null;
              this.h && (this.h.O(),
              this.h = null);
              _.ca(this.j);
              this.j = null
          }
          ;
          qa(Ob, Wd);
          var Xd = function(a, b) {
              this.l = a;
              this.j = b;
              this.h = 0;
              this.g = null
          };
          Xd.prototype.get = function() {
              if (0 < this.h) {
                  this.h--;
                  var a = this.g;
                  this.g = a.next;
                  a.next = null
              } else
                  a = this.l();
              return a
          }
          ;
          var Yd = function(a, b) {
              a.j(b);
              100 > a.h && (a.h++,
              b.next = a.g,
              a.g = b)
          };
          var Zd, $d = function() {
              var a = _.p.MessageChannel;
              "undefined" === typeof a && "undefined" !== typeof window && window.postMessage && window.addEventListener && !_.t("Presto") && (a = function() {
                  var e = _.cd(document, "IFRAME");
                  e.style.display = "none";
                  document.documentElement.appendChild(e);
                  var f = e.contentWindow;
                  e = f.document;
                  e.open();
                  e.close();
                  var g = "callImmediate" + Math.random()
                    , k = "file:" == f.location.protocol ? "*" : f.location.protocol + "//" + f.location.host;
                  e = (0,
                  _.v)(function(l) {
                      if (("*" == k || l.origin == k) && l.data == g)
                          this.port1.onmessage()
                  }, this);
                  f.addEventListener("message", e, !1);
                  this.port1 = {};
                  this.port2 = {
                      postMessage: function() {
                          f.postMessage(g, k)
                      }
                  }
              }
              );
              if ("undefined" !== typeof a && !_.t("Trident") && !_.t("MSIE")) {
                  var b = new a
                    , c = {}
                    , d = c;
                  b.port1.onmessage = function() {
                      if (void 0 !== c.next) {
                          c = c.next;
                          var e = c.Ig;
                          c.Ig = null;
                          e()
                      }
                  }
                  ;
                  return function(e) {
                      d.next = {
                          Ig: e
                      };
                      d = d.next;
                      b.port2.postMessage(0)
                  }
              }
              return function(e) {
                  _.p.setTimeout(e, 0)
              }
          };
          var ae = function() {
              this.h = this.g = null
          };
          ae.prototype.add = function(a, b) {
              var c = be.get();
              c.set(a, b);
              this.h ? this.h.next = c : this.g = c;
              this.h = c
          }
          ;
          ae.prototype.remove = function() {
              var a = null;
              this.g && (a = this.g,
              this.g = this.g.next,
              this.g || (this.h = null),
              a.next = null);
              return a
          }
          ;
          var be = new Xd(function() {
              return new ce
          }
          ,function(a) {
              return a.reset()
          }
          )
            , ce = function() {
              this.next = this.g = this.h = null
          };
          ce.prototype.set = function(a, b) {
              this.h = a;
              this.g = b;
              this.next = null
          }
          ;
          ce.prototype.reset = function() {
              this.next = this.g = this.h = null
          }
          ;
          var de, ee = !1, fe = new ae, he = function(a, b) {
              de || ge();
              ee || (de(),
              ee = !0);
              fe.add(a, b)
          }, ge = function() {
              if (_.p.Promise && _.p.Promise.resolve) {
                  var a = _.p.Promise.resolve(void 0);
                  de = function() {
                      a.then(ie)
                  }
              } else
                  de = function() {
                      var b = ie;
                      "function" !== typeof _.p.setImmediate || _.p.Window && _.p.Window.prototype && !_.t("Edge") && _.p.Window.prototype.setImmediate == _.p.setImmediate ? (Zd || (Zd = $d()),
                      Zd(b)) : _.p.setImmediate(b)
                  }
          }, ie = function() {
              for (var a; a = fe.remove(); ) {
                  try {
                      a.h.call(a.g)
                  } catch (b) {
                      ba(b)
                  }
                  Yd(be, a)
              }
              ee = !1
          };
          var je = function(a) {
              if (!a)
                  return !1;
              try {
                  return !!a.$goog_Thenable
              } catch (b) {
                  return !1
              }
          };
          var me, re, ve, we, ue, se;
          _.le = function(a, b) {
              this.g = 0;
              this.C = void 0;
              this.l = this.h = this.j = null;
              this.o = this.s = !1;
              if (a != _.ub)
                  try {
                      var c = this;
                      a.call(b, function(d) {
                          _.ke(c, 2, d)
                      }, function(d) {
                          _.ke(c, 3, d)
                      })
                  } catch (d) {
                      _.ke(this, 3, d)
                  }
          }
          ;
          me = function() {
              this.next = this.j = this.h = this.l = this.g = null;
              this.o = !1
          }
          ;
          me.prototype.reset = function() {
              this.j = this.h = this.l = this.g = null;
              this.o = !1
          }
          ;
          var ne = new Xd(function() {
              return new me
          }
          ,function(a) {
              a.reset()
          }
          )
            , oe = function(a, b, c) {
              var d = ne.get();
              d.l = a;
              d.h = b;
              d.j = c;
              return d
          };
          _.le.prototype.then = function(a, b, c) {
              return _.pe(this, "function" === typeof a ? a : null, "function" === typeof b ? b : null, c)
          }
          ;
          _.le.prototype.$goog_Thenable = !0;
          _.le.prototype.cancel = function(a) {
              if (0 == this.g) {
                  var b = new qe(a);
                  he(function() {
                      re(this, b)
                  }, this)
              }
          }
          ;
          re = function(a, b) {
              if (0 == a.g)
                  if (a.j) {
                      var c = a.j;
                      if (c.h) {
                          for (var d = 0, e = null, f = null, g = c.h; g && (g.o || (d++,
                          g.g == a && (e = g),
                          !(e && 1 < d))); g = g.next)
                              e || (f = g);
                          e && (0 == c.g && 1 == d ? re(c, b) : (f ? (d = f,
                          d.next == c.l && (c.l = d),
                          d.next = d.next.next) : se(c),
                          te(c, e, 3, b)))
                      }
                      a.j = null
                  } else
                      _.ke(a, 3, b)
          }
          ;
          ve = function(a, b) {
              a.h || 2 != a.g && 3 != a.g || ue(a);
              a.l ? a.l.next = b : a.h = b;
              a.l = b
          }
          ;
          _.pe = function(a, b, c, d) {
              var e = oe(null, null, null);
              e.g = new _.le(function(f, g) {
                  e.l = b ? function(k) {
                      try {
                          var l = b.call(d, k);
                          f(l)
                      } catch (m) {
                          g(m)
                      }
                  }
                  : f;
                  e.h = c ? function(k) {
                      try {
                          var l = c.call(d, k);
                          void 0 === l && k instanceof qe ? g(k) : f(l)
                      } catch (m) {
                          g(m)
                      }
                  }
                  : g
              }
              );
              e.g.j = a;
              ve(a, e);
              return e.g
          }
          ;
          _.le.prototype.J = function(a) {
              this.g = 0;
              _.ke(this, 2, a)
          }
          ;
          _.le.prototype.N = function(a) {
              this.g = 0;
              _.ke(this, 3, a)
          }
          ;
          _.ke = function(a, b, c) {
              if (0 == a.g) {
                  a === c && (b = 3,
                  c = new TypeError("E"));
                  a.g = 1;
                  a: {
                      var d = c
                        , e = a.J
                        , f = a.N;
                      if (d instanceof _.le) {
                          ve(d, oe(e || _.ub, f || null, a));
                          var g = !0
                      } else if (je(d))
                          d.then(e, f, a),
                          g = !0;
                      else {
                          if (_.ya(d))
                              try {
                                  var k = d.then;
                                  if ("function" === typeof k) {
                                      we(d, k, e, f, a);
                                      g = !0;
                                      break a
                                  }
                              } catch (l) {
                                  f.call(a, l);
                                  g = !0;
                                  break a
                              }
                          g = !1
                      }
                  }
                  g || (a.C = c,
                  a.g = b,
                  a.j = null,
                  ue(a),
                  3 != b || c instanceof qe || xe(a, c))
              }
          }
          ;
          we = function(a, b, c, d, e) {
              var f = !1
                , g = function(l) {
                  f || (f = !0,
                  c.call(e, l))
              }
                , k = function(l) {
                  f || (f = !0,
                  d.call(e, l))
              };
              try {
                  b.call(a, g, k)
              } catch (l) {
                  k(l)
              }
          }
          ;
          ue = function(a) {
              a.s || (a.s = !0,
              he(a.B, a))
          }
          ;
          se = function(a) {
              var b = null;
              a.h && (b = a.h,
              a.h = b.next,
              b.next = null);
              a.h || (a.l = null);
              return b
          }
          ;
          _.le.prototype.B = function() {
              for (var a; a = se(this); )
                  te(this, a, this.g, this.C);
              this.s = !1
          }
          ;
          var te = function(a, b, c, d) {
              if (3 == c && b.h && !b.o)
                  for (; a && a.o; a = a.j)
                      a.o = !1;
              if (b.g)
                  b.g.j = null,
                  ye(b, c, d);
              else
                  try {
                      b.o ? b.l.call(b.j) : ye(b, c, d)
                  } catch (e) {
                      ze.call(null, e)
                  }
              Yd(ne, b)
          }
            , ye = function(a, b, c) {
              2 == b ? a.l.call(a.j, c) : a.h && a.h.call(a.j, c)
          }
            , xe = function(a, b) {
              a.o = !0;
              he(function() {
                  a.o && ze.call(null, b)
              })
          }
            , ze = ba
            , qe = function(a) {
              _.aa.call(this, a)
          };
          _.w(qe, _.aa);
          qe.prototype.name = "cancel";
          /*

Copyright 2005, 2007 Bob Ippolito. All Rights Reserved.
Copyright The Closure Library Authors.
SPDX-License-Identifier: MIT
*/
          var Ae = function(a, b) {
              this.s = [];
              this.U = a;
              this.S = b || null;
              this.l = this.g = !1;
              this.j = void 0;
              this.N = this.ma = this.B = !1;
              this.C = 0;
              this.h = null;
              this.o = 0
          };
          _.w(Ae, Ja);
          Ae.prototype.cancel = function(a) {
              if (this.g)
                  this.j instanceof Ae && this.j.cancel();
              else {
                  if (this.h) {
                      var b = this.h;
                      delete this.h;
                      a ? b.cancel(a) : (b.o--,
                      0 >= b.o && b.cancel())
                  }
                  this.U ? this.U.call(this.S, this) : this.N = !0;
                  this.g || this.J(new Be(this))
              }
          }
          ;
          Ae.prototype.K = function(a, b) {
              this.B = !1;
              Ce(this, a, b)
          }
          ;
          var Ce = function(a, b, c) {
              a.g = !0;
              a.j = c;
              a.l = !b;
              De(a)
          }
            , Fe = function(a) {
              if (a.g) {
                  if (!a.N)
                      throw new Ee(a);
                  a.N = !1
              }
          };
          Ae.prototype.callback = function(a) {
              Fe(this);
              Ce(this, !0, a)
          }
          ;
          Ae.prototype.J = function(a) {
              Fe(this);
              Ce(this, !1, a)
          }
          ;
          var He = function(a, b, c) {
              Ge(a, b, null, c)
          }
            , Ie = function(a, b, c) {
              Ge(a, null, b, c)
          }
            , Ge = function(a, b, c, d) {
              a.s.push([b, c, d]);
              a.g && De(a)
          };
          Ae.prototype.then = function(a, b, c) {
              var d, e, f = new _.le(function(g, k) {
                  e = g;
                  d = k
              }
              );
              Ge(this, e, function(g) {
                  g instanceof Be ? f.cancel() : d(g);
                  return Je
              }, this);
              return f.then(a, b, c)
          }
          ;
          Ae.prototype.$goog_Thenable = !0;
          var Ke = function(a, b) {
              b instanceof Ae ? He(a, (0,
              _.v)(b.W, b)) : He(a, function() {
                  return b
              })
          };
          Ae.prototype.W = function(a) {
              var b = new Ae;
              Ge(this, b.callback, b.J, b);
              a && (b.h = this,
              this.o++);
              return b
          }
          ;
          var Le = function(a) {
              return _.Sb(a.s, function(b) {
                  return "function" === typeof b[1]
              })
          }
            , Je = {}
            , De = function(a) {
              if (a.C && a.g && Le(a)) {
                  var b = a.C
                    , c = Me[b];
                  c && (_.p.clearTimeout(c.g),
                  delete Me[b]);
                  a.C = 0
              }
              a.h && (a.h.o--,
              delete a.h);
              b = a.j;
              for (var d = c = !1; a.s.length && !a.B; ) {
                  var e = a.s.shift()
                    , f = e[0]
                    , g = e[1];
                  e = e[2];
                  if (f = a.l ? g : f)
                      try {
                          var k = f.call(e || a.S, b);
                          k === Je && (k = void 0);
                          void 0 !== k && (a.l = a.l && (k == b || k instanceof Error),
                          a.j = b = k);
                          if (je(b) || "function" === typeof _.p.Promise && b instanceof _.p.Promise)
                              d = !0,
                              a.B = !0
                      } catch (l) {
                          b = l,
                          a.l = !0,
                          Le(a) || (c = !0)
                      }
              }
              a.j = b;
              d && (k = (0,
              _.v)(a.K, a, !0),
              d = (0,
              _.v)(a.K, a, !1),
              b instanceof Ae ? (Ge(b, k, d),
              b.ma = !0) : b.then(k, d));
              c && (b = new Ne(b),
              Me[b.g] = b,
              a.C = b.g)
          }
            , Ee = function() {
              _.aa.call(this)
          };
          _.w(Ee, _.aa);
          Ee.prototype.message = "Deferred has already fired";
          Ee.prototype.name = "AlreadyCalledError";
          var Be = function() {
              _.aa.call(this)
          };
          _.w(Be, _.aa);
          Be.prototype.message = "Deferred was canceled";
          Be.prototype.name = "CanceledError";
          var Ne = function(a) {
              this.g = _.p.setTimeout((0,
              _.v)(this.j, this), 0);
              this.h = a
          };
          Ne.prototype.j = function() {
              delete Me[this.g];
              throw this.h;
          }
          ;
          var Me = {};
          var Oe = function(a, b) {
              this.type = a;
              this.status = b
          };
          Oe.prototype.toString = function() {
              return Pe(this) + " (" + (void 0 != this.status ? this.status : "?") + ")"
          }
          ;
          var Pe = function(a) {
              switch (a.type) {
              case Oe.g.Dg:
                  return "Unauthorized";
              case Oe.g.ng:
                  return "Consecutive load failures";
              case Oe.g.TIMEOUT:
                  return "Timed out";
              case Oe.g.yg:
                  return "Out of date module id";
              case Oe.g.Ce:
                  return "Init error";
              default:
                  return "Unknown failure type " + a.type
              }
          };
          hb.bb = Oe;
          hb.bb.g = {
              Dg: 0,
              ng: 1,
              TIMEOUT: 2,
              yg: 3,
              Ce: 4
          };
          var Qe = function() {
              Nb.call(this);
              this.g = {};
              this.j = [];
              this.l = [];
              this.S = [];
              this.h = [];
              this.C = [];
              this.s = {};
              this.ma = {};
              this.o = this.J = new Kb([],"");
              this.W = null;
              this.K = new Ae;
              this.U = !1;
              this.N = 0;
              this.X = this.ba = this.Z = !1
          };
          _.w(Qe, Nb);
          var Re = function(a, b) {
              _.aa.call(this, "Error loading " + a + ": " + b)
          };
          _.w(Re, _.aa);
          _.h = Qe.prototype;
          _.h.Kh = function(a) {
              this.U = a
          }
          ;
          _.h.bg = function(a, b) {
              if (!(this instanceof Qe))
                  this.bg(a, b);
              else if ("string" === typeof a) {
                  a = a.split("/");
                  for (var c = [], d = 0; d < a.length; d++) {
                      var e = a[d].split(":")
                        , f = e[0];
                      if (e[1]) {
                          e = e[1].split(",");
                          for (var g = 0; g < e.length; g++)
                              e[g] = c[parseInt(e[g], 36)]
                      } else
                          e = [];
                      c.push(f);
                      this.g[f] ? (f = this.g[f].Dc(),
                      f != e && f.splice.apply(f, [0, f.length].concat(e instanceof Array ? e : _.Ta(_.Sa(e))))) : this.g[f] = new Kb(e,f)
                  }
                  b && b.length ? (xa(this.j, b),
                  this.W = _.ra(b)) : this.K.g || this.K.callback();
                  Se(this)
              }
          }
          ;
          _.h.Ih = function(a, b) {
              if (this.s[a]) {
                  delete this.s[a][b];
                  for (var c in this.s[a])
                      return;
                  delete this.s[a]
              }
          }
          ;
          _.h.cg = function(a) {
              Qe.G.cg.call(this, a);
              Se(this)
          }
          ;
          _.h.pg = function() {
              return 0 < this.j.length
          }
          ;
          _.h.ph = function() {
              return 0 < this.C.length
          }
          ;
          var Ue = function(a) {
              var b = a.pg();
              b != a.Z && (Te(a, b ? "active" : "idle"),
              a.Z = b);
              b = a.ph();
              b != a.ba && (Te(a, b ? "userActive" : "userIdle"),
              a.ba = b)
          }
            , Xe = function(a, b, c) {
              var d = [];
              Aa(b, d);
              b = [];
              for (var e = {}, f = 0; f < d.length; f++) {
                  var g = d[f]
                    , k = a.g[g];
                  if (!k)
                      throw Error("F`" + g);
                  var l = new Ae;
                  e[g] = l;
                  k.g ? l.callback(a.La) : (Ve(a, g, k, !!c, l),
                  We(a, g) || b.push(g))
              }
              0 < b.length && (0 === a.j.length ? a.T(b) : (a.h.push(b),
              Ue(a)));
              return e
          }
            , Ve = function(a, b, c, d, e) {
              c.j.push(new Jb(e.callback,e));
              Lb(c, function(f) {
                  e.J(new Re(b,f))
              });
              We(a, b) ? d && (_.ta(a.C, b) || a.C.push(b),
              Ue(a)) : d && (_.ta(a.C, b) || a.C.push(b))
          };
          Qe.prototype.T = function(a, b, c) {
              var d = this;
              b || (this.N = 0);
              var e = Ye(this, a);
              this.j = e;
              this.l = this.U ? a : _.wa(e);
              Ue(this);
              if (0 !== e.length) {
                  this.S.push.apply(this.S, e);
                  if (0 < Object.keys(this.s).length && !this.B.S)
                      throw Error("G");
                  a = (0,
                  _.v)(this.B.N, this.B, _.wa(e), this.g, {
                      Pi: this.s,
                      Ri: !!c,
                      Of: function(f) {
                          var g = d.l;
                          f = null != f ? f : void 0;
                          d.N++;
                          d.l = g;
                          e.forEach(_.ob(_.va, d.S), d);
                          401 == f ? (Ze(d, new hb.bb(hb.bb.g.Dg,f)),
                          d.h.length = 0) : 410 == f ? ($e(d, new hb.bb(hb.bb.g.yg,f)),
                          bf(d)) : 3 <= d.N ? ($e(d, new hb.bb(hb.bb.g.ng,f)),
                          bf(d)) : d.T(d.l, !0, 8001 == f)
                      },
                      ak: (0,
                      _.v)(this.fa, this)
                  });
                  (b = 5E3 * Math.pow(this.N, 2)) ? _.p.setTimeout(a, b) : a()
              }
          }
          ;
          var Ye = function(a, b) {
              b = b.filter(function(e) {
                  return a.g[e].g ? (_.p.setTimeout(function() {
                      return Error("H`" + e)
                  }, 0),
                  !1) : !0
              });
              for (var c = [], d = 0; d < b.length; d++)
                  c = c.concat(cf(a, b[d]));
              Aa(c);
              return !a.U && 1 < c.length ? (b = c.shift(),
              a.h = c.map(function(e) {
                  return [e]
              }).concat(a.h),
              [b]) : c
          }
            , cf = function(a, b) {
              var c = Ia(a.S)
                , d = [];
              c[b] || d.push(b);
              b = [b];
              for (var e = 0; e < b.length; e++)
                  for (var f = a.g[b[e]].Dc(), g = f.length - 1; 0 <= g; g--) {
                      var k = f[g];
                      a.g[k].g || c[k] || (d.push(k),
                      b.push(k))
                  }
              d.reverse();
              Aa(d);
              return d
          }
            , Se = function(a) {
              a.o == a.J && (a.o = null,
              a.J.onLoad((0,
              _.v)(a.Vg, a)) && Ze(a, new hb.bb(hb.bb.g.Ce)),
              Ue(a))
          }
            , na = function(a) {
              if (a.o) {
                  var b = a.o.mb()
                    , c = [];
                  if (a.s[b]) {
                      for (var d = _.Sa(Object.keys(a.s[b])), e = d.next(); !e.done; e = d.next()) {
                          e = e.value;
                          var f = a.g[e];
                          f && !f.g && (a.Ih(b, e),
                          c.push(e))
                      }
                      Xe(a, c)
                  }
                  a.zb() || (a.g[b].onLoad((0,
                  _.v)(a.Vg, a)) && Ze(a, new hb.bb(hb.bb.g.Ce)),
                  _.va(a.C, b),
                  _.va(a.j, b),
                  0 === a.j.length && bf(a),
                  a.W && b == a.W && (a.K.g || a.K.callback()),
                  Ue(a),
                  a.o = null)
              }
          }
            , We = function(a, b) {
              if (_.ta(a.j, b))
                  return !0;
              for (var c = 0; c < a.h.length; c++)
                  if (_.ta(a.h[c], b))
                      return !0;
              return !1
          };
          Qe.prototype.load = function(a, b) {
              return Xe(this, [a], b)[a]
          }
          ;
          var la = function(a) {
              var b = _.fa;
              b.o && "synthetic_module_overhead" === b.o.mb() && (na(b),
              delete b.g.synthetic_module_overhead);
              b.g[a] && df(b, b.g[a].Dc() || [], function(c) {
                  c.g = new Ib;
                  _.va(b.j, c.mb())
              }, function(c) {
                  return !c.g
              });
              b.o = b.g[a]
          };
          Qe.prototype.fa = function() {
              $e(this, new hb.bb(hb.bb.g.TIMEOUT));
              bf(this)
          }
          ;
          var $e = function(a, b) {
              1 < a.l.length ? a.h = a.l.map(function(c) {
                  return [c]
              }).concat(a.h) : Ze(a, b)
          }
            , Ze = function(a, b) {
              var c = a.l;
              a.j.length = 0;
              for (var d = [], e = 0; e < a.h.length; e++) {
                  var f = a.h[e].filter(function(l) {
                      var m = cf(this, l);
                      return _.Sb(c, function(n) {
                          return _.ta(m, n)
                      })
                  }, a);
                  xa(d, f)
              }
              for (e = 0; e < c.length; e++)
                  _.ua(d, c[e]);
              for (e = 0; e < d.length; e++) {
                  for (f = 0; f < a.h.length; f++)
                      _.va(a.h[f], d[e]);
                  _.va(a.C, d[e])
              }
              var g = a.ma.error;
              if (g)
                  for (e = 0; e < g.length; e++) {
                      var k = g[e];
                      for (f = 0; f < d.length; f++)
                          k("error", d[f], b)
                  }
              for (e = 0; e < c.length; e++)
                  a.g[c[e]] && a.g[c[e]].Of(b);
              a.l.length = 0;
              Ue(a)
          }
            , bf = function(a) {
              for (; a.h.length; ) {
                  var b = a.h.shift().filter(function(c) {
                      return !this.g[c].g
                  }, a);
                  if (0 < b.length) {
                      a.T(b);
                      return
                  }
              }
              Ue(a)
          }
            , Te = function(a, b) {
              a = a.ma[b];
              for (var c = 0; a && c < a.length; c++)
                  a[c](b)
          }
            , df = function(a, b, c, d, e) {
              d = void 0 === d ? function() {
                  return !0
              }
              : d;
              e = void 0 === e ? {} : e;
              b = _.Sa(b);
              for (var f = b.next(); !f.done; f = b.next()) {
                  f = f.value;
                  var g = a.g[f];
                  !e[f] && d(g) && (e[f] = !0,
                  df(a, g.Dc() || [], c, d, e),
                  c(g))
              }
          };
          Qe.prototype.O = function() {
              ea(_.Fa(this.g), this.J);
              this.g = {};
              this.j = [];
              this.l = [];
              this.C = [];
              this.h = [];
              this.ma = {};
              this.X = !0
          }
          ;
          Qe.prototype.zb = function() {
              return this.X
          }
          ;
          _.ha = function() {
              return new Qe
          }
          ;
          var ef = function(a, b) {
              this.g = a[_.p.Symbol.iterator]();
              this.h = b
          };
          ef.prototype[Symbol.iterator] = function() {
              return this
          }
          ;
          ef.prototype.next = function() {
              var a = this.g.next();
              return {
                  value: a.done ? void 0 : this.h.call(void 0, a.value),
                  done: a.done
              }
          }
          ;
          var ff = function(a, b) {
              return new ef(a,b)
          };
          _.gf = function() {}
          ;
          _.gf.prototype.next = function() {
              return _.hf
          }
          ;
          _.hf = {
              done: !0,
              value: void 0
          };
          _.gf.prototype.Ja = function() {
              return this
          }
          ;
          var mf = function(a) {
              if (a instanceof jf || a instanceof kf || a instanceof lf)
                  return a;
              if ("function" == typeof a.next)
                  return new jf(function() {
                      return a
                  }
                  );
              if ("function" == typeof a[Symbol.iterator])
                  return new jf(function() {
                      return a[Symbol.iterator]()
                  }
                  );
              if ("function" == typeof a.Ja)
                  return new jf(function() {
                      return a.Ja()
                  }
                  );
              throw Error("J");
          }
            , jf = function(a) {
              this.g = a
          };
          jf.prototype.Ja = function() {
              return new kf(this.g())
          }
          ;
          jf.prototype[Symbol.iterator] = function() {
              return new lf(this.g())
          }
          ;
          jf.prototype.h = function() {
              return new lf(this.g())
          }
          ;
          var kf = function(a) {
              this.g = a
          };
          _.u(kf, _.gf);
          kf.prototype.next = function() {
              return this.g.next()
          }
          ;
          kf.prototype[Symbol.iterator] = function() {
              return new lf(this.g)
          }
          ;
          kf.prototype.h = function() {
              return new lf(this.g)
          }
          ;
          var lf = function(a) {
              jf.call(this, function() {
                  return a
              });
              this.j = a
          };
          _.u(lf, jf);
          lf.prototype.next = function() {
              return this.j.next()
          }
          ;
          _.nf = function(a, b) {
              this.h = {};
              this.g = [];
              this.j = this.size = 0;
              var c = arguments.length;
              if (1 < c) {
                  if (c % 2)
                      throw Error("y");
                  for (var d = 0; d < c; d += 2)
                      this.set(arguments[d], arguments[d + 1])
              } else if (a)
                  if (a instanceof _.nf)
                      for (c = a.Vb(),
                      d = 0; d < c.length; d++)
                          this.set(c[d], a.get(c[d]));
                  else
                      for (d in a)
                          this.set(d, a[d])
          }
          ;
          _.nf.prototype.nb = function() {
              of(this);
              for (var a = [], b = 0; b < this.g.length; b++)
                  a.push(this.h[this.g[b]]);
              return a
          }
          ;
          _.nf.prototype.Vb = function() {
              of(this);
              return this.g.concat()
          }
          ;
          _.pf = function(a, b) {
              return a.has(b)
          }
          ;
          _.nf.prototype.has = function(a) {
              return qf(this.h, a)
          }
          ;
          _.nf.prototype.pc = function() {
              return 0 == this.size
          }
          ;
          _.nf.prototype.remove = function(a) {
              qf(this.h, a) ? (delete this.h[a],
              --this.size,
              this.j++,
              this.g.length > 2 * this.size && of(this),
              a = !0) : a = !1;
              return a
          }
          ;
          var of = function(a) {
              if (a.size != a.g.length) {
                  for (var b = 0, c = 0; b < a.g.length; ) {
                      var d = a.g[b];
                      qf(a.h, d) && (a.g[c++] = d);
                      b++
                  }
                  a.g.length = c
              }
              if (a.size != a.g.length) {
                  var e = {};
                  for (c = b = 0; b < a.g.length; )
                      d = a.g[b],
                      qf(e, d) || (a.g[c++] = d,
                      e[d] = 1),
                      b++;
                  a.g.length = c
              }
          };
          _.h = _.nf.prototype;
          _.h.get = function(a, b) {
              return qf(this.h, a) ? this.h[a] : b
          }
          ;
          _.h.set = function(a, b) {
              qf(this.h, a) || (this.size += 1,
              this.g.push(a),
              this.j++);
              this.h[a] = b
          }
          ;
          _.h.forEach = function(a, b) {
              for (var c = this.Vb(), d = 0; d < c.length; d++) {
                  var e = c[d]
                    , f = this.get(e);
                  a.call(b, f, e, this)
              }
          }
          ;
          _.h.keys = function() {
              return mf(this.Ja(!0)).h()
          }
          ;
          _.h.values = function() {
              return mf(this.Ja(!1)).h()
          }
          ;
          _.h.entries = function() {
              var a = this;
              return ff(this.keys(), function(b) {
                  return [b, a.get(b)]
              })
          }
          ;
          _.h.Ja = function(a) {
              of(this);
              var b = 0
                , c = this.j
                , d = this
                , e = new _.gf;
              e.next = function() {
                  if (c != d.j)
                      throw Error("K");
                  if (b >= d.g.length)
                      return _.hf;
                  var f = d.g[b++];
                  return {
                      value: a ? f : d.h[f],
                      done: !1
                  }
              }
              ;
              return e
          }
          ;
          var qf = function(a, b) {
              return Object.prototype.hasOwnProperty.call(a, b)
          };
          var sf;
          _.rf = function() {
              this.g = new _.nf;
              this.size = 0
          }
          ;
          sf = function(a) {
              var b = typeof a;
              return "object" == b && a || "function" == b ? "o" + _.za(a) : b.charAt(0) + a
          }
          ;
          _.h = _.rf.prototype;
          _.h.add = function(a) {
              this.g.set(sf(a), a);
              this.size = this.g.size
          }
          ;
          _.h.remove = function(a) {
              a = this.g.remove(sf(a));
              this.size = this.g.size;
              return a
          }
          ;
          _.h.pc = function() {
              return 0 === this.g.size
          }
          ;
          _.h.has = function(a) {
              return _.pf(this.g, sf(a))
          }
          ;
          _.h.contains = function(a) {
              return _.pf(this.g, sf(a))
          }
          ;
          _.h.nb = function() {
              return this.g.nb()
          }
          ;
          _.h.values = function() {
              return this.g.values()
          }
          ;
          _.h.Ja = function() {
              return this.g.Ja(!1)
          }
          ;
          _.rf.prototype[Symbol.iterator] = function() {
              return this.values()
          }
          ;
          var tf = []
            , uf = function(a) {
              function b(d) {
                  d && Rb(d, function(e, f) {
                      e[f.id] = !0;
                      return e
                  }, c.kk)
              }
              var c = {
                  kk: {},
                  index: tf.length,
                  Ul: a
              };
              b(a.g);
              b(a.j);
              tf.push(c);
              a.g && _.Pb(a.g, function(d) {
                  var e = d.id;
                  e instanceof y && d.module && (e.Uj = d.module)
              })
          };
          var vf = new y("MpJwZc","MpJwZc");
          var wf = new y("UUJqVe","UUJqVe");
          new y("Wt6vjf","Wt6vjf");
          new y("byfTOb","byfTOb");
          new y("LEikZe","LEikZe");
          new y("lsjVmc","lsjVmc");
          new y("pVbxBc");
          new y("tdUkaf");
          new y("fJuxOc");
          new y("ZtVrH");
          new y("WSziFf");
          new y("ZmXAm");
          new y("BWETze");
          new y("UBSgGf");
          new y("zZa4xc");
          new y("o1bZcd");
          new y("WwG67d");
          new y("z72MOc");
          new y("JccZRe");
          new y("amY3Td");
          new y("ABma3e");
          new y("GHAeAc","GHAeAc");
          new y("gSshPb");
          new y("klpyYe");
          new y("OPbIxb");
          new y("pg9hFd");
          new y("yu4DA");
          new y("vk3Wc");
          new y("IykvEf");
          new y("J5K1Ad");
          new y("IW8Usd");
          new y("IaqD3e");
          new y("jbDgG");
          new y("b8xKu");
          new y("d0RAGb");
          new y("AzG0ke");
          new y("J4QWB");
          new y("TuDsZ");
          new y("hdXIif");
          new y("mITR5c");
          new y("DFElXb");
          new y("NGntwf");
          new y("Bgf0ib");
          new y("Xpw1of");
          new y("v5BQle");
          new y("ofuapc");
          new y("FENZqe");
          new y("tLnxq");
          uf({
              g: [{
                  id: Ob,
                  Hi: Wd,
                  multiple: !0
              }]
          });
          var xf = {};
          var yf = new qd
            , zf = function(a, b) {
              _.sd.call(this, a, b);
              this.node = b
          };
          _.u(zf, _.sd);
          /*

SPDX-License-Identifier: Apache-2.0
*/
          var Df;
          _.Af = RegExp("^(?:([^:/?#.]+):)?(?://(?:([^\\\\/?#]*)@)?([^\\\\/?#]*?)(?::([0-9]+))?(?=[\\\\/?#]|$))?([^?#]+)?(?:\\?([^#]*))?(?:#([\\s\\S]*))?$");
          _.Bf = function(a) {
              return a ? decodeURI(a) : a
          }
          ;
          _.Cf = function(a, b) {
              if (a) {
                  a = a.split("&");
                  for (var c = 0; c < a.length; c++) {
                      var d = a[c].indexOf("=")
                        , e = null;
                      if (0 <= d) {
                          var f = a[c].substring(0, d);
                          e = a[c].substring(d + 1)
                      } else
                          f = a[c];
                      b(f, e ? _.Uc(e) : "")
                  }
              }
          }
          ;
          Df = function(a, b, c) {
              if (Array.isArray(b))
                  for (var d = 0; d < b.length; d++)
                      Df(a, String(b[d]), c);
              else
                  null != b && c.push(a + ("" === b ? "" : "=" + _.Tc(b)))
          }
          ;
          /*
Copyright The Closure Library Authors.
SPDX-License-Identifier: Apache-2.0
*/
          _.Ef = function(a, b) {
              b || _.E();
              this.j = a || null
          }
          ;
          _.Ef.prototype.ja = function(a) {
              a = a({}, this.j ? this.j.g() : {});
              this.h(null, "function" == typeof _.Ff && a instanceof _.Ff ? a.Hb : null)
          }
          ;
          _.Ef.prototype.h = function() {}
          ;
          var Gf = function(a) {
              this.h = a;
              this.j = this.h.g(wf)
          };
          Gf.prototype.g = function() {
              this.h.zb() || (this.j = this.h.g(wf));
              return this.j ? this.j.g() : {}
          }
          ;
          var Hf = function(a) {
              var b = new Gf(a);
              _.Ef.call(this, b, a.get(Ob).g);
              this.l = new _.H;
              this.o = b
          };
          _.u(Hf, _.Ef);
          Hf.prototype.g = function() {
              return this.o.g()
          }
          ;
          Hf.prototype.h = function(a, b) {
              _.Ef.prototype.h.call(this, a, b);
              this.l.dispatchEvent(new zf(yf,a,b))
          }
          ;
          qa(vf, Hf);
          uf({
              g: [{
                  id: vf,
                  Hi: Hf,
                  multiple: !0
              }]
          });
          var If = function(a, b) {
              this.defaultValue = a;
              this.type = b;
              this.value = a
          };
          If.prototype.get = function() {
              return this.value
          }
          ;
          If.prototype.set = function(a) {
              this.value = a
          }
          ;
          var Jf = function(a) {
              If.call(this, a, "b")
          };
          _.u(Jf, If);
          Jf.prototype.get = function() {
              return this.value
          }
          ;
          var Kf = function() {
              this.g = {};
              this.j = "";
              this.h = {}
          };
          Kf.prototype.toString = function() {
              var a = this.j + Lf(this);
              var b = this.h;
              var c = [], d;
              for (d in b)
                  Df(d, b[d], c);
              b = c.join("&");
              c = "";
              "" != b && (c = "?" + b);
              return a + c
          }
          ;
          var Lf = function(a) {
              var b = []
                , c = (0,
              _.v)(function(d) {
                  void 0 !== this.g[d] && b.push(d + "=" + this.g[d])
              }, a);
              "1" == Mf(a, "md") ? (c("md"),
              c("k"),
              c("ck"),
              c("am"),
              c("rs"),
              c("gssmodulesetproto")) : (c("sdch"),
              c("k"),
              c("ck"),
              c("am"),
              c("rt"),
              "d"in a.g || Nf(a, "d", "0"),
              c("d"),
              c("exm"),
              c("excm"),
              c("esmo"),
              (a.g.excm || a.g.exm) && b.push("ed=1"),
              c("im"),
              c("dg"),
              c("sm"),
              "1" == Mf(a, "br") && c("br"),
              "" !== Of(a) && c("wt"),
              c("gssmodulesetproto"),
              c("rs"),
              c("ee"),
              c("cb"),
              c("m"));
              return b.join("/")
          }
            , Mf = function(a, b) {
              return a.g[b] ? a.g[b] : null
          }
            , Nf = function(a, b, c) {
              c ? a.g[b] = c : delete a.g[b]
          }
            , Of = function(a) {
              switch (Mf(a, "wt")) {
              case "0":
                  return "0";
              case "1":
                  return "1";
              case "2":
                  return "2";
              default:
                  return ""
              }
          }
            , Rf = function(a) {
              var b = void 0 === b ? !0 : b;
              var c = Pf(a)
                , d = new Kf
                , e = c.match(_.Af)[5];
              _.yc(Qf, function(g) {
                  var k = e.match("/" + g + "=([^/]+)");
                  k && Nf(d, g, k[1])
              });
              var f = -1 != a.indexOf("_/ss/") ? "_/ss/" : "_/js/";
              d.j = a.substr(0, a.indexOf(f) + f.length);
              if (!b)
                  return d;
              (a = c.match(_.Af)[6] || null) && _.Cf(a, function(g, k) {
                  d.h[g] = k
              });
              return d
          }
            , Pf = function(a) {
              return a.startsWith("https://uberproxy-pen-redirect.corp.google.com/uberproxy/pen?url=") ? a.substr(65) : a
          }
            , Qf = {
              vl: "k",
              Pk: "ck",
              il: "m",
              Yk: "exm",
              Wk: "excm",
              Zk: "esmo",
              Gk: "am",
              ul: "rt",
              dl: "d",
              Xk: "ed",
              Dl: "sv",
              Qk: "deob",
              Nk: "cb",
              Al: "rs",
              wl: "sdch",
              el: "im",
              Rk: "dg",
              Vk: "br",
              Ml: "wt",
              al: "ee",
              Cl: "sm",
              gl: "md",
              bl: "gssmodulesetproto"
          };
          _.I = function(a) {
              _.x.call(this);
              this.h = a;
              this.g = {}
          }
          ;
          _.w(_.I, _.x);
          var Sf = [];
          _.I.prototype.D = function(a, b, c, d) {
              return Tf(this, a, b, c, d)
          }
          ;
          var Tf = function(a, b, c, d, e, f) {
              Array.isArray(c) || (c && (Sf[0] = c.toString()),
              c = Sf);
              for (var g = 0; g < c.length; g++) {
                  var k = _.F(b, c[g], d || a.handleEvent, e || !1, f || a.h || a);
                  if (!k)
                      break;
                  a.g[k.key] = k
              }
              return a
          };
          _.I.prototype.Wb = function(a, b, c, d) {
              return Uf(this, a, b, c, d)
          }
          ;
          var Uf = function(a, b, c, d, e, f) {
              if (Array.isArray(c))
                  for (var g = 0; g < c.length; g++)
                      Uf(a, b, c[g], d, e, f);
              else {
                  b = _.Id(b, c, d || a.handleEvent, e, f || a.h || a);
                  if (!b)
                      return a;
                  a.g[b.key] = b
              }
              return a
          };
          _.I.prototype.ra = function(a, b, c, d, e) {
              if (Array.isArray(b))
                  for (var f = 0; f < b.length; f++)
                      this.ra(a, b[f], c, d, e);
              else
                  c = c || this.handleEvent,
                  d = _.ya(d) ? !!d.capture : !!d,
                  e = e || this.h || this,
                  c = Jd(c),
                  d = !!d,
                  b = _.xd(a) ? a.md(b, c, d, e) : a ? (a = _.Ld(a)) ? a.md(b, c, d, e) : null : null,
                  b && (_.Qd(b),
                  delete this.g[b.key]);
              return this
          }
          ;
          _.Vf = function(a) {
              _.yc(a.g, function(b, c) {
                  this.g.hasOwnProperty(c) && _.Qd(b)
              }, a);
              a.g = {}
          }
          ;
          _.I.prototype.L = function() {
              _.I.G.L.call(this);
              _.Vf(this)
          }
          ;
          _.I.prototype.handleEvent = function() {
              throw Error("T");
          }
          ;
          var Wf = function() {};
          Wf.prototype.h = null;
          var Xf = function(a) {
              return a.h || (a.h = a.l())
          };
          var Yf, Zf = function() {};
          _.w(Zf, Wf);
          Zf.prototype.g = function() {
              var a = $f(this);
              return a ? new ActiveXObject(a) : new XMLHttpRequest
          }
          ;
          Zf.prototype.l = function() {
              var a = {};
              $f(this) && (a[0] = !0,
              a[1] = !0);
              return a
          }
          ;
          var $f = function(a) {
              if (!a.j && "undefined" == typeof XMLHttpRequest && "undefined" != typeof ActiveXObject) {
                  for (var b = ["MSXML2.XMLHTTP.6.0", "MSXML2.XMLHTTP.3.0", "MSXML2.XMLHTTP", "Microsoft.XMLHTTP"], c = 0; c < b.length; c++) {
                      var d = b[c];
                      try {
                          return new ActiveXObject(d),
                          a.j = d
                      } catch (e) {}
                  }
                  throw Error("U");
              }
              return a.j
          };
          Yf = new Zf;
          var ag = function() {};
          _.w(ag, Wf);
          ag.prototype.g = function() {
              var a = new XMLHttpRequest;
              if ("withCredentials"in a)
                  return a;
              if ("undefined" != typeof XDomainRequest)
                  return new bg;
              throw Error("V");
          }
          ;
          ag.prototype.l = function() {
              return {}
          }
          ;
          var bg = function() {
              this.g = new XDomainRequest;
              this.readyState = 0;
              this.onreadystatechange = null;
              this.responseType = this.responseText = "";
              this.status = -1;
              this.statusText = "";
              this.g.onload = (0,
              _.v)(this.bi, this);
              this.g.onerror = (0,
              _.v)(this.rg, this);
              this.g.onprogress = (0,
              _.v)(this.pj, this);
              this.g.ontimeout = (0,
              _.v)(this.uj, this)
          };
          _.h = bg.prototype;
          _.h.open = function(a, b, c) {
              if (null != c && !c)
                  throw Error("W");
              this.g.open(a, b)
          }
          ;
          _.h.send = function(a) {
              if (a)
                  if ("string" == typeof a)
                      this.g.send(a);
                  else
                      throw Error("X");
              else
                  this.g.send()
          }
          ;
          _.h.abort = function() {
              this.g.abort()
          }
          ;
          _.h.setRequestHeader = function() {}
          ;
          _.h.getResponseHeader = function(a) {
              return "content-type" == a.toLowerCase() ? this.g.contentType : ""
          }
          ;
          _.h.bi = function() {
              this.status = 200;
              this.responseText = this.g.responseText;
              cg(this, 4)
          }
          ;
          _.h.rg = function() {
              this.status = 500;
              this.responseText = "";
              cg(this, 4)
          }
          ;
          _.h.uj = function() {
              this.rg()
          }
          ;
          _.h.pj = function() {
              this.status = 200;
              cg(this, 1)
          }
          ;
          var cg = function(a, b) {
              a.readyState = b;
              if (a.onreadystatechange)
                  a.onreadystatechange()
          };
          bg.prototype.getAllResponseHeaders = function() {
              return "content-type: " + this.g.contentType
          }
          ;
          _.dg = function(a, b, c) {
              if ("function" === typeof a)
                  c && (a = (0,
                  _.v)(a, c));
              else if (a && "function" == typeof a.handleEvent)
                  a = (0,
                  _.v)(a.handleEvent, a);
              else
                  throw Error("Y");
              return 2147483647 < Number(b) ? -1 : _.p.setTimeout(a, b || 0)
          }
          ;
          _.eg = function(a) {
              _.p.clearTimeout(a)
          }
          ;
          var gg, hg, og, ng, kg;
          _.fg = function(a) {
              _.H.call(this);
              this.headers = new Map;
              this.N = a || null;
              this.h = !1;
              this.J = this.g = null;
              this.o = "";
              this.l = 0;
              this.j = this.S = this.C = this.K = !1;
              this.s = 0;
              this.B = null;
              this.X = "";
              this.U = this.W = !1
          }
          ;
          _.w(_.fg, _.H);
          gg = /^https?$/i;
          hg = ["POST", "PUT"];
          _.ig = [];
          _.fg.prototype.ba = function() {
              this.O();
              _.va(_.ig, this)
          }
          ;
          _.fg.prototype.send = function(a, b, c, d) {
              if (this.g)
                  throw Error("Z`" + this.o + "`" + a);
              b = b ? b.toUpperCase() : "GET";
              this.o = a;
              this.l = 0;
              this.K = !1;
              this.h = !0;
              this.g = this.N ? this.N.g() : Yf.g();
              this.J = this.N ? Xf(this.N) : Xf(Yf);
              this.g.onreadystatechange = (0,
              _.v)(this.T, this);
              try {
                  this.S = !0,
                  this.g.open(b, String(a), !0),
                  this.S = !1
              } catch (g) {
                  jg(this);
                  return
              }
              a = c || "";
              c = new Map(this.headers);
              if (d)
                  if (Object.getPrototypeOf(d) === Object.prototype)
                      for (var e in d)
                          c.set(e, d[e]);
                  else if ("function" === typeof d.keys && "function" === typeof d.get) {
                      e = _.Sa(d.keys());
                      for (var f = e.next(); !f.done; f = e.next())
                          f = f.value,
                          c.set(f, d.get(f))
                  } else
                      throw Error("$`" + String(d));
              d = Array.from(c.keys()).find(function(g) {
                  return "content-type" == g.toLowerCase()
              });
              e = _.p.FormData && a instanceof _.p.FormData;
              !_.ta(hg, b) || d || e || c.set("Content-Type", "application/x-www-form-urlencoded;charset=utf-8");
              b = _.Sa(c);
              for (d = b.next(); !d.done; d = b.next())
                  c = _.Sa(d.value),
                  d = c.next().value,
                  c = c.next().value,
                  this.g.setRequestHeader(d, c);
              this.X && (this.g.responseType = this.X);
              "withCredentials"in this.g && this.g.withCredentials !== this.W && (this.g.withCredentials = this.W);
              try {
                  kg(this),
                  0 < this.s && ((this.U = lg(this.g)) ? (this.g.timeout = this.s,
                  this.g.ontimeout = (0,
                  _.v)(this.Z, this)) : this.B = _.dg(this.Z, this.s, this)),
                  this.C = !0,
                  this.g.send(a),
                  this.C = !1
              } catch (g) {
                  jg(this)
              }
          }
          ;
          var lg = function(a) {
              return _.z && _.tc(9) && "number" === typeof a.timeout && void 0 !== a.ontimeout
          };
          _.fg.prototype.Z = function() {
              "undefined" != typeof eb && this.g && (this.l = 8,
              this.dispatchEvent("timeout"),
              this.abort(8))
          }
          ;
          var jg = function(a) {
              a.h = !1;
              a.g && (a.j = !0,
              a.g.abort(),
              a.j = !1);
              a.l = 5;
              mg(a);
              ng(a)
          }
            , mg = function(a) {
              a.K || (a.K = !0,
              a.dispatchEvent("complete"),
              a.dispatchEvent("error"))
          };
          _.fg.prototype.abort = function(a) {
              this.g && this.h && (this.h = !1,
              this.j = !0,
              this.g.abort(),
              this.j = !1,
              this.l = a || 7,
              this.dispatchEvent("complete"),
              this.dispatchEvent("abort"),
              ng(this))
          }
          ;
          _.fg.prototype.L = function() {
              this.g && (this.h && (this.h = !1,
              this.j = !0,
              this.g.abort(),
              this.j = !1),
              ng(this, !0));
              _.fg.G.L.call(this)
          }
          ;
          _.fg.prototype.T = function() {
              this.zb() || (this.S || this.C || this.j ? og(this) : this.fa())
          }
          ;
          _.fg.prototype.fa = function() {
              og(this)
          }
          ;
          og = function(a) {
              if (a.h && "undefined" != typeof eb && (!a.J[1] || 4 != (a.g ? a.g.readyState : 0) || 2 != _.pg(a)))
                  if (a.C && 4 == (a.g ? a.g.readyState : 0))
                      _.dg(a.T, 0, a);
                  else if (a.dispatchEvent("readystatechange"),
                  4 == (a.g ? a.g.readyState : 0)) {
                      a.h = !1;
                      try {
                          _.qg(a) ? (a.dispatchEvent("complete"),
                          a.dispatchEvent("success")) : (a.l = 6,
                          mg(a))
                      } finally {
                          ng(a)
                      }
                  }
          }
          ;
          ng = function(a, b) {
              if (a.g) {
                  kg(a);
                  var c = a.g
                    , d = a.J[0] ? function() {}
                  : null;
                  a.g = null;
                  a.J = null;
                  b || a.dispatchEvent("ready");
                  try {
                      c.onreadystatechange = d
                  } catch (e) {}
              }
          }
          ;
          kg = function(a) {
              a.g && a.U && (a.g.ontimeout = null);
              a.B && (_.eg(a.B),
              a.B = null)
          }
          ;
          _.qg = function(a) {
              var b = _.pg(a);
              a: switch (b) {
              case 200:
              case 201:
              case 202:
              case 204:
              case 206:
              case 304:
              case 1223:
                  var c = !0;
                  break a;
              default:
                  c = !1
              }
              if (!c) {
                  if (b = 0 === b)
                      a = String(a.o).match(_.Af)[1] || null,
                      !a && _.p.self && _.p.self.location && (a = _.p.self.location.protocol.slice(0, -1)),
                      b = !gg.test(a ? a.toLowerCase() : "");
                  c = b
              }
              return c
          }
          ;
          _.pg = function(a) {
              try {
                  return 2 < (a.g ? a.g.readyState : 0) ? a.g.status : -1
              } catch (b) {
                  return -1
              }
          }
          ;
          _.rg = function(a) {
              try {
                  return a.g ? a.g.responseText : ""
              } catch (b) {
                  return ""
              }
          }
          ;
          var tg = function(a) {
              _.x.call(this);
              this.J = a;
              this.s = Rf(a);
              this.j = this.l = null;
              this.S = !0;
              this.H = new _.I(this);
              this.K = [];
              this.o = new Set;
              this.g = [];
              this.U = new sg;
              this.h = [];
              this.C = !1;
              a = (0,
              _.v)(this.B, this);
              xf.version = a
          };
          _.u(tg, _.x);
          var ug = function(a, b) {
              a.g.length && Ke(b, a.g[a.g.length - 1]);
              a.g.push(b);
              He(b, function() {
                  _.va(this.g, b)
              }, a)
          };
          tg.prototype.N = function(a, b, c) {
              var d = void 0 === c ? {} : c;
              c = d.Ri;
              var e = d.Of
                , f = d.ak;
              a = vg(this, a, b, d.Pi, c);
              wg(this, a, e, f, c)
          }
          ;
          var vg = function(a, b, c, d, e) {
              d = void 0 === d ? {} : d;
              var f = [];
              xg(a, b, c, d, void 0 === e ? !1 : e, function(g) {
                  f.push(g.mb())
              });
              return f
          }
            , xg = function(a, b, c, d, e, f, g) {
              g = void 0 === g ? {} : g;
              b = _.Sa(b);
              for (var k = b.next(); !k.done; k = b.next()) {
                  var l = k.value;
                  k = c[l];
                  !e && (a.o.has(l) || k.g) || g[l] || (g[l] = !0,
                  l = d[l] ? Object.keys(d[l]) : [],
                  xg(a, k.Dc().concat(l), c, d, e, f, g),
                  f(k))
              }
          }
            , wg = function(a, b, c, d, e) {
              e = void 0 === e ? !1 : e;
              var f = []
                , g = new Ae;
              b = [b];
              for (var k = function(q, r) {
                  for (var A = [], G = 0, Q = Math.floor(q.length / r) + 1, rd = 0; rd < r; rd++) {
                      var kc = (rd + 1) * Q;
                      A.push(q.slice(G, kc));
                      G = kc
                  }
                  return A
              }, l = b.shift(); l; ) {
                  var m = yg(a, l, !!e, !0);
                  if (2E3 >= m.length) {
                      if (l = zg(a, l, e))
                          f.push(l),
                          Ke(g, l.g)
                  } else
                      b = k(l, Math.ceil(m.length / 2E3)).concat(b);
                  l = b.shift()
              }
              var n = new Ae;
              ug(a, n);
              He(n, function() {
                  return Ag(a, f, c, d)
              });
              Ie(n, function() {
                  var q = new Bg;
                  q.j = !0;
                  q.h = -1;
                  Ag(this, [q], c, d)
              }, a);
              He(g, function() {
                  return n.callback()
              });
              g.callback()
          }
            , zg = function(a, b, c) {
              var d = yg(a, b, !(void 0 === c || !c));
              a.K.push(d);
              b = _.Sa(b);
              for (c = b.next(); !c.done; c = b.next())
                  a.o.add(c.value);
              if (a.C)
                  a = _.cd(document, "SCRIPT"),
                  _.La(a, _.Hb(d)),
                  a.type = "text/javascript",
                  a.async = !1,
                  document.body.appendChild(a);
              else {
                  var e = new Bg
                    , f = new _.fg(0 < a.h.length ? new ag : void 0);
                  a.H.D(f, "success", (0,
                  _.v)(e.C, e, f));
                  a.H.D(f, "error", (0,
                  _.v)(e.s, e, f));
                  a.H.D(f, "timeout", (0,
                  _.v)(e.B, e));
                  Tf(a.H, f, "ready", f.O, !1, f);
                  f.s = 3E4;
                  Cg(a.U, function() {
                      f.send(d);
                      return e.g
                  });
                  return e
              }
              return null
          }
            , Ag = function(a, b, c, d) {
              for (var e = !1, f, g = !1, k = 0; k < b.length; k++) {
                  var l = b[k];
                  if (!f && l.j) {
                      e = !0;
                      f = l.h;
                      break
                  } else
                      l.l && (g = !0)
              }
              var m = _.wa(a.g);
              (e || g) && -1 != f && (a.g.length = 0);
              if (e)
                  c && c(f);
              else if (g)
                  d && d();
              else
                  for (k = 0; k < b.length; k++)
                      l = b[k],
                      Dg(l.o, l.ac) || c && c(8001);
              (e || g) && -1 != f && _.Pb(m, function(n) {
                  n.cancel()
              })
          };
          tg.prototype.L = function() {
              this.H.O();
              delete xf.version;
              _.x.prototype.L.call(this)
          }
          ;
          tg.prototype.B = function() {
              return Mf(this.s, "k")
          }
          ;
          var yg = function(a, b, c, d) {
              d = void 0 === d ? !1 : d;
              var e = _.Bf(a.J.match(_.Af)[3] || null);
              if (0 < a.h.length && !_.ta(a.h, e) && null != e && window.location.hostname != e)
                  throw Error("ca`" + e);
              e = Rf(a.s.toString());
              delete e.g.m;
              delete e.g.exm;
              delete e.g.ed;
              Nf(e, "m", b.join(","));
              a.l && (Nf(e, "ck", a.l),
              a.j && Nf(e, "rs", a.j));
              Nf(e, "d", "0");
              c && (a = _.Vc(),
              e.h.zx = a);
              a = e.toString();
              if (d && 0 == a.lastIndexOf("/", 0)) {
                  e = document.location.href.match(_.Af);
                  d = e[1];
                  b = e[2];
                  c = e[3];
                  e = e[4];
                  var f = "";
                  d && (f += d + ":");
                  c && (f += "//",
                  b && (f += b + "@"),
                  f += c,
                  e && (f += ":" + e));
                  a = f + a
              }
              return a
          }
            , Dg = function(a, b) {
              var c = "";
              if (1 < a.length && "\n" === a.charAt(a.length - 1)) {
                  var d = a.lastIndexOf("\n", a.length - 2);
                  0 <= d && (c = a.substring(d + 1, a.length - 1))
              }
              if (_.Ub(c, "Google Inc.") || 0 == c.lastIndexOf("//# sourceMappingURL=", 0))
                  try {
                      c = window;
                      var e = _.Cb(a + "\r\n//# sourceURL=" + b)
                        , f = _.Bb(e);
                      c.eval(f) === f && c.eval(f.toString())
                  } catch (g) {
                      return !1
                  }
              else
                  return !1;
              return !0
          }
            , Eg = function(a) {
              var b = _.Bf(a.match(_.Af)[5] || null) || "";
              b = _.Bf(Pf(b).match(_.Af)[5] || null);
              return null !== b && b.match("(/_/js/)|(/_/ss/)") && b.match("/k=") ? a : null
          }
            , Bg = function() {
              this.g = new Ae;
              this.ac = this.o = "";
              this.j = !1;
              this.h = 0;
              this.l = !1
          };
          Bg.prototype.C = function(a) {
              this.o = _.rg(a);
              this.ac = String(a.o);
              this.g.callback()
          }
          ;
          Bg.prototype.s = function(a) {
              this.j = !0;
              this.h = _.pg(a);
              this.g.callback()
          }
          ;
          Bg.prototype.B = function() {
              this.l = !0;
              this.g.callback()
          }
          ;
          var sg = function() {
              this.g = 0;
              this.h = []
          }
            , Cg = function(a, b) {
              a.h.push(b);
              Fg(a)
          }
            , Fg = function(a) {
              for (; 5 > a.g && a.h.length; )
                  Gg(a, a.h.shift())
          }
            , Gg = function(a, b) {
              a.g++;
              He(b(), function() {
                  this.g--;
                  Fg(this)
              }, a)
          };
          var Hg = new Jf(!1)
            , Ig = document.location.href;
          uf({
              h: {
                  dml: Hg
              },
              initialize: function(a) {
                  var b = Hg.get()
                    , c = ""
                    , d = "";
                  window && window._F_cssRowKey && (c = window._F_cssRowKey,
                  window._F_combinedSignature && (d = window._F_combinedSignature));
                  if (c && "function" !== typeof window._F_installCss)
                      throw Error("aa");
                  var e, f = _.p._F_jsUrl;
                  f && (e = Eg(f));
                  !e && (f = document.getElementById("base-js")) && (e = f.src ? f.src : f.getAttribute("href"),
                  e = Eg(e));
                  e || (e = Eg(Ig));
                  e || (e = document.getElementsByTagName("script"),
                  e = Eg(e[e.length - 1].src));
                  if (!e)
                      throw Error("ba");
                  e = new tg(e);
                  c && (e.l = c);
                  d && (e.j = d);
                  e.C = b;
                  b = ja();
                  b.B = e;
                  b.Kh(!0);
                  b = ja();
                  b.cg(a);
                  a.j(b)
              }
          });

          _._ModuleManager_initialize = function(a, b) {
              if (!_.fa) {
                  if (!_.ha)
                      return;
                  _.ia()
              }
              _.fa.bg(a, b)
          }
          ;

          _._ModuleManager_initialize('b/sy0/el_conf:1/sy2/sy4/sy3:4/sy1:1,3,5/el_main:6/el_sect:6/ajaxproxy/website_error/navigationui:5/_stam:3,4/n73qwf/MpJwZc', ['sy0', 'el_conf']);

      } catch (e) {
          _._DumpException(e)
      }
      try {/*

Copyright The Closure Library Authors.
SPDX-License-Identifier: Apache-2.0
*/

      } catch (e) {
          _._DumpException(e)
      }
      try {
          _.ma("el_conf");

          var Jg, J;
          _._exportVersion = function(a) {
              _.rb("google.translate.v", a)
          }
          ;
          _._getCallbackFunction = function(a) {
              return _.ib(a)
          }
          ;
          _._exportMessages = function() {
              _.rb("google.translate.m", J)
          }
          ;
          Jg = function(a) {
              var b = document.getElementsByTagName("head")[0];
              b || (b = document.body.parentNode.appendChild(document.createElement("head")));
              b.appendChild(a)
          }
          ;
          _._loadJs = function(a) {
              var b = _.cd(document, "SCRIPT");
              b.type = "text/javascript";
              b.charset = "UTF-8";
              _.La(b, _.Hb(a));
              Jg(b)
          }
          ;
          _._loadCss = function(a) {
              var b = document.createElement("link");
              b.type = "text/css";
              b.rel = "stylesheet";
              b.charset = "UTF-8";
              b.href = a;
              Jg(b)
          }
          ;
          _._isNS = function(a) {
              a = a.split(".");
              for (var b = window, c = 0; c < a.length; ++c)
                  if (!(b = b[a[c]]))
                      return !1;
              return !0
          }
          ;
          _._setupNS = function(a) {
              a = a.split(".");
              for (var b = window, c = 0; c < a.length; ++c)
                  b.hasOwnProperty ? b.hasOwnProperty(a[c]) ? b = b[a[c]] : b = b[a[c]] = {} : b = b[a[c]] || (b[a[c]] = {});
              return b
          }
          ;
          J = {};
          MSG_TRANSLATE = "Translate";
          J[0] = MSG_TRANSLATE;
          MSG_CANCEL = "Cancel";
          J[1] = MSG_CANCEL;
          MSG_CLOSE = "Close";
          J[2] = MSG_CLOSE;
          MSGFUNC_PAGE_TRANSLATED_TO = function(a) {
              return "Google has automatically translated this page to: " + a
          }
          ;
          J[3] = MSGFUNC_PAGE_TRANSLATED_TO;
          MSGFUNC_TRANSLATED_TO = function(a) {
              return "Translated to: " + a
          }
          ;
          J[4] = MSGFUNC_TRANSLATED_TO;
          MSG_GENERAL_ERROR = "Error: The server could not complete your request. Try again later.";
          J[5] = MSG_GENERAL_ERROR;
          MSG_LEARN_MORE = "Learn more";
          J[6] = MSG_LEARN_MORE;
          MSGFUNC_POWERED_BY = function(a) {
              return "Powered by " + a
          }
          ;
          J[7] = MSGFUNC_POWERED_BY;
          MSG_TRANSLATE_PRODUCT_NAME = "Translate";
          J[8] = MSG_TRANSLATE_PRODUCT_NAME;
          MSG_TRANSLATION_IN_PROGRESS = "Translation in progress";
          J[9] = MSG_TRANSLATION_IN_PROGRESS;
          MSGFUNC_TRANSLATE_PAGE_TO = function(a) {
              return "Translate this page to: " + a + " using Google Translate?"
          }
          ;
          J[10] = MSGFUNC_TRANSLATE_PAGE_TO;
          MSGFUNC_VIEW_PAGE_IN = function(a) {
              return "View this page in: " + a
          }
          ;
          J[11] = MSGFUNC_VIEW_PAGE_IN;
          MSG_RESTORE = "Show original";
          J[12] = MSG_RESTORE;
          MSG_SSL_INFO_LOCAL_FILE = "The content of this local file will be sent to Google for translation using a secure connection.";
          J[13] = MSG_SSL_INFO_LOCAL_FILE;
          MSG_SSL_INFO_SECURE_PAGE = "The content of this secure page will be sent to Google for translation using a secure connection.";
          J[14] = MSG_SSL_INFO_SECURE_PAGE;
          MSG_SSL_INFO_INTRANET_PAGE = "The content of this intranet page will be sent to Google for translation using a secure connection.";
          J[15] = MSG_SSL_INFO_INTRANET_PAGE;
          MSG_SELECT_LANGUAGE = "Select Language";
          J[16] = MSG_SELECT_LANGUAGE;
          MSGFUNC_TURN_OFF_TRANSLATION = function(a) {
              return "Turn off " + a + " translation"
          }
          ;
          J[17] = MSGFUNC_TURN_OFF_TRANSLATION;
          MSGFUNC_TURN_OFF_FOR = function(a) {
              return "Turn off for: " + a
          }
          ;
          J[18] = MSGFUNC_TURN_OFF_FOR;
          MSG_ALWAYS_HIDE_AUTO_POPUP_BANNER = "Always hide";
          J[19] = MSG_ALWAYS_HIDE_AUTO_POPUP_BANNER;
          MSG_ORIGINAL_TEXT = "Original text:";
          J[20] = MSG_ORIGINAL_TEXT;
          MSG_FILL_SUGGESTION = "Contribute a better translation";
          J[21] = MSG_FILL_SUGGESTION;
          MSG_SUBMIT_SUGGESTION = "Contribute";
          J[22] = MSG_SUBMIT_SUGGESTION;
          MSG_SHOW_TRANSLATE_ALL = "Translate all";
          J[23] = MSG_SHOW_TRANSLATE_ALL;
          MSG_SHOW_RESTORE_ALL = "Restore all";
          J[24] = MSG_SHOW_RESTORE_ALL;
          MSG_SHOW_CANCEL_ALL = "Cancel all";
          J[25] = MSG_SHOW_CANCEL_ALL;
          MSG_TRANSLATE_TO_MY_LANGUAGE = "Translate sections to my language";
          J[26] = MSG_TRANSLATE_TO_MY_LANGUAGE;
          MSGFUNC_TRANSLATE_EVERYTHING_TO = function(a) {
              return "Translate everything to " + a
          }
          ;
          J[27] = MSGFUNC_TRANSLATE_EVERYTHING_TO;
          MSG_SHOW_ORIGINAL_LANGUAGES = "Show original languages";
          J[28] = MSG_SHOW_ORIGINAL_LANGUAGES;
          MSG_OPTIONS = "Options";
          J[29] = MSG_OPTIONS;
          MSG_TURN_OFF_TRANSLATION_FOR_THIS_SITE = "Turn off translation for this site";
          J[30] = MSG_TURN_OFF_TRANSLATION_FOR_THIS_SITE;
          J[31] = null;
          MSG_ALT_SUGGESTION = "Show alternative translations";
          J[32] = MSG_ALT_SUGGESTION;
          MSG_ALT_ACTIVITY_HELPER_TEXT = "Click on words above to get alternative translations";
          J[33] = MSG_ALT_ACTIVITY_HELPER_TEXT;
          MSG_USE_ALTERNATIVES = "Use";
          J[34] = MSG_USE_ALTERNATIVES;
          MSG_DRAG_TIP = "Drag with shift key to reorder";
          J[35] = MSG_DRAG_TIP;
          MSG_CLICK_FOR_ALT = "Click for alternative translations";
          J[36] = MSG_CLICK_FOR_ALT;
          MSG_DRAG_INSTUCTIONS = "Hold down the shift key, click, and drag the words above to reorder.";
          J[37] = MSG_DRAG_INSTUCTIONS;
          MSG_SUGGESTION_SUBMITTED = "Thank you for contributing your translation suggestion to Google Translate.";
          J[38] = MSG_SUGGESTION_SUBMITTED;
          MSG_MANAGE_TRANSLATION_FOR_THIS_SITE = "Manage translation for this site";
          J[39] = MSG_MANAGE_TRANSLATION_FOR_THIS_SITE;
          MSG_ALT_AND_CONTRIBUTE_ACTIVITY_HELPER_TEXT = "Click a word for alternative translations, or double-click to edit directly";
          J[40] = MSG_ALT_AND_CONTRIBUTE_ACTIVITY_HELPER_TEXT;
          MSG_ORIGINAL_TEXT_NO_COLON = "Original text";
          J[41] = MSG_ORIGINAL_TEXT_NO_COLON;
          J[42] = "Translate";
          J[43] = "Translate";
          J[44] = "Your correction has been submitted.";
          MSG_LANGUAGE_UNSUPPORTED = "Error: The language of the webpage is not supported.";
          J[45] = MSG_LANGUAGE_UNSUPPORTED;
          MSG_LANGUAGE_TRANSLATE_WIDGET = "Language Translate Widget";
          J[46] = MSG_LANGUAGE_TRANSLATE_WIDGET;
          _.rb("_exportVersion", _._exportVersion);
          _.rb("_getCallbackFunction", _._getCallbackFunction);
          _.rb("_exportMessages", _._exportMessages);
          _.rb("_loadJs", _._loadJs);
          _.rb("_loadCss", _._loadCss);
          _.rb("_isNS", _._isNS);
          _.rb("_setupNS", _._setupNS);
          window.addEventListener && "undefined" == typeof document.readyState && window.addEventListener("DOMContentLoaded", function() {
              document.readyState = "complete"
          }, !1);

          _.oa();

      } catch (e) {
          _._DumpException(e)
      }
  }).call(this, this.default_tr);
  
  
  // Google Inc.

  //# sourceURL=/_/translate_http/_/js/k=translate_http.tr.en_US.oOC1Oa7Rttc.O/d=1/rs=AN8SPfrSx_NPtaSO9FSK_pNCvaEYOQSpFQ/m=el_conf
  // Configure Constants
  (function() {
      let gtConstEvalStartTime = new Date();
      if (_isNS('google.translate.Element')) {
          return
      }

      (function() {
          const c = _setupNS('google.translate._const');

          c._cest = gtConstEvalStartTime;
          gtConstEvalStartTime = undefined;
          // hide this eval start time constant
          c._cl = 'en-US';
          c._cuc = 'cr.googleTranslate.onTranslateElementLoad';
          c._cac = '';
          c._cam = 'lib';
          c._ctkk = '459921.2020309657';
          const h = 'translate.googleapis.com';
          const s = 'https' + '://';
          c._pah = h;
          c._pas = s;
          const b = s + 'translate.googleapis.com';
          const staticPath = '/translate_static/';
          c._pci = b + staticPath + 'img/te_ctrl3.gif';
          c._pmi = b + staticPath + 'img/mini_google.png';
          c._pbi = b + staticPath + 'img/te_bk.gif';
          c._pli = b + staticPath + 'img/loading.gif';
          c._ps = b + staticPath + 'css\/translateelement.css';
          c._plla = 'translate-pa.googleapis.com' + '\/v1\/supportedLanguages';
          c._puh = 'translate.google.com';
          c._cnal = {};
          c._cjlc = _getCallbackFunction('cr.googleTranslate.onLoadJavascript');
          _getCallbackFunction('cr.googleTranslate.onLoadCSS')(c._ps);
          c._cjlc('https:\/\/translate.googleapis.com\/translate_static\/js\/element\/main.js');
          _exportMessages();
          _exportVersion('TE_20220615');
      }
      )();
  })();
} catch(error) {
  cr.googleTranslate.onTranslateElementError(error);
}
