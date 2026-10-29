// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
    gap: var(--leo-spacing-xl);
    padding: var(--leo-spacing-2xl);
    font: var(--leo-font-default-regular);
  }

  h1 {
    font: var(--leo-font-heading-h2);
    margin: 0;
  }

  .subtitle {
    color: var(--leo-color-text-secondary);
    font: var(--leo-font-small-regular);
    margin-top: var(--leo-spacing-s);
  }

  .toolbar {
    display: flex;
    align-items: center;
    gap: var(--leo-spacing-m);
    flex-wrap: wrap;
  }

  .search {
    flex: 1 1 240px;
    min-width: 180px;
  }

  .level-filter,
  .source-filter {
    flex: 0 0 160px;
  }

  .live-tail {
    margin-inline-start: auto;
  }

  .count {
    color: var(--leo-color-text-tertiary);
    font: var(--leo-font-small-regular);
  }
`
