package org.chromium.chrome.browser.crypto_wallet.util;

import android.content.Context;
import android.os.Build;
import android.text.Html;
import android.text.Spanned;
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

    public static Spanned formatHTML(String html) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            return Html.fromHtml(html, Html.FROM_HTML_MODE_LEGACY);
        } else {
            return Html.fromHtml(html);
        }
    }
}
