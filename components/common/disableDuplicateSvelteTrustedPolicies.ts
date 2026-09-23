// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// This fixes the
// `Refused to create a TrustedTypePolicy named 'svelte-trusted-html'
// because a policy with that name already exists and the Content
// Security Policy directive does not 'allow-duplicates'` error.

// On some pages (like chrome://settings) we generate multiple bundles
// with their own copies of @brave/leo. Each of these bundles will
// attempt to register their own `svelte-trusted-html` policy.

// To workaround this, we disable the creation of a second
// `svelte-trusted-html` policy by importing this file which returns
// undefined. This is safe because we don't register Web Components
// more than once, so the inner page will use the components registered
// by the outer page (which have a valid `svelte-trusted-html` policy).

// This is a bit of a hack but the proper fix is to vendor Svelte and serve
// it from chrome://resources so all bundles can share the same copy.

// TODO(https://github.com/brave/brave-browser/issues/55656): share Svelte
// so we only include a single copy in chrome://resources

if (typeof window.trustedTypes !== 'undefined') {
  const originalCreatePolicy = window.trustedTypes?.createPolicy
  window.trustedTypes.createPolicy = (name, options) => {
    if (name === 'svelte-trusted-html') {
      return undefined as any
    }

    return originalCreatePolicy?.(name, options)
  }
}
