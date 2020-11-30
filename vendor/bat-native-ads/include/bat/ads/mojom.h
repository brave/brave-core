/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_MOJOM_H_
#define BAT_ADS_MOJOM_H_

#include "brave/vendor/bat-native-ads/include/bat/ads/public/interfaces/ads.mojom.h"
#include "brave/vendor/bat-native-ads/include/bat/ads/public/interfaces/ads_database.mojom.h"

namespace ads {

using Environment = mojom::BraveAdsEnvironment;

using InternalsInfo = mojom::BraveAdsInternalsInfo;
using InternalsInfoPtr = mojom::BraveAdsInternalsInfoPtr;

using BuildChannel = mojom::BraveAdsBuildChannel;
using BuildChannelPtr = mojom::BraveAdsBuildChannelPtr;

using AdNotificationEventType = mojom::BraveAdsAdNotificationEventType;
using NewTabPageAdEventType = mojom::BraveAdsNewTabPageAdEventType;

using UrlRequest = mojom::BraveAdsUrlRequest;
using UrlRequestPtr = mojom::BraveAdsUrlRequestPtr;
using UrlRequestMethod = mojom::BraveAdsUrlRequestMethod;

using P2AEventType = mojom::BraveAdsP2AEventType;

using UrlResponse = mojom::BraveAdsUrlResponse;
using UrlResponsePtr = mojom::BraveAdsUrlResponsePtr;

using DBCommand = ads_database::mojom::DBCommand;
using DBCommandPtr = ads_database::mojom::DBCommandPtr;
using DBCommandBinding = ads_database::mojom::DBCommandBinding;
using DBCommandBindingPtr = ads_database::mojom::DBCommandBindingPtr;
using DBCommandResult = ads_database::mojom::DBCommandResult;
using DBCommandResultPtr = ads_database::mojom::DBCommandResultPtr;
using DBCommandResponse = ads_database::mojom::DBCommandResponse;
using DBCommandResponsePtr = ads_database::mojom::DBCommandResponsePtr;
using DBRecord = ads_database::mojom::DBRecord;
using DBRecordPtr = ads_database::mojom::DBRecordPtr;
using DBTransaction = ads_database::mojom::DBTransaction;
using DBTransactionPtr = ads_database::mojom::DBTransactionPtr;
using DBValue = ads_database::mojom::DBValue;
using DBValuePtr = ads_database::mojom::DBValuePtr;

}  // namespace ads

#endif  // BAT_ADS_MOJOM_H_
