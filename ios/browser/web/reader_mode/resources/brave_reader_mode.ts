// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {CrWebApi, gCrWeb} from
    '//ios/web/public/js_messaging/resources/gcrweb.js';

// Readability.js and Readability-readerable.js are injected as separate
// FeatureScripts before this one and expose their APIs as globals.
declare class Readability<T = string> {
  constructor(
      document: Document,
      options?: {
        debug?: boolean; maxElemsToParse?: number; nbTopCandidates?: number;
        charThreshold?: number;
        classesToPreserve?: string[];
        keepClasses?: boolean;
        serializer?: (node: Node) => T;
        disableJSONLD?: boolean;
        allowedVideoRegex?: RegExp;
      });
  parse(): null|{
    title: string | null | undefined;
    content: T | null | undefined;
    textContent: string | null | undefined;
    length: number | null | undefined;
    excerpt: string | null | undefined;
    byline: string | null | undefined;
    dir: string | null | undefined;
    siteName: string | null | undefined;
    lang: string | null | undefined;
    publishedTime: string | null | undefined;
  };
}

declare function isProbablyReaderable(
    document: Document,
    options?: {
      minContentLength?: number;
      minScore?: number;
      visibilityChecker?: (node: Node) => boolean;
    }): boolean;

type ReadabilityParseResult = ReturnType<Readability['parse']>;

interface ExtendedParseResult extends NonNullable<ReadabilityParseResult> {
  cspMetaTags?: string[];
  documentLanguage?: string;
}

const kReaderModeURL = /^internal:\/\/local\/reader-mode/;

const kBlockImagesSelector =
    '.content p > img:only-child, ' +
    '.content p > a:only-child > img:only-child, ' +
    '.content .wp-caption img, ' +
    '.content figure img';

let readabilityResult: ExtendedParseResult|null = null;

interface ReaderModeStyle {
  theme?: string;
  fontSize?: number;
  fontType?: string;
}

let currentStyle: ReaderModeStyle|null = null;

function checkReadability(): string|null {
  if (!isProbablyReaderable(document)) {
    return null;
  }

  // Short circuit if Readability already ran (back/forward cache hit).
  if (readabilityResult && readabilityResult['content']) {
    return JSON.stringify(readabilityResult);
  }

  // Serialize then re-parse to avoid cloneNode crashes (bug 1128774).
  const docStr = new XMLSerializer().serializeToString(document);

  // Skip documents with <frameset> to avoid WKWebView crashes (bug 1489543).
  if (docStr.includes('<frameset ')) {
    return null;
  }

  const doc = new DOMParser().parseFromString(docStr, 'text/html');
  const result = new Readability(doc, {}).parse();

  if (!result) {
    return null;
  }

  readabilityResult = result as ExtendedParseResult;

  const cspMetaTags = document.querySelectorAll(
      'meta[http-equiv="Content-Security-Policy"]');
  if (cspMetaTags.length > 0) {
    readabilityResult.cspMetaTags =
        Array.from(cspMetaTags).map(
            (e) => new XMLSerializer().serializeToString(e));
  }

  const documentLanguage =
      document.documentElement.lang ||
      document.querySelector('meta[http-equiv="Content-Language"]')
          ?.getAttribute('content') ||
      null;
  if (documentLanguage) {
    readabilityResult.documentLanguage = documentLanguage;
  }

  return JSON.stringify(readabilityResult);
}

function setStyle(style: ReaderModeStyle): void {
  if (currentStyle?.theme) {
    document.body.classList.remove(currentStyle.theme);
  }
  if (style?.theme) {
    document.body.classList.add(style.theme);
  }

  if (currentStyle?.fontSize !== undefined) {
    document.body.classList.remove('font-size' + currentStyle.fontSize);
  }
  if (style?.fontSize !== undefined) {
    document.body.classList.add('font-size' + style.fontSize);
  }

  if (currentStyle?.fontType) {
    document.body.classList.remove(currentStyle.fontType);
  }
  if (style?.fontType) {
    document.body.classList.add(style.fontType);
  }

  currentStyle = style;
}

function updateImageMargins(): void {
  const contentElement = document.getElementById('reader-content');
  if (!contentElement) {
    return;
  }

  const windowWidth = window.innerWidth;
  const contentWidth = contentElement.offsetWidth;
  const maxWidthStyle = windowWidth + 'px !important';

  type ImgWithOriginalWidth =
      HTMLImageElement&{_originalWidth?: number};

  const setImageMargins = (img: ImgWithOriginalWidth): void => {
    if (!img._originalWidth) {
      img._originalWidth = img.offsetWidth;
    }

    let imgWidth = img._originalWidth;
    if (imgWidth < contentWidth && imgWidth > windowWidth * 0.55) {
      imgWidth = windowWidth;
    }

    const sideMargin =
        Math.max(
            (contentWidth - windowWidth) / 2,
            (contentWidth - imgWidth) / 2);

    img.style.cssText =
        'max-width: ' + maxWidthStyle + ';' +
        'width: ' + imgWidth + 'px !important;' +
        'margin-left: ' + sideMargin + 'px !important;' +
        'margin-right: ' + sideMargin + 'px !important;';
  };

  const imgs =
      document.querySelectorAll<ImgWithOriginalWidth>(kBlockImagesSelector);
  for (let i = imgs.length - 1; i >= 0; i--) {
    const img = imgs[i];
    if (!img) {
      continue;
    }
    if (img.width > 0) {
      setImageMargins(img);
    } else {
      img.onload = () => setImageMargins(img);
    }
  }
}

function showContent(): void {
  const messageElement = document.getElementById('reader-message');
  if (messageElement) {
    messageElement.style.display = 'none';
  }
  const headerElement = document.getElementById('reader-header');
  if (headerElement) {
    headerElement.style.display = 'block';
  }
  const contentElement = document.getElementById('reader-content');
  if (contentElement) {
    contentElement.style.display = 'block';
  }
}

function configureReader(): void {
  const styleAttr = document.body.getAttribute('data-readerStyle');
  if (styleAttr) {
    setStyle(JSON.parse(styleAttr) as ReaderModeStyle);
  }
  showContent();
  updateImageMargins();
}

window.addEventListener('load', () => {
  if (document.location.href.match(kReaderModeURL)) {
    configureReader();
  }
});

const readerModeApi = new CrWebApi('readerMode');
readerModeApi.addFunction('checkReadability', checkReadability);
readerModeApi.addFunction('setStyle', setStyle);
gCrWeb.registerApi(readerModeApi);
