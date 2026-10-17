// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { showElementPicker } from './element_picker'
import { ChromiumElementPickerAPI } from './element_picker_api_chromium'

showElementPicker(new ChromiumElementPickerAPI())
