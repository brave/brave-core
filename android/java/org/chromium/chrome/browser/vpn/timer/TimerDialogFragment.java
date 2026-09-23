/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.timer;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.vpn.utils.BraveVpnProfileUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class TimerDialogFragment extends BottomSheetDialogFragment
        implements TimerItemAdapter.TimerItemClickListener {
    public static final String TAG = TimerDialogFragment.class.getName();

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setStyle(STYLE_NORMAL, R.style.TimerBottomSheetDialogTheme);
    }

    @Nullable
    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_timer_dialog_list_dialog, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        ((BottomSheetDialog) getDialog())
                .getBehavior()
                .setState(BottomSheetBehavior.STATE_EXPANDED);

        final RecyclerView recyclerView = (RecyclerView) view.findViewById(R.id.timer_action_list);
        recyclerView.setLayoutManager(new LinearLayoutManager(getContext()));
        List<TimerItemModel> timerItemModels =
                new ArrayList<>(
                        Arrays.asList(
                                new TimerItemModel(
                                        requireContext().getString(R.string.pause_for_15_minutes),
                                        R.drawable.ic_pause_filled,
                                        TimerItemModel.TimerDuration.MINUTES_15),
                                new TimerItemModel(
                                        requireContext().getString(R.string.pause_for_1_hour),
                                        R.drawable.ic_pause_filled,
                                        TimerItemModel.TimerDuration.MINUTES_60),
                                new TimerItemModel(
                                        requireContext().getString(R.string.disable),
                                        R.drawable.ic_disabled,
                                        TimerItemModel.TimerDuration.NONE),
                                new TimerItemModel(
                                        requireContext().getString(android.R.string.cancel),
                                        R.drawable.ic_cancel_filled,
                                        TimerItemModel.TimerDuration.CANCEL)));
        recyclerView.setAdapter(new TimerItemAdapter(timerItemModels, TimerDialogFragment.this));
    }

    @Override
    public void onTimerItemClick(TimerItemModel timerItemModel) {
        if (getActivity() == null) {
            return;
        }

        switch (timerItemModel.getTimerDuration()) {
            case MINUTES_15:
            case MINUTES_60:
                BraveVpnProfileUtils.getInstance().stopVpn(getActivity());
                TimerUtils.scheduleVpnAction(
                        getActivity(), timerItemModel.getTimerDuration().getMinutes());
                break;
            case NONE:
                BraveVpnProfileUtils.getInstance().stopVpn(getActivity());
                break;
            default:
                break;
        }
        dismiss();
    }
}
