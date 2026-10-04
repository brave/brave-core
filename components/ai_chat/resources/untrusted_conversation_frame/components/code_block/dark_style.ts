// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Properties sourced from https://github.com/highlightjs/highlight.js/blob/main/src/styles/github-dark.css

export default {
  'hljs': {
    'display': 'block',
    'overflowX': 'auto',
    'padding': '0.5em',
    'color': '#c9d1d9',
    'background': '#0d1117',
  },
  'hljs-comment': {
    'color': '#8b949e',
  },
  'hljs-quote': {
    'color': '#7ee787',
  },
  'hljs-keyword': {
    'color': '#ff7b72',
  },
  'hljs-selector-tag': {
    'color': '#7ee787',
  },
  'hljs-subst': {
    'color': '#c9d1d9',
    'fontWeight': 'normal',
  },
  'hljs-number': {
    'color': '#79c0ff',
  },
  'hljs-literal': {
    'color': '#79c0ff',
  },
  'hljs-variable': {
    'color': '#79c0ff',
  },
  'hljs-template-variable': {
    'color': '#ff7b72',
  },
  'hljs-tag .hljs-attr': {
    'color': '#79c0ff',
  },
  'hljs-string': {
    'color': '#a5d6ff',
  },
  'hljs-doctag': {
    'color': '#ff7b72',
  },
  'hljs-title': {
    'color': '#d2a8ff',
  },
  'hljs-section': {
    'color': '#1f6feb',
    'fontWeight': 'bold',
  },
  'hljs-selector-id': {
    'color': '#79c0ff',
  },
  'hljs-type': {
    'color': '#ff7b72',
  },
  'hljs-class .hljs-title': {
    'color': '#d2a8ff',
  },
  'hljs-tag': {
    'color': '#7ee787',
    'fontWeight': 'normal',
  },
  'hljs-name': {
    'color': '#7ee787',
    'fontWeight': 'normal',
  },
  'hljs-attribute': {
    'color': '#79c0ff',
    'fontWeight': 'normal',
  },
  'hljs-regexp': {
    'color': '#a5d6ff',
  },
  'hljs-link': {
    'color': '#a5d6ff',
  },
  'hljs-symbol': {
    'color': '#ffa657',
  },
  'hljs-bullet': {
    'color': '#f2cc60',
  },
  'hljs-built_in': {
    'color': '#ffa657',
  },
  'hljs-builtin-name': {
    'color': '#ffa657',
  },
  'hljs-meta': {
    'color': '#79c0ff',
  },
  'hljs-deletion': {
    'color': '#ffdcd7',
    'background': '#67060c',
  },
  'hljs-addition': {
    'color': '#aff5b4',
    'background': '#033a16',
  },
  'hljs-emphasis': {
    'color': '#c9d1d9',
    'fontStyle': 'italic',
  },
  'hljs-strong': {
    'color': '#c9d1d9',
    'fontWeight': 'bold',
  },
} as { [key: string]: React.CSSProperties }
