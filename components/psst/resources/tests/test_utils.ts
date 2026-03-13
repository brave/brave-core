// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { fireEvent } from '@testing-library/react'

/** Clicks a Leo web component button (button is inside shadowRoot) */
export const clickLeoButton = (button: Element) => {
  fireEvent.click(button.shadowRoot?.querySelector('button') as Element)
}
