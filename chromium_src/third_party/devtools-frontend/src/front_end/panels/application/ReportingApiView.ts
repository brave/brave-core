/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as Root from '../../core/root/root.js'
import * as UI from '../../ui/legacy/legacy.js'

export function PatchReportingApiView(
  ReportingApiView: new (...args: any[]) => UI.Widget.VBox,
): any {
  return class extends ReportingApiView {
    constructor(...args: any[]) {
      super(...args)
      // Missing when the browser isn't Brave, e.g. when remote debugging.
      if (
        (Root.Runtime.hostConfig as any).braveReportingApi?.enabled === false
      ) {
        const infobar = new UI.Infobar.Infobar(
          UI.Infobar.Type.INFO,
          'Brave disables the Reporting API, so reports are not sent. '
            + 'To enable it, visit brave://flags/#brave-reporting-api',
        )
        this.element.prepend(infobar.element)
      }
    }
  }
}
