/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.custom_layout;

import android.content.Context;
import android.util.AttributeSet;

import androidx.annotation.Nullable;
import androidx.appcompat.widget.AppCompatEditText;

public class PasteEditText extends AppCompatEditText {

    public interface Listener {
        void onPaste();
    }

    @Nullable private Listener mListener;

    public PasteEditText(Context context) {
        super(context);
    }

    public PasteEditText(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public PasteEditText(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public void setListener(@Nullable final Listener listener) {
        mListener = listener;
    }

    @Override
    public boolean onTextContextMenuItem(int id) {
        if (id == android.R.id.paste && mListener != null) {
            mListener.onPaste();
            return true;
        }
        return super.onTextContextMenuItem(id);
    }
}
