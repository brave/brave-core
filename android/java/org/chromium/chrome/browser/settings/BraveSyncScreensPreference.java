/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

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
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.hardware.Camera;
import android.os.Build;
import android.os.Bundle;
import android.text.Editable;
import android.text.SpannableString;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.text.style.ForegroundColorSpan;
import android.text.style.RelativeSizeSpan;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.ScrollView;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.widget.AppCompatImageView;
import androidx.core.app.ActivityCompat;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.google.android.gms.vision.MultiProcessor;
import com.google.android.gms.vision.barcode.Barcode;
import com.google.android.gms.vision.barcode.BarcodeDetector;
import com.google.zxing.BarcodeFormat;
import com.google.zxing.MultiFormatWriter;
import com.google.zxing.WriterException;
import com.google.zxing.common.BitMatrix;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.task.PostTask;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveSyncWorker;
import org.chromium.chrome.browser.BraveSyncReflectionUtils;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.qrreader.BarcodeTracker;
import org.chromium.chrome.browser.qrreader.BarcodeTrackerFactory;
import org.chromium.chrome.browser.qrreader.CameraSource;
import org.chromium.chrome.browser.qrreader.CameraSourcePreview;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.chrome.browser.settings.SettingsActivity;
import org.chromium.chrome.browser.settings.SettingsLauncher;
import org.chromium.chrome.browser.sync.BraveSyncDevices;
import org.chromium.chrome.browser.sync.ProfileSyncService;
import org.chromium.chrome.browser.sync.settings.BraveManageSyncSettings;
import org.chromium.content_public.browser.UiThreadTaskTraits;
import org.chromium.ui.KeyboardVisibilityDelegate;
import org.chromium.ui.base.DeviceFormFactor;

import java.io.IOException;
import java.lang.Runnable;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Settings fragment that allows to control Sync functionality.
 */
public class BraveSyncScreensPreference extends BravePreferenceFragment
        implements View.OnClickListener, SettingsActivity.OnBackPressedListener,
                   BarcodeTracker.BarcodeGraphicTrackerCallback,
                   BraveSyncDevices.DeviceInfoChangedListener,
                   ProfileSyncService.SyncStateChangedListener {
    public static final int BIP39_WORD_COUNT = 24;
    private static final String TAG = "SYNC";
    // Permission request codes need to be < 256
    private static final int RC_HANDLE_CAMERA_PERM = 2;
    // Intent request code to handle updating play services if needed.
    private static final int RC_HANDLE_GMS = 9001;
    // For QR code generation
    private static final int WHITE = 0xFFFFFFFF;
    private static final int BLACK = 0xFF000000;
    private static final int WIDTH = 300;
    // For view sizes limit
    private static final int MAX_WIDTH = 512;
    private static final int MAX_HEIGHT = 1024;

    // The have a sync code button displayed in the Sync view.
    private Button mScanChainCodeButton;
    private Button mStartNewChainButton;
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
    private Button mRemoveDeviceButton;
    private Button mShowCategoriesButton;
    private Button mQRCodeButton;
    private Button mCodeWordsButton;
    // Brave Sync message text view
    private TextView mBraveSyncTextViewInitial;
    private TextView mBraveSyncTextViewSyncChainCode;
    private TextView mBraveSyncTextViewAddMobileDevice;
    private TextView mBraveSyncTextViewAddLaptop;
    private TextView mBraveSyncWarningTextViewAddMobileDevice;
    private TextView mBraveSyncWarningTextViewAddLaptop;
    private TextView mBraveSyncTextDevicesTitle;
    private TextView mBraveSyncWordCountTitle;
    private TextView mBraveSyncAddDeviceCodeWords;
    private CameraSource mCameraSource;
    private CameraSourcePreview mCameraSourcePreview;
    private String mDeviceName = "";
    private ListView mDevicesListView;
    private ArrayAdapter<String> mDevicesAdapter;
    private List<String> mDevicesList;
    private ScrollView mScrollViewSyncInitial;
    private ScrollView mScrollViewSyncChainCode;
    private ScrollView mScrollViewSyncStartChain;
    private ScrollView mScrollViewAddMobileDevice;
    private ScrollView mScrollViewAddLaptop;
    private ScrollView mScrollViewEnterCodeWords;
    private ScrollView mScrollViewSyncDone;
    private LayoutInflater mInflater;
    private ImageView mQRCodeImage;
    private LinearLayout mLayoutSyncStartChain;
    private EditText mCodeWords;
    private FrameLayout mLayoutMobile;
    private FrameLayout mLayoutLaptop;

    BraveSyncWorker getBraveSyncWorker() {
        Object object = BraveSyncReflectionUtils.getSyncWorker();
        if (object == null) {
            return null;
        }
        return (BraveSyncWorker)object;
    }

    @Override
    public void deviceInfoChanged() {
        onDevicesAvailable();
    }
    boolean deviceInfoObserverSet = false;

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

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
        InvalidateCodephrase();

        if (ensureCameraPermission()) {
            createCameraSource(true, false);
        }
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
        // We still allow to enter words
        // getActivity().onBackPressed();
    }

    public void onSyncError(String message) {
        try {
            if (null == getActivity()) {
                return;
            }
            if (null != message && !message.isEmpty()) {
                message = " [" + message + "]";
            }
            final String messageFinal = (null == message) ? "" : message;
            getActivity().runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    showEndDialog(
                            getResources().getString(R.string.sync_device_failure) + messageFinal);
                }
            });
        } catch (Exception exc) {
            Log.e(TAG, "onSyncError exception: " + exc);
        }
    }

    public void onDevicesAvailable() {
        try {
            if (null == getActivity()) {
                return;
            }
            getActivity().runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    if (View.VISIBLE != mScrollViewSyncDone.getVisibility()) {
                        Log.w(TAG, "No need to load devices for other pages");
                        return;
                    }
                    ArrayList<BraveSyncDevices.SyncDeviceInfo> deviceInfos =
                            BraveSyncDevices.get().GetSyncDeviceList();
                    Log.v(TAG, "Got " + deviceInfos.size() + " devices");
                    ViewGroup insertPoint =
                            (ViewGroup) getView().findViewById(R.id.brave_sync_devices);
                    insertPoint.removeAllViews();
                    int index = 0;
                    for (BraveSyncDevices.SyncDeviceInfo device : deviceInfos) {
                        View separator = (View) mInflater.inflate(R.layout.menu_separator, null);
                        View listItemView =
                                (View) mInflater.inflate(R.layout.brave_sync_device, null);
                        if (null != listItemView && null != separator && null != insertPoint) {
                            TextView textView = (TextView) listItemView.findViewById(
                                    R.id.brave_sync_device_text);
                            if (null != textView) {
                                textView.setText(device.mName);
                            }

                            if (device.mIsCurrentDevice) {
                                mDeviceName = device.mName;
                                // Current device is deleted by button on the bottom
                                if (null != textView) {
                                    // Highlight curret device
                                    textView.setTextColor(ApiCompatibilityUtils.getColor(
                                            getActivity().getResources(),
                                            R.color.brave_theme_color));
                                    String currentDevice = device.mName + " "
                                            + getResources().getString(
                                                    R.string.brave_sync_this_device_text);
                                    textView.setText(currentDevice);
                                }
                                // mRemoveDeviceButton is always visible, we can leave the chain
                                // in any time with sync v2 (except we are now doing reset)
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
                }
            });
        } catch (Exception exc) {
            Log.e(TAG, "onDevicesAvailable exception: " + exc);
        }
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        getActivity().setTitle(R.string.sign_in_sync);

        mScrollViewSyncInitial = (ScrollView) getView().findViewById(R.id.view_sync_initial);
        mScrollViewSyncChainCode = (ScrollView) getView().findViewById(R.id.view_sync_chain_code);
        mScrollViewSyncStartChain = (ScrollView) getView().findViewById(R.id.view_sync_start_chain);
        mScrollViewAddMobileDevice =
                (ScrollView) getView().findViewById(R.id.view_add_mobile_device);
        mScrollViewAddLaptop = (ScrollView) getView().findViewById(R.id.view_add_laptop);
        mScrollViewEnterCodeWords = (ScrollView) getView().findViewById(R.id.view_enter_code_words);
        mScrollViewSyncDone = (ScrollView) getView().findViewById(R.id.view_sync_done);

        if (!DeviceFormFactor.isTablet()) {
            clearBackground(mScrollViewSyncInitial);
            clearBackground(mScrollViewSyncChainCode);
            clearBackground(mScrollViewSyncStartChain);
            clearBackground(mScrollViewAddMobileDevice);
            clearBackground(mScrollViewAddLaptop);
            clearBackground(mScrollViewEnterCodeWords);
            clearBackground(mScrollViewSyncDone);
        }

        mLayoutSyncStartChain =
                (LinearLayout) getView().findViewById(R.id.view_sync_start_chain_layout);

        mScanChainCodeButton = (Button) getView().findViewById(R.id.brave_sync_btn_scan_chain_code);
        if (mScanChainCodeButton != null) {
            mScanChainCodeButton.setOnClickListener(this);
        }

        mStartNewChainButton = (Button) getView().findViewById(R.id.brave_sync_btn_start_new_chain);
        if (mStartNewChainButton != null) {
            mStartNewChainButton.setOnClickListener(this);
        }

        mEnterCodeWordsButton =
                (Button) getView().findViewById(R.id.brave_sync_btn_enter_code_words);
        if (mEnterCodeWordsButton != null) {
            mEnterCodeWordsButton.setOnClickListener(this);
        }

        mQRCodeImage = (ImageView) getView().findViewById(R.id.brave_sync_qr_code_image);

        mDoneButton = (Button) getView().findViewById(R.id.brave_sync_btn_done);
        if (mDoneButton != null) {
            mDoneButton.setOnClickListener(this);
        }

        mDoneLaptopButton = (Button) getView().findViewById(R.id.brave_sync_btn_add_laptop_done);
        if (mDoneLaptopButton != null) {
            mDoneLaptopButton.setOnClickListener(this);
        }

        mUseCameraButton = (Button) getView().findViewById(R.id.brave_sync_btn_use_camera);
        if (mUseCameraButton != null) {
            mUseCameraButton.setOnClickListener(this);
        }

        mConfirmCodeWordsButton =
                (Button) getView().findViewById(R.id.brave_sync_confirm_code_words);
        if (mConfirmCodeWordsButton != null) {
            mConfirmCodeWordsButton.setOnClickListener(this);
        }

        mMobileButton = (ImageButton) getView().findViewById(R.id.brave_sync_btn_mobile);
        if (mMobileButton != null) {
            mMobileButton.setOnClickListener(this);
        }

        mLaptopButton = (ImageButton) getView().findViewById(R.id.brave_sync_btn_laptop);
        if (mLaptopButton != null) {
            mLaptopButton.setOnClickListener(this);
        }

        mPasteButton = (ImageButton) getView().findViewById(R.id.brave_sync_paste_button);
        if (mPasteButton != null) {
            mPasteButton.setOnClickListener(this);
        }

        mCopyButton = (Button) getView().findViewById(R.id.brave_sync_copy_button);
        if (mCopyButton != null) {
            mCopyButton.setOnClickListener(this);
        }

        mBraveSyncTextViewInitial = (TextView) getView().findViewById(R.id.brave_sync_text_initial);
        mBraveSyncTextViewSyncChainCode =
                (TextView) getView().findViewById(R.id.brave_sync_text_sync_chain_code);
        mBraveSyncTextViewAddMobileDevice =
                (TextView) getView().findViewById(R.id.brave_sync_text_add_mobile_device);
        mBraveSyncTextViewAddLaptop =
                (TextView) getView().findViewById(R.id.brave_sync_text_add_laptop);
        mBraveSyncWarningTextViewAddMobileDevice =
                (TextView) getView().findViewById(R.id.brave_sync_warning_text_add_mobile_device);
        mBraveSyncWarningTextViewAddLaptop =
                (TextView) getView().findViewById(R.id.brave_sync_warning_text_add_laptop);
        mBraveSyncTextDevicesTitle =
                (TextView) getView().findViewById(R.id.brave_sync_devices_title);
        mBraveSyncWordCountTitle =
                (TextView) getView().findViewById(R.id.brave_sync_text_word_count);
        mBraveSyncWordCountTitle.setText(getString(R.string.brave_sync_word_count_text, 0));
        mBraveSyncAddDeviceCodeWords =
                (TextView) getView().findViewById(R.id.brave_sync_add_device_code_words);
        setMainSyncText();
        mCameraSourcePreview = (CameraSourcePreview) getView().findViewById(R.id.preview);

        mAddDeviceButton = (Button) getView().findViewById(R.id.brave_sync_btn_add_device);
        if (null != mAddDeviceButton) {
            mAddDeviceButton.setOnClickListener(this);
        }

        mRemoveDeviceButton = (Button) getView().findViewById(R.id.brave_sync_btn_remove_device);
        if (null != mRemoveDeviceButton) {
            mRemoveDeviceButton.setOnClickListener(this);
        }

        mShowCategoriesButton =
                (Button) getView().findViewById(R.id.brave_sync_btn_show_categories);
        if (null != mShowCategoriesButton) {
            mShowCategoriesButton.setOnClickListener(this);
        }

        mQRCodeButton = (Button) getView().findViewById(R.id.brave_sync_qr_code_off_btn);
        if (null != mQRCodeButton) {
            mQRCodeButton.setOnClickListener(this);
        }

        mCodeWordsButton = (Button) getView().findViewById(R.id.brave_sync_code_words_off_btn);
        if (null != mCodeWordsButton) {
            mCodeWordsButton.setOnClickListener(this);
        }

        mCodeWords = (EditText) getView().findViewById(R.id.code_words);

        mLayoutMobile = (FrameLayout) getView().findViewById(R.id.brave_sync_frame_mobile);
        mLayoutLaptop = (FrameLayout) getView().findViewById(R.id.brave_sync_frame_laptop);

        setAppropriateView();

        super.onActivityCreated(savedInstanceState);
    }

    private void setAppropriateView() {
        getActivity().getWindow().setSoftInputMode(
                WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);
        getActivity().setTitle(R.string.sync_category_title);
        if (getBraveSyncWorker() == null) {
            return;
        }
        boolean firstSetupComplete = getBraveSyncWorker().IsFirstSetupComplete();

        Log.v(TAG, "setAppropriateView first setup complete " + firstSetupComplete);
        if (!firstSetupComplete) {
            if (null != mCameraSourcePreview) {
                mCameraSourcePreview.stop();
            }
            if (null != mScrollViewSyncInitial) {
                adjustWidth(mScrollViewSyncInitial, false);
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

    private void setMainSyncText() {
        setSyncText(getResources().getString(R.string.brave_sync_official),
                getResources().getString(R.string.brave_sync_description_page_1_part_1) + "\n\n"
                        + getResources().getString(R.string.brave_sync_description_page_1_part_2),
                mBraveSyncTextViewInitial);
    }

    private void setQRCodeText() {
        setSyncText("", getResources().getString(R.string.brave_sync_qrcode_message_v2),
                mBraveSyncTextViewSyncChainCode);
    }

    private void setSyncText(String title, String message, TextView textView) {
        String text = "";
        if (title.length() > 0) {
            text = title + "\n\n";
        }
        text += message;
        SpannableString formatedText = new SpannableString(text);
        formatedText.setSpan(new RelativeSizeSpan(1.25f), 0, title.length(), 0);
        textView.setText(formatedText);
    }

    /** OnClickListener for the clear button. We show an alert dialog to confirm the action */
    @Override
    public void onClick(View v) {
        if (getBraveSyncWorker() == null) {
            // It is not expected to reach here
            assert false;
            return;
        }
        if ((getActivity() == null)
                || (v != mScanChainCodeButton && v != mStartNewChainButton
                        && v != mEnterCodeWordsButton && v != mDoneButton && v != mDoneLaptopButton
                        && v != mUseCameraButton && v != mConfirmCodeWordsButton
                        && v != mMobileButton && v != mLaptopButton && v != mPasteButton
                        && v != mCopyButton && v != mRemoveDeviceButton
                        && v != mShowCategoriesButton && v != mAddDeviceButton && v != mQRCodeButton
                        && v != mCodeWordsButton))
            return;

        if (mScanChainCodeButton == v) {
            setJoinExistingChainLayout();
        } else if (mStartNewChainButton == v) {
            // Creating a new chain
            GetCodephrase();
            setNewChainLayout();
            seedWordsReceived(mCodephrase);
        } else if (mMobileButton == v) {
            setAddMobileDeviceLayout();
        } else if (mLaptopButton == v) {
            setAddLaptopLayout();
        } else if (mDoneButton == v) {
            setSyncDoneLayout();
        } else if (mDoneLaptopButton == v) {
            setSyncDoneLayout();
        } else if (mUseCameraButton == v) {
            setJoinExistingChainLayout();
        } else if (mQRCodeButton == v) {
            setAddMobileDeviceLayout();
        } else if (mCodeWordsButton == v) {
            setAddLaptopLayout();
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
            String[] words = mCodeWords.getText()
                                     .toString()
                                     .trim()
                                     .replace("   ", " ")
                                     .replace("\n", " ")
                                     .split(" ");
            if (BIP39_WORD_COUNT != words.length) {
                Log.e(TAG, "Confirm code words - wrong words count " + words.length);
                onSyncError(getResources().getString(R.string.brave_sync_word_count_error));
                return;
            }

            String hexString = getBraveSyncWorker().GetSeedHexFromWords(
                    TextUtils.join(" ", words));
            if (hexString == null || hexString.isEmpty()) {
                Log.e(TAG, "Confirm code words - wrong codephrase");
                onSyncError(getResources().getString(R.string.brave_sync_wrong_code_error));
                return;
            }

            String codephraseCandidate = TextUtils.join(" ", words);
            // Validate the code words with GetSeedHexFromWords
            String seedHex = getBraveSyncWorker().GetSeedHexFromWords(codephraseCandidate);
            if (null == seedHex || seedHex.isEmpty()) {
                onSyncError(getResources().getString(R.string.brave_sync_wrong_code_error));
                return;
            }

            // Code phrase looks valid, we can pass it down to sync system
            mCodephrase = codephraseCandidate;
            seedWordsReceived(mCodephrase);
            setAppropriateView();
        } else if (mEnterCodeWordsButton == v) {
            getActivity().getWindow().setSoftInputMode(
                    WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
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
                adjustWidth(mScrollViewEnterCodeWords, false);
                mScrollViewEnterCodeWords.setVisibility(View.VISIBLE);
            }
            getActivity().setTitle(R.string.brave_sync_code_words_title);
            if (null != mCodeWords && null != mBraveSyncWordCountTitle) {
                mCodeWords.addTextChangedListener(new TextWatcher() {
                    @Override
                    public void afterTextChanged(Editable s) {}

                    @Override
                    public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                    }

                    @Override
                    public void onTextChanged(CharSequence s, int start, int before, int count) {
                        int wordCount = mCodeWords.getText().toString().length();
                        if (0 != wordCount) {
                            String[] words = mCodeWords.getText()
                                                     .toString()
                                                     .trim()
                                                     .replace("   ", " ")
                                                     .replace("\n", " ")
                                                     .split(" ");
                            wordCount = words.length;
                        }
                        mBraveSyncWordCountTitle.setText(
                                getString(R.string.brave_sync_word_count_text, wordCount));
                        mBraveSyncWordCountTitle.invalidate();
                    }
                });
            }
        } else if (mRemoveDeviceButton == v) {
            deleteDeviceDialog(mDeviceName);
        } else if (mShowCategoriesButton == v) {
            SettingsLauncher settingsLauncher = new SettingsLauncherImpl();
            settingsLauncher.launchSettingsActivity(getContext(), BraveManageSyncSettings.class);
        } else if (mAddDeviceButton == v) {
            setNewChainLayout();
        }
    }

    // This function used when we have scanned the QR code to connect to the chain
    private void seedHexReceived(String seedHex) {
        assert seedHex != null && !seedHex.isEmpty();
        assert isBarCodeValid(seedHex);

        if (null == getActivity()) {
            return;
        }
        getActivity().runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (getBraveSyncWorker() != null) {
                    String seedWords = getBraveSyncWorker().GetWordsFromSeedHex(seedHex);
                    seedWordsReceivedImpl(seedWords);
                }
                setAppropriateView();
            }
        });
    }

    // This function used in two cases:
    // 1) when we have entered the code words to connect to the chain
    // 2) when we have created a new chain
    private void seedWordsReceived(String seedWords) {
        if (null == getActivity()) {
            return;
        }
        getActivity().runOnUiThread(new Runnable() {
            @Override
            public void run() {
                seedWordsReceivedImpl(seedWords);
            }
        });
    }

    private void seedWordsReceivedImpl(String seedWords) {
        if (getBraveSyncWorker() != null) {
            getBraveSyncWorker().RequestSync();
            getBraveSyncWorker().SaveCodephrase(seedWords);
            getBraveSyncWorker().FinalizeSyncSetup();
        }
    }

    private void showMainSyncScrypt() {
        if (null != mScrollViewSyncInitial) {
            adjustWidth(mScrollViewSyncInitial, false);
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
        setMainSyncText();
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
        if (deviceInfoObserverSet) {
            BraveSyncDevices.get().removeDeviceInfoChangedListener(this);
            deviceInfoObserverSet = false;
        }
    }

    // Barcode is valid when its raw representation has length of 64
    // and when it's possible to convert the barcode to bip39 code words
    private boolean isBarCodeValid(String barcode) {
        Log.v(TAG, "isBarCodeValid barcode length=" + barcode.length());
        if (barcode == null) {
            Log.e(TAG, "Barcode is empty");
            return false;
        }
        if (barcode.length() != 64) {
            Log.e(TAG, "Wrong barcode data length " + barcode.length() + " instead of 64");
            return false;
        }
        if (getBraveSyncWorker() == null) {
            Log.e(TAG, "Could not get sync worker");
            return false;
        }
        String seedWords = getBraveSyncWorker().GetWordsFromSeedHex(barcode);
        if (seedWords == null || seedWords.isEmpty()) {
            Log.e(TAG, "Wrong sync words converted from barcode");
            return false;
        }
        return true;
    }

    @Override
    public void onDetectedQrCode(Barcode barcode) {
        if (barcode != null) {
            final String barcodeValue = barcode.displayValue;
            Log.e(TAG, "onDetectedQrCode barcodeValue=" + barcodeValue);
            if (!isBarCodeValid(barcodeValue)) {
                getActivity().runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        showEndDialog(
                                getResources().getString(R.string.brave_sync_wrong_qrcode_error));
                        showMainSyncScrypt();
                    }
                });
                return;
            }

            String seedHex = barcodeValue;
            // seedHexReceived will call setAppropriateView
            seedHexReceived(seedHex);
        }
    }

    private void showEndDialog(String message) {
        AlertDialog.Builder alert =
                new AlertDialog.Builder(getActivity(), R.style.Theme_Chromium_AlertDialog);
        if (null == alert) {
            return;
        }
        DialogInterface.OnClickListener onClickListener = new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int button) {}
        };
        AlertDialog alertDialog =
                alert.setTitle(getResources().getString(R.string.brave_sync_device))
                        .setMessage(message)
                        .setPositiveButton(R.string.ok, onClickListener)
                        .create();
        alertDialog.getDelegate().setHandleNativeActionModesEnabled(false);
        alertDialog.show();
    }

    private void deleteDeviceDialog(String deviceName) {
        Log.v(TAG, "deleteDeviceDialog deviceName=" + deviceName);
        AlertDialog.Builder alert = new AlertDialog.Builder(getActivity(), R.style.Theme_Chromium_AlertDialog);
        if (null == alert) {
            return;
        }
        DialogInterface.OnClickListener onClickListener = new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int button) {
                if (button == AlertDialog.BUTTON_POSITIVE) {
                    if (getBraveSyncWorker() != null) {
                        getBraveSyncWorker().ResetSync();
                        startLeaveSyncChainOperations();
                    }
                }
            }
        };
        String deviceNameToDisplay =
                deviceName + " " + getResources().getString(R.string.brave_sync_this_device_text);
        AlertDialog alertDialog =
                alert.setTitle(getResources().getString(R.string.brave_sync_remove_device_text))
                        .setMessage(
                                getString(R.string.brave_sync_delete_device, deviceNameToDisplay))
                        .setPositiveButton(R.string.ok, onClickListener)
                        .setNegativeButton(R.string.cancel, onClickListener)
                        .create();
        alertDialog.getDelegate().setHandleNativeActionModesEnabled(false);
        alertDialog.show();
    }

    private boolean mLeaveSyncChainInProgress = false;

    private void startLeaveSyncChainOperations() {
        mLeaveSyncChainInProgress = true;
        InvalidateCodephrase();

        mBraveSyncTextDevicesTitle.setText(
                getResources().getString(R.string.brave_sync_leaving_sync_chain_title));

        mShowCategoriesButton.setVisibility(View.GONE);
        mAddDeviceButton.setVisibility(View.GONE);
        mRemoveDeviceButton.setVisibility(View.GONE);

        ProfileSyncService.get().addSyncStateChangedListener(this);

        PostTask.postDelayedTask(
                UiThreadTaskTraits.USER_VISIBLE, () -> leaveSyncChainComplete(), 5 * 1000);

        ViewGroup devicesUiGroup = (ViewGroup) getView().findViewById(R.id.brave_sync_devices);
        if (devicesUiGroup != null) {
            devicesUiGroup.removeAllViews();
        }
    }

    private void leaveSyncChainComplete() {
        if (mLeaveSyncChainInProgress) {
            mLeaveSyncChainInProgress = false;

            ProfileSyncService.get().removeSyncStateChangedListener(this);

            mShowCategoriesButton.setVisibility(View.VISIBLE);
            mAddDeviceButton.setVisibility(View.VISIBLE);
            mRemoveDeviceButton.setVisibility(View.VISIBLE);

            setAppropriateView();
        }
    }

    @Override
    public void syncStateChanged() {
        if (ProfileSyncService.get().isFirstSetupComplete() == false && mLeaveSyncChainInProgress) {
            leaveSyncChainComplete();
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
      setQRCodeText();
      getActivity().setTitle(R.string.brave_sync_scan_chain_code);
      if (null != mScrollViewSyncChainCode) {
          adjustWidth(mScrollViewSyncChainCode, false);
          mScrollViewSyncChainCode.setVisibility(View.VISIBLE);
      }
      if (null != mCameraSourcePreview) {
          int rc = ActivityCompat.checkSelfPermission(getActivity().getApplicationContext(), Manifest.permission.CAMERA);
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
      adjustImageButtons(getActivity().getApplicationContext().getResources().getConfiguration().orientation);
  }

  private String mCodephrase;
  public String GetCodephrase() {
      if (mCodephrase == null || mCodephrase.isEmpty()) {
          if (getBraveSyncWorker() != null) {
              mCodephrase = getBraveSyncWorker().GetCodephrase();
          }
      }
      return mCodephrase;
  }

  public void InvalidateCodephrase() {
      mCodephrase = null;
  }

  private void setAddMobileDeviceLayout() {
      getActivity().setTitle(R.string.brave_sync_btn_mobile);
      if (null != mBraveSyncTextViewAddMobileDevice) {
          setSyncText(getResources().getString(R.string.brave_sync_scan_sync_code),
                  getResources().getString(R.string.brave_sync_add_mobile_device_text_part_1)
                          + "\n\n"
                          + getResources().getString(
                                  R.string.brave_sync_add_mobile_device_text_part_2)
                          + "\n",
                  mBraveSyncTextViewAddMobileDevice);
      }
      if (null != mBraveSyncWarningTextViewAddMobileDevice) {
        String braveSyncCodeWarning = getResources().getString(R.string.brave_sync_code_warning);
        SpannableString braveSyncCodeWarningSpanned = new SpannableString(braveSyncCodeWarning);

        ForegroundColorSpan foregroundSpan = new ForegroundColorSpan(getResources().getColor(R.color.default_red));
        braveSyncCodeWarningSpanned.setSpan(foregroundSpan, 0, braveSyncCodeWarningSpanned.length()-1, 0);
        mBraveSyncWarningTextViewAddMobileDevice.setText(braveSyncCodeWarningSpanned);
      }
      if (null != mScrollViewSyncInitial) {
          mScrollViewSyncInitial.setVisibility(View.GONE);
      }
      if (null != mScrollViewEnterCodeWords) {
          mScrollViewEnterCodeWords.setVisibility(View.GONE);
      }
      if (null != mScrollViewAddMobileDevice) {
          adjustWidth(mScrollViewAddMobileDevice, false);
          mScrollViewAddMobileDevice.setVisibility(View.VISIBLE);
      }
      if (null != mScrollViewAddLaptop) {
          mScrollViewAddLaptop.setVisibility(View.GONE);
      }
      if (null != mScrollViewSyncStartChain) {
          mScrollViewSyncStartChain.setVisibility(View.GONE);
      }
      getActivity().runOnUiThread(new Runnable() {
          @Override
          public void run() {
              if (getBraveSyncWorker() != null) {
                  String seedHex =
                          getBraveSyncWorker().GetSeedHexFromWords(GetCodephrase());
                  if (null == seedHex || seedHex.isEmpty()) {
                      // Give up, seed must be valid
                      Log.e(TAG, "setAddMobileDeviceLayout seedHex is empty");
                      assert false;
                  } else {
                      fillQrCode(seedHex);
                  }
              }
          }
      });
  }

  private void fillQrCode(String qrDataFinal) {
      if (!isBarCodeValid(qrDataFinal)) {
          Log.e(TAG, "fillQrCode - invalid QR code");
          // Normally must not reach here ever, because the code is validated right after scan
          assert false;
          showEndDialog(getResources().getString(R.string.sync_device_failure));
          return;
      }

      new Thread(new Runnable() {
          @Override
          public void run() {
              // Generate QR code
              BitMatrix result;
              try {
                  result = new MultiFormatWriter().encode(
                          qrDataFinal, BarcodeFormat.QR_CODE, WIDTH, WIDTH, null);
              } catch (WriterException e) {
                  Log.e(TAG, "QR code unsupported format: " + e);
                  return;
              }
              int w = result.getWidth();
              int h = result.getHeight();
              int[] pixels = new int[w * h];
              for (int y = 0; y < h; y++) {
                  int offset = y * w;
                  for (int x = 0; x < w; x++) {
                      pixels[offset + x] = result.get(x, y) ? BLACK : WHITE;
                  }
              }
              Bitmap bitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
              bitmap.setPixels(pixels, 0, WIDTH, 0, 0, w, h);
              getActivity().runOnUiThread(new Runnable() {
                  @Override
                  public void run() {
                      mQRCodeImage.setImageBitmap(bitmap);
                      mQRCodeImage.invalidate();
                  }
              });
          }
      }).start();
  }

  private void setAddLaptopLayout() {
      getActivity().setTitle(R.string.brave_sync_btn_laptop);
      if (null != mBraveSyncTextViewAddLaptop) {
          setSyncText(getResources().getString(R.string.brave_sync_add_laptop_text_title),
                        getResources().getString(R.string.brave_sync_add_laptop_text_part_1) + "\n\n" +
                        getResources().getString(R.string.brave_sync_add_laptop_text_part_2_new) + "\n", mBraveSyncTextViewAddLaptop);
      }

      if (null != mBraveSyncWarningTextViewAddLaptop) {
        String braveSyncCodeWarning = getResources().getString(R.string.brave_sync_code_warning);
        SpannableString braveSyncCodeWarningSpanned = new SpannableString(braveSyncCodeWarning);

        ForegroundColorSpan foregroundSpan = new ForegroundColorSpan(getResources().getColor(R.color.default_red));
        braveSyncCodeWarningSpanned.setSpan(foregroundSpan, 0, braveSyncCodeWarningSpanned.length()-1, 0);
        mBraveSyncWarningTextViewAddLaptop.setText(braveSyncCodeWarningSpanned);
      }

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
          adjustWidth(mScrollViewAddLaptop, false);
          mScrollViewAddLaptop.setVisibility(View.VISIBLE);
      }
      if (null != mScrollViewSyncStartChain) {
          mScrollViewSyncStartChain.setVisibility(View.GONE);
      }
      getActivity().runOnUiThread(new Runnable() {
          @Override
          public void run() {
              String codePhrase = GetCodephrase();
              assert codePhrase != null && !codePhrase.isEmpty();
              mBraveSyncAddDeviceCodeWords.setText(codePhrase);
          }
      });
  }

  private void setSyncDoneLayout() {
      if (getBraveSyncWorker() == null) {
          assert false;
          return;
      }

      boolean firstSetupComplete = getBraveSyncWorker().IsFirstSetupComplete();
      getBraveSyncWorker().SaveCodephrase(GetCodephrase());
      getBraveSyncWorker().FinalizeSyncSetup();

      if (!deviceInfoObserverSet) {
          BraveSyncDevices.get().addDeviceInfoChangedListener(this);
          deviceInfoObserverSet = true;
      }

      getActivity().getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);
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
          adjustWidth(mScrollViewSyncDone, false);
          mScrollViewSyncDone.setVisibility(View.VISIBLE);
      }
      // With sync v2 we may leave the sync chain even if we don't see other
      // devices in chain, mRemoveDeviceButton is always visible
      BraveActivity mainActivity = BraveActivity.getBraveActivity();
      if (null != mainActivity) {
          mBraveSyncTextDevicesTitle.setText(getResources().getString(R.string.brave_sync_loading_devices_title));
      }

      onDevicesAvailable();
  }

  private void adjustWidth(View view, boolean special) {
      if (DeviceFormFactor.isTablet()) {
          DisplayMetrics metrics = new DisplayMetrics();
          getActivity().getWindowManager().getDefaultDisplay().getMetrics(metrics);
          LayoutParams params = view.getLayoutParams();
          if (special) {
              params.width = metrics.widthPixels * 3 / 5;
              params.height = metrics.heightPixels * 3 / 5;
          } else {
              int width = (metrics.widthPixels > metrics.heightPixels) ? metrics.widthPixels : metrics.heightPixels;
              width = width / 2;
              params.width = width;
              params.height = LayoutParams.MATCH_PARENT;
          }
          view.setLayoutParams(params);
      }
  }

  private void adjustImageButtons(int orientation) {
      if ((null != mLayoutSyncStartChain) && (null != mScrollViewSyncStartChain) && (View.VISIBLE == mScrollViewSyncStartChain.getVisibility())) {
          adjustWidth(mScrollViewSyncStartChain, true);
          LayoutParams params = mLayoutMobile.getLayoutParams();
          if (orientation == Configuration.ORIENTATION_LANDSCAPE) {
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
