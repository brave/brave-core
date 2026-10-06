// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { showElementPicker } from '//brave/components/cosmetic_filters/resources/data/element_picker.js'
import { elementPickerTemplate } from '//brave/components/cosmetic_filters/resources/data/element_picker_template.js'
import { WebKitElementPickerAPI } from '//brave/ios/browser/brave_shields/element_picker/resources/element_picker_api_webkit.js'

showElementPicker(new WebKitElementPickerAPI(), elementPickerTemplate)
