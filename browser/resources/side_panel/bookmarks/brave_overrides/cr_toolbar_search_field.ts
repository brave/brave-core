// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { CrToolbarSearchFieldElement } from '//resources/cr_elements/cr_toolbar/cr_toolbar_search_field.js'

import { css } from '//resources/lit/v3_0/lit.rollup.js'
import { injectStyle } from '//resources/brave/lit_overriding.js'

injectStyle(CrToolbarSearchFieldElement, css`
    :host {
      outline: 1px solid var(--leo-color-divider-subtle);
    }
`)
