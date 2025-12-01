/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, effect, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const narrowBreakpoint = '900px'
export const threeColumnBreakpoint = '1275px'
export const horizontalContentPadding = 24

const topControlsNarrowBreakpoint = '1075px'
const topControlsWideBreakpoint = threeColumnBreakpoint

export const style = scoped.css`
  & {
    --search-transition-duration: 120ms;
    --top-controls-text-shadow: rgba(0, 0, 0, 0.33) 0 1px 2px;
  }

  @keyframes background-scroll-fade {
    from {
      background: rgba(0, 0, 0, 0);
      backdrop-filter: blur(0);
    }
    50% {
      backdrop-filter: blur(0);
    }
    to {
      background: rgba(0, 0, 0, 0.65);
      backdrop-filter: blur(32px);
    }
  }

  .background-filter {
    position: fixed;
    inset: 0;
    z-index: 1;

    animation: linear background-scroll-fade both;
    animation-timeline: scroll();
    animation-range: 0px 100vh;
  }

  .settings {
    --leo-icon-size: 18px;

    position: absolute;
    z-index: 2;
    inset-block-start: 0;
    inset-inline-end: 0;
    margin: 24px;

    padding: 8px;
    border-radius: 50%;
    color: #fff;
    opacity: .9;
    filter: drop-shadow(var(--top-controls-text-shadow));

    &:hover {
      background: rgba(255, 255, 255, .3);
      box-shadow: ${effect.elevation['01']};
      cursor: pointer;
    }

    @container (width < ${topControlsNarrowBreakpoint}) {
      margin: 12px;
    }
  }

  .clock {
    position: absolute;
    z-index: 2;
    inset-block-start: 0;
    inset-inline-start: 0;
    margin: 24px;

    padding: 8px;
    font: ${font.large.semibold};
    font-size: 56px;
    font-weight: 500;
    line-height: 100%;
    text-shadow: var(--top-controls-text-shadow);

    color: #fff;
    opacity: .9;

    .ntp-top-sites-wide & {
      @container (width < ${topControlsWideBreakpoint}) {
        font-size: 16px;
      }
    }

    @container (width < ${topControlsNarrowBreakpoint}) {
      margin: 12px;
      font-size: 16px;
    }
  }

  .allow-background-pointer-events {
    /* This element will allow pointer events to target the background. */
    pointer-events: none;

    /* But children will not (unless they explicitly allow it). */
    > :not(.allow-background-pointer-events) {
      pointer-events: auto;
    }

    /* And not when a popover is open. When a popover is open and the background
       contains an interactive iframe, pointer events on a background iframe
       will not "light-dismiss" the popover. */
    :scope:has(:popover-open) & {
      pointer-events: auto;
    }
  }

  main {
    container-type: inline-size;
    view-timeline-name: --ntp-main-view-timeline;
    position: relative;
    z-index: 1;
    display: flex;
    flex-direction: column;
    align-items: center;
    min-height: 100vh;
    gap: 16px;
    padding: 16px ${horizontalContentPadding}px;

    > * {
      transition:
        opacity var(--search-transition-duration),
        transform var(--search-transition-duration);

      .search-box-expanded & {
        opacity: 0;
        transform: scale(0.9);
      }
    }
  }

  .topsites-container {
    padding: 16px 0;
    align-self: stretch;
    display: flex;
    gap: 16px;
  }

  .searchbox-container {
    align-self: stretch;

    .search-box-expanded & {
      opacity: 1;
      transform: none;
    }
  }

  .spacer {
    flex: 1 1 auto;
    align-self: stretch;

    display: flex;
    align-items: stretch;
    justify-content: center;

    @container (width > ${narrowBreakpoint}) {
      min-height: 200px;
    }
  }

  .caption-container {
    @container (width > ${narrowBreakpoint}) {
      position: absolute;
      position-anchor: --ntp-widget-container;
      inset-block-end: anchor(end);
      inset-inline-end: anchor(start);
      inset-inline-start: 0;
      margin-inline-start: 16px;
      margin-inline-end: 16px;

      min-width: fit-content;
      min-height: 30px;

      position-try-fallbacks: --try-captions-above;
    }
  }

  @position-try --try-captions-above {
    inset-block-end: anchor(start);
    inset-inline-start: anchor(start);
    inset-inline-end: unset;
    margin-block-end: 16px;
    margin-inline-start: 0;
    margin-inline-end: 0;
  }

  .widget-container {
    --widget-height: 128px;
    --widget-min-width: 380px;
    --widget-max-width: 512px;
    --widget-gap: 16px;

    anchor-name: --ntp-widget-container;

    flex: 0 0 auto;
    min-height: var(--widget-height);

    display: grid;
    grid-auto-columns: minmax(var(--widget-min-width), var(--widget-max-width));
    grid-auto-rows: minmax(var(--widget-height), auto);
    grid-auto-flow: column;
    justify-content: center;
    align-items: stretch;
    gap: var(--widget-gap);

    &:empty {
      min-height: 0;
    }

    @container (width <= ${narrowBreakpoint}) {
      grid-auto-flow: row;
    }

    @container (width > ${narrowBreakpoint}) {
      &:has(> :nth-child(2)) {
        justify-content: space-between;
        align-self: stretch;
      }

      &:has(> :nth-child(3)) {
        justify-content: center;
        align-self: center;
      }
    }
  }

  .news-container {
    position: relative;
    z-index: 1;
  }
`

style.passthrough.css`
  & {
    font: ${font.default.regular};
    color: ${color.text.primary};
    interpolate-size: allow-keywords;
  }

  button {
    margin: 0;
    padding: 0;
    background: 0;
    border: none;
    text-align: unset;
    width: unset;
    font: inherit;
    cursor: pointer;

    &:disabled {
      cursor: default;
    }
  }

  h2 {
    font: ${font.heading.h2};
    margin: 0;
  }

  h3 {
    font: ${font.heading.h3};
    margin: 0;
  }

  h4 {
    font: ${font.heading.h4};
    margin: 0;
  }

  p {
    margin: 0;
  }

  dialog, [popover] {
    border: none;
    color: inherit;
    margin: 0;
    padding: 0;
    background: none;

    &::backdrop {
      background-color: transparent;
    }
  }

  .popover-menu {
    padding: 4px;
    border-radius: 8px;
    border: solid 1px ${color.divider.subtle};
    background: ${color.container.background};
    box-shadow: 0 1px 0 0 rgba(0, 0, 0, 0.05);
    display: flex;
    flex-direction: column;
    gap: 4px;
    min-width: 180px;

    .divider {
      height: 1px;
      background: ${color.divider.subtle};
    }

    button {
      --leo-icon-size: 20px;
      padding: 8px 24px 8px 8px;
      border-radius: 4px;
      display: flex;
      align-items: center;
      gap: 16px;

      &:hover, &.highlight {
        background: ${color.container.highlight};
      }
    }
  }

  .skeleton {
    --self-animation-color: rgba(0, 0, 0, 0.1);

    background: rgba(255, 255, 255, 0.25);
    position: relative;
    overflow: hidden;
    opacity: .7;

    animation: skeleton-fade-in 1s ease-in-out both 250ms;

    @media (prefers-color-scheme: dark) {
      --self-animation-color: rgba(255, 255, 255, 0.1);
    }
  }

  .skeleton:after {
    content: '';
    position: absolute;
    transform: translateX(-100%);
    inset: 0;
    background: linear-gradient(
      90deg, transparent, var(--self-animation-color), transparent);
    animation: skeleton-background-cycle 2s linear 0.5s infinite;
  }

  @keyframes skeleton-fade-in {
    0% { opacity: 0; }
    100% { opacity: .7; }
  }

  @keyframes skeleton-background-cycle {
    0% { transform: translateX(-100%); }
    50% { transform: translateX(100%); }
    100% { transform: translateX(100%); }
  }
`
