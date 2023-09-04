/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.playlist;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveDialogFragment;

public class PlaylistWarningDialogFragment
        extends BraveDialogFragment implements View.OnClickListener {
    public interface PlaylistWarningDialogListener {
        public void onActionClicked();
        public void onSettingsClicked();
    }

    private PlaylistWarningDialogListener playlistWarningDialogListener;

    public void setPlaylistWarningDialogListener(
            PlaylistWarningDialogListener playlistWarningDialogListener) {
        this.playlistWarningDialogListener = playlistWarningDialogListener;
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_playlist_warning_dialog, container, false);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        Button addMediaButton = view.findViewById(R.id.btn_add_media);
        addMediaButton.setOnClickListener(this);

        Button settingsButton = view.findViewById(R.id.btn_settings);
        settingsButton.setOnClickListener(this);

        ImageView btnClose = view.findViewById(R.id.modal_close);
        btnClose.setOnClickListener(this);
    }

    @Override
    public void onClick(View view) {
        if (playlistWarningDialogListener == null) {
            return;
        }
        if (view.getId() == R.id.btn_add_media) {
            playlistWarningDialogListener.onActionClicked();
        } else if (view.getId() == R.id.btn_settings) {
            playlistWarningDialogListener.onSettingsClicked();
        }
        if (view.getId() == R.id.btn_add_media || view.getId() == R.id.modal_close) {
            dismiss();
        }
    }
}
