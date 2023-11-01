// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// utils
import { handleEndpointError } from '../../../utils/api-utils'
import { generateQRCode } from '../../../utils/qr-code-utils'

export const qrCodeEndpoints = ({ query }: WalletApiEndpointBuilderParams) => {
  return {
    getQrCodeImage: query<string, string>({
      queryFn: async (dataArg, { endpoint }) => {
        try {
          const image = await generateQRCode(dataArg)
          return {
            data: image
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Failed to generate QR Code for: ${dataArg}`,
            error
          )
        }
      }
    })
  }
}
