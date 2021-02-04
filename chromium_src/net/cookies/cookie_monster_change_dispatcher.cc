/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/cookies/cookie_monster.h"

#define CookieMonster ChromiumCookieMonster
#include "../../../../net/cookies/cookie_monster_change_dispatcher.cc"
#undef CookieMonster
