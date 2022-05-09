/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.KeyringInfo;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.KeyringServiceObserver;
import org.chromium.mojo.system.MojoException;

public class KeyringModel implements KeyringServiceObserver {
    private KeyringService mKeyringService;
    private MutableLiveData<KeyringInfo> _mKeyringInfoLiveData;
    public LiveData<KeyringInfo> mKeyringInfoLiveData;
    // Todo: create method to interact with keyring

    public KeyringModel(KeyringService mKeyringService) {
        this.mKeyringService = mKeyringService;
        _mKeyringInfoLiveData = new MutableLiveData<>(null);
        mKeyringInfoLiveData = _mKeyringInfoLiveData;
        init();
    }

    private void init() {
        mKeyringService.addObserver(this);
    }

    private void update() {
        mKeyringService.getKeyringInfo(BraveWalletConstants.DEFAULT_KEYRING_ID,
                keyringInfo -> { _mKeyringInfoLiveData.postValue(keyringInfo); });
    }

    public KeyringInfo getKeyringInfo() {
        return _mKeyringInfoLiveData.getValue();
    }

    public void setKeyringService(KeyringService keyringService) {
        this.mKeyringService = keyringService;
        init();
    }

    @Override
    public void keyringCreated(String keyringId) {
        update();
    }

    @Override
    public void keyringRestored(String keyringId) {
        update();
    }

    @Override
    public void keyringReset() {
        update();
    }

    @Override
    public void locked() {
        update();
    }

    @Override
    public void unlocked() {
        update();
    }

    @Override
    public void backedUp() {
        update();
    }

    @Override
    public void accountsChanged() {
        update();
    }

    @Override
    public void autoLockMinutesChanged() {}

    @Override
    public void selectedAccountChanged(int coin) {
        update();
    }

    @Override
    public void onConnectionError(MojoException e) {}

    @Override
    public void close() {}
}
