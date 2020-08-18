// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// Note that we cannot predict the order that
// modules are executed in. It will be different between non-optimized
// (the browser controls which es module is downloaded and executed,
// where it is not consistant due to "network" responses) and
// optimized (rollup controls the order in which modules are executed).

import './about_page.js'
import './appearance_page.js'
import './basic_page.js'
import './clear_browsing_data_dialog.js'
import './cookies_page.js'
import './default_browser_page.js'
import './icons.js'
import './import_data_dialog.js'
import './page_visibility.js'
import './payments_section.js'
import './people_page.js'
import './security_page.js'
import './settings_menu.js'
import './settings_section.js'
import './settings_ui.js'
import './site_settings_page.js'
import './sync_account_control.js'
import './sync_controls.js'
