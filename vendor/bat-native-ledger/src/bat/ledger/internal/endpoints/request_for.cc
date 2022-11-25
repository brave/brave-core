/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoints/request_for.h"

namespace ledger::endpoints {

void SendImpl(LedgerImpl* ledger,
              absl::optional<std::tuple<mojom::UrlRequestPtr,
                                        client::LoadURLCallback,
                                        base::TimeDelta>> params) {
  DCHECK(ledger);

  if (params) {
    auto [request, callback, delta] = std::move(*params);

    auto load_url_callback =
        base::BindOnce(
            [](mojom::UrlRequestPtr req, client::LoadURLCallback cb,
               const mojom::UrlResponse& res) -> decltype(params) {
              if (/*response.status_code == net::HTTP_TOO_MANY_REQUESTS &&*/
                  req->retry_on_rate_limiting--) {
                LogUrlResponse(__func__, res);

                return std::tuple(std::move(req), std::move(cb),
                                  base::Seconds(5));
              } else {
                std::move(cb).Run(res);
                return absl::nullopt;
              }
            },
            request->Clone(), std::move(callback))
            .Then(base::BindOnce(&SendImpl, ledger));

    if (delta.is_positive()) {
      base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
          FROM_HERE,
          base::BindOnce(static_cast<void (LedgerImpl::*)(
                             mojom::UrlRequestPtr, client::LoadURLCallback)>(
                             &LedgerImpl::LoadURL),
                         base::Unretained(ledger), std::move(request),
                         std::move(load_url_callback)),
          std::move(delta));
    } else {
      ledger->LoadURL(std::move(request), std::move(load_url_callback));
    }
  }
}

}  // namespace ledger::endpoints
