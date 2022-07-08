package org.chromium.chrome.browser.app.domain;

import android.content.Context;

import androidx.lifecycle.LiveData;

public interface CryptoSharedData {
    int getCoinType();
    String getChainId();
    Context getContext();
    LiveData<Integer> getCoinTypeLd();
    String[] getEnabledKeyrings();
}
