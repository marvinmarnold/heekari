/*******************************************************************************
 * Copyright (c) 2013 Nordic Semiconductor. All Rights Reserved.
 * 
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 * Licensees are granted free, non-transferable use of the information. NO WARRANTY of ANY KIND is provided. 
 * This heading must NOT be removed from the file.
 ******************************************************************************/
package no.nordicsemi.android.nrftoolbox.hts;

import java.text.DecimalFormat;
import java.util.UUID;

import no.nordicsemi.android.nrftoolbox.R;
import no.nordicsemi.android.nrftoolbox.profile.BleManager;
import no.nordicsemi.android.nrftoolbox.profile.BleProfileActivity;
import android.os.Bundle;
import android.view.Menu;
import android.widget.TextView;

/**
 * HTSActivity is the main Health Thermometer activity. It implements {@link HTSManagerCallbacks} to receive callbacks from {@link HTSManager} class. The activity supports portrait and landscape
 * orientations.
 */
public class HTSActivity extends BleProfileActivity implements HTSManagerCallbacks {
	@SuppressWarnings("unused")
	private final String TAG = "HTSActivity";

	private TextView mHTSValue;

	@Override
	protected void onCreateView(Bundle savedInstanceState) {
		setContentView(R.layout.activity_feature_hts);
		setGUI();
	}

	private void setGUI() {
		mHTSValue = (TextView) findViewById(R.id.text_hts_value);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.help, menu);
		return true;
	}

	@Override
	protected int getAboutTextId() {
		return R.string.hts_about_text;
	}

	@Override
	protected int getDefaultDeviceName() {
		return R.string.hts_default_name;
	}

	@Override
	protected UUID getFilterUUID() {
		return HTSManager.HT_SERVICE_UUID;
	}

	@Override
	protected BleManager<HTSManagerCallbacks> initializeManager() {
		HTSManager manager = HTSManager.getHTSManager();
		manager.setGattCallbacks(this);
		return manager;
	}

	@Override
	public void onServicesDiscovered(boolean optionalServicesFound) {
		// this may notify user or show some views
	}

	@Override
	public void onHTIndicationEnabled() {
		// nothing to do here
	}

	@Override
	public void onHTValueReceived(double value) {
		setHTSValueOnView(value);
	}

	private void setHTSValueOnView(final double value) {
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				DecimalFormat formatedTemp = new DecimalFormat("#0.00");
				mHTSValue.setText(formatedTemp.format(value));
			}
		});
	}

	@Override
	protected void setDefaultUI() {
		mHTSValue.setText(R.string.not_available_value);
	}
}
