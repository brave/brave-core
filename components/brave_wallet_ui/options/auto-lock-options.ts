// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { AutoLockOption } from "../constants/types";

export const autoLockOptions: AutoLockOption[] = [
  {
    label: '5 minutes',
    value: 5
  },
  {
    label: '10 minutes',
    value: 10
  },
  {
    label: '1 hour',
    value: 60
  },
  {
    label: '3 hours',
    value: 180
  }
]
