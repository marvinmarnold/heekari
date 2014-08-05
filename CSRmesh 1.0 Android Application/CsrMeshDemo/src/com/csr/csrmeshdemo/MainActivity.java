/******************************************************************************
 *  Copyright (C) Cambridge Silicon Radio Limited 2014
 *
 *  This software is provided to the customer for evaluation
 *  purposes only and, as such early feedback on performance and operation
 *  is anticipated. The software source code is subject to change and
 *  not intended for production. Use of developmental release software is
 *  at the user's own risk. This software is provided "as is," and CSR
 *  cautions users to determine for themselves the suitability of using the
 *  beta release version of this software. CSR makes no warranty or
 *  representation whatsoever of merchantability or fitness of the product
 *  for any particular purpose or use. In no event shall CSR be liable for
 *  any consequential, incidental or special damages whatsoever arising out
 *  of the use of or inability to use this software, even if the user has
 *  advised CSR of the possibility of such damages.
 *
 ******************************************************************************/

package com.csr.csrmeshdemo;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;
import java.util.UUID;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.app.ActionBar;
import android.app.Activity;
import android.app.ProgressDialog;
import android.bluetooth.BluetoothDevice;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.ParcelUuid;
import android.os.SystemClock;
import android.util.Log;
import android.util.SparseIntArray;
import android.view.Window;
import android.widget.ArrayAdapter;
import android.widget.Toast;

import com.csr.csrmeshdemo.Device.DeviceType;
import com.csr.mesh.AttentionModel;
import com.csr.mesh.ConfigModel;
import com.csr.mesh.ConfigModel.DeviceInfo;
import com.csr.mesh.LightModel;
import com.csr.mesh.MeshService;
import com.csr.mesh.PingModel;
import com.csr.mesh.PowerModel;
import com.csr.mesh.PowerModel.PowerState;
import com.csr.mesh.SwitchModel;

public class MainActivity extends Activity implements DeviceController {
    private static final String TAG = "MainActivity";

    // How often to send a colour - i.e. how often the periodic timer fires.
    private static final int TRANSMIT_PERIOD_MS = 240;
    
    // How often to send RSSI value
    private static final int RSSI_PERIOD_MS = 5000;

    // Time to wait for group acks.
    private static final int GROUP_ACK_WAIT_TIME_MS = (30 * 1000);

    // Time to wait for device UUID after removing a device.
    private static final int REMOVE_ACK_WAIT_TIME_MS = (10 * 1000);

    // Time to wait until giving up on connection.
    private static final int CONNECT_WAIT_TIME_MS = (10 * 1000);
    
    private static final int CONNECT_RETRIES = 3;
    
    // Watch for disconnect for this time period and reconnect.
    private static final long DISCONNECT_WATCH_TIME_MS = 500;
    
    private int mNumConnectAttempts = 0;
    
    private long mConnectTime = 0;
    
    private boolean mConnected = false;
    
    private DeviceStore mDeviceStore;

    // The address to send packets to.
    private int mSendDeviceId = Device.DEVICE_ID_UNKNOWN;

    // The colour sent every time the periodic timer fires (if mNewColor is true).
    // This will be updated by calls to setLightColor.
    private int mColorToSend = Color.rgb(0, 0, 0);

    // A new colour is only sent every TRANSMIT_PERIOD_MS if this is true. Set to true by setLightColour.
    private boolean mNewColor = false;

    private int mGroupAcksWaiting = 0;
        
    private ArrayList<Integer> mNewGroups = new ArrayList<Integer>();
    private List <Integer> mGroupsToSend;    
    
    private int mNextGroupId;    

    private SparseIntArray mDeviceIdtoUuidHash = new SparseIntArray();
    
    private MeshService mService = null;

    private boolean mDevicesLoaded;
    private boolean mFirstRun;

    // True if authorization should be used during key distribution.
    private boolean mAuthRequired;

    private String mNetworkKeyPhrase;

    private int mRemovedUuidHash;
    private int mRemovedDeviceId;

    private ProgressDialog mProgress;

    private SimpleNavigationListener mNavListener;

    private String mConnectedAddress;
    private ArrayList<BluetoothDevice> mDevices = new ArrayList<BluetoothDevice>();
    int mDeviceIndex = 0;
    
    // Keys used to save settings
    private static final String SETTING_FIRST_RUN = "first_run";
    private static final String SETTING_NETWORK_PHRASE = "network_phrase";
    private static final String SETTING_AUTHORISE = "authorise";

    // Listeners
    private GroupListener mGroupAckListener;
    private InfoListener mInfoListener;
    private AssociationListener mAssListener;
    private AssociationStartedListener mAssStartedListener;
    private RemovedListener mRemovedListener;
    

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);

        mProgress = new ProgressDialog(this);

        mDeviceStore = new DeviceStore();
        
        showProgress(getString(R.string.connecting));        
        
        // Get the device to connect to that was passed to us by the scan results Activity.
        Intent intent = getIntent();        
        mDevices = intent.getExtras().getParcelableArrayList(BluetoothDevice.EXTRA_DEVICE);        
        if (mDevices.get(mDeviceIndex) != null) {
            // Make a connection to MeshService to enable us to use its services.
            Intent bindIntent = new Intent(this, MeshService.class);
            bindService(bindIntent, mServiceConnection, Context.BIND_AUTO_CREATE);
        }
        
    }

    @Override
    public void onStop() {
        super.onStop();
        saveSettings();
    }

    @Override
    public void onBackPressed()
    {
        mService.disconnectLe();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mMeshHandler.removeCallbacksAndMessages(null);        
        unbindService(mServiceConnection);
    }

    /**
     * Callbacks for changes to the state of the connection.
     */
    private ServiceConnection mServiceConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className, IBinder rawBinder) {
            mService = ((MeshService.LocalBinder) rawBinder).getService();
            if (mService != null) {
                restoreSettings();

                if (mFirstRun) {
                    mDeviceStore.addLightGroup(new LightDevice(Device.GROUP_ADDR_BASE, 0, getString(R.string.all_lights), true));
                    mDeviceStore.addLightGroup(new LightDevice(Device.GROUP_ADDR_BASE + 1, 0, getString(R.string.group) + " 1", true));
                    mDeviceStore.addLightGroup(new LightDevice(Device.GROUP_ADDR_BASE + 2, 0, getString(R.string.group) + " 2", true));
                    mDeviceStore.addLightGroup(new LightDevice(Device.GROUP_ADDR_BASE + 3, 0, getString(R.string.group) + " 3", true));
                    mDeviceStore.addLightGroup(new LightDevice(Device.GROUP_ADDR_BASE + 4, 0, getString(R.string.group) + " 4", true));
                    mNextGroupId = Device.GROUP_ADDR_BASE + 5;
                    mDevicesLoaded = true;
                }

                // Start the required transport.                
                mMeshHandler.postDelayed(connectTimeout, CONNECT_WAIT_TIME_MS);
                connect();
            }
        }

        public void onServiceDisconnected(ComponentName classname) {
            mService = null;
        }
    };

    private void connect() {
    	BluetoothDevice bridgeDevice = mDevices.get(mDeviceIndex);
    	mConnectedAddress = bridgeDevice.getAddress();
    	if (mDeviceIndex < mDevices.size() - 1) {
    		mDeviceIndex++;
    	}
    	else {
    		mDeviceIndex = 0;
    	}
    	mNumConnectAttempts++;
    	Log.d(TAG, "Connect attempt " + mNumConnectAttempts + ", address: " + bridgeDevice.getAddress());
    	mService.initLe(mMeshHandler, bridgeDevice);
    }
    
    /**
     * Executed when LE link to bridge is connected.
     */
    private void onConnected() {    	
    	mMeshHandler.removeCallbacks(connectTimeout);
        hideProgress();

        mConnectTime = SystemClock.elapsedRealtime();
        mConnected = true;
        
        ActionBar actionBar = getActionBar();
        actionBar.setNavigationMode(ActionBar.NAVIGATION_MODE_LIST);

        // Set up drop down list in action bar that selects fragments.
        String[] screenNames = getResources().getStringArray(R.array.screen_names);
        ArrayAdapter<String> screenNamesAdapter =
                new ArrayAdapter<String>(getActionBar().getThemedContext(), android.R.layout.simple_list_item_1,
                        screenNames);
        mNavListener = new SimpleNavigationListener(getFragmentManager(), this);
        actionBar.setListNavigationCallbacks(screenNamesAdapter, mNavListener);

        // Don't show app title as navigation list is being used.
        actionBar.setDisplayShowTitleEnabled(false);

        // Set active fragment based on settings.
        if (mNetworkKeyPhrase == null) {
            getActionBar().setSelectedNavigationItem(SimpleNavigationListener.POSITION_NETWORK_SETTINGS);
            mNavListener.setNavigationEnabled(false);
        }
        startPeriodicColorTransmit();
        mMeshHandler.postDelayed(rssiCallback, RSSI_PERIOD_MS);
    }

    /**
     * Handle messages from mesh service.
     */
    private final Handler mMeshHandler = new MeshHandler(this);

    private static class MeshHandler extends Handler {
        private final WeakReference<MainActivity> mActivity;

        public MeshHandler(MainActivity activity) {
            mActivity = new WeakReference<MainActivity>(activity);
        }

        public void handleMessage(Message msg) {
            MainActivity parentActivity = mActivity.get();
            switch (msg.what) {
            case MeshService.MESSAGE_LE_CONNECTED: {            	
                parentActivity.onConnected();
                break;
            }
            case MeshService.MESSAGE_PING_RESPONSE: {
            	Log.d(TAG, "RRRRRRRReceived PING");
//              int rssi = msg.getData().getByte(MeshService.EXTRA_PING_RSSI_AT_RX);                
                
                break;
            }
            case MeshService.MESSAGE_LE_DISCONNECTED: {
            	// If we haven't been connected at all yet then retry the connection.
            	long upTime = SystemClock.elapsedRealtime() - parentActivity.mConnectTime;
            	if (upTime < DISCONNECT_WATCH_TIME_MS) {
            		parentActivity.connect();
            	}
            	else {
					Toast.makeText(parentActivity.getApplicationContext(),
							parentActivity.getString(R.string.disconnected),
							Toast.LENGTH_SHORT).show();
	                parentActivity.finish();
            	}
                break;
            }
            case MeshService.MESSAGE_TIMEOUT:
                int expectedMsg = msg.getData().getInt(MeshService.EXTRA_EXPECTED_MESSAGE);
                parentActivity.onMessageTimeout(expectedMsg);
                break;
            case MeshService.MESSAGE_DEVICE_UUID: {
                ParcelUuid uuid = msg.getData().getParcelable(MeshService.EXTRA_UUID);                
                int uuidHash = msg.getData().getInt(MeshService.EXTRA_UUIDHASH_31);
                int rssi = msg.getData().getInt(MeshService.EXTRA_RSSI);                
                if (parentActivity.mRemovedListener != null && parentActivity.mRemovedUuidHash == uuidHash) {
                    // This was received after a device was removed, so let the removed listener know.
                    parentActivity.mDeviceStore.removeDevice(parentActivity.mRemovedDeviceId);
                    parentActivity.mRemovedListener.onDeviceRemoved(parentActivity.mRemovedDeviceId, true);
                    parentActivity.mRemovedListener = null;
                    parentActivity.mRemovedUuidHash = 0;
                    parentActivity.mRemovedDeviceId = 0;
                    parentActivity.mService.setDiscoveryEnabled(false);
                    removeCallbacks(parentActivity.removeDeviceTimeout);
                }
                else if (parentActivity.mAssListener != null) {
                    // This was received after discover was enabled so let the uuid listener know.                    
                    parentActivity.mAssListener.newUuid(uuid.getUuid(), uuidHash, rssi);
                }
                break;
            }
            case MeshService.MESSAGE_DEVICE_ASSOCIATED: {
                // New device has been associated and is telling us its device id.
                // Request supported models before adding to DeviceStore, and the UI.
                int deviceId = msg.getData().getInt(MeshService.EXTRA_DEVICE_ID);
                int uuidHash = msg.getData().getInt(MeshService.EXTRA_UUIDHASH_31);
                Log.d(TAG, "New device associated with id " + String.format("0x%x", deviceId));

                if (parentActivity.mDeviceStore.getDevice(deviceId) == null) {
                    // Save the device id with the uuid hash so that we can store the uuid hash in the device
                    // object when MESSAGE_CONFIG_MODELS is received.
                    parentActivity.mDeviceIdtoUuidHash.put(deviceId, uuidHash);

                    // If we don't already know about this device request its model support.
                    // We only need the lower 64-bits, so just request those.
                    parentActivity.mService.getConfigModel().requestInfo(DeviceInfo.MODEL_LOW, deviceId);                                       
                }
                break;
            }
            case MeshService.MESSAGE_CONFIG_DEVICE_INFO:
                int deviceId = msg.getData().getInt(MeshService.EXTRA_DEVICE_ID);
                int uuidHash = parentActivity.mDeviceIdtoUuidHash.get(deviceId);
                long bitmap = msg.getData().getLong(MeshService.EXTRA_DEVICE_INFORMATION);
                ConfigModel.DeviceInfo infoType =
                        ConfigModel.DeviceInfo.values()[msg.getData().getByte(MeshService.EXTRA_DEVICE_INFO_TYPE)];
                if (infoType == DeviceInfo.MODEL_LOW) {
                    // If the uuidHash was saved for this device id then this is an expected message, so process it.
                    if (uuidHash != 0) {
                        // Remove the uuidhash from the array as we have received its model support now.
                        parentActivity.mDeviceIdtoUuidHash
                                .removeAt(parentActivity.mDeviceIdtoUuidHash.indexOfKey(deviceId));                    
                        if ((bitmap & MeshService.getMcpModelMask(LightModel.MODEL_NUMBER)) == 
                                MeshService.getMcpModelMask(LightModel.MODEL_NUMBER)) {
                            if (parentActivity.mAssListener != null) {
                                parentActivity.mAssListener.deviceAssociated(true);
                                parentActivity.addLight(new LightDevice(deviceId, uuidHash, 
                                		String.format(parentActivity.getString(R.string.light) + " 0x%x",
                                        deviceId), false));                            
                            }
                        }
                        else if ((bitmap & MeshService.getMcpModelMask(SwitchModel.MODEL_NUMBER)) == 
                                MeshService.getMcpModelMask(SwitchModel.MODEL_NUMBER)) {
                            parentActivity.mAssListener.deviceAssociated(true);
                            parentActivity.addSwitch(new SwitchDevice(deviceId, uuidHash, 
                            		String.format(parentActivity.getString(R.string.switch_) + " 0x%x",
                                    deviceId), false));
                        }
                        else if (parentActivity.mAssListener != null) {
                            Log.e(TAG, String.format("Model support not recognised: 0x%x", bitmap));
                            parentActivity.mAssListener.deviceAssociated(false);
                        }
                    }
                }
                break;            
            case MeshService.MESSAGE_GROUP_NUM_GROUPIDS: {
                if (parentActivity.mGroupAckListener != null) {
                    int numIds = msg.getData().getByte(MeshService.EXTRA_NUM_GROUP_IDS);
                    Device currentDev = parentActivity.mDeviceStore.getDevice(parentActivity.mSendDeviceId);
                    currentDev.setNumGroupIndices(numIds);
                    parentActivity.mDeviceStore.updateDevice(currentDev);
                    parentActivity.assignGroups(numIds);
                }
                break;
            }
            case MeshService.MESSAGE_GROUP_MODEL_GROUPID: {            	
                // This is the ACK returned after calling setModelGroupId.
                if (parentActivity.mGroupAckListener != null && parentActivity.mGroupAcksWaiting > 0) {                	
                    parentActivity.mGroupAcksWaiting--;
                    int index = msg.getData().getByte(MeshService.EXTRA_GROUP_INDEX);
                    int groupId = msg.getData().getInt(MeshService.EXTRA_GROUP_ID);                    
                    // Update the group membership of this device in the device store.
                    Device updatedDev = parentActivity.mDeviceStore.getDevice(parentActivity.mSendDeviceId);
                    updatedDev.setGroupId(index, groupId);
                    parentActivity.mDeviceStore.updateDevice(updatedDev);

                    if (parentActivity.mGroupAcksWaiting == 0) {
                        // Tell the listener that the update was OK.
						parentActivity.mGroupAckListener.groupsUpdated(
								parentActivity.mSendDeviceId, true,
								parentActivity.getString(R.string.group_update_ok));
						removeCallbacks(parentActivity.groupAckTimeout);
                    }
                }
                break;
            }
            case MeshService.MESSAGE_FIRMWARE_VERSION:
                parentActivity.mInfoListener.onFirmwareVersion(msg.getData().getInt(MeshService.EXTRA_DEVICE_ID), msg
                        .getData().getInt(MeshService.EXTRA_VERSION_MAJOR),
                        msg.getData().getInt(MeshService.EXTRA_VERSION_MINOR), true);
                parentActivity.mInfoListener = null;
                break;

            }                            
        }
    }

    /**
     * Called when a response is not seen to a sent command.
     * 
     * @param expectedMessage
     *            The message that would have been received in the Handler if there hadn't been a timeout.
     */
    private void onMessageTimeout(int expectedMessage) {
        switch (expectedMessage) {
        case MeshService.MESSAGE_DEVICE_ASSOCIATED:
            // Fall through.
        case MeshService.MESSAGE_CONFIG_MODELS:
            // If we couldn't find out the model support for the device then we have to report association failed.
            if (mAssListener != null) {
                mAssListener.deviceAssociated(false);
            }
            break;
        case MeshService.MESSAGE_FIRMWARE_VERSION:
            if (mInfoListener != null) {
                mInfoListener.onFirmwareVersion(0, 0, 0, false);
            }
            break;
        case MeshService.MESSAGE_GROUP_NUM_GROUPIDS:
            if (mGroupAckListener != null) {
                mGroupAckListener.groupsUpdated(mSendDeviceId, false, getString(R.string.group_query_fail));
            }
            break;
        }
    }

    /**
     * Send group assign messages to the currently selected device using the groups contained in mNewGroups.
     */
    private void assignGroups(int numSupportedGroups) {
        if (mSendDeviceId == Device.DEVICE_ID_UNKNOWN)
            return;
        // Check the number of supported groups matches the number requested to be set.
        if (numSupportedGroups >= mNewGroups.size()) {

            mGroupAcksWaiting = 0;            
            
            // Make a copy of existing groups for this device.
            mGroupsToSend = mDeviceStore.getDevice(mSendDeviceId).getGroupMembershipValues();
            // Loop through existing groups.
            for (int i = 0; i < mGroupsToSend.size(); i++) {
                int groupId = mGroupsToSend.get(i);
                if (groupId != 0) {
	                int foundIndex = mNewGroups.indexOf(groupId);                
	                if (foundIndex > -1) {
	                    // The device is already a member of this group so remove it from the list of groups to add.
	                    mNewGroups.remove(foundIndex);
	                }
	                else {
	                    // The device should no longer be a member of this group, so set that index to -1 to flag
	                    // that a message must be sent to update this index.
	                    mGroupsToSend.set(i, -1);
	                }
                }
            }
            // Now loop through currentGroups, and for every index set to -1 or zero send a group update command for 
            // that index with one of our new groups if one is available. If there are no new groups to set, then just
            // send a message for all indices set to -1, to set them to zero.
            boolean commandSent = false;            
            for (int i = 0; i < mGroupsToSend.size(); i++) {
                int groupId = mGroupsToSend.get(i);
                if (groupId == -1 || groupId == 0) {
                    if (mNewGroups.size() > 0) {
                        int newGroup = mNewGroups.get(0);
                        mNewGroups.remove(0);
                        commandSent = true;
                        sendGroupCommands(mSendDeviceId, i, newGroup);
                    }
                    else if (groupId == -1) {
                    	commandSent = true;
                        sendGroupCommands(mSendDeviceId, i, 0);
                    }
                }
            }
            if (!commandSent) {
                // There were no changes to the groups so no updates were sent. Just tell the listener
            	// that the operation is complete.
            	if (mGroupAckListener != null) {
            		mGroupAckListener.groupsUpdated(mSendDeviceId, true, getString(R.string.group_no_changes));
            	}
            }
            else {
	            // Start a timer so that we don't wait for the acks forever.
	            mMeshHandler.postDelayed(groupAckTimeout, GROUP_ACK_WAIT_TIME_MS);
            }
        }
        else {
            // Not enough groups supported on device.
            if (mGroupAckListener != null) {
                mGroupAckListener.groupsUpdated(mSendDeviceId, false,
                getString(R.string.group_max_fail) + " " + numSupportedGroups + " " + getString(R.string.groups));
            }
        }
    }

    private void sendGroupCommands(int deviceId, int index, int group) {    	
        if (mDeviceStore.getDevice(deviceId).getDeviceType() == DeviceType.LIGHT) { 
            mGroupAcksWaiting++;
            mService.getGroupModel().setModelGroupId(PowerModel.MODEL_NUMBER, index, 0, group, deviceId);
            mGroupAcksWaiting++;
            mService.getGroupModel().setModelGroupId(LightModel.MODEL_NUMBER, index, 0, group, deviceId);
        }
        else if (mDeviceStore.getDevice(deviceId).getDeviceType() == DeviceType.SWITCH) {
            mGroupAcksWaiting++;
            mService.getGroupModel().setModelGroupId(SwitchModel.MODEL_NUMBER, index, 0, group, deviceId);
        }
    }
    
   
    
    /**
     * This is the implementation of the periodic timer that will call sendLightRgb() every TRANSMIT_PERIOD_MS if
     * mNewColor is set to TRUE.
     */    
    private Runnable transmitCallback = new Runnable() {
        @Override
        public void run() {
            if (mNewColor) {
                if (mSendDeviceId != Device.DEVICE_ID_UNKNOWN) {                    
                    byte red = (byte) (Color.red(mColorToSend) & 0xFF);
                    byte green = (byte) (Color.green(mColorToSend) & 0xFF);
                    byte blue = (byte) (Color.blue(mColorToSend) & 0xFF);

                    mService.getLightModel().setRgb(red, green, blue, mSendDeviceId);

                    LightDevice light = (LightDevice) mDeviceStore.getDevice(mSendDeviceId);
                    if (light != null) {
                        light.setRed(red);
                        light.setGreen(green);
                        light.setBlue(blue);
                        light.setStateKnown(true);
                        mDeviceStore.updateLight(light);
                    }
                }
                // Colour sent so clear the flag.
                mNewColor = false;
            }                        
            mMeshHandler.postDelayed(this, TRANSMIT_PERIOD_MS);
        }
    };

    /**
     * Sends RSSI every period
     */    
    private Runnable rssiCallback = new Runnable() {
        @Override
        public void run() {
        	Log.d(TAG, "!!!!!!****" + "SENDING PINGs");
        	for( Device d : mDeviceStore.getAllLights()){
        		int dId = d.getDeviceId();
        		Log.d(TAG, "@@@@@!!!!!!****" + Integer.toString(dId));
        		PingModel mPingModel = mService.getPingModel();
                mPingModel.ping(dId, (byte) 0xffffff, dId);
        	}
            mMeshHandler.postDelayed(this, RSSI_PERIOD_MS);
        }
    };
    
    // Runnables that execute after a timeout /////
    
    private Runnable groupAckTimeout = new Runnable() {
        @Override
        public void run() {
            // Handle group assignment ACK timeouts.
            if (mGroupAcksWaiting > 0) {
                if (mGroupAckListener != null) {
                    // Timed out waiting for group update ACK.
                    mGroupAckListener.groupsUpdated(mSendDeviceId, false,
                            getString(R.string.group_timeout));
                }
                mGroupAcksWaiting = 0;
            }            
        }
    };
    
    private Runnable removeDeviceTimeout = new Runnable() {
		@Override
		public void run() {
			// Handle timeouts on removing devices.
            if (mRemovedListener != null) {
                // Timed out waiting for device UUID that indicates device removal happened.
                mRemovedListener.onDeviceRemoved(mRemovedDeviceId, false);
                mRemovedListener = null;
                mRemovedUuidHash = 0;
                mRemovedDeviceId = 0;
                mService.setDiscoveryEnabled(false);                
            }
		}
    };
    
    private Runnable connectTimeout = new Runnable() {
        @Override
        public void run() {
        	if (mNumConnectAttempts < CONNECT_RETRIES) {
        		mMeshHandler.postDelayed(connectTimeout, CONNECT_WAIT_TIME_MS);
        		connect();
        	}
        	else {
	        	hideProgress();
	        	Toast.makeText(getApplicationContext(), getString(R.string.connect_faiL), Toast.LENGTH_SHORT).show();
	        	finish();
        	}
        }        
    };        
        
    // End of timeout handlers /////
    
    /**
     * Start transmitting colours to the current send address. Colours are sent every TRANSMIT_PERIOD_MS ms.
     */
    private void startPeriodicColorTransmit() {
        mMeshHandler.postDelayed(transmitCallback, TRANSMIT_PERIOD_MS);
    }

    /**
     * Show a modal progress dialogue until hideProgress is called.
     * 
     * @param message
     *            The message to display in the dialogue.
     */
    private void showProgress(String message) {
        mProgress.setMessage(message);
        mProgress.setProgressStyle(ProgressDialog.STYLE_SPINNER);
        mProgress.setIndeterminate(true);
        mProgress.setCancelable(false);        
        mProgress.show();
    }

    /**
     * Hide the progress dialogue.
     */
    private void hideProgress() {
        mProgress.dismiss();
    }

    /**
     * Add a light device to the device store and display a message.
     * 
     * @param light
     *            LightDevice object to add.
     */
    private void addLight(final LightDevice light) {
        if (mDeviceStore.getLight(light.getDeviceId()) == null) {
            mDeviceStore.addLight(light);
            Toast.makeText(this, light.getName() + " " + getString(R.string.added), Toast.LENGTH_LONG).show();
        }
    }

    /**
     * Add a switch device to the device store and display a message.
     * 
     * @param switchDevice
     *            SwitchDevice object to add.
     */
    private void addSwitch(final SwitchDevice switchDevice) {
        if (mDeviceStore.getSwitch(switchDevice.getDeviceId()) == null) {
            mDeviceStore.addSwitch(switchDevice);
            Toast.makeText(this, switchDevice.getName() + " added", Toast.LENGTH_LONG).show();
        }
    }

    /**
     * Save app settings including devices and groups.
     */
    private void saveSettings() {
        SharedPreferences activityPrefs = getPreferences(Activity.MODE_PRIVATE);
        SharedPreferences.Editor editor = activityPrefs.edit();
        editor.clear();
        editor.commit();

        // Save devices including groups.
        if (mDevicesLoaded) {
            List<Device> lights = mDeviceStore.getAllLights();
            List<Device> lightGroups = mDeviceStore.getAllLightGroups();
            List<Device> switches = mDeviceStore.getAllSwitches();

            for (Device light : lights) {
                editor.putString(String.valueOf(light.getDeviceId()), light.toJson());
            }
            for (Device lightGroup : lightGroups) {
                editor.putString(String.valueOf(lightGroup.getDeviceId()), lightGroup.toJson());
            }
            for (Device sw : switches) {
                editor.putString(String.valueOf(sw.getDeviceId()), sw.toJson());
            }
            // Put a value indicating not first run.
            editor.putBoolean(SETTING_FIRST_RUN, false);
        }
        else {
            editor.putBoolean(SETTING_FIRST_RUN, true);
        }

        // Save the security settings.
        if (mNetworkKeyPhrase != null) {
            editor.putString(SETTING_NETWORK_PHRASE, mNetworkKeyPhrase);
        }
        editor.putBoolean(SETTING_AUTHORISE, mAuthRequired);
        editor.commit();
    }

    /**
     * Restore app settings including devices and groups.
     */
    private void restoreSettings() {
        SharedPreferences activityPrefs = getPreferences(Activity.MODE_PRIVATE);

        mFirstRun = activityPrefs.getBoolean(SETTING_FIRST_RUN, true);
        mNetworkKeyPhrase = activityPrefs.getString(SETTING_NETWORK_PHRASE, null);
        mAuthRequired = activityPrefs.getBoolean(SETTING_AUTHORISE, false);
        if (mNetworkKeyPhrase != null) {
            mService.setNetworkPassPhrase(mNetworkKeyPhrase);
        }
        if (!mFirstRun) {
            mFirstRun = false;
            Map<String, ?> savedDevices = activityPrefs.getAll();
            int maxId = Device.DEVICE_ADDR_BASE;
            for (Map.Entry<String, ?> entry : savedDevices.entrySet()) {
                // If key is an integer then this should be a Device object.
                if (entry.getKey().matches("[-+]?\\d+")) {
                    Device newDevice = Device.fromJson((String) entry.getValue());
                    if (newDevice != null) {

                        switch (newDevice.getDeviceType()) {
                        case LIGHT:
                            if (newDevice.getDeviceId() > maxId) {
                                maxId = newDevice.getDeviceId();
                            }
                            mDeviceStore.addLight((LightDevice) newDevice);
                            break;
                        case LIGHT_GROUP:
                            if (newDevice.getDeviceId() > mNextGroupId) {
                                mNextGroupId = newDevice.getDeviceId();
                            }
                            mDeviceStore.addLightGroup((LightDevice) newDevice);
                            break;
                        case SWITCH:
                            if (newDevice.getDeviceId() > maxId) {
                                maxId = newDevice.getDeviceId();
                            }
                            mDeviceStore.addSwitch((SwitchDevice) newDevice);
                            break;
                        default:
                            Log.e(TAG, "Attempt to load unsupported device type : " + newDevice.getDeviceType());
                            break;
                        }
                    }
                    else {
                        Log.e(TAG, "Failed to load device from JSON : " + (String) entry.getValue());
                    }
                }
            }
            if (maxId > 0) {
                mService.setNextDeviceId(maxId + 1);
            }
            mNextGroupId++;
            mDevicesLoaded = true;
        }
        else {
            mFirstRun = true;
        }
    }

    @Override
    public void discoverDevices(boolean enabled, AssociationListener listener) {
        if (enabled) {
            mAssListener = listener;
        }
        else {
            mAssListener = null;
        }
        mService.setDiscoveryEnabled(enabled);
    }

    @Override
    public void associateDevice(int uuidHash) {
        mService.associateDevice(uuidHash, 0);
    }

    @Override
    public boolean associateDevice(int uuidHash, String shortCode) {
        return mService.associateDevice(uuidHash, shortCode);
    }

    @Override
    public List<Device> getDevices(DeviceType type) {
        switch (type) {
        case LIGHT:
            return mDeviceStore.getAllLights();
        case SWITCH:
            return mDeviceStore.getAllSwitches();
        default:
            throw new IllegalArgumentException("Invalid device type");
        }
    }

    @Override
    public List<Device> getGroups(DeviceType type) {
        switch (type) {
        case LIGHT_GROUP:
            return mDeviceStore.getAllLightGroups();
        default:
            throw new IllegalArgumentException("Invalid device type");
        }
    }

    @Override
    public Device getDevice(int deviceId) {
        return mDeviceStore.getDevice(deviceId);
    }

    @Override
    public void setSelectedDeviceId(final int deviceId) {
        Log.d(TAG, String.format("Device id is now 0x%x", deviceId));
        mSendDeviceId = deviceId;
    }

    @Override
    public int getSelectedDeviceId() {
        return mSendDeviceId;
    }

    @Override
    public void setLightColor(final int color, final int brightness) {
        if (brightness < 0 || brightness > 99) {
            throw new NumberFormatException("Brightness value should be between 0 and 99");
        }

        // Convert currentColor to HSV space and make the brightness (value) calculation. Then convert back to RGB to
        // make the colour to send.
        // Don't modify currentColor with the brightness or else it will deviate from the HS colour selected on the
        // wheel due to accumulated errors in the calculation after several brightness changes.
        float[] hsv = new float[3];
        Color.colorToHSV(color, hsv);
        hsv[2] = ((float) brightness + 1) / 100.0f;
        mColorToSend = Color.HSVToColor(hsv);

        // Indicate that there is a new colour for next time the timer fires.
        mNewColor = true;
    }

    @Override
    public void setLightPower(PowerState state) {
        mService.getPowerModel().setPowerState(state, mSendDeviceId);
        setLocalLightPower(state);
    }
    
    
    @Override
    public void setLocalLightPower(PowerState state) {
        LightDevice light = (LightDevice)mDeviceStore.getDevice(mSendDeviceId);
        if (light != null) {
	        light.setPower(state);
	        mDeviceStore.updateDevice(light);
        }
    }
    
    @Override
    public void removeDevice(RemovedListener listener) {
        if (mSendDeviceId < Device.DEVICE_ADDR_BASE && mSendDeviceId >= Device.GROUP_ADDR_BASE) {
            mDeviceStore.removeLightGroup(mSendDeviceId);
            listener.onDeviceRemoved(mSendDeviceId, true);
            mSendDeviceId = Device.GROUP_ADDR_BASE;
        }
        else {
            mRemovedUuidHash = mDeviceStore.getDevice(mSendDeviceId).getUuidHash();
            mRemovedDeviceId = mSendDeviceId;
            mRemovedListener = listener;
            // Enable discovery so that the device uuid message is received when the device is unassociated.
            mService.setDiscoveryEnabled(true);            
            // Send CONFIG_RESET
            mService.getConfigModel().resetDevice(mSendDeviceId);
            mSendDeviceId = Device.GROUP_ADDR_BASE;
            // Start a timer so that we don't wait for the ack forever.
            mMeshHandler.postDelayed(removeDeviceTimeout, REMOVE_ACK_WAIT_TIME_MS);
        }
    }

    @Override
    public void getFwVersion(InfoListener listener) {
        mInfoListener = listener;
        mService.getFirmwareModel().requestVersionInfo(mSendDeviceId);
    }

    @Override
    public void setDeviceGroups(List<Integer> groups, GroupListener listener) {
        if (mSendDeviceId == Device.DEVICE_ID_UNKNOWN)
            return;
        mNewGroups.clear();
        mGroupAckListener = listener;
        for (int group : groups) {
            mNewGroups.add(group);
        }
        Device selectedDev = mDeviceStore.getDevice(mSendDeviceId);
        int numIds = selectedDev.getNumGroupIndices();
        if (numIds == 0) {
            // Send message to find out how many group ids the device supports.
            // Once a response is received to this command sendGroupAssign will be called to assign the groups.
            if (selectedDev.getDeviceType() == DeviceType.LIGHT) {
                // Only query light model and assume power model supports the same number.
                mService.getGroupModel().getNumModelGroupIds(LightModel.MODEL_NUMBER, mSendDeviceId);
            }
            else if (selectedDev.getDeviceType() == DeviceType.SWITCH) {
                mService.getGroupModel().getNumModelGroupIds(SwitchModel.MODEL_NUMBER, mSendDeviceId);
            }
        }
        else {
            // We already know the number of supported groups from a previous query, so go straight to assigning.
            assignGroups(numIds);
        }
    }

    @Override
    public void setDeviceName(int deviceId, String name) {
        Device dev = mDeviceStore.getDevice(deviceId);
        if (dev == null) {
            throw new IndexOutOfBoundsException("A device with that id does not exist");
        }
        else {
            dev.setName(name);
            mDeviceStore.updateDevice(dev);
        }
    }

    @Override
    public void setSecurity(String networkKeyPhrase, boolean authRequired) {
        mNetworkKeyPhrase = networkKeyPhrase;
        mAuthRequired = authRequired;
        if (mNetworkKeyPhrase != null) {
            mService.setNetworkPassPhrase(mNetworkKeyPhrase);
            mNavListener.setNavigationEnabled(true);
            getActionBar().setSelectedNavigationItem(SimpleNavigationListener.POSITION_ASSOCIATION);
        }
    }

    @Override
    public boolean isAuthRequired() {
        return mAuthRequired;
    }

    @Override
    public String getNetworkKeyPhrase() {
        return mNetworkKeyPhrase;
    }

    @Override
    public void associateWithQrCode(AssociationStartedListener listener) {
        mAssStartedListener = listener;
        try {
            Intent intent = new Intent("com.google.zxing.client.android.SCAN");
            intent.putExtra("SCAN_MODE", "QR_CODE_MODE");
            startActivityForResult(intent, 0);
        }
        catch (Exception e) {
            Uri marketUri = Uri.parse("market://details?id=com.google.zxing.client.android");
            Intent marketIntent = new Intent(Intent.ACTION_VIEW, marketUri);
            startActivity(marketIntent);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == 0) {
            if (resultCode == RESULT_OK) {
                String url = data.getStringExtra("SCAN_RESULT");

                UUID uuid = null;
                long auth = 0;                
                Pattern pattern =
                        Pattern.compile("(http[s]?://[A-Z\\.0-9]+/add\\?|\\&)UUID=([0-9A-F]{8})"
                                + "([0-9A-F]{8})([0-9A-F]{8})([0-9A-F]{8})" + "\\&AC=([0-9A-F]{8})([0-9A-F]{8})",
                                Pattern.CASE_INSENSITIVE);
                Matcher matcher = pattern.matcher(url);
                if (matcher.find()) {
                    long uuidMsb =
                            ((Long.parseLong(matcher.group(2), 16) & 0xFFFFFFFFFFFFFFFFL) << 32)
                                    | ((Long.parseLong(matcher.group(3), 16) & 0xFFFFFFFFFFFFFFFFL));
                    long uuidLsb =
                            ((Long.parseLong(matcher.group(4), 16) & 0xFFFFFFFFFFFFFFFFL) << 32)
                                    | ((Long.parseLong(matcher.group(5), 16) & 0xFFFFFFFFFFFFFFFFL));

                    uuid = new UUID(uuidMsb, uuidLsb);
                    auth =
                            ((Long.parseLong(matcher.group(6), 16) & 0xFFFFFFFFFFFFFFFFL) << 32)
                                    | ((Long.parseLong(matcher.group(7), 16) & 0xFFFFFFFFFFFFFFFFL));
                    if (mAssStartedListener != null) {
                        mAssStartedListener.associationStarted();
                    }                    
                    mService.associateDevice(uuid, auth);
                }
                else {
                    // Bad QR code
                    Toast.makeText(this, getString(R.string.qr_to_uuid_fail), Toast.LENGTH_LONG).show();
                }
            }
        }
    }

    @Override
    public Device addLightGroup(String groupName) {
        LightDevice result = new LightDevice(mNextGroupId++, 0, groupName, true);
        mDeviceStore.addLightGroup(result);
        return result;
    }

	@Override
	public void setAttentionEnabled(boolean enabled) {
		mService.getAttentionModel().setAttentionState(enabled, AttentionModel.DURATION_INFINITE, mSendDeviceId);
	}

    @Override
    public void removeDeviceLocally(RemovedListener removedListener) {
        mDeviceStore.removeDevice(mSendDeviceId);
        removedListener.onDeviceRemoved(mSendDeviceId, true);
        mSendDeviceId = Device.GROUP_ADDR_BASE;
        removedListener = null;        
    }

	@Override
	public String getBridgeAddress() {
		if (mConnected) {
			return mConnectedAddress;
		}
		else {
			return null;
		}
	}
}
