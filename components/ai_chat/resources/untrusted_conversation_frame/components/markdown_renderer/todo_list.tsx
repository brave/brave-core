// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Checkbox from "@brave/leo/react/checkbox";
import * as React from "react";

export const createLiClickHandler: (offset: number | undefined, text: string) => React.MouseEventHandler = (offset: number, text: string) => (e) => {
  // Note: Unfortunately we need to handle the checkboxes checking here
  // as the generated markdown doesn't generate this as the label.
  // We also need to call `preventDefault` so when the user clicks the checkbox
  // we don't instantaneously check and uncheck it.
  const checkBox = e.currentTarget?.querySelector<
    HTMLElement & { checked: boolean }
  >('leo-checkbox')
  if (!checkBox) return

  e.preventDefault()
  checkBox.checked = !checkBox.checked

  if (offset === undefined) return

  // Check or uncheck the checkbox part of the string
  const modified = text.slice(0, offset + 3)
  + (checkBox.checked ? 'x' : ' ')
  + text.slice(offset + 4)
  console.log(modified)
}

export const checkboxRenderer = (props: any) => {
  if (props.type !== 'checkbox') {
    return null
  }

  return <Checkbox checked={props.checked} />
}
