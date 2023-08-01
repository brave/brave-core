package org.chromium.chrome.browser.crypto_wallet.web_ui;

import android.app.Activity;

import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.HashMap;
import java.util.Map;

/**
 * Type of WebUi Activity.
 * @see Utils#openBuySendSwapActivity(Activity, WebUiActivityType).
 */
public enum WebUiActivityType {
    BUY(0),
    SEND(1),
    SWAP(2);

    private final int value;
    private static final Map<Integer, WebUiActivityType> map = new HashMap<>();

    WebUiActivityType(final int value) {
        this.value = value;
    }

    static {
        for (WebUiActivityType webUiActivityType : WebUiActivityType.values()) {
            map.put(webUiActivityType.value, webUiActivityType);
        }
    }

    public static WebUiActivityType valueOf(int activityType) {
        return map.get(activityType);
    }

    public int getValue() {
        return value;
    }
}
