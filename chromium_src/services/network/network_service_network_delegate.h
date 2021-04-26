/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_NETWORK_SERVICE_NETWORK_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_NETWORK_SERVICE_NETWORK_DELEGATE_H_

#define FinishedCanSendReportingReports                                   \
  NotUsed() {}                                                            \
  bool OnCanGetCookies_ChromiumImpl(const net::URLRequest& request,       \
                                    bool allowed_from_caller);            \
  bool OnCanSetCookie_ChromiumImpl(                                       \
      const net::URLRequest& request, const net::CanonicalCookie& cookie, \
      net::CookieOptions* options, bool allowed_from_caller);             \
  void FinishedCanSendReportingReports

#include "../../../../services/network/network_service_network_delegate.h"

#undef FinishedCanSendReportingReports

#endif  // BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_NETWORK_SERVICE_NETWORK_DELEGATE_H_
