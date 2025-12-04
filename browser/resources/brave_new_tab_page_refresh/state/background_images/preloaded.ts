/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { BraveBackground } from '../background_state'
import * as preloadedData from './preloaded.json'

// Pre-loaded background image resources that can be used if a new profile opens
// the NTP before the NTPBackgroundImagesService has finished loading the
// current collection of Brave backgrounds.
export const preloadedBackgrounds: BraveBackground[] = preloadedData
