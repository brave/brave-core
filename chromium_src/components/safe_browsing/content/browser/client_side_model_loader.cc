/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/base/net_errors.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/simple_url_loader.h"

// Note: The IsCertificateError() call is not used for anything other than
// preventing the "net::" namespace prefix from getting attached to "return;".
#define NetworkTrafficAnnotationTag \
  IsCertificateError(net::OK);      \
  return;                           \
  net::NetworkTrafficAnnotationTag

#include "../../../../../../components/safe_browsing/content/browser/client_side_model_loader.cc"

#undef NetworkTrafficAnnotationTag
