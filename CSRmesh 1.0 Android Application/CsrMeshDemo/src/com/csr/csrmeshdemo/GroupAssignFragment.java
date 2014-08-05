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

import java.util.ArrayList;
import java.util.List;

import com.csr.csrmeshdemo.R;
import com.csr.csrmeshdemo.CheckedListFragment.ItemListener;
import com.csr.csrmeshdemo.Device.DeviceType;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Fragment;
import android.app.FragmentTransaction;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.text.Editable;
import android.text.InputFilter;
import android.text.InputType;
import android.text.TextWatcher;
import android.util.Log;
import android.util.SparseArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

/**
 * Fragment used to configure devices. Handles assigning devices to groups, get firmware version, remove a device or
 * group, rename a device or group and add a new group. Contains two side by side CheckedListFragment fragments.
 * 
 */
public class GroupAssignFragment extends Fragment implements ItemListener, GroupListener, InfoListener, RemovedListener {
    private static final String TAG = "GroupAssignFragment";
    
    private static final int STATE_NORMAL = 0;
    private static final int STATE_EDIT_GROUP = 1;
    private static final int STATE_EDIT_DEVICE = 2;

    private int mState = STATE_NORMAL;

    private View mRootView;

    private CheckedListFragment mGroupFragment;
    private CheckedListFragment mDeviceFragment;

    protected static final String GROUPS_FRAGMENT_TAG = "groupsfrag";
    protected static final String DEVICE_FRAGMENT_TAG = "lightsfrag";

    private DeviceController mController;

    // Id of device or group with attention enabled.
    private int mAttentionId;
    
    // Used when editing group membership of a single selected device.
    private GroupList mEditGroupList = null;
    // Used when a group is selected; add each modified list of groups to this list.
    private SparseArray<GroupList> mModifiedGroups = new SparseArray<GroupList>();

    private Button mApplyButton;
    private Button mCancelButton;
    private Button mAddGroupButton;

    private Device mSelectedDevice = null;

    private ProgressDialog mProgress;

    private Fragment mMainFragment;

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);

        try {
            mController = (DeviceController) activity;
        }
        catch (ClassCastException e) {
            throw new ClassCastException(activity.toString() + " must implement DeviceController callback interface.");
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mMainFragment = this;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        if (mRootView == null) {
            mRootView = inflater.inflate(R.layout.group_assignment, container, false);
        }

        mProgress = new ProgressDialog(getActivity());

        // Add the child fragments. Nested fragments are supported from API level 17
        FragmentTransaction ft = getChildFragmentManager().beginTransaction();
        mGroupFragment = new CheckedListFragment();
        mDeviceFragment = new CheckedListFragment();

        Bundle groupBundle = new Bundle();
        groupBundle.putInt(CheckedListFragment.EXTRA_MENU_RESOURCE, R.menu.config_popup_group);
        Bundle deviceBundle = new Bundle();
        deviceBundle.putInt(CheckedListFragment.EXTRA_MENU_RESOURCE, R.menu.config_popup_device);

        mGroupFragment.setArguments(groupBundle);
        mDeviceFragment.setArguments(deviceBundle);

        ft.add(R.id.groupsFrame, mGroupFragment, GROUPS_FRAGMENT_TAG);
        ft.add(R.id.lightsFrame, mDeviceFragment, DEVICE_FRAGMENT_TAG);
        ft.commit();

        mApplyButton = (Button) mRootView.findViewById(R.id.buttonApply);
        mCancelButton = (Button) mRootView.findViewById(R.id.buttonCancel);
        mAddGroupButton = (Button) mRootView.findViewById(R.id.buttonNewGroup);
        hideButtons();

        // Apply changes to group membership.
        mApplyButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                hideButtons();

                if (mState == STATE_EDIT_GROUP) {
                    if (mModifiedGroups.size() > 0) {
                        mEditGroupList = mModifiedGroups.valueAt(0);
                        mSelectedDevice = mDeviceFragment.getItem(mEditGroupList.deviceId).getDevice();
                        mModifiedGroups.removeAt(0);
                        sendGroupUpdate();
                    }
                    else {
                        // Nothing was changed.
                    	Toast.makeText(getActivity(), getActivity().getString(R.string.group_no_changes), 
                    			Toast.LENGTH_SHORT).show();
                        resetUI();
                        mEditGroupList = null;
                    }
                }
                else if (mState == STATE_EDIT_DEVICE) {
                    sendGroupUpdate();
                }
            }
        });

        // Cancel changes to group membership.
        mCancelButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                resetUI();
                mEditGroupList = null;
            }
        });

        // Add a new group.
        mAddGroupButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                newGroup();
            }
        });

        return mRootView;
    }

    @Override
    public void onResume() {
        super.onResume();

        List<Device> groups = mController.getGroups(DeviceType.LIGHT_GROUP);
        mGroupFragment.addItems(groups);
        mDeviceFragment.addItems(mController.getDevices(DeviceType.LIGHT));
        mDeviceFragment.addItems(mController.getDevices(DeviceType.SWITCH));
        if (mDeviceFragment.getDevices().size() == 0) {
            mGroupFragment.setClickEnabled(false);
        }
    }

    @Override
    public void onStop() {
        super.onStop();
        mGroupFragment.clearItems();
        mDeviceFragment.clearItems();
        disableDeviceAttention();
    }

    /**
     * Show the Apply and Cancel buttons, hide Add Group button.
     */
    private void showButtons() {
        mApplyButton.setVisibility(View.VISIBLE);
        mCancelButton.setVisibility(View.VISIBLE);
        mAddGroupButton.setVisibility(View.INVISIBLE);
    }

    /**
     * Hide the Apply and Cancel buttons, show Add Group button.
     */
    private void hideButtons() {
        mApplyButton.setVisibility(View.INVISIBLE);
        mCancelButton.setVisibility(View.INVISIBLE);
        mAddGroupButton.setVisibility(View.VISIBLE);
    }

    /**
     * Event handler called when a group or a light is selected.
     * 
     * @param fragmentId
     *            Resource id of the fragment that generated the event. Used to differentiate lights and groups.
     * @param deviceId
     *            Id of the light or the group (device id).
     */
    @Override
    public void onItemSelected(String fragmentTag, int deviceId) {
        if (fragmentTag.compareTo(GROUPS_FRAGMENT_TAG) == 0) {
            onGroupSelected(deviceId, mGroupFragment.getSelectedItemPosition());
        }
        else if (fragmentTag.compareTo(DEVICE_FRAGMENT_TAG) == 0) {
            onDeviceSelected(deviceId, mDeviceFragment.getSelectedItemPosition());
        }
    }

    /**
     * Event handler called when a group's or a light's checkbox is checked or unchecked.
     * 
     * @param fragmentId
     *            Resource id of the fragment that generated the event. Used to differentiate lights and groups.
     * @param deviceId
     *            Id of the device or the group that was checked or unchecked.
     * @param checked
     *            Checkbox state.
     */
    @Override
    public void onItemCheckStatusChanged(String fragmentTag, int deviceId, boolean checked) {
        if (fragmentTag.compareTo(GROUPS_FRAGMENT_TAG) == 0 && mEditGroupList != null) {
            // The check event was on the group fragment so we must be editing a single light.
            // Is this device id already related to device object being edited?
            boolean inGroup = mEditGroupList.groupMembership.contains(deviceId);
            if (checked) {
                if (!inGroup) {
                    mEditGroupList.groupMembership.add(deviceId);
                }
            }
            else if (inGroup) {
                int index = mEditGroupList.groupMembership.indexOf(deviceId);
                mEditGroupList.groupMembership.remove(index);
            }
        }
        else if (fragmentTag.compareTo(DEVICE_FRAGMENT_TAG) == 0) {
            // The check event was on the device fragment so we must be editing a whole group.                        
            int groupId = mSelectedDevice.getDeviceId();            
            List<Integer> membership = mDeviceFragment.getItem(deviceId).getDevice().getGroupMembership();
            
            GroupList deviceGroups = mModifiedGroups.get(deviceId);
            if (deviceGroups == null) {
                deviceGroups = new GroupList(deviceId);
                deviceGroups.groupMembership.addAll(membership);
                mModifiedGroups.put(deviceId, deviceGroups);
            }
            
            if (checked) {
                if (!deviceGroups.groupMembership.contains(groupId)) {
                    deviceGroups.groupMembership.add(groupId);
                }
            }
            else {
                int index = deviceGroups.groupMembership.indexOf(groupId);
                deviceGroups.groupMembership.remove(index);
            }
        }
    }

    /**
     * Event handler called when a device is selected.
     * 
     * @param deviceId
     *            The id of the selected light.
     */
    private void onDeviceSelected(int deviceId, int position) {
        if (mState == STATE_NORMAL) {
            mState = STATE_EDIT_DEVICE;
                        
            enableDeviceAttention(deviceId);
            
            mGroupFragment.setContextMenuEnabled(false);
            mDeviceFragment.setContextMenuEnabled(false);

            // Make a new group list for the selected device for editing.
            mSelectedDevice = mDeviceFragment.getSelectedDevice();
            mEditGroupList = new GroupList(mSelectedDevice.getDeviceId());
            mEditGroupList.groupMembership.addAll(mSelectedDevice.getGroupMembership());

            // Show the save / cancel menu.
            showButtons();
            mDeviceFragment.setCheckBoxesVisible(false);
            mGroupFragment.setCheckBoxesVisible(true);

            // Set check box states of groups that have this light in.
            mGroupFragment.setAllCheckboxes(false);
            for (int groupId : mEditGroupList.groupMembership) {
                mGroupFragment.setDeviceCheckBoxState(groupId, true, false);
            }
        }
        else if (mState == STATE_EDIT_GROUP) {
            if (mDeviceFragment.getSelectedDevice().getDeviceType() == DeviceType.LIGHT || 
                    mDeviceFragment.getSelectedDevice().getDeviceType() == DeviceType.SWITCH) {
                // Editing a group so interpret this click as a request to toggle the checkbox.
                boolean checkedState = mDeviceFragment.getCheckBoxState(position);
                mDeviceFragment.setCheckBoxState(position, !checkedState, true);
                
            }
        }
    }

    /**
     * Event handler called when a group is selected.
     * 
     * @param groupId
     *            The id of the selected group.
     */
    private void onGroupSelected(int groupId, int position) {
        if (mState == STATE_NORMAL) {
            mState = STATE_EDIT_GROUP;

            mGroupFragment.setContextMenuEnabled(false);
            mDeviceFragment.setContextMenuEnabled(false);

            mSelectedDevice = mGroupFragment.getSelectedDevice();

            // Show the save / cancel menu.
            showButtons();
            mDeviceFragment.setCheckBoxesVisible(true);
            mGroupFragment.setCheckBoxesVisible(false);

            // Set check box states of devices that belong to this group
            mDeviceFragment.setAllCheckboxes(false);
            for (Device dev : mDeviceFragment.getDevices()) {
                if (dev.getGroupMembershipValues().contains(mSelectedDevice.getDeviceId())
                        && (dev.getDeviceType() == DeviceType.LIGHT || dev.getDeviceType() == DeviceType.SWITCH)) {
                    mDeviceFragment.setDeviceCheckBoxState(dev.getDeviceId(), true, false);
                }
            }
        }
        else if (mState == STATE_EDIT_DEVICE) {
            // Editing a device so interpret this click as a request to toggle the checkbox
            boolean checkedState = mGroupFragment.getCheckBoxState(position);
            mGroupFragment.setCheckBoxState(position, !checkedState, true);
        }
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
     * Send a group update to the remote device. The device with the edited groups is contained in mEditDevice.
     */
    private void sendGroupUpdate() {
        showProgress(getActivity().getString(R.string.group_update));
        mController.setSelectedDeviceId(mEditGroupList.deviceId);
        mController.setDeviceGroups(mEditGroupList.groupMembership, (GroupListener) mMainFragment);        
    }

    private void enableDeviceAttention(int deviceId) {
    	if (deviceId > 0) {
	    	mAttentionId = deviceId;
	    	// Turn on attention for this device.
	        mController.setSelectedDeviceId(deviceId);
	        mController.setAttentionEnabled(true);
    	}
    }

    private void disableDeviceAttention() {
    	// Send message to clear attention state
        if (mAttentionId > 0) {
        	mController.setSelectedDeviceId(mAttentionId);
        	mController.setAttentionEnabled(false);
        	mAttentionId = 0;
        }
    }

    /**
     * Reset user interface to initial state and set state variable to indicate groups are not being edited.
     */
    private void resetUI() {
        hideButtons();
        mGroupFragment.setContextMenuEnabled(true);
        mDeviceFragment.setContextMenuEnabled(true);
        mGroupFragment.setCheckBoxesVisible(false);
        mDeviceFragment.setCheckBoxesVisible(false);
        mGroupFragment.selectNone();
        mDeviceFragment.selectNone();
        mDeviceFragment.clearSelection();
        mGroupFragment.clearSelection();
        disableDeviceAttention();
        mState = STATE_NORMAL;
    }

    
    /**
     * Show dialogue asking user to confirm if a device should be removed, and remove it if OK is pressed.
     * 
     * @param deviceId
     *            The id of the device to be removed.
     */
    private void confirmRemove(final int deviceId) {
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        builder.setMessage(getActivity().getString(R.string.confirm_remove)).setCancelable(false)
                .setPositiveButton(getActivity().getString(R.string.yes), new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        // Perform remove operation.
                        mController.setSelectedDeviceId(deviceId);
                        mController.removeDevice((RemovedListener) mMainFragment);
                        if (deviceId >= Device.DEVICE_ADDR_BASE) {
                            // Removing groups is instant so no need for progress.
                            showProgress(getActivity().getString(R.string.removing));
                        }
                    }
                }).setNegativeButton(getActivity().getString(R.string.no), new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        dialog.cancel();
                    }
                });
        AlertDialog alert = builder.create();
        alert.show();
    }

    private void confirmRemoveLocal(final int deviceId) {
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        builder.setMessage(getActivity().getString(R.string.confirm_remove_local)).setCancelable(false)
                .setPositiveButton(getActivity().getString(R.string.yes), new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        // Perform remove operation.
                        mController.setSelectedDeviceId(deviceId);
                        mController.removeDeviceLocally((RemovedListener) mMainFragment);
                    }
                }).setNegativeButton(getActivity().getString(R.string.no), new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        dialog.cancel();
                    }
                });
        AlertDialog alert = builder.create();
        alert.show();
    }
    
    /**
     * Add a new group.
     */
    private void newGroup() {
        // Show dialogue to allow user to enter a name for the new group.
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        builder.setTitle(getActivity().getString(R.string.enter_group_name));
        final EditText input = new EditText(getActivity());
        input.setInputType(InputType.TYPE_CLASS_TEXT);
        input.setFilters(new InputFilter[] { new InputFilter.LengthFilter(CheckedListFragment.MAX_DEVICE_NAME_LENGTH) });
        builder.setView(input);

        builder.setPositiveButton(getActivity().getString(R.string.ok), new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                String newName = input.getText().toString();
                Device newGroup = mController.addLightGroup(newName);
                mGroupFragment.addItem(newGroup);
            }
        });
        builder.setNegativeButton(getActivity().getString(R.string.cancel), new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.cancel();
            }
        });

        final AlertDialog dialog = builder.show();
        dialog.getButton(AlertDialog.BUTTON_POSITIVE).setEnabled(false);

        input.addTextChangedListener(new TextWatcher() {
            @Override
            public void afterTextChanged(Editable text) {

                if (text.length() <= 0) {
                    dialog.getButton(AlertDialog.BUTTON_POSITIVE).setEnabled(false);
                }
                else {
                    dialog.getButton(AlertDialog.BUTTON_POSITIVE).setEnabled(true);
                }
            }

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                // No behaviour.
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                // No behaviour.
            }
        });

    }

    @Override
    public void groupsUpdated(int deviceId, boolean success, String msg) {
        if (success) {
            mSelectedDevice.setGroupIds(mEditGroupList.groupMembership);
            if (mState == STATE_EDIT_DEVICE || mModifiedGroups.size() == 0) {
                Toast.makeText(getActivity(), msg, Toast.LENGTH_LONG).show();
                resetUI();
                hideProgress();
            }
            else if (mState == STATE_EDIT_GROUP) {
                if (mModifiedGroups.size() > 0) {
                    mEditGroupList = mModifiedGroups.valueAt(0);
                    mSelectedDevice = mDeviceFragment.getItem(mEditGroupList.deviceId).getDevice();
                    mModifiedGroups.removeAt(0);
                    sendGroupUpdate();
                }
                else {
                    Toast.makeText(getActivity(), msg, Toast.LENGTH_LONG).show();
                    resetUI();
                    hideProgress();
                }
            }
        }
        else {
            Toast.makeText(getActivity(), msg, Toast.LENGTH_LONG).show();
            resetUI();
            hideProgress();
        }
    }

    @Override
    public void itemRename(String fragmentTag, int deviceId, String name) {
        mController.setDeviceName(deviceId, name);
    }

    @Override
    public void itemRemove(String fragmentTag, int deviceId) {
        confirmRemove(deviceId);
    }

    @Override
    public void itemInfo(String fragmentTag, int deviceId) {
        showProgress(getActivity().getString(R.string.waiting_for_response));
        mController.setSelectedDeviceId(deviceId);
        mController.getFwVersion((InfoListener) mMainFragment);
    }

    @Override
    public void onItemContextMenuClick(int menuGroupId, int position, int menuId) {
        if (menuGroupId == R.id.group_menu_group) {
            mGroupFragment.handleContextMenu(position, menuId);
        }
        else if (menuGroupId == R.id.device_menu_group) {
            mDeviceFragment.handleContextMenu(position, menuId);
        }
    }

    @Override
    public void onFirmwareVersion(int deviceId, int major, int minor, boolean success) {
        hideProgress();
        if (success) {
            AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
            builder.setMessage(getActivity().getString(R.string.firmware_version_is) + " " + major + "." + minor).setCancelable(false)
                    .setPositiveButton(getActivity().getString(R.string.ok), new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int id) {
                            dialog.cancel();
                        }
                    });
            AlertDialog alert = builder.create();
            alert.show();
        }
        else {
            Toast.makeText(getActivity(), getActivity().getString(R.string.firmware_get_fail), Toast.LENGTH_SHORT).show();
        }
    }

    @Override
    public void onDeviceRemoved(int deviceId, boolean success) {
        hideProgress();
        if (success) {
            mGroupFragment.clearItems();
            mDeviceFragment.clearItems();
            mGroupFragment.addItems(mController.getGroups(DeviceType.LIGHT_GROUP));
            mDeviceFragment.addItems(mController.getDevices(DeviceType.LIGHT));
            mDeviceFragment.addItems(mController.getDevices(DeviceType.SWITCH));
            if (mDeviceFragment.getDevices().size() == 0) {
                mGroupFragment.setClickEnabled(false);
            }
        }
        else {
            confirmRemoveLocal(deviceId);
        }
    }
    
    private class GroupList {
        public int deviceId;
        public ArrayList<Integer> groupMembership = new ArrayList<Integer>();
        
        public GroupList(int deviceId) {
            this.deviceId = deviceId;
        }
    }
}
