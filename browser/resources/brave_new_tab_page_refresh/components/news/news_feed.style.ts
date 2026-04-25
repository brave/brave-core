/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    margin: 0 30px 30px;
  }
`

style.passthrough.css`
  @keyframes news-control-fade {
    from {
      opacity: 0;
      visibility: hidden;
    }
    80% {
      opacity: 0;
      visibility: visible;
    }
    to {
      opacity: 1;
      visibility: visible;
    }
  }

  .brave-news-feed-controls, .brave-news-load-new-content-button {
    animation: linear news-control-fade both;
    animation-timeline: scroll();
    animation-range: 0px 100vh;
  }
`
