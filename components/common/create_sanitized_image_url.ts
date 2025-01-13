// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// This simple string concat is made to a utility so it can be overriden in
// Storybook. The C++ WebUI must add either SanitizedImageSource or
// UntrustedSanitizedImageSource.
export default function createSanitizedImageUrl(imageUrl: string) {
  return `//image?url=${encodeURIComponent(imageUrl)}`
}
