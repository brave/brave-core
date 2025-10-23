// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/brave_shields_util.h"

#include <utility>

#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "content/public/common/referrer.h"
#include "url/gurl.h"
#include "url/origin.h"

#if !DCHECK_IS_ON()
#include "base/notreached.h"
#endif

using content::Referrer;

namespace brave_shields {

bool AreReferrersAllowed(HostContentSettingsMap* map, const GURL& url) {
  const ContentSetting setting =
      map->GetContentSetting(url, GURL(), ContentSettingsType::BRAVE_REFERRERS);

  return setting == CONTENT_SETTING_ALLOW;
}

bool IsSameOriginNavigation(const GURL& referrer, const GURL& target_url) {
  const url::Origin original_referrer = url::Origin::Create(referrer);
  const url::Origin target_origin = url::Origin::Create(target_url);

  return original_referrer.IsSameOriginWith(target_origin);
}

bool MaybeChangeReferrer(bool allow_referrers,
                         bool shields_up,
                         const GURL& current_referrer,
                         const GURL& target_url,
                         Referrer* output_referrer) {
  DCHECK(output_referrer);
  if (allow_referrers || !shields_up || current_referrer.is_empty()) {
    return false;
  }

  if (IsSameOriginNavigation(current_referrer, target_url)) {
    // Do nothing for same-origin requests. This check also prevents us from
    // sending referrer from HTTPS to HTTP.
    return false;
  }

  // Cap the referrer to "strict-origin-when-cross-origin". More restrictive
  // policies should be already applied.
  // See https://github.com/brave/brave-browser/issues/13464
  url::Origin current_referrer_origin = url::Origin::Create(current_referrer);
  *output_referrer = Referrer::SanitizeForRequest(
      target_url,
      Referrer(current_referrer_origin.GetURL(),
               network::mojom::ReferrerPolicy::kStrictOriginWhenCrossOrigin));

  return true;
}

}  // namespace brave_shields
