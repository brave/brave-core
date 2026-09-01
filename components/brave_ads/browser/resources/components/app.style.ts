/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

const wideWidth = '800px'

export const style = scoped.css`
  & {
    display: flex;
    block-size: 100vh;
    container-type: inline-size;

    @media (prefers-color-scheme: dark) {
      scrollbar-color: rgba(255, 255, 255, 0.25) rgba(0, 0, 0, 0);
    }
  }

  .sidebar {
    min-width: 250px;
    display: flex;
    flex-direction: column;
    background: ${color.container.background};

    @container (width < ${wideWidth}) {
      position: fixed;
      inset-block-start: 0;
      inset-block-end: 0;
      inset-inline-start: 0;
      z-index: 2;
      border-inline-end: solid 1px ${color.divider.subtle};
      box-shadow: 0px 4px 13px -2px rgba(0, 0, 0, 0.08);

      transform: translateX(-110%);
      transition: transform 250ms;

      &.open {
        transform: translateX(0);
      }
    }
  }

  nav {
    flex: 1 0 auto;
    overflow: auto;
  }

  .page-content {
    flex: 1 1 auto;
    padding: 32px;
    overflow: auto;
    scrollbar-gutter: stable;
    position: relative;
  }

  .sidebar-toggle {
    position: absolute;
    inset-block-start: 24px;
    inset-inline-start: 24px;
    z-index: 1;

    @container (width >= ${wideWidth}) {
      display: none;
    }
  }

  header {
    padding: 24px;
  }

  main {
    margin: 0 auto;
    max-width: 1024px;
    display: flex;
    flex-direction: column;
    gap: 24px;
  }

  .card-group {
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  h1 {
    padding: 0 32px;
  }

  ul {
    list-style-type: none;
    padding: 0;
    margin: 0;
  }

  li {
    position: relative;

    a {
      width: 100%;
      color: ${color.text.secondary};
      font: ${font.components.navbutton};
      text-decoration: none;
      display: flex;
      align-items: center;
      gap: 16px;
      padding: 13px 24px;

      &:hover {
        background: ${color.container.highlight};
      }
    }

    a.current {
      color: ${color.text.interactive};

      &::before {
        content: '';
        position: absolute;
        inset-block-start: 8px;
        inset-inline-start: 0;
        display: block;
        inline-size: 4px;
        block-size: 32px;
        background: ${color.icon.interactive};
        border-start-end-radius: 2px;
        border-end-end-radius: 2px;
      }
    }
  }
`

style.passthrough.css`
  & {
    font: ${font.default.regular};
    color: ${color.text.primary};
  }

  h1 {
    margin: 0;
    font: ${font.heading.h3};
    text-align: center;
  }

  .disclaimer {
    text-align: center;
  }

  .header-actions {
    display: flex;
    justify-content: flex-end;

    leo-button {
      flex: 0 0 auto;
      width: auto;
    }
  }

  h4 {
    font: ${font.heading.h4};
    margin: 0;
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 8px;

    > * {
      flex: 0 1 auto;
    }
  }

  leo-toggle {
    font: ${font.small.semibold};
    margin-inline-end: 4px;
  }

  .title {
    flex: 1 1 auto;
  }

  input, select {
    font: ${font.default.regular};
    color: ${color.text.primary};
    background: ${color.container.background};
    padding: 8px;
    border-radius: 8px;
    border: solid 1px ${color.divider.strong};
  }

  input {
    min-width: 320px;
  }

  /* Wide enough for any locale's date format (e.g. "dd/mm/yyyy" or
     "mm/dd/yyyy") plus the calendar picker icon; width: min-content broke
     the native calendar picker's positioning, so this is a fixed size
     instead. */
  input[type='date'] {
    min-width: 0;
    width: 150px;
  }

  /* The native calendar-picker glyph is rendered dark-on-transparent by
     default, which is nearly invisible against this page's dark background. */
  input[type='date']::-webkit-calendar-picker-indicator {
    filter: invert(1);
  }

  .key-value-list {
    display: flex;
    flex-direction: column;
    gap: 4px;
    padding: 8px;

    > div {
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      justify-content: space-between;
      gap: 16px;
    }
  }

  .url-pattern-wildcard {
    color: ${color.systemfeedback.warningText};
    font-weight: 600;
  }

  .content-card a {
    color: ${color.text.interactive};
  }

  .subsection-title {
    font: ${font.default.semibold};
    margin: 0;
    padding: 8px;
  }

  .content-card section.nested-section {
    width: calc(100% - 16px);
    margin-inline-start: 16px;
  }

  .diagnostic-problem {
    color: ${color.systemfeedback.errorText};
    background: ${color.systemfeedback.warningBackground};
    border-radius: 4px;
    padding: 2px 6px;
  }

  .diagnostic-muted {
    color: ${color.text.tertiary};
  }

  .monospace-value {
    font-family: monospace;
  }

  .json-block {
    pre {
      margin: 4px 0;
      white-space: pre-wrap;
    }
  }

  /* Clips the collapsed (minified) view to 2 lines with an ellipsis, rather
     than letting a long JSON value balloon the row's height by default. */
  .json-block-collapsed {
    display: -webkit-box;
    -webkit-box-orient: vertical;
    -webkit-line-clamp: 2;
    overflow: hidden;
  }

  /* Keeps an id and its reaction icon on the same line; two adjacent inline
     elements with no text between them can otherwise still wrap apart at a
     narrow width. */
  .id-with-reaction {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    white-space: nowrap;
    --leo-icon-size: 14px;
  }

  .condition-matcher-match {
    color: ${color.systemfeedback.successText};
  }

  .condition-matcher-no-match {
    color: ${color.systemfeedback.errorText};
  }

  .condition-matcher-unknown {
    color: ${color.systemfeedback.warningText};
  }

  .campaign-active {
    color: ${color.systemfeedback.successText};
  }

  .campaign-not-started {
    color: ${color.systemfeedback.warningText};
  }

  .campaign-ended {
    color: ${color.systemfeedback.errorText};
  }

  .diagnostic-masked {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    cursor: pointer;
  }

  .diagnostic-masked-dot {
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: ${color.text.secondary};
  }

  .diagnostic-divider {
    width: 100%;
    border: none;
    border-top: solid 1px ${color.divider.subtle};
  }

  /* Keeps its layout space so a header row doesn't change height/jump when
     this becomes visible, e.g. a Clear button that only appears once there's
     something to clear. */
  .invisible-reserved {
    visibility: hidden;
    pointer-events: none;
  }

  .diagnostic-id-input {
    font-family: monospace;
    box-sizing: border-box;
    /* A v4 UUID is always 36 characters; a bit of headroom over 36ch since
       ch-unit sizing is only an approximation of the widest glyph. */
    width: 38ch;
  }

  /* Sized down from the default icon size so it reads as part of the
     surrounding sentence rather than dominating the line. vertical-align
     is needed because an inline-flex box aligns by its own bottom edge
     against surrounding text by default, not the text's baseline, which
     otherwise makes it sit visibly higher than the rest of the sentence. */
  .inline-icon-value {
    display: inline-flex;
    align-items: center;
    vertical-align: middle;
    gap: 4px;
    --leo-icon-size: 14px;
  }

  /* A fixed width (rather than reserving space via margin on the input
     itself) so the Pref Path/Condition inputs' own boxes line up exactly,
     whichever row currently has an icon showing after it. */
  .test-condition-matcher-value {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    width: 356px;
  }

  .test-condition-matcher-input {
    flex: 0 0 auto;
  }

  .icon-success {
    --leo-icon-color: ${color.systemfeedback.successIcon};
  }

  .icon-error {
    --leo-icon-color: ${color.systemfeedback.errorIcon};
  }

  tr.confirmation-stuck-orange td {
    color: ${color.systemfeedback.warningText};
  }

  tr.confirmation-stuck-red td {
    color: ${color.systemfeedback.errorText};
  }

  tr.diagnostic-problem-row td {
    color: ${color.systemfeedback.errorText};
  }

  .copyable-text {
    display: inline-block;
    max-width: 100%;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    vertical-align: bottom;
    cursor: pointer;
    text-decoration: underline dotted;

    &:hover {
      color: ${color.text.interactive};
    }
  }

  /* A URL within free-flowing log text should read in full and wrap like
     the rest of the line, not clip with an ellipsis the way a short ID
     does in a narrow table column. */
  .copyable-text-wrap {
    overflow: visible;
    text-overflow: clip;
    white-space: normal;
    overflow-wrap: anywhere;
  }

  .copyable-text-mono {
    font-family: monospace;
  }

  /* A plain in-text action, as opposed to .copyable-text's dotted underline
     (which specifically means "click to copy"); e.g. "show all" to expand a
     long list inline instead of dumping it into the heading unconditionally. */
  .text-link {
    color: ${color.text.interactive};
    text-decoration: underline;
    cursor: pointer;
  }

  /* For an entire table cell that's click-to-copy, as opposed to
     .copyable-text which wraps just a value within otherwise plain
     content; display: inline-block doesn't apply to table cells, so this
     is cursor/text-decoration only. */
  .copyable-cell {
    cursor: pointer;
    text-decoration: underline dotted;

    &:hover {
      color: ${color.text.interactive};
    }
  }

  table {
    flex-grow: 1;
    margin: 8px 0;
    width: 100%;
    table-layout: fixed;

    th {
      text-align: left;
      font: ${font.small.semibold};
      color: ${color.text.secondary};
    }

    td, th {
      padding: 8px 4px;
      /* A column without an explicit width class (below) or .truncate-cell
         shares whatever width table-layout: fixed leaves over; a long
         unbroken token (a UUID, "ad_notification") has no natural break
         point and would otherwise overflow past its own column into the
         next one instead of wrapping. */
      overflow-wrap: anywhere;
    }

    tfoot td {
      font: ${font.default.semibold};
      border-top: solid 1px ${color.divider.strong};
    }

    .truncate-cell {
      max-width: 0;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
    }

    .wide-column {
      width: 50%;
    }

    /* Enough to comfortably fit a full v4 UUID by itself, unlike
       .rule-column, which turned out too tight once tested. */
    .extra-wide-column {
      width: 40%;
    }

    .rule-column {
      width: 30%;
    }

    .status-column {
      width: 10%;
    }

    /* For tables with two ID columns side by side, so they read as the same
       kind of thing rather than one looking more important than the other. */
    .wide-id-column {
      width: 25%;
    }

    .value-column {
      width: 20%;
    }

    .narrow-value-column {
      width: 15%;
    }

    .nowrap-cell {
      white-space: nowrap;
    }

    /* Always present, even with zero rows, unlike a border on the first
       tbody row (which only exists when there's at least one row). */
    thead th {
      border-bottom: solid 1px ${color.divider.subtle};
    }
  }

  .content-card {
    border-radius: 16px;
    padding: 4px;
    background-color: rgba(255, 255, 255, 0.55);
    display: flex;
    flex-direction: column;
    gap: 4px;

    section {
      border-radius: 12px;
      background: ${color.container.background};
      width: 100%;
      overflow-x: auto;
      padding: 8px;
    }

    h4 {
      padding: 8px;
    }

    > p {
      margin: 0;
      padding: 0 8px 8px;
    }

    @media (prefers-color-scheme: dark) {
      background-color: rgba(37, 37, 37, 0.58);
    }
  }
`
