/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { componentStyle, css } from '../lib/component_style'

export const ModalStyle = componentStyle('modal', css`
  & {
    --self-animation-duration: 200ms;
    --modal-header-padding-bottom: 32px;

    @container style(--is-narrow-view) {
      --modal-header-padding-bottom: 24px;
    }
  }

  .modal-skip-animations & {
    --self-animation-duration: 0;
  }

  .modal-backdrop {
    position: fixed;
    top: 0;
    left: 0;
    bottom: 0;
    right: 0;
    overflow: auto;
    background: rgba(0, 0, 0, 0.15);
    backdrop-filter: blur(10px);
    z-index: 9999;
    display: flex;
    flex-direction: column;
    align-items: center;

    animation-name: modal-backdrop-fade;
    animation-timing-function: ease-out;
    animation-duration: var(--self-animation-duration);
    animation-fill-mode: both;
  }

  @keyframes modal-backdrop-fade {
    from {
      background: rgba(0, 0, 0, 0);
      backdrop-filter: blur(0);
    }
    to {
      background: rgba(0, 0, 0, 0.15);
      backdrop-filter: blur(10px);
    }
  }

  .top-spacer {
    flex: 45 0 auto;
  }

  .modal-content {
    position: relative;
    flex: 0 0 auto;
    border-radius: 16px;
    padding: 32px;
    margin: 0 12px;
    background: ${color.container.background};
    animation-name: modal-content-fade-in;
    animation-timing-function: ease-out;
    animation-duration: var(--self-animation-duration);
    animation-fill-mode: both;

    @container style(--is-narrow-view) {
      margin: 0;
      border-radius: 16px 16px 0 0;
      padding: 16px;
      animation-name: modal-content-slide-in;
    }
  }

  @keyframes modal-content-slide-in {
    from { transform: translate(0, calc(100% + 20px)); }
    to { transform: translate(0, 0); }
  }

  @keyframes modal-content-fade-in {
    from { opacity: 0; }
    to { opacity: 1; }
  }

  .bottom-spacer {
    flex: 55 0 auto;

    @container style(--is-narrow-view) {
      flex: 0 0 0;
    }
  }
`)

export const CloseButtonStyle = componentStyle('modal-close', css`
  & {
    color: ${color.icon.default};
    --leo-icon-size: 24px;
  }

  button {
    margin: 0;
    padding: 6px;
    background: none;
    border: none;
    cursor: pointer;
  }

  button:hover {
    background: color-mix(in srgb, ${color.button.background} 5% , transparent);
    border-radius: 6px;
  }

  button:active {
    background: color-mix(in srgb, ${color.button.background} 9% , transparent);
    border-radius: 6px;
  }

  button:disabled {
    color: ${color.icon.disabled};
    background: none;
    cursor: default;
  }
`)

export const HeaderStyle = componentStyle('modal-header', css`
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
    position: absolute;
    top: 26px;
    right: 26px;

    @container style(--is-narrow-view) {
      top: 20px;
      right: 10px;
    }
  }
`)

export const ActionsStyle = componentStyle('modal-actions', css`
  & {
    display: flex;
    gap: 16px;
    margin-top: 32px;

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
`)
