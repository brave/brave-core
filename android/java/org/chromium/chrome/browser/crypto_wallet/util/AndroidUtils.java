package org.chromium.chrome.browser.crypto_wallet.util;

import android.content.Context;
import android.util.TypedValue;
import android.view.View;

import androidx.annotation.IdRes;

public class AndroidUtils {
    public static int getToolBarHeight(Context context) {
        TypedValue tv = new TypedValue();
        if (context.getTheme().resolveAttribute(android.R.attr.actionBarSize, tv, true)) {
            return TypedValue.complexToDimensionPixelSize(
                    tv.data, context.getResources().getDisplayMetrics());
        }
        return 0;
    }

    public static void disableViewsByIds(View view, int... ids) {
        if (view != null) {
            for (int id : ids) {
                disableView(view, id);
            }
        }
    }

    public static void disableView(View containerView, @IdRes int id) {
        if (containerView == null) return;
        View view = containerView.findViewById(id);
        if (view != null) {
            view.setEnabled(false);
            view.setClickable(false);
        }
    }
}
