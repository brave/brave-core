// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { scoped } from '$web-common/scoped_css'
import { LANDSCAPE_QUERY } from './app.style'

export const style = scoped.css`
  .share {
    margin-inline: var(--leo-spacing-2xl);
    padding: var(--leo-spacing-m);
    border-radius: var(--leo-radius-xxl);
    background: var(--recap-glass-thick);
    backdrop-filter: var(--recap-glass-blur);
  }

  .card-preview {
    aspect-ratio: 336 / 408;
    overflow: hidden;
    border-radius: var(--leo-radius-xl);
    background: var(--recap-card-background);
    box-shadow: var(--leo-effect-elevation-02);
  }

  .card-preview img {
    display: block;
    width: 100%;
    height: 100%;
    animation: recap-preview-in var(--leo-duration-m) var(--leo-easing-out);
  }

  @keyframes recap-preview-in {
    from {
      opacity: 0;
      transform: scale(0.97);
    }
  }

  .share-actions {
    display: flex;
    flex-direction: column;
    gap: var(--leo-spacing-m);
    padding: var(--leo-spacing-xl);
    --leo-button-color: var(--leo-color-white);
  }

  .share-hint {
    font: var(--leo-font-small-regular);
    text-align: center;
  }

  .share-actions > * {
    animation: recap-action-in var(--leo-duration-m) var(--leo-easing-out) both;
  }

  .share-actions > :nth-child(2) {
    animation-delay: var(--leo-duration-xs);
  }

  .share-actions > :nth-child(3) {
    animation-delay: calc(2 * var(--leo-duration-xs));
  }

  @keyframes recap-action-in {
    from {
      opacity: 0;
      transform: translateY(var(--leo-spacing-s));
    }
  }

  @media (prefers-reduced-motion: no-preference) {
    .slide-frame[data-motion='enter'] & .share-actions > * {
      animation-delay: var(--leo-duration-l);
    }

    .slide-frame[data-motion='enter'] & .share-actions > :nth-child(2) {
      animation-delay: calc(var(--leo-duration-l) + var(--leo-duration-xs));
    }

    .slide-frame[data-motion='enter'] & .share-actions > :nth-child(3) {
      animation-delay: calc(var(--leo-duration-l) + 2 * var(--leo-duration-xs));
    }
  }

  @media ${LANDSCAPE_QUERY} {
    /* The image fills the height of its column, with the post buttons beside
     * it. Only the image sits on glass. */
    .share {
      align-self: stretch;
      display: grid;
      grid-template-columns: 1fr 1fr;
      grid-template-rows: minmax(0, 1fr);
      align-items: center;
      margin: 0;
      padding: 0;
      border-radius: 0;
      background: none;
      backdrop-filter: none;
    }

    .share-frame {
      /* Content-box, so the ratio is the image's and the glass goes around it. */
      box-sizing: content-box;
      justify-self: center;
      height: calc(100% - 2 * var(--leo-spacing-m));
      aspect-ratio: 336 / 408;
      padding: var(--leo-spacing-m);
      border-radius: var(--leo-radius-xxl);
      background: var(--recap-glass-thick);
      backdrop-filter: var(--recap-glass-blur);
    }

    .share-actions {
      gap: var(--leo-spacing-xl);
      padding: 0;
      padding-inline-end: var(--leo-spacing-2xl);
      /* Filled black, like the card's own call to action. Leo's filled kind
       * keeps its content white whenever the button color is set. */
      --leo-button-color: var(--leo-color-black);
    }
  }

  @media (prefers-reduced-motion: reduce) {
    .card-preview img {
      animation: fade-in var(--leo-duration-m) var(--leo-easing-out);
    }

    .share-actions > * {
      animation: none;
    }
  }
`
