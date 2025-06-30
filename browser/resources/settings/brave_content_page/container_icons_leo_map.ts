// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Icon } from '../containers.mojom-webui.js'

export default new Map<Icon, string>([
  [Icon.kPersonal, 'container-personal'],
  [Icon.kWork, 'container-work'],
  [Icon.kShopping, 'container-shopping'],
  [Icon.kSocial, 'container-social'],
  [Icon.kEvents, 'container-events'],
  [Icon.kBanking, 'container-banking'],
  [Icon.kStar, 'container-star'],
  [Icon.kTravel, 'container-travel'],
  [Icon.kSchool, 'container-school'],
  [Icon.kPrivate, 'container-private'],
  [Icon.kMessaging, 'container-messaging'],
]);
