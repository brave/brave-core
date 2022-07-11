/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  width: 410px;
  padding: 13px;
  font-family: var(--brave-font-heading);

  /* Expand the height of the panel for tall overlays. See
     |panel_overlays.tsx|. */

  .panel-overlay-rewards-tour & {
    min-height: 554px;
  }

  .panel-overlay-grant-captcha & {
    min-height: 576px;
  }

  /* The Brave Talk opt-in is a special-case: for historical reasons it's not
     presented as an overlay. Instead it should replace the panel content. */

  .panel-overlay-brave-talk-opt-in & {
    padding: 0;

    .rewards-panel {
      display: none;
    }
  }
`

export const loading = styled.div`
  height: 380px;
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--brave-color-brandBat);

  .icon {
    flex: 0 0 auto;
    height: 32px;
    width: auto;

    animation-name: fade-in;
    animation-delay: 1s;
    animation-duration: .5s;
    animation-fill-mode: both;

    @keyframes fade-in {
      from { opacity: 0; }
      to { opacity: .8; }
    }
  }
`
