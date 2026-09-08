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
    /* 100vh is pinned to the initial viewport, not WKWebView's toolbar
       hiding/reappearing on scroll, so the true bottom becomes
       unreachable once the toolbar's height no longer matches. 100dvh
       tracks the actual viewport. */
    block-size: 100dvh;

    @media (prefers-color-scheme: dark) {
      scrollbar-color: rgba(255, 255, 255, 0.25) rgba(0, 0, 0, 0);
    }
  }

  .sidebar {
    min-width: 250px;
    display: flex;
    flex-direction: column;
    background: ${color.container.background};
    /* Stops a drag on the sidebar's own chrome from chaining to
       page-content's scroll. Not enough for nav's own list, which needs
       its own overscroll-behavior below. */
    overscroll-behavior: contain;

    /* Plain media query, not container query: container-type on an
       ancestor would make it the containing block for this fixed sidebar
       instead of the viewport, clipping it. */
    @media (width < ${wideWidth}) {
      position: fixed;
      inset-block-start: 0;
      inset-block-end: 0;
      inset-inline-start: 0;
      /* Without an upper bound, min-width alone sizes to the widest
         content (e.g. "Confirmation Queue"), which can exceed a narrow
         phone's viewport and force a horizontal scrollbar. */
      width: min(320px, 85vw);
      z-index: 4;
      border-inline-end: solid 1px ${color.divider.subtle};
      box-shadow: 0px 4px 13px -2px rgba(0, 0, 0, 0.08);

      /* display: none, not a transform slide: this WKWebView can leave a
         transformed fixed element hit-testable/paintable at its old
         position despite the correct computed style. Loses the slide
         animation but leaves no remnant. */
      display: none;

      &.open {
        display: flex;
      }
    }
  }

  /* Sits above .page-content but below .sidebar, so a touch outside the
     drawer hits this instead of the content underneath. Hidden at wide
     widths in case sidebarOpen is stale from before a resize. */
  .sidebar-backdrop {
    position: fixed;
    inset: 0;
    z-index: 3;

    @media (width >= ${wideWidth}) {
      display: none;
    }
  }

  nav {
    /* flex-shrink: 1, not 0: nav must shrink to .sidebar's leftover
       space to actually overflow and scroll. flex-shrink: 0 kept it at
       full content height with nothing to scroll, cutting the list off
       instead. */
    flex: 1 1 auto;
    min-block-size: 0;
    overflow: auto;
    /* nav is the actual scrollable element being dragged; only its own
       containment stops a chain once the tab list itself hits its scroll
       limit (a non-scrolling ancestor's overscroll-behavior doesn't help). */
    overscroll-behavior: contain;
  }

  .page-content {
    flex: 1 1 auto;
    overflow: auto;
    scrollbar-gutter: stable;
    /* A normal overflow: auto element contains the bounce locally on its
       own; without this, dragging past the top/bottom chains into the
       WKWebView's own document-level bounce. */
    overscroll-behavior: contain;
  }

  .page-header {
    position: relative;
    padding: 32px 32px 0;
  }

  /* Relative to .page-header, not the viewport, so it scrolls away with
     the title instead of floating over whatever content is underneath. */
  .sidebar-toggle {
    position: absolute;
    inset-block-start: 24px;
    inset-inline-start: 24px;
    z-index: 1;

    @media (width >= ${wideWidth}) {
      display: none;
    }
  }

  header {
    padding: 24px;
  }

  /* Needs its own close control since .page-header's toggle button lives
     in .page-content and may be scrolled out of view. Only relevant once
     .sidebar is an overlay. */
  .sidebar-close {
    display: flex;
    justify-content: flex-end;

    @media (width >= ${wideWidth}) {
      display: none;
    }
  }

  main {
    margin: 0 auto;
    max-width: 1024px;
    padding: 0 32px 32px;
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
    max-width: 1024px;
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
    margin: 0 auto;
    font: ${font.heading.h3};
    text-align: center;
  }

  .disclaimer {
    max-width: 1024px;
    margin: 0 auto 24px;
    padding: 0 32px;
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

  /* Pins this element (and anything after it) to the end of an h4 row,
     regardless of whether a sibling ".title" actually grows enough to push
     it there on its own. */
  .header-end-action {
    margin-inline-start: auto;
  }

  leo-toggle {
    font: ${font.small.semibold};
    margin-inline-end: 4px;
  }

  .title {
    flex: 1 1 auto;

    /* white-space: nowrap so a title's own words (and any id/value inside
       it) move to the next line together as one unit via h4's own
       flex-wrap: wrap, rather than the browser's default text wrapping
       breaking the phrase apart mid-title when the row runs out of room.
       Only needed once the row is narrow enough to run out of room. */
    @media (width < ${wideWidth}) {
      white-space: nowrap;
    }
  }

  /* A flex-basis of 100% claims the whole row for itself, which always
     pushes a following h4 child onto its own line below, regardless of
     whether both would otherwise fit side by side; e.g. a Campaign ID and
     its Advertiser ID, which read as two separate facts rather than one
     title. */
  .title-own-line {
    flex: 1 1 100%;
  }

  /* A flex-basis of 100% claims the whole row for itself, which always
     pushes a following h4 child onto its own line below, regardless of
     whether both would otherwise fit side by side; e.g. a Campaign ID and
     its Advertiser ID, which read as two separate facts rather than one
     title. */
  .title-own-line {
    flex: 1 1 100%;
  }

  /* Wraps a leo-button rather than applying to it directly: leo-button's
     own shadow DOM sets width: 100%, which becomes its flex-basis and
     balloons it to fill the row instead of its content. */
  .fixed-flex-item {
    display: flex;
    flex: 0 0 auto;
  }

  input, select {
    font: ${font.default.regular};
    /* iOS zooms the whole page on focus for any input with a computed
       font-size under 16px; overrides the token's smaller size here only. */
    font-size: 16px;
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

    /* .copyable-text's own narrow-screen 16ch cap (see its definition
       below) is for an id floating in free-flowing text with nothing else
       to clip against; here the row's own flex layout already gives the
       value its real available width, which is usually wider than 16ch,
       so deferring to it means this only ellipsizes once it actually
       doesn't fit rather than always. */
    .copyable-text {
      max-width: 100%;
    }

    > div {
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      justify-content: space-between;
      gap: 16px;

      /* The label never shrinks or wraps; a long value stays on the same
         line and wraps its own text instead of pushing the whole row onto
         a new line below the label. Only needed once the row is narrow
         enough that flex-wrap above would otherwise kick in. */
      @media (width < ${wideWidth}) {
        flex-wrap: nowrap;

        > :first-child {
          flex: 0 0 auto;
        }

        > :last-child {
          flex: 1 1 auto;
          min-width: 0;
          text-align: right;
        }
      }
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

  .diagnostic-success {
    color: ${color.systemfeedback.successText};
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
    border: none;
    background: none;
    padding: 0;
    font: inherit;
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

  /* Groups the "ID" label with its input as a single atomic flex item, so
     h4's own flex-wrap: wrap moves them to the next line together rather
     than treating them as independently-wrappable items (which could split
     the label from its input across two lines). */
  .id-input-group {
    display: flex;
    align-items: center;
    gap: 8px;

    /* flex-grow: 1 so this (and the input inside it) can actually claim
       leftover row space on a narrow screen once copy/generate have
       wrapped away, instead of sitting at its narrow min-content width.
       flex-shrink: 1 and min-width: 0: once alone on its own line, it
       still needs to be able to shrink to the card's own width rather
       than overflowing past it outright if 38ch doesn't fit. Not needed
       on a wide screen, where the input keeps its fixed 38ch box. */
    @media (width < ${wideWidth}) {
      flex: 1 1 auto;
      min-width: 0;
      max-width: 100%;
    }
  }

  .diagnostic-id-input {
    font-family: monospace;
    box-sizing: border-box;
    /* A v4 UUID is always 36 characters; a bit of headroom over 36ch since
       ch-unit sizing is only an approximation of the widest glyph. */
    width: 38ch;

    /* Shrinks along with .id-input-group rather than spilling past the
       card if even 38ch alone doesn't fit. */
    @media (width < ${wideWidth}) {
      flex: 1 1 38ch;
      width: auto;
      min-width: 0;
      max-width: 100%;
    }
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

    /* The trailing validation icon (warning or match) sits flush right
       regardless of the input's own (fixed, sometimes narrower) width,
       rather than immediately after it with a gap. Targets the icon type
       specifically, not :last-child: when no icon is shown yet, the input
       itself is last, and this must not apply to it. */
    > leo-icon {
      margin-inline-start: auto;
    }
  }

  .test-condition-matcher-input {
    flex: 0 0 auto;
  }

  .test-condition-matcher-value-column {
    display: flex;
    flex-direction: column;
  }

  .test-condition-matcher-hint {
    text-align: right;
    font: ${font.xSmall.regular};
    color: ${color.text.tertiary};
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
    /* Only forced down to a fixed 16ch on narrow screens, where an ID
       inline in free-flowing text has nothing else to clip against and
       would otherwise grow to its full length on a narrow line. On wider
       screens there's usually room to show the full value, so this
       shouldn't force an ellipsis that isn't actually needed. */
    @media (width < ${wideWidth}) {
      max-width: 16ch;
    }
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
     does. */
  .copyable-text-wrap {
    max-width: 100%;
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

    /* .copyable-text's narrow-screen 16ch cap (see its definition below)
       is for an id floating in free-flowing text with nothing else to
       clip against. Inside a cell, the width classes above already
       constrain the real available space, which can be under 16ch on
       mobile; deferring to the cell's own width here lets its
       overflow/ellipsis apply correctly instead. */
    .copyable-text {
      max-width: 100%;
    }

    /* .id-with-reaction's white-space: nowrap makes it size to its full
       content width regardless of the cell's real width (shrink-to-fit
       never goes below min-content), overflowing the cell. width: 100%
       forces it to the cell's actual width; its copyable-text child needs
       min-width: 0 to actually shrink and ellipsize within that row,
       while the reaction icon keeps its own fixed size. */
    .id-with-reaction {
      width: 100%;
      min-width: 0;

      .copyable-text {
        min-width: 0;
      }

      leo-icon {
        flex-shrink: 0;
      }
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
