package org.chromium.chrome.browser.rewards;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.fragment.app.Fragment;

import org.chromium.chrome.R;



public class BraveRewardsTipFailureFragment extends Fragment {

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        return inflater.inflate(R.layout.brave_rewards_tip_failure_fragment, container, false);
    }
}