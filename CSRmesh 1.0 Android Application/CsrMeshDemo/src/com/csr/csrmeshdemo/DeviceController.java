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

import java.util.List;

import com.csr.csrmeshdemo.Device.DeviceType;
import com.csr.mesh.PowerModel;
import com.csr.mesh.PowerModel.PowerState;

/**
 * Defines an interface that Fragments can use to communicate with the Activity.
 * 
 */
public interface DeviceController {

    /**
     * Discover devices that are advertising to be associated.
     */
    public void discoverDevices(boolean enabled, AssociationListener listener);

    /**
     * Associate a deivce.
     * 
     * @param uuidHash
     *            31-bit uuid hash of device.
     */
    public void associateDevice(int uuidHash);

    /**
     * Associate a device.
     * 
     * @param uuidHash
     *            31-bit uuid hash of device.
     * @param shortCode
     *            Device's short code.
     * @return True if deviceHash matches short code deviceHash, false if it didn't match and association was cancelled.
     */
    public boolean associateDevice(int uuidHash, String shortCode);

    /**
     * Get a list of devices of a particular type.
     * 
     * @param type
     *            The type of device.
     * @return List of Device objects that belong to type.
     */
    public List<Device> getDevices(final DeviceType type);

    /**
     * Get a list of groups for a particular type.
     * 
     * @param type
     *            The type of device.
     * @return List of Device objects, each on representing a group.
     */
    public List<Device> getGroups(final DeviceType type);

    /**
     * Get an associated device by device id.
     * 
     * @param deviceId
     *            The device id to look for.
     * @return Device object representing the device.
     */
    public Device getDevice(final int deviceId);

    /**
     * Set the name of a device or a group.
     * 
     * @param deviceId
     *            Id of device or group to modify.
     * @param name
     *            The new name for the device or group.
     */
    public void setDeviceName(final int deviceId, final String name);

    /**
     * Get the currently selected device.
     * 
     * @return Device object representing the selected device.
     */
    public int getSelectedDeviceId();    

    /**
     * Set group membership for the currently selected device.
     * 
     * @param groups
     *            List of groups to assign the current device to. List should contain integer group ids.
     * @param listener
     *            The listener that will receive updates when the group assignment is complete.
     */
    public void setDeviceGroups(final List<Integer> groups, final GroupListener listener);

    /**
     * Add a new light group.
     * 
     * @param groupName
     *            The name for the new group.
     * @return The Device object representing the group.
     */
    public Device addLightGroup(String groupName);

    /**
     * Send a colour to the light.
     * 
     * @param color
     *            RGB hue and saturation of light encoded as an int.
     * @param brightness
     *            Value that will be added to the hue and saturation (color).
     */
    public void setLightColor(final int color, final int brightness);

    /**
     * Send command to a light to set the power state.
     * 
     * @param state
     *          State to set.
     */
    public void setLightPower(PowerModel.PowerState state);
    
    /**
     * Set power of device locally but don't send a command to remote device.
     * Used because RGB command has a side effect on the remote light that it also set the power
     * state to on if it is currently off.
     * 
     * @param state
     * 			State to set.
     * @param deviceId
     * 			Device id of device to update.
     */
    public void setLocalLightPower(PowerState state);
    
    
    /**
     * Set which device to send commands to.
     * 
     * @param type
     *            The type of device.
     * @param deviceId
     *            Device id.
     */
    public void setSelectedDeviceId(final int deviceId);

    /**
     * Reset the currently selected device (remove association).
     */
    public void removeDevice(RemovedListener listener);    

	/**
	 * Remove device locally without sending a message to remove association on remote device.
	 *
	 */
    public void removeDeviceLocally(RemovedListener listener);
    
    /**
     * Get the firmware version of the currently selected device. To receive the returned state a Fragment must have
     * registered itself via <code>registerForLightConfigUpdates</code>
     */
    public void getFwVersion(InfoListener listener);

    /**
     * Set security settings.
     * 
     * @param networkKeyPhrase
     *            The phrase used to generate the network key.
     * @param authRequired
     *            True if authorisation should be used when associating (will cause short code to be prompted for).
     */
    public void setSecurity(String networkKeyPhrase, boolean authRequired);

    /**
     * Find out if authorisation is currently enabled.
     * 
     * @return True if authorisation is required.
     */
    public boolean isAuthRequired();

    /**
     * Get the set network key phrase used to generate the network key.
     * 
     * @return The network phrase.
     */
    public String getNetworkKeyPhrase();

    /**
     * Open the QR code reader, and start association with the device retrieved from the QR code.
     * 
     * @param listener
     *            Call back to indicate to caller that association was started with a device.
     */
    public void associateWithQrCode(AssociationStartedListener listener);
    
    /**
     * Control if a devices attention mechanism (e.g. flashing a light) is enabled or not.
     * @param enabled True if attention should be enabled.
     */
    public void setAttentionEnabled(boolean enabled);

    
    /**
     * Get the address of the mesh bridge device that is currently connected. 
     * @return String containing bridge device Bluetooth address.
     */
    public String getBridgeAddress();
}
