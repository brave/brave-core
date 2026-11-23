// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { scoped } from '$web-common/scoped_css'

// Each dot size is a still mask over a moving fill, so the animation is all
// transforms and runs on the compositor.
export const style = scoped.css`
  & {
    --recap-dot-pitch: 12px;
    /* How far the clusters reach past the page, so their drift never shows
     * an edge. */
    --recap-dot-bleed: 200px;
    --recap-shimmer-width: 70cqw;
    /* About 0.17 of a pixel across for every pixel down. */
    --recap-shimmer-slant: -9.65deg;

    position: absolute;
    inset: 0;
    z-index: -1;
    overflow: hidden;
    container-type: size;
    pointer-events: none;
    background: radial-gradient(
        circle,
        var(--recap-dot-color) 0.3px,
        transparent 0.9px
      )
      0 0 / var(--recap-dot-pitch) var(--recap-dot-pitch);
  }

  .dots {
    position: absolute;
    inset: 0;
    mask: radial-gradient(
        circle,
        #000 calc(var(--dot-radius) - 0.5px),
        transparent calc(var(--dot-radius) + 0.5px)
      )
      0 0 / var(--recap-dot-pitch) var(--recap-dot-pitch);
  }

  /* Larger dots only show nearer the middle of a cluster. */
  .dots.medium {
    --dot-radius: 1.2px;
    --cluster-core: 25%;
    --cluster-edge: 100%;
  }

  .dots.large {
    --dot-radius: 1.8px;
    --cluster-core: 15%;
    --cluster-edge: 65%;
  }

  /* Two overlapping tilings of different sizes, so the pattern doesn't read
   * as a grid. */
  .clusters {
    --cluster: radial-gradient(
      closest-side,
      #000 var(--cluster-core),
      transparent var(--cluster-edge)
    );
    position: absolute;
    inset: calc(-1 * var(--recap-dot-bleed));
    background: var(--recap-dot-color);
    mask:
      var(--cluster) 0 0 / 640px 520px,
      var(--cluster) 300px 200px / 980px 760px;
    animation: recap-dot-drift 24s var(--leo-easing-in-out) infinite alternate;
  }

  @keyframes recap-dot-drift {
    from {
      transform: translate(-160px, -120px);
    }
    to {
      transform: translate(160px, 120px);
    }
  }

  /* Rests off the left of the page, and sweeps across over the first part of
   * each cycle. */
  .shimmer {
    position: absolute;
    inset-block: 0;
    left: var(--recap-dot-bleed);
    width: var(--recap-shimmer-width);
    transform-origin: 0 var(--recap-dot-bleed);
    transform: translateX(calc(-1 * var(--recap-shimmer-width)))
      skewX(var(--recap-shimmer-slant));
    background: linear-gradient(
      to right,
      transparent,
      color-mix(in srgb, var(--recap-highlight) 25%, transparent) 25%,
      color-mix(in srgb, var(--recap-highlight) 50%, transparent),
      color-mix(in srgb, var(--recap-highlight) 25%, transparent) 75%,
      transparent
    );
    animation: recap-dot-shimmer 9s var(--leo-easing-in-out) infinite;
  }

  @keyframes recap-dot-shimmer {
    55%,
    100% {
      transform: translateX(calc(100cqw + 17cqh))
        skewX(var(--recap-shimmer-slant));
    }
  }

  @media (prefers-reduced-motion: reduce) {
    .clusters,
    .shimmer {
      animation: none;
    }
  }
`
