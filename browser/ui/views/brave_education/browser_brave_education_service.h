/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_EDUCATION_BROWSER_BRAVE_EDUCATION_SERVICE_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_EDUCATION_BROWSER_BRAVE_EDUCATION_SERVICE_H_

namespace user_education {
class TutorialRegistry;
}  // namespace user_education

void MaybeRegisterBraveTutorials(user_education::TutorialRegistry& registry);

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_EDUCATION_BROWSER_BRAVE_EDUCATION_SERVICE_H_
