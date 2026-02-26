/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped, addStyles } from '$web-common/scoped_css'

export const style = scoped.css`
  h4 {
    display: flex;
    align-items: center;
    justify-content: space-between;

    &:empty {
      display: none;
    }
  }

  .banner {
    display: flex;
    border-radius: 12px;
    overflow: hidden;

    img {
      width: 100%;
      height: auto;
    }
  }

  section:empty {
    display: none;
  }

  section a {
    display: flex;
    gap: 16px;
    text-decoration: none;
    color: ${color.text.primary};
    padding: 8px;
    border-bottom: solid 1px ${color.divider.subtle};

    &:last-child {
      border-bottom: none;
    }
  }

  .thumbnail {
    flex: 0 0 56px;
    height: 56px;
    width: 56px;
    overflow: hidden;
    border-radius: 12px;
    border: solid 1px ${color.divider.subtle};
    display: flex;
    align-items: center;
    justify-content: center;

    img {
      width: 100%;
      height: auto;
    }

    .favicon {
      width: 24px;
      height: auto;
      border-radius: 4px;
    }
  }

  .item-info {
    display: flex;
    flex-direction: column;

    .title {
      font: ${font.default.semibold};
    }

    .description {
      font: ${font.xSmall.regular};
      color: ${color.text.tertiary};
    }
  }

  &.deep-link-highlight {
    position: relative;

    &::before {
      content: "";
      position: absolute;
      inset: 0;
      border-radius: 16px;
      padding: 2px;
      background:
        conic-gradient(
          from var(--conic-angle),
          rgba(255, 255, 255, 0) 0%,
          rgb(255, 255, 255) var(--conic-stop-1),
          rgb(255, 255, 255) var(--conic-stop-2),
          rgba(255, 255, 255, 0.5) var(--conic-stop-3),
          rgba(255, 255, 255, 0) 100%
        ),
        linear-gradient(279deg, #fa7250 3%, #ff1893 40%, #a78aff 99%)
          border-box;
      mask:
        linear-gradient(#fff 0 0) content-box,
        linear-gradient(#fff 0 0);
      mask-composite: exclude;
      opacity: 0;
      animation: highlight-border 1.2s ease-in-out forwards;
      pointer-events: none;
    }

    &::after {
      content: "";
      position: absolute;
      inset: 0;
      border-radius: 16px;
      background:
        linear-gradient(
          288deg,
          rgba(255, 255, 255, 0.05) var(--linear-stop-1),
          rgba(255, 255, 255, 0.05) var(--linear-stop-2),
          rgba(250, 114, 80, 0.05) var(--linear-stop-3),
          rgba(255, 24, 147, 0.05) var(--linear-stop-4),
          rgba(167, 138, 255, 0.05) var(--linear-stop-5),
          rgba(255, 255, 255, 0.05) var(--linear-stop-6)
        );
      background-position-x: 38%;
      background-repeat: no-repeat;
      background-size: 200% 100%;
      opacity: 0;
      animation: highlight-content 1.2s ease-in-out forwards;
      pointer-events: none;
    }

    @media (prefers-color-scheme: dark) {
      &::before {
        background:
          conic-gradient(
            from var(--conic-angle),
            rgba(37, 37, 41, 0) 0%,
            rgb(37, 37, 41) var(--conic-stop-1),
            rgb(37, 37, 41) var(--conic-stop-2),
            rgba(37, 37, 41, 0.5) var(--conic-stop-3),
            rgba(37, 37, 41, 0) 100%
          ),
          linear-gradient(279deg, #fa7250 3%, #ff1893 40%, #a78aff 99%)
            border-box;
      }

      &::after {
        background:
          linear-gradient(
            288deg,
            rgba(37, 37, 41, 0.05) var(--linear-stop-1),
            rgba(37, 37, 41, 0.05) var(--linear-stop-2),
            rgba(250, 114, 80, 0.05) var(--linear-stop-3),
            rgba(255, 24, 147, 0.05) var(--linear-stop-4),
            rgba(167, 138, 255, 0.05) var(--linear-stop-5),
            rgba(37, 37, 41, 0.05) var(--linear-stop-6)
          );
        background-position-x: 38%;
        background-repeat: no-repeat;
        background-size: 200% 100%;
      }
    }
  }
`

// CSS properties supporting card highlight animation.
addStyles(
  'card-view-highlight-animation-properties',
  `
  @property --conic-angle {
    syntax: "<angle>";
    inherits: false;
    initial-value: 90deg;
  }

  @property --conic-stop-1 {
    syntax: "<percentage>";
    inherits: false;
    initial-value: 10%;
  }

  @property --conic-stop-2 {
    syntax: "<percentage>";
    inherits: false;
    initial-value: 87%;
  }

  @property --conic-stop-3 {
    syntax: "<percentage>";
    inherits: false;
    initial-value: 89%;
  }

  @property --linear-stop-1 {
    syntax: "<percentage>";
    inherits: false;
    initial-value: 0%;
  }

  @property --linear-stop-2 {
    syntax: "<percentage>";
    inherits: false;
    initial-value: 43%;
  }

  @property --linear-stop-3 {
    syntax: "<percentage>";
    inherits: false;
    initial-value: 64%;
  }

  @property --linear-stop-4 {
    syntax: "<percentage>";
    inherits: false;
    initial-value: 82%;
  }

  @property --linear-stop-5 {
    syntax: "<percentage>";
    inherits: false;
    initial-value: 92%;
  }

  @property --linear-stop-6 {
    syntax: "<percentage>";
    inherits: false;
    initial-value: 100%;
  }

  @keyframes highlight-border {
    0% {
      --conic-angle: 90deg;
      --conic-stop-1: 10%;
      --conic-stop-2: 87%;
      --conic-stop-3: 89%;
      opacity: 0;
    }

    25% {
      --conic-angle: 134deg;
      --conic-stop-1: 10%;
      --conic-stop-2: 87%;
      --conic-stop-3: 89%;
      opacity: 1;
    }

    50% {
      --conic-angle: 270deg;
      --conic-stop-1: 10%;
      --conic-stop-2: 47%;
      --conic-stop-3: 61%;
      opacity: 1;
    }

    75% {
      --conic-angle: 321deg;
      --conic-stop-1: 28%;
      --conic-stop-2: 88%;
      --conic-stop-3: 93%;
      opacity: 1;
    }

    to {
      --conic-angle: 450deg;
      --conic-stop-1: 10%;
      --conic-stop-2: 87%;
      --conic-stop-3: 89%;
      opacity: 0;
    }
  }

  @keyframes highlight-content {
    0% {
      --linear-stop-1: 0%;
      --linear-stop-2: 43%;
      --linear-stop-3: 64%;
      --linear-stop-4: 82%;
      --linear-stop-5: 92%;
      --linear-stop-6: 100%;
      background-position-x: 38%;
      opacity: 0;
    }

    25% {
      --linear-stop-1: 0%;
      --linear-stop-2: 43%;
      --linear-stop-3: 64%;
      --linear-stop-4: 82%;
      --linear-stop-5: 92%;
      --linear-stop-6: 100%;
      background-position-x: 38%;
      opacity: 1;
    }

    50% {
      --linear-stop-1: 0%;
      --linear-stop-2: 13%;
      --linear-stop-3: 22%;
      --linear-stop-4: 42%;
      --linear-stop-5: 54%;
      --linear-stop-6: 68%;
      background-position-x: -38%;
      opacity: 1;
    }

    75% {
      --linear-stop-1: 0%;
      --linear-stop-2: 13%;
      --linear-stop-3: 22%;
      --linear-stop-4: 42%;
      --linear-stop-5: 54%;
      --linear-stop-6: 68%;
      background-position-x: -100%;
      opacity: 1;
    }

    to {
      --linear-stop-1: 0%;
      --linear-stop-2: 13%;
      --linear-stop-3: 22%;
      --linear-stop-4: 42%;
      --linear-stop-5: 54%;
      --linear-stop-6: 68%;
      background-position-x: -100%;
      opacity: 0;
    }
  }
`,
)
