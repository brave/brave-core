// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  sendTokenizedWebKitMessageSynchronously,
  messageHandlerName,
} from '//brave/ios/web/js_messaging/resources/utils.js'

const scriptlets: string[] | null = sendTokenizedWebKitMessageSynchronously(
  messageHandlerName,
  {},
)

// Documents enforcing `require-trusted-types-for 'script'` reject a plain
// string passed to the `Function` constructor, so wrap the source in a
// `TrustedScript` where the API is available. Page CSP does not apply to
// `WKUserScript` injection, but it does apply to the evaluation this script
// performs, which is why the scriptlet sources need this treatment.
const toTrustedScript = (() => {
  const policyFactory = window.trustedTypes
  if (!policyFactory) {
    return (source: string): string => source
  }
  try {
    const policy = policyFactory.createPolicy('brave-scriptlets', {
      createScript: (source: string): string => source,
    })
    // `Function` accepts a `TrustedScript` when Trusted Types is enforced,
    // but its type definition only admits strings.
    return (source: string): string =>
      policy.createScript(source) as unknown as string
  } catch {
    // A `trusted-types` directive may disallow this policy name. Fall back to
    // the raw source so documents without enforcement still run scriptlets.
    return (source: string): string => source
  }
})()

if (scriptlets) {
  for (const scriptlet of scriptlets) {
    // Isolate failures so that one scriptlet cannot prevent the rest from
    // running.
    try {
      new Function(toTrustedScript(scriptlet))()
    } catch (error) {
      console.error('Brave failed to inject a scriptlet', error)
    }
  }
}
