/**
 * Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.externalnav;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.url.GURL;

/**
 * Utility class for checking/opening external applications.
 */
public class BraveExternalNavigationUtils {
    private static final String TAG = "BraveExternalNavigationUtils";
    
    private static Intent createViewIntentFromUrl(GURL url) {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setData(Uri.parse(url.getSpec()));
        intent.addCategory(Intent.CATEGORY_BROWSABLE);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        Log.i(TAG, "Creating intent for URL: " + url.getSpec());
        Log.i(TAG, "Intent: " + intent.toString());
        return intent;
    }

    private static boolean verifyResolveInfo(ResolveInfo resolveInfo) {
        return resolveInfo != null
                && resolveInfo.activityInfo != null
                && resolveInfo.activityInfo.name != null
                && resolveInfo.activityInfo.applicationInfo.packageName != null
                // ResolveInfo may return browser itself (due to default browser settings), so filter this out.
                && !resolveInfo.activityInfo.applicationInfo.packageName
                        .equals(ContextUtils.getApplicationContext().getPackageName());
    }

    public static ResolveInfo resolveUrl(GURL url, Context context) {
        try {
            Intent intent = createViewIntentFromUrl(url);
            ResolveInfo resolveInfo = context.getPackageManager().resolveActivity(intent, PackageManager.MATCH_DEFAULT_ONLY);
            Log.i(TAG, "Resolved URL: " + url.getSpec());
            Log.i(TAG, "ResolveInfo: " + (resolveInfo != null ? resolveInfo.toString() : "null"));
            return resolveInfo;
        } catch (Exception e) {
            Log.e(TAG, "Error resolving URI: " + e.getMessage());
            return null;
        }
    }

    public static boolean canResolveUrl(GURL url, Context context) {
        return verifyResolveInfo(resolveUrl(url, context));
    }

    public static boolean openUrl(GURL url, Context context) {
        try {
            ResolveInfo resolveInfo = resolveUrl(url, context);
            if (resolveInfo == null) {
                Log.e(TAG, "No application can handle the URL: " + url.getSpec());
                return false;
            }
            if (!verifyResolveInfo(resolveInfo)) {
                Log.e(TAG, "Invalid ResolveInfo for URL: " + url.getSpec());
                Log.e(TAG, "ResolveInfo: " + resolveInfo.toString());
                return false;
            }
            Intent intent = createViewIntentFromUrl(url);
            intent.setClassName(resolveInfo.activityInfo.applicationInfo.packageName, resolveInfo.activityInfo.name);
            Log.i(TAG, "Opening URL: " + url.getSpec() + " with intent: " + intent.toString());
            context.startActivity(intent);
            return true;
        } catch (Exception e) {
            Log.e(TAG, "Error opening URL: " + e.getMessage());
            return false;
        }
    }
}
