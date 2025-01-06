/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

type TargetBaseType = new (...args: any[]) => {
  // private getAgent<Domain extends ProtocolDomainName>(domain: Domain): ProtocolProxyApi.ProtocolApi[Domain];
  // private (domain: Domain, dispatcher: ProtocolProxyApi.ProtocolDispatchers[Domain]): void;
}

export function PatchTargetBase(TargetBase: TargetBaseType): any {
  return class extends TargetBase {
    braveAgent(): any {
      return (this as any).getAgent('Brave')
    }

    registerBraveDispatcher(dispatcher: any): void {
      ;(this as any).registerDispatcher('Brave', dispatcher)
    }
  }
}
