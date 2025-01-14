// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {RegisterPolymerComponentToIgnore} from 'chrome://resources/brave/polymer_overriding.js'

// Replacing chromium polymer components with subclasses of them is a
// 2-step process:
// 1. Make sure we ignore the chromium components when they are defined
// 2. Override the chromium components with a subclass and define the
//    components with their original chromium name.
// (This is because the chomium components define themselves via
// customElements.define in their module, so we want to register to ignore the
// component before the module is imported).

RegisterPolymerComponentToIgnore('add-site-dialog')
RegisterPolymerComponentToIgnore('settings-autofill-page')
RegisterPolymerComponentToIgnore('settings-clear-browsing-data-dialog')
RegisterPolymerComponentToIgnore('settings-search-page')
RegisterPolymerComponentToIgnore('settings-site-settings-page')
