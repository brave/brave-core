package org.chromium.chrome.browser.util;

import android.app.AppOpsManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;

import androidx.annotation.NonNull;

/**
 * Auxiliary class containing static methods that helps to determine if picture in picture mode is
 * supported by device and enabled via permission settings.
 */
public class PictureInPictureUtils {
    /** Returns {@code true} if picture in picture permission is enabled in app settings. */
    public static boolean hasPictureInPicturePermissionEnabled(@NonNull final Context context) {
        final AppOpsManager appOps =
                (AppOpsManager) context.getSystemService(Context.APP_OPS_SERVICE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            return appOps != null
                    && appOps.unsafeCheckOpNoThrow(
                                    AppOpsManager.OPSTR_PICTURE_IN_PICTURE,
                                    android.os.Process.myUid(),
                                    context.getPackageName())
                            == AppOpsManager.MODE_ALLOWED;
        } else {
            //noinspection deprecation
            return appOps != null
                    && appOps.checkOpNoThrow(
                                    AppOpsManager.OPSTR_PICTURE_IN_PICTURE,
                                    android.os.Process.myUid(),
                                    context.getPackageName())
                            == AppOpsManager.MODE_ALLOWED;
        }
    }

    /**
     * Launches picture in picture permission settings screen. Note: this method should be called
     * only when a device correctly supports picture in picture mode.
     */
    public static void launchPictureInPictureSettings(@NonNull final Context context) {
        context.startActivity(
                new Intent(
                        "android.settings.PICTURE_IN_PICTURE_SETTINGS",
                        Uri.parse(String.format("package:%s", context.getPackageName()))));
    }

    /** Returns {code true} if a device supports picture in picture mode. */
    public static boolean deviceSupportedPictureInPictureMode(@NonNull final Context context) {
        return context.getPackageManager()
                .hasSystemFeature(PackageManager.FEATURE_PICTURE_IN_PICTURE);
    }
}
