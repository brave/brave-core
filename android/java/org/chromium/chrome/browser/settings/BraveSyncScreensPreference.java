/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.ActivityNotFoundException;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.DialogInterface;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.hardware.Camera;
import android.os.Build;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.widget.AppCompatImageView;
import androidx.core.app.ActivityCompat;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentContainerView;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.google.android.gms.vision.MultiProcessor;
import com.google.android.gms.vision.barcode.Barcode;
import com.google.android.gms.vision.barcode.BarcodeDetector;
import com.google.android.material.tabs.TabLayout;

import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveSyncWorker;
import org.chromium.chrome.browser.back_press.BackPressHelper;
import org.chromium.chrome.browser.init.ChromeBrowserInitializer;
import org.chromium.chrome.browser.qrreader.BarcodeTracker;
import org.chromium.chrome.browser.qrreader.BarcodeTrackerFactory;
import org.chromium.chrome.browser.qrreader.CameraSource;
import org.chromium.chrome.browser.qrreader.CameraSourcePreview;
import org.chromium.chrome.browser.share.qrcode.QRCodeGenerator;
import org.chromium.chrome.browser.sync.BraveSyncDevices;
import org.chromium.chrome.browser.sync.SyncServiceFactory;
import org.chromium.chrome.browser.sync.settings.BraveManageSyncSettings;
import org.chromium.components.browser_ui.settings.SettingsNavigation;
import org.chromium.components.sync.SyncService;
import org.chromium.ui.base.DeviceFormFactor;

import java.io.IOException;
import java.time.LocalDateTime;
import java.time.ZoneOffset;
import java.util.ArrayList;

/** Settings fragment that allows to control Sync functionality. */
public class BraveSyncScreensPreference extends BravePreferenceFragment
        implements View.OnClickListener,
                BackPressHelper.ObsoleteBackPressedHandler,
                BarcodeTracker.BarcodeGraphicTrackerCallback,
                BraveSyncDevices.DeviceInfoChangedListener,
                SyncService.SyncStateChangedListener {
    public static final int BIP39_WORD_COUNT = 24;
    private static final String TAG = "SYNC";
    // Permission request codes need to be < 256
    private static final int RC_HANDLE_CAMERA_PERM = 2;
    // Intent request code to handle updating play services if needed.
    private static final int RC_HANDLE_GMS = 9001;

    // The have a sync code button displayed in the Sync view.
    private Button mScanChainCodeButton;
    private Button mStartNewChainButton;
    private Button mAdvancedOptionsButton;
    private Button mEnterCodeWordsButton;
    private Button mDoneButton;
    private Button mDoneLaptopButton;
    private Button mUseCameraButton;
    private Button mConfirmCodeWordsButton;
    private ImageButton mMobileButton;
    private ImageButton mLaptopButton;
    private ImageButton mPasteButton;
    private Button mCopyButton;
    private Button mAddDeviceButton;
    private Button mShowCategoriesButton;
    private Button mDeleteAccountButton;
    private Button mNewCodeWordsButton;
    private Button mNewQrCodeButton;
    private TextView mBraveSyncTextDevicesTitle;
    private TextView mBraveSyncWordCountTitle;
    private TextView mBraveSyncAddDeviceCodeWords;
    private CameraSource mCameraSource;
    private CameraSourcePreview mCameraSourcePreview;
    private ScrollView mScrollViewSyncInitial;
    private ScrollView mScrollViewSyncChainCode;
    private ScrollView mScrollViewSyncStartChain;
    private ScrollView mScrollViewAddMobileDevice;
    private ScrollView mScrollViewAddLaptop;
    private ScrollView mScrollViewEnterCodeWords;
    private ScrollView mScrollViewSyncDone;
    private ViewGroup mAddDeviceTab;
    private LayoutInflater mInflater;
    private ImageView mQRCodeImage;
    private LinearLayout mQRContainer;
    private LinearLayout mLayoutSyncStartChain;
    private EditText mCodeWords;
    private FrameLayout mLayoutMobile;
    private FrameLayout mLayoutLaptop;
    private AlertDialog mFinalWarningDialog;
    private TabLayout mTabLayout;

    // Below enum is matching the values of GetDeviceTypeString() in brave_device_info.cc
    public enum DeviceType {
        UNKNOWN("unknown"),
        DESKTOP("desktop_or_laptop"),
        PHONE("phone"),
        TABLET("tablet");

        final String mType;

        DeviceType(final String type) {
            mType = type;
        }

        public String getValue() {
            return mType;
        }
    }

    private final ObservableSupplierImpl<String> mPageTitle = new ObservableSupplierImpl<>();

    BraveSyncWorker getBraveSyncWorker() {
        return BraveSyncWorker.get();
    }

    @Override
    public void deviceInfoChanged() {
        onDevicesAvailable();
    }

    boolean mDeviceInfoObserverSet;

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        if (isRemoving() || isDetached()) {
            return;
        }

        // Checks the orientation of the screen
        if (newConfig.orientation != Configuration.ORIENTATION_UNDEFINED
                && null != mCameraSourcePreview) {
            mCameraSourcePreview.stop();
            try {
                startCameraSource();
            } catch (SecurityException exc) {
            }
        }
        adjustImageButtons(newConfig.orientation);
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        SyncServiceFactory.getForProfile(getProfile()).addSyncStateChangedListener(this);

        invalidateCodephrase();

        mInflater = inflater;
        // Read which category we should be showing.
        return mInflater.inflate(R.layout.brave_sync_layout, container, false);
    }

    private boolean ensureCameraPermission() {
        if (ActivityCompat.checkSelfPermission(
                    getActivity().getApplicationContext(), Manifest.permission.CAMERA)
                == PackageManager.PERMISSION_GRANTED) {
            return true;
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(new String[] {Manifest.permission.CAMERA}, RC_HANDLE_CAMERA_PERM);
        }

        return false;
    }

    @Override
    public void onRequestPermissionsResult(
            int requestCode, String[] permissions, int[] grantResults) {
        if (requestCode != RC_HANDLE_CAMERA_PERM) {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults);
            return;
        }

        if (grantResults.length != 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            // we have permission, so create the camerasource
            createCameraSource(true, false);
            return;
        }

        Log.e(TAG,
                "Permission not granted: results len = " + grantResults.length + " Result code = "
                        + (grantResults.length > 0 ? grantResults[0] : "(empty)"));

        Toast.makeText(getActivity().getApplicationContext(),
                     getResources().getString(
                             R.string.sync_camera_permission_was_not_granted_toast),
                     Toast.LENGTH_LONG)
                .show();

        // Go to main sync settings screen
        getActivity().onBackPressed();
    }

    public void onSyncError(String message) {
        try {
            if (null == getActivity()) {
                return;
            }
            final String messageFinal = (null == message || message.isEmpty())
                    ? getResources().getString(R.string.sync_device_failure)
                    : message;
            getActivity().runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    showEndDialog(messageFinal, () -> {});
                }
            });
        } catch (Exception exc) {
            Log.e(TAG, "onSyncError exception: " + exc);
        }
    }

    private void fillDevices() {
        try {
            if (getActivity() == null
                    || getView() == null
                    || View.VISIBLE != mScrollViewSyncDone.getVisibility()) {
                Log.w(TAG, "No need to load devices for other pages");
                return;
            }
            ArrayList<BraveSyncDevices.SyncDeviceInfo> deviceInfos =
                    BraveSyncDevices.get().getSyncDeviceList();

            ViewGroup insertPoint = (ViewGroup) getView().findViewById(R.id.brave_sync_devices);
            insertPoint.removeAllViews();
            int index = 0;
            for (BraveSyncDevices.SyncDeviceInfo device : deviceInfos) {
                View separator = (View) mInflater.inflate(R.layout.menu_separator, null);
                View listItemView = (View) mInflater.inflate(R.layout.brave_sync_device, null);
                if (null != listItemView && null != separator && null != insertPoint) {
                    ImageView imageView =
                            (ImageView) listItemView.findViewById(R.id.brave_sync_device_image);
                    int deviceTypeRes = R.drawable.ic_laptop;
                    if (DeviceType.PHONE.getValue().equals(device.mType)) {
                        deviceTypeRes = R.drawable.ic_smartphone;
                    } else if (DeviceType.TABLET.getValue().equals(device.mType)) {
                        deviceTypeRes = R.drawable.ic_tablet;
                    }
                    imageView.setImageResource(deviceTypeRes);

                    TextView textView =
                            (TextView) listItemView.findViewById(R.id.brave_sync_device_text);
                    if (null != textView) {
                        if (device.mIsCurrentDevice) {
                            String currentDevice =
                                    device.mName
                                            + " "
                                            + getResources()
                                                    .getString(
                                                            R.string.brave_sync_this_device_text);
                            textView.setText(currentDevice);
                        } else {
                            textView.setText(device.mName);
                        }
                    }

                    AppCompatImageView deleteButton =
                            (AppCompatImageView)
                                    listItemView.findViewById(R.id.brave_sync_remove_device);
                    if (device.mSupportsSelfDelete || device.mIsCurrentDevice) {
                        deleteButton.setTag(device);
                        deleteButton.setOnClickListener(
                                v -> {
                                    BraveSyncDevices.SyncDeviceInfo deviceToDelete =
                                            (BraveSyncDevices.SyncDeviceInfo) v.getTag();
                                    deleteDeviceDialog(deviceToDelete);
                                });
                    } else {
                        deleteButton.setVisibility(View.GONE);
                    }

                    insertPoint.addView(separator, index++);
                    insertPoint.addView(listItemView, index++);
                }
            }

            if (index > 0) {
                mBraveSyncTextDevicesTitle.setText(
                        getResources().getString(R.string.brave_sync_devices_title));
                View separator = (View) mInflater.inflate(R.layout.menu_separator, null);
                if (null != insertPoint && null != separator) {
                    insertPoint.addView(separator, index++);
                }
            }
        } catch (Exception ex) {
            Log.e(TAG, "fillDevices exception: ", ex);
        }
    }

    public void onDevicesAvailable() {
        PostTask.postTask(TaskTraits.UI_USER_VISIBLE, () -> fillDevices());
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        mPageTitle.set(getString(R.string.sync_category_title));

        mScrollViewSyncInitial = getView().findViewById(R.id.view_sync_initial);
        mScrollViewSyncChainCode = getView().findViewById(R.id.view_sync_chain_code);
        mScrollViewSyncStartChain = getView().findViewById(R.id.view_sync_start_chain);
        mScrollViewAddMobileDevice = getView().findViewById(R.id.view_add_mobile_device);
        mScrollViewAddLaptop = getView().findViewById(R.id.view_add_laptop);
        mScrollViewEnterCodeWords = getView().findViewById(R.id.view_enter_code_words);
        mScrollViewSyncDone = getView().findViewById(R.id.view_sync_done);

        if (!DeviceFormFactor.isTablet()) {
            clearBackground(mScrollViewSyncInitial);
            clearBackground(mScrollViewSyncChainCode);
            clearBackground(mScrollViewSyncStartChain);
            clearBackground(mScrollViewAddMobileDevice);
            clearBackground(mScrollViewAddLaptop);
            clearBackground(mScrollViewEnterCodeWords);
            clearBackground(mScrollViewSyncDone);
        }

        mLayoutSyncStartChain = getView().findViewById(R.id.view_sync_start_chain_layout);

        mScanChainCodeButton = getView().findViewById(R.id.brave_sync_btn_scan_chain_code);
        if (mScanChainCodeButton != null) {
            mScanChainCodeButton.setOnClickListener(this);
        }

        mStartNewChainButton = getView().findViewById(R.id.brave_sync_btn_start_new_chain);
        if (mStartNewChainButton != null) {
            mStartNewChainButton.setOnClickListener(this);
        }

        mAdvancedOptionsButton = getView().findViewById(R.id.brave_sync_btn_advanced_options);
        if (mAdvancedOptionsButton != null) {
            mAdvancedOptionsButton.setOnClickListener(this);
        }

        mEnterCodeWordsButton = getView().findViewById(R.id.brave_sync_btn_enter_code_words);
        if (mEnterCodeWordsButton != null) {
            mEnterCodeWordsButton.setOnClickListener(this);
        }

        mQRCodeImage = getView().findViewById(R.id.brave_sync_qr_code_image);

        mQRContainer = getView().findViewById(R.id.brave_sync_qr_containter);

        mDoneButton = getView().findViewById(R.id.brave_sync_btn_done);
        if (mDoneButton != null) {
            mDoneButton.setOnClickListener(this);
        }

        mDoneLaptopButton = getView().findViewById(R.id.brave_sync_btn_add_laptop_done);
        if (mDoneLaptopButton != null) {
            mDoneLaptopButton.setOnClickListener(this);
        }

        mUseCameraButton = getView().findViewById(R.id.brave_sync_btn_use_camera);
        if (mUseCameraButton != null) {
            mUseCameraButton.setOnClickListener(this);
        }

        mConfirmCodeWordsButton = getView().findViewById(R.id.brave_sync_confirm_code_words);
        if (mConfirmCodeWordsButton != null) {
            mConfirmCodeWordsButton.setOnClickListener(this);
        }

        mMobileButton = getView().findViewById(R.id.brave_sync_btn_mobile);
        if (mMobileButton != null) {
            mMobileButton.setOnClickListener(this);
        }

        mLaptopButton = getView().findViewById(R.id.brave_sync_btn_laptop);
        if (mLaptopButton != null) {
            mLaptopButton.setOnClickListener(this);
        }

        mPasteButton = getView().findViewById(R.id.brave_sync_paste_button);
        if (mPasteButton != null) {
            mPasteButton.setOnClickListener(this);
        }

        mCopyButton = getView().findViewById(R.id.brave_sync_copy_button);
        if (mCopyButton != null) {
            mCopyButton.setOnClickListener(this);
        }

        mBraveSyncTextDevicesTitle = getView().findViewById(R.id.brave_sync_devices_title);
        mBraveSyncWordCountTitle = getView().findViewById(R.id.brave_sync_text_word_count);
        mBraveSyncWordCountTitle.setText(getString(R.string.brave_sync_word_count_text, 0));
        mBraveSyncAddDeviceCodeWords =
                getView().findViewById(R.id.brave_sync_add_device_code_words);

        mCameraSourcePreview = getView().findViewById(R.id.preview);

        mAddDeviceButton = getView().findViewById(R.id.brave_sync_btn_add_device);
        if (null != mAddDeviceButton) {
            mAddDeviceButton.setOnClickListener(this);
        }

        mDeleteAccountButton = getView().findViewById(R.id.brave_sync_btn_delete_account);
        if (null != mDeleteAccountButton) {
            mDeleteAccountButton.setOnClickListener(this);
        }

        mShowCategoriesButton = getView().findViewById(R.id.brave_sync_btn_show_categories);
        if (null != mShowCategoriesButton) {
            mShowCategoriesButton.setOnClickListener(this);
        }

        mAddDeviceTab = getView().findViewById(R.id.view_add_device_tab);

        mCodeWords = getView().findViewById(R.id.code_words);

        mNewCodeWordsButton = getView().findViewById(R.id.brave_sync_btn_add_laptop_new_code);
        assert mNewCodeWordsButton != null;
        mNewCodeWordsButton.setOnClickListener(this);

        mNewQrCodeButton = getView().findViewById(R.id.brave_sync_btn_add_mobile_new_code);
        assert mNewQrCodeButton != null;
        mNewQrCodeButton.setOnClickListener(this);

        mTabLayout = getView().findViewById(R.id.tab_layout);
        mTabLayout.addOnTabSelectedListener(
                new TabLayout.OnTabSelectedListener() {
                    @Override
                    public void onTabSelected(TabLayout.Tab tab) {
                        if (0 == tab.getPosition()) {
                            setAddMobileDeviceLayout();
                        } else if (1 == tab.getPosition()) {
                            setAddLaptopLayout();
                        } else {
                            assert false : "We have only two tabs";
                        }
                    }

                    @Override
                    public void onTabReselected(TabLayout.Tab tab) {}

                    @Override
                    public void onTabUnselected(TabLayout.Tab tab) {}
                });

        mLayoutMobile = getView().findViewById(R.id.brave_sync_frame_mobile);
        mLayoutLaptop = getView().findViewById(R.id.brave_sync_frame_laptop);

        setAppropriateView();

        super.onActivityCreated(savedInstanceState);
    }

    @Override
    public ObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    private void setAppropriateView() {
        if (null == getActivity()) {
            // We can reach here if we were joining the chain, but we closed the preferences.
            return;
        }

        getActivity()
                .getWindow()
                .setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);
        getActivity().setTitle(R.string.sync_category_title);

        boolean firstSetupComplete = getBraveSyncWorker().isInitialSyncFeatureSetupComplete();

        Log.v(TAG, "setAppropriateView first setup complete " + firstSetupComplete);
        if (!firstSetupComplete) {
            if (null != mCameraSourcePreview) {
                mCameraSourcePreview.stop();
            }
            if (null != mScrollViewSyncInitial) {
                mScrollViewSyncInitial.setVisibility(View.VISIBLE);
            }
            if (null != mScrollViewSyncChainCode) {
                mScrollViewSyncChainCode.setVisibility(View.GONE);
            }
            if (null != mScrollViewEnterCodeWords) {
                mScrollViewEnterCodeWords.setVisibility(View.GONE);
            }
            if (null != mScrollViewAddMobileDevice) {
                mScrollViewAddMobileDevice.setVisibility(View.GONE);
            }
            if (null != mScrollViewAddLaptop) {
                mScrollViewAddLaptop.setVisibility(View.GONE);
            }
            if (null != mAddDeviceTab) {
                mAddDeviceTab.setVisibility(View.GONE);
            }
            if (null != mScrollViewSyncStartChain) {
                mScrollViewSyncStartChain.setVisibility(View.GONE);
            }
            if (null != mScrollViewSyncDone) {
                mScrollViewSyncDone.setVisibility(View.GONE);
            }
            if (null != mCodeWords) {
                mCodeWords.setText("");
            }
            return;
        }
        setSyncDoneLayout();
    }

    private String getWordsValidationString(String words) {
        int validationResult = getBraveSyncWorker().getWordsValidationResult(words);
        Log.v(TAG, "validationResult is " + validationResult);
        switch (validationResult) {
            case 0:
                // kValid, empty string indicates there is no error
                return "";
            case 2:
                // kVersionDeprecated
                return getResources().getString(R.string.brave_sync_code_from_deprecated_version);
            case 3:
                // kExpired
                return getResources().getString(R.string.brave_sync_code_expired);
            case 4:
                // kValidForTooLong
                return getResources().getString(R.string.brave_sync_code_valid_for_too_long);

            default:
                // These three different types of errors have the same message
                // kWrongWordsNumber
                // kNotValidPureWords
                return getResources().getString(R.string.brave_sync_wrong_code_error);
        }
    }

    /** OnClickListener for the clear button. We show an alert dialog to confirm the action */
    @Override
    public void onClick(View v) {
        if ((getActivity() == null)
                || (v != mScanChainCodeButton
                        && v != mStartNewChainButton
                        && v != mEnterCodeWordsButton
                        && v != mDoneButton
                        && v != mDoneLaptopButton
                        && v != mUseCameraButton
                        && v != mConfirmCodeWordsButton
                        && v != mMobileButton
                        && v != mLaptopButton
                        && v != mPasteButton
                        && v != mCopyButton
                        && v != mShowCategoriesButton
                        && v != mAddDeviceButton
                        && v != mDeleteAccountButton
                        && v != mNewCodeWordsButton
                        && v != mNewQrCodeButton)) {
            return;
        }

        if (mScanChainCodeButton == v) {
            setJoinExistingChainLayout();
        } else if (mStartNewChainButton == v) {
            // Creating a new chain
            getPureWords();
            setNewChainLayout();
            seedWordsReceived(mCodephrase, SyncInputType.NEW);
        } else if (mMobileButton == v) {
            setAddMobileDeviceLayout();
            selectAddMobileTab();
        } else if (mLaptopButton == v) {
            setAddLaptopLayout();
            selectAddLaptopTab();
        } else if (mDoneButton == v) {
            setSyncDoneLayout();
        } else if (mDoneLaptopButton == v) {
            setSyncDoneLayout();
        } else if (mUseCameraButton == v) {
            setJoinExistingChainLayout();
        } else if (mPasteButton == v) {
            if (null != mCodeWords) {
                ClipboardManager clipboard = (ClipboardManager) getActivity().getSystemService(
                        Context.CLIPBOARD_SERVICE);
                ClipData clipData = clipboard.getPrimaryClip();
                if (null != clipData && clipData.getItemCount() > 0) {
                    mCodeWords.setText(clipData.getItemAt(0).coerceToText(
                            getActivity().getApplicationContext()));
                }
            }
        } else if (mCopyButton == v) {
            if (null != mBraveSyncAddDeviceCodeWords) {
                ClipboardManager clipboard = (ClipboardManager) getActivity().getSystemService(
                        Context.CLIPBOARD_SERVICE);
                ClipData clip = ClipData.newPlainText("", mBraveSyncAddDeviceCodeWords.getText());
                clipboard.setPrimaryClip(clip);
                Toast.makeText(getActivity().getApplicationContext(),
                             getResources().getString(R.string.brave_sync_copied_text),
                             Toast.LENGTH_LONG)
                        .show();
            }
        } else if (mConfirmCodeWordsButton == v) {
            String words = mCodeWords.getText().toString();
            String validationError = getWordsValidationString(words);
            if (!validationError.isEmpty()) {
                Log.e(TAG, "Confirm code words - wrong codephrase");
                onSyncError(validationError);
                return;
            }

            String codephraseCandidate = getBraveSyncWorker().getPureWordsFromTimeLimited(words);
            assert codephraseCandidate != null && !codephraseCandidate.isEmpty();

            showFinalSecurityWarning(
                    FinalWarningFor.CODE_WORDS,
                    () -> {
                        // We have the confirmation from user
                        // Code phrase looks valid, we can pass it down to sync system
                        mCodephrase = codephraseCandidate;
                        seedWordsReceived(mCodephrase, SyncInputType.JOIN);
                    },
                    () -> {});
        } else if (mEnterCodeWordsButton == v) {
            getActivity()
                    .getWindow()
                    .setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
            if (null != mScrollViewSyncInitial) {
                mScrollViewSyncInitial.setVisibility(View.GONE);
            }
            if (null != mScrollViewAddMobileDevice) {
                mScrollViewAddMobileDevice.setVisibility(View.GONE);
            }
            if (null != mScrollViewAddLaptop) {
                mScrollViewAddLaptop.setVisibility(View.GONE);
            }
            if (null != mScrollViewSyncStartChain) {
                mScrollViewSyncStartChain.setVisibility(View.GONE);
            }
            if (null != mCameraSourcePreview) {
                mCameraSourcePreview.stop();
            }
            if (null != mScrollViewSyncChainCode) {
                mScrollViewSyncChainCode.setVisibility(View.GONE);
            }
            if (null != mScrollViewEnterCodeWords) {
                mScrollViewEnterCodeWords.setVisibility(View.VISIBLE);
            }
            getActivity().setTitle(R.string.brave_sync_code_words_title);
            if (null != mCodeWords && null != mBraveSyncWordCountTitle) {
                mCodeWords.addTextChangedListener(
                        new TextWatcher() {
                            @Override
                            public void afterTextChanged(Editable s) {}

                            @Override
                            public void beforeTextChanged(
                                    CharSequence s, int start, int count, int after) {}

                            @Override
                            public void onTextChanged(
                                    CharSequence s, int start, int before, int count) {
                                int wordCount =
                                        getBraveSyncWorker()
                                                .getWordsCount(mCodeWords.getText().toString());
                                mBraveSyncWordCountTitle.setText(
                                        getString(R.string.brave_sync_word_count_text, wordCount));
                                mBraveSyncWordCountTitle.invalidate();
                            }
                        });
            }
        } else if (mShowCategoriesButton == v) {
            SettingsNavigation settingsLauncher =
                    SettingsNavigationFactory.createSettingsNavigation();
            settingsLauncher.startSettings(getContext(), BraveManageSyncSettings.class);
        } else if (mAddDeviceButton == v) {
            setNewChainLayout();
        } else if (mDeleteAccountButton == v) {
            permanentlyDeleteAccount();
        } else if (mNewCodeWordsButton == v) {
            generateNewCodeWords();
        } else if (mNewQrCodeButton == v) {
            generateNewQrCode();
        }
    }

    private void selectAddLaptopTab() {
        mTabLayout.getTabAt(1).select();
    }

    private void selectAddMobileTab() {
        mTabLayout.getTabAt(0).select();
    }

    void generateNewCodeWords() {
        ThreadUtils.assertOnUiThread();

        String codePhrase = getPureWords();
        assert codePhrase != null && !codePhrase.isEmpty();
        String timeLimitedWords = getBraveSyncWorker().getTimeLimitedWordsFromPure(codePhrase);
        assert timeLimitedWords != null && !timeLimitedWords.isEmpty();
        mBraveSyncAddDeviceCodeWords.setVisibility(View.VISIBLE);
        mBraveSyncAddDeviceCodeWords.setText(timeLimitedWords);
        mCopyButton.setVisibility(View.VISIBLE);
        mNewCodeWordsButton.setVisibility(View.GONE);

        LocalDateTime notAfterTime =
                getBraveSyncWorker().getNotAfterFromFromTimeLimitedWords(timeLimitedWords);
        setWordsCountDown(notAfterTime);
    }

    void generateNewQrCode() {
        ThreadUtils.assertOnUiThread();

        String seedHex = getBraveSyncWorker().getSeedHexFromWords(getPureWords());
        if (null == seedHex || seedHex.isEmpty()) {
            // Give up, seed must be valid
            Log.e(TAG, "generateNewQrCode seedHex is empty");
            assert false;
        } else {
            if (!isSeedHexValid(seedHex)) {
                Log.e(TAG, "fillQrCode - invalid QR code");
                // Normally must not reach here ever, because the code is
                // validated right
                // after scan
                assert false;
                showEndDialog(getResources().getString(R.string.sync_device_failure), () -> {});
                return;
            } else {
                String qrCodeString = getBraveSyncWorker().getQrDataJson(seedHex);
                assert qrCodeString != null && !qrCodeString.isEmpty();

                LocalDateTime notAfterTime = getNotAfterFromQrCodeString(qrCodeString);

                setQrCountDown(notAfterTime);

                ChromeBrowserInitializer.getInstance()
                        .runNowOrAfterFullBrowserStarted(() -> fillQrCode(qrCodeString));

                mQRContainer.setVisibility(View.VISIBLE);
                mNewQrCodeButton.setVisibility(View.GONE);
            }
        }
    }

    private LocalDateTime getNotAfterFromQrCodeString(String qrCodeString) {
        try {
            JSONObject result = new JSONObject(qrCodeString);
            assert result.getInt("version") == 2;
            int notAfterSecondsSinceUnixEpoch = result.getInt("not_after");
            LocalDateTime notAfterTime =
                    LocalDateTime.ofEpochSecond(notAfterSecondsSinceUnixEpoch, 0, ZoneOffset.UTC);
            return notAfterTime;
        } catch (JSONException e) {
            Log.e(TAG, "generateNewQrCode JSONException error " + e);
        } catch (IllegalStateException e) {
            Log.e(TAG, "generateNewQrCode IllegalStateException error " + e);
        }
        return null;
    }

    // This function used when we have scanned the QR code to connect to the chain
    private void seedHexReceived(String seedHex) {
        assert seedHex != null && !seedHex.isEmpty();
        boolean seedHexValid = isSeedHexValid(seedHex);
        assert seedHexValid;

        if (null == getActivity()) {
            return;
        }
        getActivity()
                .runOnUiThread(
                        new Runnable() {
                            @Override
                            public void run() {
                                String seedWords =
                                        getBraveSyncWorker().getWordsFromSeedHex(seedHex);
                                seedWordsReceivedImpl(seedWords, SyncInputType.JOIN);
                            }
                        });
    }

    // This function used in two cases:
    // 1) when we have entered the code words to connect to the chain
    // 2) when we have created a new chain
    private void seedWordsReceived(String seedWords, SyncInputType syncInputType) {
        if (null == getActivity()) {
            return;
        }
        getActivity().runOnUiThread(new Runnable() {
            @Override
            public void run() {
                seedWordsReceivedImpl(seedWords, syncInputType);
            }
        });
    }

    private enum FinalWarningFor { QR_CODE, CODE_WORDS }
    private void showFinalSecurityWarning(
            FinalWarningFor warningFor, Runnable runWhenYes, Runnable runWhenCancel) {
        ThreadUtils.assertOnUiThread();
        AlertDialog.Builder confirmDialog = new AlertDialog.Builder(getActivity());
        confirmDialog.setTitle(
                getActivity().getResources().getString(R.string.sync_final_warning_title));

        String text = "";
        if (warningFor == FinalWarningFor.QR_CODE) {
            text = getActivity().getResources().getString(R.string.sync_final_warning_text_qr_code);
        } else if (warningFor == FinalWarningFor.CODE_WORDS) {
            text = getActivity().getResources().getString(
                    R.string.sync_final_warning_text_code_words);
        } else {
            assert false;
        }

        confirmDialog.setMessage(text);
        confirmDialog.setPositiveButton(
                getActivity().getResources().getString(R.string.sync_final_warning_yes_button),
                (dialog, which) -> {
                    runWhenYes.run();
                    dialog.dismiss();
                    mFinalWarningDialog = null;
                });
        confirmDialog.setNegativeButton(
                getActivity().getResources().getString(android.R.string.cancel),
                (dialog, which) -> {
                    runWhenCancel.run();
                    dialog.dismiss();
                    mFinalWarningDialog = null;
                });
        confirmDialog.setOnCancelListener((dialog) -> {
            runWhenCancel.run();
            mFinalWarningDialog = null;
        });

        mFinalWarningDialog = confirmDialog.show();
    }

    private void allowNewQrScan() {
        synchronized (mQrInProcessingOrFinalizedLock) {
            if (mQrInProcessingOrFinalized) {
                mQrInProcessingOrFinalized = false;
            }
        }
    }

    private enum SyncInputType { NEW, JOIN }
    private void seedWordsReceivedImpl(String seedWords, SyncInputType syncInputType) {
        pauseSyncStateChangedObserver();

        getBraveSyncWorker()
                .setJoinSyncChainCallback(
                        (Boolean result) -> {
                            if (result) {
                                if (syncInputType == SyncInputType.JOIN) {
                                    setAppropriateView();
                                }
                            } else {
                                // TODO(AlexeyBarabash): consider to have error code if there will
                                // be more than only one case of failure
                                onSyncError(
                                        getResources()
                                                .getString(
                                                        R.string
                                                                .brave_sync_joining_deleted_account));
                                allowNewQrScan();
                            }
                        });

        getBraveSyncWorker().requestSync();
        getBraveSyncWorker().saveCodephrase(seedWords);
        getBraveSyncWorker().finalizeSyncSetup();
    }

    private void showMainSyncScrypt() {
        if (null != mScrollViewSyncInitial) {
            mScrollViewSyncInitial.setVisibility(View.VISIBLE);
        }
        if (null != mScrollViewAddMobileDevice) {
            mScrollViewAddMobileDevice.setVisibility(View.GONE);
        }
        if (null != mScrollViewAddLaptop) {
            mScrollViewAddLaptop.setVisibility(View.GONE);
        }
        if (null != mScrollViewSyncStartChain) {
            mScrollViewSyncStartChain.setVisibility(View.GONE);
        }
        if (null != mScrollViewSyncChainCode) {
            mScrollViewSyncChainCode.setVisibility(View.GONE);
        }
        if (null != mScrollViewEnterCodeWords) {
            mScrollViewEnterCodeWords.setVisibility(View.GONE);
        }
    }

    // Handles the requesting of the camera permission.
    private void requestCameraPermission() {
        Log.w(TAG, "Camera permission is not granted. Requesting permission");

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            final String[] permissions = new String[] {Manifest.permission.CAMERA};

            requestPermissions(permissions, RC_HANDLE_CAMERA_PERM);
        }
    }

    @SuppressLint("InlinedApi")
    private void createCameraSource(boolean autoFocus, boolean useFlash) {
        Context context = getActivity().getApplicationContext();

        // A barcode detector is created to track barcodes.  An associated multi-processor instance
        // is set to receive the barcode detection results, track the barcodes, and maintain
        // graphics for each barcode on screen.  The factory is used by the multi-processor to
        // create a separate tracker instance for each barcode.
        BarcodeDetector barcodeDetector =
                new BarcodeDetector.Builder(context).setBarcodeFormats(Barcode.ALL_FORMATS).build();
        BarcodeTrackerFactory barcodeFactory = new BarcodeTrackerFactory(this);
        barcodeDetector.setProcessor(new MultiProcessor.Builder<>(barcodeFactory).build());

        if (!barcodeDetector.isOperational()) {
            // Note: The first time that an app using the barcode or face API is installed on a
            // device, GMS will download a native libraries to the device in order to do detection.
            // Usually this completes before the app is run for the first time.  But if that
            // download has not yet completed, then the above call will not detect any barcodes.
            //
            // isOperational() can be used to check if the required native libraries are currently
            // available.  The detectors will automatically become operational once the library
            // downloads complete on device.
            Log.w(TAG, "Detector dependencies are not yet available.");
        }

        // Creates and starts the camera.  Note that this uses a higher resolution in comparison
        // to other detection examples to enable the barcode detector to detect small barcodes
        // at long distances.
        DisplayMetrics metrics = new DisplayMetrics();
        getActivity().getWindowManager().getDefaultDisplay().getMetrics(metrics);

        CameraSource.Builder builder =
                new CameraSource.Builder(getActivity().getApplicationContext(), barcodeDetector)
                        .setFacing(CameraSource.CAMERA_FACING_BACK)
                        .setRequestedPreviewSize(metrics.widthPixels, metrics.heightPixels)
                        .setRequestedFps(24.0f);

        // make sure that auto focus is an available option
        builder = builder.setFocusMode(
                autoFocus ? Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE : null);

        mCameraSource =
                builder.setFlashMode(useFlash ? Camera.Parameters.FLASH_MODE_TORCH : null).build();
    }

    private void startCameraSource() throws SecurityException {
        if (mCameraSource != null && mCameraSourcePreview.mCameraExist) {
            // check that the device has play services available.
            try {
                int code = GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(
                        getActivity().getApplicationContext());
                if (code != ConnectionResult.SUCCESS) {
                    Dialog dlg = GoogleApiAvailability.getInstance().getErrorDialog(
                            getActivity(), code, RC_HANDLE_GMS);
                    if (null != dlg) {
                        dlg.show();
                    }
                }
            } catch (ActivityNotFoundException e) {
                Log.e(TAG, "Unable to start camera source.", e);
                mCameraSource.release();
                mCameraSource = null;

                return;
            }
            try {
                mCameraSourcePreview.start(mCameraSource);
            } catch (IOException e) {
                Log.e(TAG, "Unable to start camera source.", e);
                mCameraSource.release();
                mCameraSource = null;
            }
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        try {
            if (null != mCameraSourcePreview
                    && View.GONE != mScrollViewSyncChainCode.getVisibility()) {
                startCameraSource();
            }
        } catch (SecurityException se) {
            Log.e(TAG, "Do not have permission to start the camera", se);
        } catch (RuntimeException e) {
            Log.e(TAG, "Could not start camera source.", e);
        }

        if (mFinalWarningDialog != null) {
            mFinalWarningDialog.dismiss();
            synchronized (mQrInProcessingOrFinalizedLock) {
                mQrInProcessingOrFinalized = false;
            }
        }
    }

    @Override
    public void onPause() {
        super.onPause();
        if (mCameraSourcePreview != null) {
            mCameraSourcePreview.stop();
        }
        InputMethodManager imm =
                (InputMethodManager) getActivity().getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.hideSoftInputFromWindow(getView().getWindowToken(), 0);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mCameraSourcePreview != null) {
            mCameraSourcePreview.release();
        }

        SyncServiceFactory.getForProfile(getProfile()).removeSyncStateChangedListener(this);

        if (mDeviceInfoObserverSet) {
            BraveSyncDevices.get().removeDeviceInfoChangedListener(this);
            mDeviceInfoObserverSet = false;
        }
    }

    // Barcode is valid when its raw representation has length of 64
    // and when it's possible to convert the barcode to bip39 code words
    private boolean isSeedHexValid(String barcode) {
        Log.v(TAG, "isSeedHexValid barcode length=" + barcode.length());
        if (barcode == null) {
            Log.e(TAG, "Barcode is empty");
            return false;
        }
        if (barcode.length() != 64) {
            Log.e(TAG, "Wrong barcode data length " + barcode.length() + " instead of 64");
            return false;
        }
        String seedWords = getBraveSyncWorker().getWordsFromSeedHex(barcode);
        if (seedWords == null || seedWords.isEmpty()) {
            Log.e(TAG, "Wrong sync words converted from barcode");
            return false;
        }
        return true;
    }

    private String getQrCodeValidationString(String jsonQr) {
        Log.v(TAG, "getQrCodeValidationString jsonQr length=" + jsonQr.length());
        int validationResult = getBraveSyncWorker().getQrCodeValidationResult(jsonQr);
        Log.v(TAG, "validationResult is " + validationResult);
        switch (validationResult) {
            case 0:
                // kValid, empty string indicates there is no error
                return "";
            case 3:
                // kVersionDeprecated
                return getResources().getString(R.string.brave_sync_code_from_deprecated_version);
            case 4:
                // kExpired
                return getResources().getString(R.string.brave_sync_code_expired);
            case 5:
                // kValidForTooLong
                return getResources().getString(R.string.brave_sync_code_valid_for_too_long);
            default:
                // These three different types of errors have the same message
                // kNotWellFormed
                // kVersionNotRecognized
                return getResources().getString(R.string.brave_sync_wrong_qrcode_error);
        }
    }

    private final Object mQrInProcessingOrFinalizedLock = new Object();
    private boolean mQrInProcessingOrFinalized;

    @Override
    public void onDetectedQrCode(Barcode barcode) {
        if (barcode != null) {
            synchronized (mQrInProcessingOrFinalizedLock) {
                // If camera looks on the QR image and final security warning is shown,
                // we could arrive here again and show 2nd alert while the 1st is not yet closed;
                // Also when QR is scanned, verified and accepted, we don't anything more
                if (mQrInProcessingOrFinalized) {
                    return;
                }
                mQrInProcessingOrFinalized = true;
            }

            final String jsonQr = barcode.displayValue;
            String validationError = getQrCodeValidationString(jsonQr);

            if (!validationError.isEmpty()) {
                getActivity().runOnUiThread(() -> {
                    showEndDialog(validationError, new Runnable() {
                        @Override
                        public void run() {
                            synchronized (mQrInProcessingOrFinalizedLock) {
                                assert mQrInProcessingOrFinalized;
                                mQrInProcessingOrFinalized = false;
                            }
                        }
                    });
                });
                return;
            }

            String seedHex = getBraveSyncWorker().getSeedHexFromQrJson(jsonQr);

            // It is supposed to be
            // getActivity().runOnUiThread(() -> {
            //     showFinalSecurityWarning(... ,
            //         () -> {},
            //         () -> {}
            //     )
            // })
            // but cl format then makes a glitchy formatting and lint does not
            // accept the well-formatted change. So use the full syntax for Runnable.

            getActivity().runOnUiThread(() -> {
                showFinalSecurityWarning(FinalWarningFor.QR_CODE,
                        new Runnable() {
                            @Override
                            public void run() {
                                synchronized (mQrInProcessingOrFinalizedLock) {
                                    assert mQrInProcessingOrFinalized;
                                    // Supposed to set here mQrInProcessingOrFinalized
                                    // to true, because we validated and accepted the QR code
                                    // and don't need the new QR codes to be detected.
                                    // To allow new QR scans, at setJoinExistingChainLayout
                                    // the flag mQrInProcessingOrFinalized will be reset.
                                }

                                // We have the confirmation from user
                                // seedHexReceived will call setAppropriateView
                                seedHexReceived(seedHex);
                            }
                        },
                        new Runnable() {
                            @Override
                            public void run() {
                                synchronized (mQrInProcessingOrFinalizedLock) {
                                    assert mQrInProcessingOrFinalized;
                                    mQrInProcessingOrFinalized = false;
                                }
                            }
                        });
            });
        }
    }

    private void showEndDialog(String message, Runnable runWhenDismissed) {
        AlertDialog.Builder alertBuilder =
                new AlertDialog.Builder(getActivity(), R.style.ThemeOverlay_BrowserUI_AlertDialog);
        DialogInterface.OnClickListener onClickListener = new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int button) {}
        };
        AlertDialog alertDialog =
                alertBuilder.setTitle(getResources().getString(R.string.brave_sync_device))
                        .setMessage(message)
                        .setPositiveButton(R.string.ok, onClickListener)
                        .create();
        alertDialog.getDelegate().setHandleNativeActionModesEnabled(false);
        alertDialog.setOnDismissListener((dialog) -> { runWhenDismissed.run(); });
        alertDialog.show();
    }

    private void deleteDeviceDialog(BraveSyncDevices.SyncDeviceInfo device) {
        AlertDialog.Builder alertBuilder =
                new AlertDialog.Builder(getActivity(), R.style.ThemeOverlay_BrowserUI_AlertDialog);
        DialogInterface.OnClickListener onClickListener =
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int button) {
                        if (button == AlertDialog.BUTTON_POSITIVE) {
                            if (device.mIsCurrentDevice) {
                                resumeSyncStateChangedObserver();
                                getBraveSyncWorker().resetSync();
                                startLeaveSyncChainOperations();
                            } else {
                                BraveSyncDevices.get().deleteDevice(device.mGuid);
                            }
                        }
                    }
                };
        String deviceNameToDisplay = device.mName;
        if (device.mIsCurrentDevice) {
            deviceNameToDisplay = deviceNameToDisplay + " "
                    + getResources().getString(R.string.brave_sync_this_device_text);
        }
        AlertDialog alertDialog =
                alertBuilder
                        .setTitle(getResources().getString(R.string.brave_sync_remove_device_text))
                        .setMessage(
                                getString(R.string.brave_sync_delete_device, deviceNameToDisplay))
                        .setPositiveButton(R.string.ok, onClickListener)
                        .setNegativeButton(R.string.cancel, onClickListener)
                        .create();
        alertDialog.getDelegate().setHandleNativeActionModesEnabled(false);
        alertDialog.show();
    }

    private void permanentlyDeleteAccount() {
        AlertDialog.Builder alertBuilder =
                new AlertDialog.Builder(getActivity(), R.style.ThemeOverlay_BrowserUI_AlertDialog);
        DialogInterface.OnClickListener onClickListener = new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int button) {
                if (button == AlertDialog.BUTTON_POSITIVE) {
                    permanentlyDeleteAccountImpl();
                }
            }
        };

        AlertDialog alertDialog =
                alertBuilder
                        .setTitle(getResources().getString(R.string.brave_sync_delete_account_text))
                        .setMessage(getString(R.string.brave_sync_delete_account_message))
                        .setPositiveButton(
                                R.string.brave_sync_delete_account_button, onClickListener)
                        .setNegativeButton(R.string.cancel, onClickListener)
                        .create();
        alertDialog.getDelegate().setHandleNativeActionModesEnabled(false);
        alertDialog.show();
    }

    private void permanentlyDeleteAccountImpl() {
        getBraveSyncWorker()
                .permanentlyDeleteAccount(
                        (String result) -> {
                            permanentlyDeletedAccount(result);
                        });
    }

    private void permanentlyDeletedAccount(String result) {
        FragmentActivity activity = getActivity();
        if (activity == null) {
            return;
        }
        if (result == null || result.isEmpty()) {
            showToast(
                    activity,
                    getResources().getString(R.string.brave_sync_delete_account_succeeded_toast)
                            + " "
                            + result);
        } else {
            showToast(
                    activity,
                    getResources().getString(R.string.brave_sync_delete_account_failed_toast)
                            + " "
                            + result);
        }
    }

    private void showToast(FragmentActivity activity, String text) {
        Toast.makeText(activity.getApplicationContext(), text, Toast.LENGTH_LONG).show();
    }

    private boolean mLeaveSyncChainInProgress;

    private void startLeaveSyncChainOperations() {
        mLeaveSyncChainInProgress = true;
        invalidateCodephrase();

        mBraveSyncTextDevicesTitle.setText(
                getResources().getString(R.string.brave_sync_leaving_sync_chain_title));

        mShowCategoriesButton.setVisibility(View.GONE);
        mAddDeviceButton.setVisibility(View.GONE);

        ViewGroup devicesUiGroup = (ViewGroup) getView().findViewById(R.id.brave_sync_devices);
        if (devicesUiGroup != null) {
            devicesUiGroup.removeAllViews();
        }
    }

    private void leaveSyncChainComplete() {
        if (mLeaveSyncChainInProgress) {
            mLeaveSyncChainInProgress = false;

            mShowCategoriesButton.setVisibility(View.VISIBLE);
            mAddDeviceButton.setVisibility(View.VISIBLE);
        }
    }

    private boolean mSyncStateChangedObserverPaused;

    private void pauseSyncStateChangedObserver() {
        mSyncStateChangedObserverPaused = true;
    }

    private void resumeSyncStateChangedObserver() {
        mSyncStateChangedObserverPaused = false;
    }

    private boolean isSyncStateChangedObserverPaused() {
        return mSyncStateChangedObserverPaused;
    }

    @Override
    public void syncStateChanged() {
        if (isSyncStateChangedObserverPaused()) {
            return;
        }
        if (!SyncServiceFactory.getForProfile(getProfile()).isInitialSyncFeatureSetupComplete()) {
            if (mLeaveSyncChainInProgress) {
                leaveSyncChainComplete();
            } else {
                invalidateCodephrase();
            }
            setAppropriateView();
        }
    }

    private void setJoinExistingChainLayout() {
        if (null != mScrollViewSyncInitial) {
            mScrollViewSyncInitial.setVisibility(View.GONE);
        }
        if (null != mScrollViewEnterCodeWords) {
            mScrollViewEnterCodeWords.setVisibility(View.GONE);
        }
        if (null != mScrollViewAddMobileDevice) {
            mScrollViewAddMobileDevice.setVisibility(View.GONE);
        }
        if (null != mScrollViewAddLaptop) {
            mScrollViewAddLaptop.setVisibility(View.GONE);
        }
        if (null != mScrollViewSyncStartChain) {
            mScrollViewSyncStartChain.setVisibility(View.GONE);
        }

        if (ensureCameraPermission()) {
            createCameraSource(true, false);
        }

        getActivity().setTitle(R.string.brave_sync_scan_chain_code);
        if (null != mScrollViewSyncChainCode) {
            mScrollViewSyncChainCode.setVisibility(View.VISIBLE);
        }

        synchronized (mQrInProcessingOrFinalizedLock) {
            mQrInProcessingOrFinalized = false;
        }

        if (null != mCameraSourcePreview) {
            int rc =
                    ActivityCompat.checkSelfPermission(
                            getActivity().getApplicationContext(), Manifest.permission.CAMERA);
            if (rc == PackageManager.PERMISSION_GRANTED) {
                try {
                    startCameraSource();
                } catch (SecurityException exc) {
                }
            }
        }
    }

    private void setNewChainLayout() {
        getActivity().setTitle(R.string.brave_sync_start_new_chain);
        if (null != mScrollViewSyncInitial) {
            mScrollViewSyncInitial.setVisibility(View.GONE);
        }
        if (null != mScrollViewEnterCodeWords) {
            mScrollViewEnterCodeWords.setVisibility(View.GONE);
        }
        if (null != mScrollViewAddMobileDevice) {
            mScrollViewAddMobileDevice.setVisibility(View.GONE);
        }
        if (null != mScrollViewAddLaptop) {
            mScrollViewAddLaptop.setVisibility(View.GONE);
        }
        if (null != mScrollViewSyncStartChain) {
            mScrollViewSyncStartChain.setVisibility(View.VISIBLE);
        }
        if (null != mScrollViewSyncDone) {
            mScrollViewSyncDone.setVisibility(View.GONE);
        }
        adjustImageButtons(
                getActivity()
                        .getApplicationContext()
                        .getResources()
                        .getConfiguration()
                        .orientation);
    }

    private String mCodephrase;

    public String getPureWords() {
        if (mCodephrase == null || mCodephrase.isEmpty()) {
            mCodephrase = getBraveSyncWorker().getPureWords();
        }
        return mCodephrase;
    }

    public void invalidateCodephrase() {
        mCodephrase = null;
    }

    private void setAddMobileDeviceLayout() {
        getActivity().setTitle(R.string.brave_sync_btn_mobile);

        if (null != mScrollViewSyncInitial) {
            mScrollViewSyncInitial.setVisibility(View.GONE);
        }
        if (null != mScrollViewEnterCodeWords) {
            mScrollViewEnterCodeWords.setVisibility(View.GONE);
        }
        if (null != mAddDeviceTab) {
            mAddDeviceTab.setVisibility(View.VISIBLE);
        }
        if (null != mScrollViewAddMobileDevice) {
            mScrollViewAddMobileDevice.setVisibility(View.VISIBLE);
        }
        if (null != mScrollViewAddLaptop) {
            mScrollViewAddLaptop.setVisibility(View.GONE);
        }
        if (null != mScrollViewSyncStartChain) {
            mScrollViewSyncStartChain.setVisibility(View.GONE);
        }

        generateNewQrCode();
    }

    private void setQrCountDown(LocalDateTime notAfterTime) {
        FragmentContainerView the_view =
                (FragmentContainerView) getView().findViewById(R.id.brave_sync_count_down_qr);
        BraveSyncCodeCountdownFragment countdown = the_view.getFragment();
        countdown.setExpiredRunnable(
                () -> {
                    mNewQrCodeButton.setVisibility(View.VISIBLE);
                    mQRContainer.setVisibility(View.GONE);
                });
        countdown.setNotAfter(notAfterTime);
    }

    private void fillQrCode(String qrDataFinal) {
        Bitmap bitmap = QRCodeGenerator.generateBitmap(qrDataFinal);
        mQRCodeImage.setImageBitmap(bitmap);
        mQRCodeImage.invalidate();
    }

    private void setAddLaptopLayout() {
        getActivity().setTitle(R.string.brave_sync_btn_laptop);

        if (null != mScrollViewSyncInitial) {
            mScrollViewSyncInitial.setVisibility(View.GONE);
        }
        if (null != mScrollViewEnterCodeWords) {
            mScrollViewEnterCodeWords.setVisibility(View.GONE);
        }
        if (null != mScrollViewAddMobileDevice) {
            mScrollViewAddMobileDevice.setVisibility(View.GONE);
        }
        if (null != mAddDeviceTab) {
            mAddDeviceTab.setVisibility(View.VISIBLE);
        }
        if (null != mScrollViewAddLaptop) {
            mScrollViewAddLaptop.setVisibility(View.VISIBLE);
        }
        if (null != mScrollViewSyncStartChain) {
            mScrollViewSyncStartChain.setVisibility(View.GONE);
        }

        generateNewCodeWords();
    }

    private void setWordsCountDown(LocalDateTime notAfterTime) {
        FragmentContainerView containerView =
                (FragmentContainerView)
                        getView().findViewById(R.id.brave_sync_count_down_code_words);
        BraveSyncCodeCountdownFragment countdown = containerView.getFragment();
        countdown.setExpiredRunnable(
                () -> {
                    mNewCodeWordsButton.setVisibility(View.VISIBLE);
                    mBraveSyncAddDeviceCodeWords.setVisibility(View.GONE);
                    mCopyButton.setVisibility(View.GONE);
                });
        countdown.setNotAfter(notAfterTime);
    }

    private void setSyncDoneLayout() {
        if (!mDeviceInfoObserverSet) {
            BraveSyncDevices.get().addDeviceInfoChangedListener(this);
            mDeviceInfoObserverSet = true;
        }

        getActivity()
                .getWindow()
                .setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);
        getActivity().setTitle(R.string.sync_category_title);
        if (null != mCameraSourcePreview) {
            mCameraSourcePreview.stop();
        }
        if (null != mScrollViewSyncInitial) {
            mScrollViewSyncInitial.setVisibility(View.GONE);
        }
        if (null != mScrollViewSyncChainCode) {
            mScrollViewSyncChainCode.setVisibility(View.GONE);
        }
        if (null != mScrollViewEnterCodeWords) {
            mScrollViewEnterCodeWords.setVisibility(View.GONE);
        }
        if (null != mAddDeviceTab) {
            mAddDeviceTab.setVisibility(View.GONE);
        }
        if (null != mScrollViewAddMobileDevice) {
            mScrollViewAddMobileDevice.setVisibility(View.GONE);
        }
        if (null != mScrollViewAddLaptop) {
            mScrollViewAddLaptop.setVisibility(View.GONE);
        }
        if (null != mScrollViewSyncStartChain) {
            mScrollViewSyncStartChain.setVisibility(View.GONE);
        }
        if (null != mScrollViewSyncDone) {
            mScrollViewSyncDone.setVisibility(View.VISIBLE);
        }

        mBraveSyncTextDevicesTitle.setText(
                getResources().getString(R.string.brave_sync_loading_devices_title));

        onDevicesAvailable();
        resumeSyncStateChangedObserver();
    }

    private void adjustWidth(View view, int orientation) {
        if (orientation != Configuration.ORIENTATION_LANDSCAPE && DeviceFormFactor.isTablet()) {
            DisplayMetrics metrics = new DisplayMetrics();
            getActivity().getWindowManager().getDefaultDisplay().getMetrics(metrics);
            LayoutParams params = view.getLayoutParams();
            params.width = metrics.widthPixels * 3 / 5;
            params.height = metrics.heightPixels * 3 / 5;
            view.setLayoutParams(params);
        }
    }

    private void adjustImageButtons(int orientation) {
        if ((null != mLayoutSyncStartChain)
                && (null != mScrollViewSyncStartChain)
                && (View.VISIBLE == mScrollViewSyncStartChain.getVisibility())) {
            adjustWidth(mScrollViewSyncStartChain, orientation);
            LayoutParams params = mLayoutMobile.getLayoutParams();
            if (orientation == Configuration.ORIENTATION_LANDSCAPE
                    && !DeviceFormFactor.isTablet()) {
                mLayoutSyncStartChain.setOrientation(LinearLayout.HORIZONTAL);
                params.width = 0;
                params.height = LayoutParams.MATCH_PARENT;
            } else {
                mLayoutSyncStartChain.setOrientation(LinearLayout.VERTICAL);
                params.width = LayoutParams.MATCH_PARENT;
                params.height = 0;
            }
            mLayoutMobile.setLayoutParams(params);
            mLayoutLaptop.setLayoutParams(params);
        }
    }

    private void clearBackground(View view) {
        if (null != view) {
            view.setBackgroundColor(Color.TRANSPARENT);
        }
    }

    private Context getBaseApplicationContext() {
        Context context = ContextUtils.getApplicationContext();
        if (context instanceof ContextWrapper) {
            return ((ContextWrapper) context).getBaseContext();
        } else {
            return context;
        }
    }

    // Handles 'Back' button. Returns true if it is handled, false otherwise.
    @Override
    public boolean onBackPressed() {
        if ((View.VISIBLE == mScrollViewSyncChainCode.getVisibility())
                || (View.VISIBLE == mScrollViewSyncStartChain.getVisibility())) {
            setAppropriateView();
            return true;
        } else if ((View.VISIBLE == mScrollViewAddMobileDevice.getVisibility())
                || (View.VISIBLE == mScrollViewAddLaptop.getVisibility())) {
            setNewChainLayout();
            return true;
        } else if (View.VISIBLE == mScrollViewEnterCodeWords.getVisibility()) {
            setJoinExistingChainLayout();
            return true;
        }
        return false;
    }

    @Override
    public void onCreatePreferences(Bundle bundle, String s) {} /**/
}
