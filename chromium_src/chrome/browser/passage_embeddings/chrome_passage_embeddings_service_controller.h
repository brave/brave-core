/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PASSAGE_EMBEDDINGS_CHROME_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PASSAGE_EMBEDDINGS_CHROME_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_

// Inject `friend class BravePassageEmbeddingsServiceController` into the
// private section so Brave's subclass can invoke Chrome's private
// constructor and destructor. The friendship travels through the
// `InitializeCpuLogger` anchor: we wrap its declaration with a
// dummy member + friend + a fresh `private:` to preserve the
// original access specifier.
#define InitializeCpuLogger                             \
  Unused_brave_friend() {}                              \
                                                        \
 public:                                                \
  friend class BravePassageEmbeddingsServiceController; \
                                                        \
 private:                                               \
  void InitializeCpuLogger

#include <chrome/browser/passage_embeddings/chrome_passage_embeddings_service_controller.h>  // IWYU pragma: export

#undef InitializeCpuLogger

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PASSAGE_EMBEDDINGS_CHROME_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_
