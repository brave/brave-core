/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_NETWORK_ANNOTATIONS_H_
#define BRAVE_COMPONENTS_P3A_NETWORK_ANNOTATIONS_H_

#include <string_view>

#include "net/traffic_annotation/network_traffic_annotation.h"

namespace p3a {

net::NetworkTrafficAnnotationTag GetRandomnessRequestAnnotation();
net::NetworkTrafficAnnotationTag GetRandomnessServerInfoAnnotation();

net::NetworkTrafficAnnotationTag GetP3AUploadAnnotation(
    std::string_view upload_type,
    bool is_constellation);

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_NETWORK_ANNOTATIONS_H_
