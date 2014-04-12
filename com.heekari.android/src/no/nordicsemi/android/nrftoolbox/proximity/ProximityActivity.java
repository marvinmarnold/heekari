/*******************************************************************************
 * Copyright (c) 2013 Nordic Semiconductor. All Rights Reserved.
 * 
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 * Licensees are granted free, non-transferable use of the information. NO WARRANTY of ANY KIND is provided. 
 * This heading must NOT be removed from the file.
 ******************************************************************************/
package no.nordicsemi.android.nrftoolbox.proximity;

import java.util.UUID;

import no.nordicsemi.android.nrftoolbox.R;
import no.nordicsemi.android.nrftoolbox.profile.BleProfileService;
import no.nordicsemi.android.nrftoolbox.profile.BleProfileServiceReadyActivity;
import no.nordicsemi.android.nrftoolbox.utility.DebugLogger;
import android.app.FragmentManager;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;

public class ProximityActivity extends BleProfileServiceReadyActivity<ProximityService.ProximityBinder> {
	private static final String TAG = "ProximityActivity";

	private static final String IMMEDIATE_ALERT_STATUS = "immediate_alert_status";
	private boolean isImmediateAlertOn = false;

	private Button mFindMeButton;
	private ImageView mLockImage;

	@Override
	protected void onCreateView(Bundle savedInstanceState) {
		setContentView(R.layout.activity_feature_proximity);
		setGUI();
	}

	private void setGUI() {
		mFindMeButton = (Button) findViewById(R.id.action_findme);
		mLockImage = (ImageView) findViewById(R.id.imageLock);
	}

	@Override
	protected void onSaveInstanceState(Bundle outState) {
		super.onSaveInstanceState(outState);
		outState.putBoolean(IMMEDIATE_ALERT_STATUS, isImmediateAlertOn);
	}

	@Override
	protected void onRestoreInstanceState(Bundle savedInstanceState) {
		super.onRestoreInstanceState(savedInstanceState);

		isImmediateAlertOn = savedInstanceState.getBoolean(IMMEDIATE_ALERT_STATUS);
		if (isDeviceConnected()) {
			showOpenLock();

			if (isImmediateAlertOn) {
				showSilentMeOnButton();
			}
		}
	}

	@Override
	protected void onServiceBinded(final ProximityService.ProximityBinder binder) {
		// you may get the binder instance here 
	}

	@Override
	protected void onServiceUnbinded() {
		// you may release the binder instance here
	}

	@Override
	protected Class<? extends BleProfileService> getServiceClass() {
		return ProximityService.class;
	}

	@Override
	protected int getAboutTextId() {
		return R.string.proximity_about_text;
	}

	@Override
	protected int getDefaultDeviceName() {
		return R.string.proximity_default_name;
	}

	@Override
	protected UUID getFilterUUID() {
		return ProximityManager.LINKLOSS_SERVICE_UUID;
	}

	/**
	 * Callback of FindMe button on ProximityActivity
	 */
	public void onFindMeClicked(final View view) {
		if (isBLEEnabled()) {
			if (!isDeviceConnected()) {
				// do nothing
			} else if (!isImmediateAlertOn) {
				showSilentMeOnButton();
				((ProximityService.ProximityBinder) getService()).startImmediateAlert();
				isImmediateAlertOn = true;
			} else {
				showFindMeOnButton();
				((ProximityService.ProximityBinder) getService()).stopImmediateAlert();
				isImmediateAlertOn = false;
			}
		} else {
			showBLEDialog();
		}
	}

	@Override
	protected void setDefaultUI() {
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				mFindMeButton.setText(R.string.proximity_action_findme);
				mLockImage.setImageResource(R.drawable.proximity_lock_closed);
			}
		});
	}

	@Override
	public void onServicesDiscovered(boolean optionalServicesFound) {
		// this may notify user or update views
	}

	@Override
	public void onDeviceConnected() {
		super.onDeviceConnected();
		showOpenLock();
	}

	@Override
	public void onDeviceDisconnected() {
		super.onDeviceDisconnected();
		showClosedLock();
	}

	@Override
	public void onBondingRequired() {
		showClosedLock();
	}

	@Override
	public void onBonded() {
		showOpenLock();
	}

	@Override
	public void onLinklossOccur() {
		super.onLinklossOccur();
		showClosedLock();
		resetForLinkloss();

		DebugLogger.w(TAG, "Linkloss occur");

		String deviceName = getDeviceName();
		if (deviceName == null) {
			deviceName = getString(R.string.proximity_default_name);
		}

		showLinklossDialog(deviceName);
	}

	private void resetForLinkloss() {
		isImmediateAlertOn = false;
		setDefaultUI();
	}

	private void showFindMeOnButton() {
		mFindMeButton.setText(R.string.proximity_action_findme);
	}

	private void showSilentMeOnButton() {
		mFindMeButton.setText(R.string.proximity_action_silentme);
	}

	private void showOpenLock() {
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				mFindMeButton.setEnabled(true);
				mLockImage.setImageResource(R.drawable.proximity_lock_open);
			}
		});
	}

	private void showClosedLock() {
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				mFindMeButton.setEnabled(false);
				mLockImage.setImageResource(R.drawable.proximity_lock_closed);
			}
		});
	}

	private void showLinklossDialog(final String name) {
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				FragmentManager fm = getFragmentManager();
				LinklossFragment dialog = LinklossFragment.getInstance(name);
				dialog.show(fm, "scan_fragment");
			}
		});
	}
}
