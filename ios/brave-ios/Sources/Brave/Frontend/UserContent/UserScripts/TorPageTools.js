// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.

import { Readability, isProbablyReaderable } from "@mozilla/readability";
import * as DarkReader from "darkreader";

// Run only in the isolated WebKit world of an existing Tor tab.
// Dark Reader's stylesheet requests use this web view's Tor-routed fetch.
DarkReader.setFetchMethod(window.fetch.bind(window));
let original = null;
let nightMode = false;
let readerArticle = null;

const allowedTags = new Set([
  "P", "DIV", "SECTION", "ARTICLE", "H1", "H2", "H3", "H4", "H5", "H6",
  "A", "IMG", "UL", "OL", "LI", "DL", "DT", "DD", "BLOCKQUOTE", "PRE", "CODE",
  "B", "STRONG", "I", "EM", "U", "S", "BR", "HR", "SUP", "SUB", "SPAN",
  "FIGURE", "FIGCAPTION", "TABLE", "THEAD", "TBODY", "TR", "TH", "TD", "CAPTION"
]);
const discardedTags = new Set([
  "SCRIPT", "STYLE", "IFRAME", "OBJECT", "EMBED", "FORM", "INPUT", "BUTTON",
  "TEXTAREA", "LINK", "META", "BASE", "NOSCRIPT", "SVG", "MATH", "AUDIO", "VIDEO"
]);

function safeURL(value) {
  try {
    const url = new URL(value, document.baseURI);
    return ["http:", "https:"].includes(url.protocol) ? url.href : null;
  } catch {
    return null;
  }
}

// Copy a strict subset of article markup without event handlers or executable URLs.
function copyContent(node) {
  if (node.nodeType === Node.TEXT_NODE) {
    return document.createTextNode(node.textContent);
  }
  if (node.nodeType !== Node.ELEMENT_NODE || discardedTags.has(node.tagName)) {
    return document.createDocumentFragment();
  }
  const result = allowedTags.has(node.tagName)
    ? document.createElement(node.tagName.toLowerCase()) : document.createDocumentFragment();
  if (node.tagName === "A" || node.tagName === "IMG") {
    const attribute = node.tagName === "A" ? "href" : "src";
    const value = node.getAttribute(attribute);
    const url = value && safeURL(value);
    if (url) result.setAttribute(attribute, url);
    if (node.tagName === "IMG") {
      result.setAttribute("alt", node.getAttribute("alt") || "");
      result.style.maxWidth = "100%";
    }
  }
  for (const child of node.childNodes) result.appendChild(copyContent(child));
  return result;
}

function applyReaderColors() {
  if (!readerArticle) return;
  const background = nightMode ? "#181818" : "#fff";
  readerArticle.style.color = nightMode ? "#eee" : "#222";
  readerArticle.style.backgroundColor = background;
  document.body.style.setProperty("background", background, "important");
  document.documentElement.style.setProperty("background", background, "important");
}

function restoreStyle(element, value) {
  if (value === null) element.removeAttribute("style");
  else element.setAttribute("style", value);
}

function toggleReader() {
  if (original) {
    document.body.replaceChildren(original.nodes);
    restoreStyle(document.body, original.bodyStyle);
    restoreStyle(document.documentElement, original.rootStyle);
    window.scrollTo(0, original.scrollY);
    original = null;
    readerArticle = null;
    return false;
  }
  if (!document.body) return false;
  // Parse an inert document, matching the existing WebKit reader's approach.
  const copy = new DOMParser().parseFromString(document.documentElement.outerHTML, "text/html");
  const base = copy.createElement("base");
  base.href = document.baseURI;
  copy.head.prepend(base);
  const article = new Readability(copy, {
    serializer: element => element
  }).parse();
  if (!article?.content) return false;
  const content = copyContent(article.content);
  const heading = document.createElement("h1");
  heading.textContent = article.title || document.title;
  const host = document.createElement("div");
  host.style.setProperty("all", "initial", "important");
  host.style.setProperty("display", "block", "important");
  const shadow = host.attachShadow({ mode: "closed" });
  readerArticle = document.createElement("article");
  readerArticle.style.cssText =
    "display:block;box-sizing:border-box;max-width:48rem;margin:auto;padding:2rem;" +
    "font:1.15rem/1.7 Georgia,serif;overflow-wrap:anywhere;min-height:100vh";
  readerArticle.append(heading, content);
  shadow.appendChild(readerArticle);
  original = {
    nodes: document.createDocumentFragment(),
    bodyStyle: document.body.getAttribute("style"),
    rootStyle: document.documentElement.getAttribute("style"),
    scrollY: window.scrollY
  };
  while (document.body.firstChild) original.nodes.appendChild(document.body.firstChild);
  document.body.style.setProperty("margin", "0", "important");
  document.body.style.setProperty("padding", "0", "important");
  document.body.appendChild(host);
  applyReaderColors();
  window.scrollTo(0, 0);
  return true;
}

Object.defineProperty(globalThis, "__braveTorPageTools", {
  value: Object.freeze({
    isReaderAvailable: () => original !== null || isProbablyReaderable(document),
    toggleReader,
    setNightMode(enabled) {
      nightMode = enabled;
      if (enabled) DarkReader.enable();
      else DarkReader.disable();
      applyReaderColors();
    }
  })
});
