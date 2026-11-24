// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { scoped } from '$web-common/scoped_css'

import blockedImage from '../images/blocked.webp'
import dataImage from '../images/data.webp'
import introImage from '../images/intro.webp'
import shareImage from '../images/share.webp'
import timeImage from '../images/time.webp'

// Desktops and landscape tablets get the wide card.
export const LANDSCAPE_QUERY =
  '(orientation: landscape) and (min-width: 768px) and (min-height: 540px)'

export const style = scoped.css`
  /* Always dark: the page sets data-theme="dark" so Leo's semantic tokens
   * resolve to dark values. */
  & {
    --recap-stage-background: var(--leo-color-primitive-neutral-5);
    --recap-dot-color: var(--leo-color-black);
    --recap-text: var(--leo-color-white);
    --recap-highlight: var(--leo-color-primitive-brands-rorange-2);
    --recap-glass: var(--leo-color-material-thin);
    --recap-glass-thick: var(--leo-color-material-thick);
    /* Leo's material blur tokens have no usable web value yet. */
    --recap-glass-blur: blur(50px);
    --recap-card-background: linear-gradient(
      var(--leo-color-primitive-brands-rorange-1),
      var(--leo-color-primitive-brands-rorange-2),
      var(--leo-color-primitive-brands-rorange-3)
    );
    /* No Leo type role is this large. */
    --recap-hero-size: 85px;
    --recap-card-max-height: 800px;
    --recap-card-min-height: 600px;
    --recap-landscape-max-width: 800px;
    --recap-landscape-action-width: 200px;
    --recap-slide-duration: 8s;
    /* The background photos drift slowly for as long as their slide is up. */
    --recap-drift-duration: calc(
      var(--recap-slide-duration) + var(--leo-duration-l)
    );
    --recap-drift-scale: 1.1;
    /* Slides that wait for the person drift for longer and ease into a stop. */
    --recap-settle-duration: 20s;

    position: relative;
    isolation: isolate;
    box-sizing: border-box;
    min-height: 100%;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: var(--leo-spacing-3xl);
    background: var(--recap-stage-background);
    color: var(--recap-text);
    font: var(--leo-font-default-regular);
    text-wrap: pretty;
  }

  .recap-card {
    position: relative;
    isolation: isolate;
    container-type: inline-size;
    box-sizing: border-box;
    display: flex;
    flex-direction: column;
    --recap-card-height: min(
      var(--recap-card-max-height),
      max(var(--recap-card-min-height), 100dvh - 2 * var(--leo-spacing-3xl))
    );
    height: var(--recap-card-height);
    width: calc(var(--recap-card-height) * 450 / 800);
    /* The width a slide's content column gets. */
    --recap-column-width: 100cqi;
    overflow: hidden;
    border-radius: var(--leo-radius-m);
    background: var(--recap-card-background);
    user-select: none;
    -webkit-touch-callout: none;
  }

  /* Taps navigate, so a double tap or pinch mustn't zoom the page. */
  & {
    touch-action: pan-x pan-y;
  }

  .backgrounds,
  .background,
  .background-image {
    position: absolute;
    inset: 0;
  }

  .backgrounds {
    z-index: -1;
  }

  .background {
    z-index: 0;
    opacity: 0;
    /* Leaving: stay fully visible under the incoming photo until it has faded
     * in, then drop out. */
    transition: opacity 0s linear var(--leo-duration-l);
  }

  .background[data-active] {
    z-index: 1;
    opacity: 1;
    transition: opacity var(--leo-duration-l) var(--leo-easing-out);
  }

  /* The scale leaves more room than the drift uses, so the photo always
   * covers the card. */
  .background-image {
    background: center / cover no-repeat;
    animation: recap-drift var(--recap-drift-duration) var(--leo-easing-linear)
      both;
  }

  .background[data-slide='intro'] .background-image,
  .background[data-slide='share'] .background-image {
    animation-duration: var(--recap-settle-duration);
    animation-timing-function: var(--leo-easing-out);
  }

  .background:not([data-active]) .background-image,
  .backgrounds[data-paused] .background-image {
    animation-play-state: paused;
  }

  @keyframes recap-drift {
    to {
      transform: translate(var(--drift-x), var(--drift-y))
        scale(var(--recap-drift-scale));
    }
  }

  .background[data-slide='intro'] {
    --drift-x: -2.5%;
    --drift-y: -1.5%;
  }

  .background[data-slide='intro'] .background-image {
    background-image: url(${introImage});
  }

  .background[data-slide='blocked'] {
    --drift-x: 2.5%;
    --drift-y: -1.5%;
  }

  .background[data-slide='blocked'] .background-image {
    background-image: url(${blockedImage});
  }

  .background[data-slide='time'] {
    --drift-x: -1.5%;
    --drift-y: 2.5%;
  }

  .background[data-slide='time'] .background-image {
    background-image: url(${timeImage});
  }

  .background[data-slide='data'] {
    --drift-x: 2.5%;
    --drift-y: 1.5%;
  }

  .background[data-slide='data'] .background-image {
    background-image: url(${dataImage});
  }

  .background[data-slide='share'] {
    --drift-x: -1.5%;
    --drift-y: -2.5%;
  }

  .background[data-slide='share'] .background-image {
    background-image: url(${shareImage});
  }

  .progress {
    display: flex;
    gap: var(--leo-spacing-s);
    padding: var(--leo-spacing-xl);
    padding-top: max(var(--leo-spacing-xl), env(safe-area-inset-top));
    background: linear-gradient(
      color-mix(in srgb, var(--leo-color-black) 10%, transparent),
      transparent
    );
  }

  .step {
    flex: 1;
    height: var(--leo-spacing-s);
    overflow: hidden;
    border-radius: var(--leo-radius-2xs);
    background: color-mix(in srgb, var(--leo-color-white) 40%, transparent);
  }

  .fill {
    display: block;
    height: 100%;
    background: var(--leo-color-white);
    transform: scaleX(0);
    transform-origin: left;
  }

  .fill:dir(rtl) {
    transform-origin: right;
  }

  .fill.done {
    transform: none;
  }

  .fill.running {
    animation: recap-step var(--recap-slide-duration) linear forwards;
  }

  .fill[data-paused] {
    animation-play-state: paused;
  }

  @keyframes recap-step {
    to {
      transform: none;
    }
  }

  .slides {
    flex: 1;
    min-height: 0;
    display: grid;
  }

  /* The current and leaving slides share a cell and push together. Transform
   * only: fading would make the frame a backdrop root and drop the glass blur
   * until the animation ends. */
  .slide-frame {
    --recap-inline: 1;
    grid-area: 1 / 1;
    min-height: 0;
    display: flex;
    flex-direction: column;
    /* A transition, not keyframes, so reversing mid-push carries on from
     * where the slide is instead of jumping. */
    transition: transform var(--leo-duration-l) var(--leo-easing-in-out);
  }

  .slide-frame:dir(rtl) {
    --recap-inline: -1;
  }

  .slide-frame[data-motion='enter'][data-direction='forward'] {
    --recap-slide-offset: calc(100% * var(--recap-inline));
  }

  .slide-frame[data-motion='enter'][data-direction='back'] {
    --recap-slide-offset: calc(-100% * var(--recap-inline));
  }

  @starting-style {
    .slide-frame[data-motion='enter'] {
      transform: translateX(var(--recap-slide-offset));
    }
  }

  .slide-frame[data-motion='exit'] {
    pointer-events: none;
  }

  .slide-frame[data-motion='exit'][data-direction='forward'] {
    transform: translateX(calc(-100% * var(--recap-inline)));
  }

  .slide-frame[data-motion='exit'][data-direction='back'] {
    transform: translateX(calc(100% * var(--recap-inline)));
  }

  /* Shared by the slide styles. */
  @keyframes fade-in {
    from {
      opacity: 0;
    }
  }

  @keyframes recap-panel-in {
    from {
      opacity: 0;
      transform: translateY(var(--recap-panel-from));
    }
  }

  @keyframes recap-card-in {
    from {
      opacity: 0;
      transform: scale(0.92);
    }
  }

  .help {
    max-width: 240px;
    font: var(--leo-font-default-regular);
  }

  .bottom-bar {
    display: flex;
    align-items: flex-end;
    gap: var(--leo-spacing-m);
    padding: var(--leo-spacing-2xl);
    padding-bottom: max(var(--leo-spacing-2xl), env(safe-area-inset-bottom));
    /* Leo has no light "on media" button kind, so the filled buttons here are
     * re-themed to white with black content. */
    --leo-color-button-background: var(--leo-color-white);
    --leo-color-schemes-on-primary: var(--leo-color-black);
  }

  .bottom-bar > leo-button {
    flex: 1;
  }

  .bottom-bar > leo-tooltip,
  .bottom-bar > .icon-button {
    flex: 0 0 auto;
  }

  /* Hidden once a press is clearly a hold, shown again as soon as it ends. */
  .progress,
  .bottom-bar {
    transition: opacity var(--leo-duration-s) var(--leo-easing-out);
  }

  .recap-card[data-holding] .progress,
  .recap-card[data-holding] .bottom-bar {
    opacity: 0;
    pointer-events: none;
    transition: opacity var(--leo-duration-m) var(--leo-easing-out)
      var(--recap-hold-delay);
  }

  /* On desktops and tablets the card floats on the stage, so on arrival the
   * whole card fades in and grows into place. */
  @media (min-width: 601px) {
    .recap-card {
      animation: recap-card-in var(--leo-duration-l) var(--leo-easing-out) both;
    }
  }

  /* Phones and narrow windows: the card takes the whole view. */
  @media (max-width: 600px) {
    & {
      padding: 0;
    }

    .recap-card {
      width: 100%;
      height: 100dvh;
      border-radius: 0;
    }
  }

  @media ${LANDSCAPE_QUERY} {
    .recap-card {
      --recap-card-width: min(
        var(--recap-landscape-max-width),
        100vw - 2 * var(--leo-spacing-3xl),
        (100dvh - 2 * var(--leo-spacing-3xl)) * 16 / 10
      );
      --recap-column-width: 50cqi;
      width: var(--recap-card-width);
      height: calc(var(--recap-card-width) * 10 / 16);
    }

    .bottom-bar {
      justify-content: space-between;
    }

    .bottom-bar > leo-button {
      flex: 0 1 auto;
      min-width: var(--recap-landscape-action-width);
    }
  }

  /* Slides change in place and the photos stay still; the crossfade, which
   * doesn't move anything, is kept. */
  @media (prefers-reduced-motion: reduce) {
    .slide-frame {
      transition: none;
    }

    .background-image {
      animation: none;
    }
  }

  @media (min-width: 601px) and (prefers-reduced-motion: reduce) {
    .recap-card {
      animation: fade-in var(--leo-duration-m) var(--leo-easing-out) both;
    }
  }
`

// Slides have their own scopes, so these reach into them.
style.passthrough.css`
  .slide {
    flex: 1;
    min-height: 0;
    /* Hidden, not auto, so content sliding in doesn't flash a scrollbar. */
    overflow-x: hidden;
    overflow-y: auto;
    display: flex;
    flex-direction: column;
    justify-content: center;
    gap: var(--leo-spacing-5xl);
    cursor: pointer;
  }

  .slide-intro .slide,
  .slide-empty .slide {
    gap: 0;
  }

  .slide-share .slide {
    gap: var(--leo-spacing-s);
  }

  .slide-share .slide,
  .slide-empty .slide {
    cursor: default;
  }

  .slide h2,
  .slide p {
    margin: 0;
  }

  .visually-hidden {
    position: absolute;
    width: 1px;
    height: 1px;
    overflow: hidden;
    clip-path: inset(50%);
    white-space: nowrap;
  }

  /* On the wide card, each slide splits into two columns. */
  @media ${LANDSCAPE_QUERY} {
    .recap-card .slide {
      flex-direction: row;
      align-items: safe center;
      justify-content: normal;
      gap: 0;
    }

    .slide > * {
      flex: 1 1 0;
      min-width: 0;
    }
  }
`
