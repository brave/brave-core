/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_NET_URL_REQUEST_MAGNET_PROTOCOL_HANDLER_H_
#define BRAVE_NET_URL_REQUEST_MAGNET_PROTOCOL_HANDLER_H_

#include "net/url_request/url_request_job_factory.h"

class MagnetProtocolHandler : public net::URLRequestJobFactory::ProtocolHandler {
  public:
    MagnetProtocolHandler();
    ~MagnetProtocolHandler() override;

    net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

  private:
    GURL TranslateURL(const GURL& url) const;
    DISALLOW_COPY_AND_ASSIGN(MagnetProtocolHandler);
};

#endif // BRAVE_NET_URL_REQUEST_MAGNET_PROTOCOL_HANDLER_H_
