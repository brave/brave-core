// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Import web components here. They will be available on the page
// as <leo-{component}></leo-{component}>.
import '@brave/leo/web-components/button'
import '@brave/leo/web-components/dropdown'
import { setIconBasePath } from '@brave/leo/web-components/icon'
import iconsMeta from '@brave/leo/icons/meta'

setIconBasePath('//resources/brave-icons')

window['leoIcons'] = new Set(Object.keys(iconsMeta.icons))
