// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import '../../../../browser/resources/settings/email_aliases_page/email_aliases'
import { RemoteMappingService } from '../../../../browser/resources/settings/email_aliases_page/remote_mapping_service'

const root = document.getElementById('email-aliases-bubble-root')!
root.attachShadow({ mode: 'open' })
; (window as any).mountModal(root.shadowRoot, new RemoteMappingService())
