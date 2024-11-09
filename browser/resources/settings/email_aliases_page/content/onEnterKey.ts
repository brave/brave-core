// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { InputEventDetail } from "@brave/leo/react/input";
import { KeyboardEvent } from "react";

const onEnterKeyForDiv = (onSubmit: () => void) => (e: KeyboardEvent) => {
  if (e.key === 'Enter') {
    onSubmit();
  }
}

const onEnterKeyForInput = (onSubmit: () => void) => (e: InputEventDetail) => {
  const innerEvent = e.innerEvent as unknown as KeyboardEvent;
  return onEnterKeyForDiv(onSubmit)(innerEvent);
}

export { onEnterKeyForInput, onEnterKeyForDiv };