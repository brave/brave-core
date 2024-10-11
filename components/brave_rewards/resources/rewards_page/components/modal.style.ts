/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../lib/scoped_css'

export const modalStyle = scoped.css`
  & {
    --self-animation-duration: 250ms;
    --modal-header-padding-bottom: 32px;
    --modal-padding: 32px;

    border: none;
    border-radius: 16px;
    padding: var(--modal-padding);
    background: var(--modal-background, ${color.container.background});
    color: ${color.text.primary};
    outline: none;

    display: flex;
    flex-direction: column;
    align-items: stretch;

    animation-name: modal-content-fade-in;
    animation-timing-function: ease-out;
    animation-duration: var(--self-animation-duration);
    animation-fill-mode: both;

    @container style(--is-narrow-view) {
      --modal-header-padding-bottom: 24px;
      --modal-padding: 16px;

      animation-name: modal-content-slide-in;
      width: 100%;
      max-width: 100%;
      max-height: calc(100% - 12px);
      border-radius: 16px 16px 0 0;
      margin: auto 0 0 0;
    }
  }

  @keyframes modal-content-fade-in {
    from { opacity: 0; }
    to { opacity: 1; }
  }

  @keyframes modal-content-slide-in {
    from { transform: translate(0, 100%); }
    to { transform: translate(0, 0); }
  }

  &::backdrop {
    background: rgba(0, 0, 0, 0.15);
    backdrop-filter: blur(8px);

    animation-name: modal-backdrop-fade;
    animation-timing-function: ease-out;
    animation-duration: var(--self-animation-duration);
    animation-fill-mode: both;
  }

  .modal-skip-animations &::backdrop {
    animation-duration: 0ms;
  }

  @keyframes modal-backdrop-fade {
    from {
      background: rgba(0, 0, 0, 0);
      backdrop-filter: blur(0);
    }
    to {
      background: rgba(0, 0, 0, 0.15);
      backdrop-filter: blur(8px);
    }
  }
`

export const headerStyle = scoped.css`
  & {
    padding-bottom: var(--modal-header-padding-bottom);
    padding-right: 42px;

    @container style(--is-narrow-view) {
      padding-top: 8px;
    }
  }

  .title {
    font: ${font.heading.h2};
    color: ${color.text.primary};

    @container style(--is-narrow-view) {
      font: ${font.heading.h3};
    }
  }

  .close {
    --leo-icon-size: 24px;

    position: absolute;
    inset-block-start: 26px;
    inset-inline-end: 26px;
    color: ${color.icon.default};

    @container style(--is-narrow-view) {
      inset-block-start: 20px;
      inset-inline-end: 10px;
    }

    button {
      margin: 0;
      padding: 6px;
      background: none;
      border: none;
      cursor: pointer;
    }

    button:hover {
      background:
        color-mix(in srgb, ${color.button.background} 5% , transparent);
      border-radius: 6px;
    }

    button:active {
      background:
        color-mix(in srgb, ${color.button.background} 9% , transparent);
      border-radius: 6px;
    }

    button:disabled {
      color: ${color.icon.disabled};
      background: none;
      cursor: default;
    }
  }
`

export const actionsStyle = scoped.css`
  & {
    display: flex;
    gap: 16px;
    margin-top: 24px;
    justify-content: flex-end;

    @container style(--is-wide-view) {
      leo-button:only-of-type {
        flex-grow: 0;
        min-width: 134px;
      }
    }

    @container style(--is-narrow-view) {
      margin-top: 16px;
      flex-direction: column;
    }
  }

  & > * {
    order: 1;
  }

  @container style(--is-narrow-view) {
    & > .primary-action {
      order: 0;
    }
  }
`
