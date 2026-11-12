// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
// Run from the repository root with: node --test ios/brave-ios/Tests/JavaScript/TorPageTools.test.mjs

import test from "node:test";
import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import { JSDOM } from "jsdom";
import { Readability, isProbablyReaderable } from "@mozilla/readability";

const source = (await readFile(new URL(
  "../../Sources/Brave/Frontend/UserContent/UserScripts/TorPageTools.js", import.meta.url
), "utf8")).replace(/^import .*;$/gm, "");

test("Tor reader sanitizes article markup and restores the original live DOM", () => {
  const paragraph = "An engineering article contains detailed explanations, examples, and calculations. ".repeat(20);
  const dom = new JSDOM(`<!doctype html><html><head><title>Engineering</title></head>
    <body style="color:red"><nav id="navigation">Navigation</nav><article>
    <h1>Engineering</h1><p>${paragraph}</p><p>${paragraph}</p>
    <a href="/next" onclick="evil()">Next</a><a href="javascript:evil()">Unsafe</a>
    <img src="/image.png" onerror="evil()"><iframe src="/frame"></iframe>
    <form><input value="secret"></form></article></body></html>`,
    { url: "https://article.example/page", runScripts: "outside-only" });
  try {
    const { window } = dom;
    let enabled = false, readerRoot, fetchMethod;
    window.Readability = Readability;
    window.isProbablyReaderable = isProbablyReaderable;
    window.scrollTo = () => {};
    window.fetch = () => { throw Error("Unexpected request"); };
    window.DarkReader = {
      setFetchMethod(method) { fetchMethod = method; },
      enable() { enabled = true; },
      disable() { enabled = false; }
    };
    const attachShadow = window.HTMLElement.prototype.attachShadow;
    window.HTMLElement.prototype.attachShadow = function(options) {
      readerRoot = attachShadow.call(this, options);
      return readerRoot;
    };
    const navigation = window.document.querySelector("#navigation");
    const before = window.document.body.innerHTML;
    const style = window.document.body.getAttribute("style");
    window.eval(source);
    const tools = window.__braveTorPageTools;
    assert.equal(tools.toggleReader(), true);
    assert.ok(readerRoot.querySelector("article").textContent.includes("engineering article"));
    assert.equal(readerRoot.querySelectorAll("script,iframe,form,input,svg,[onclick],[onerror]").length, 0);
    assert.equal(readerRoot.querySelectorAll('[href^="javascript:"]').length, 0);
    assert.equal(readerRoot.querySelector("a[href]")?.href, "https://article.example/next");
    assert.equal(tools.isReaderAvailable(), true);
    tools.setNightMode(true);
    assert.equal(enabled, true);
    assert.equal(tools.toggleReader(), false);
    assert.equal(window.document.body.innerHTML, before);
    assert.equal(window.document.body.getAttribute("style"), style);
    assert.equal(window.document.querySelector("#navigation"), navigation);
    tools.setNightMode(false);
    assert.equal(enabled, false);
    assert.throws(() => fetchMethod("/stylesheet.css"), /Unexpected request/);
  } finally {
    dom.window.close();
  }
});
