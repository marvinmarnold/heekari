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
import java.util.LinkedHashMap;
import java.util.List;

import com.csr.csrmeshdemo.Device.DeviceType;

/**
 * Stores all groups and associated devices as a collection of Device objects.
 * 
 */
public class DeviceStore {

    private LinkedHashMap<Integer, LightDevice> lights = new LinkedHashMap<Integer, LightDevice>();
    private LinkedHashMap<Integer, SwitchDevice> switches = new LinkedHashMap<Integer, SwitchDevice>();
    private LinkedHashMap<Integer, LightDevice> lightGroups = new LinkedHashMap<Integer, LightDevice>();

    /**
     * Get a device by id
     * 
     * @param id
     *            The device id to get.
     * @return Device object that contains the device's state or null if the device doesn't exist.
     */
    public Device getDevice(final int deviceId) {
        if (lights.containsKey(deviceId)) {
            return new LightDevice(lights.get(deviceId));
        }
        else if (switches.containsKey(deviceId)) {
            return new SwitchDevice(switches.get(deviceId));
        }
        else if (lightGroups.containsKey(deviceId)) {
            return new LightDevice(lightGroups.get(deviceId));
        }
        return null;
    }

    /**
     * Get a single light device.
     * 
     * @param deviceId
     *            The device id of the light to find.
     * @return A LightDevice object, or null if the device id wasn't found.
     */
    public LightDevice getLight(int deviceId) {
        if (lights.containsKey(deviceId)) {
            return new LightDevice(lights.get(deviceId));
        }
        else {
            return null;
        }
    }

    /**
     * Get all the light devices.
     * 
     * @return A List of light devices, or an empty List if none exist.
     */
    public List<Device> getAllLights() {
        ArrayList<Device> devices = new ArrayList<Device>();
        for (LightDevice d : lights.values()) {
            devices.add(new LightDevice(d));
        }
        return devices;
    }

    /**
     * Get the first light in the list of light devices.
     * 
     * @return LightDevice object.
     */
    public LightDevice getFirstLight() {
        if (!lights.isEmpty()) {
            return new LightDevice(lights.entrySet().iterator().next().getValue());
        }
        return null;
    }

    /**
     * Update a light or light group. This is used when saving the state of the colour control UI for a particular
     * group.
     * 
     * @param updated
     *            The light from which to copy parameters into the existing light.
     */
    public void updateLight(LightDevice updated) {
        LightDevice existing = lights.get(updated.getDeviceId());
        if (existing == null) {
            existing = lightGroups.get(updated.getDeviceId());
        }
        if (existing == null) {
            throw new IllegalArgumentException("Device id does not exist");
        }
        else {
            existing.copy(updated);
        }
    }

    /**
     * Update a device.
     * 
     * @param updated
     *            Device object containing the device id of the device to update and the state to update it with.
     */
    public void updateDevice(Device updated) {
        if (updated.getDeviceType() == DeviceType.LIGHT || updated.getDeviceType() == DeviceType.LIGHT_GROUP) {
            updateLight((LightDevice) updated);
        }
        else if (updated.getDeviceType() == DeviceType.SWITCH || updated.getDeviceType() == DeviceType.SWITCH_GROUP) {
            updateSwitch((SwitchDevice) updated);
        }
    }

    /**
     * Get a single switch device.
     * 
     * @param deviceId
     *            The device id of the switch to find.
     * @return A SwitchDevice object, or null if the device id wasn't found.
     */
    public SwitchDevice getSwitch(int deviceId) {
        if (switches.containsKey(deviceId)) {
            return new SwitchDevice(switches.get(deviceId));
        }
        else {
            return null;
        }
    }

    /**
     * Get all the switch devices.
     * 
     * @return A List of switch devices, or an empty List if none exist.
     */
    public List<Device> getAllSwitches() {
        ArrayList<Device> devices = new ArrayList<Device>();
        for (SwitchDevice d : switches.values()) {
            devices.add(new SwitchDevice(d));
        }
        return devices;
    }

    /**
     * Update the state of a switch device.
     * 
     * @param updated
     *            The SwitchDevice object to update with.
     */
    public void updateSwitch(SwitchDevice updated) {
        SwitchDevice existing = switches.get(updated.getDeviceId());
        if (existing == null) {
            throw new IllegalArgumentException("Device id does not exist");
        }
        else {
            existing.copy(updated);
        }
    }

    /**
     * Get all light groups.
     * 
     * @return A List of light groups, or an empty List if none exist.
     */
    public List<Device> getAllLightGroups() {
        ArrayList<Device> groups = new ArrayList<Device>();
        for (LightDevice d : lightGroups.values()) {
            groups.add(new LightDevice(d));
        }
        return groups;
    }

    /**
     * Add a new light. If the device id already exists then update the existing light.
     * 
     * @param lightDevice
     *            The light to add.
     * @throws DuplicateDeviceIdException
     *             If a switch device already exists with the same device id.
     */
    public void addLight(LightDevice lightDevice) {
        if (switches.containsKey(lightDevice.getDeviceId())) {
            throw new DuplicateDeviceIdException("A switch device already exists with the same device id.");
        }
        else {
            lights.put(lightDevice.getDeviceId(), new LightDevice(lightDevice));
        }
    }

    /**
     * Add a new switch. If the device id already exists then update the existing switch.
     * 
     * @param switchDevice
     *            The switch to add.
     * @throws DuplicateDeviceIdException
     *             If a light device already exists with the same device id.
     */
    public void addSwitch(SwitchDevice switchDevice) {
        if (lights.containsKey(switchDevice.getDeviceId())) {
            throw new DuplicateDeviceIdException("A light device already exists with the same device id.");
        }
        else {
            switches.put(switchDevice.getDeviceId(), new SwitchDevice(switchDevice));
        }
    }

    /**
     * Add a light group.
     * 
     * @param lightGroup
     *            The light device object representing the light group. Must have an id in the group address range.
     */
    public void addLightGroup(LightDevice lightGroup) {
        if (lightGroup.getDeviceId() < Device.GROUP_ADDR_BASE || lightGroup.getDeviceId() >= Device.DEVICE_ADDR_BASE) {
            throw new IllegalArgumentException("Specified group id is out of range.");
        }
        else {
            lightGroups.put(lightGroup.getDeviceId(), new LightDevice(lightGroup));
        }
    }

    /**
     * Remove a device.
     * 
     * @param deviceId
     *            Device id of device to remove.
     */
    public void removeDevice(int deviceId) {
        if (lights.containsKey(deviceId)) {
            removeLight(deviceId);
        }
        else if (switches.containsKey(deviceId)) {
            removeSwitch(deviceId);
        }
        else if (lightGroups.containsKey(deviceId)) {
            removeLightGroup(deviceId);
        }
    }

    /**
     * Remove an existing light.
     * 
     * @param deviceId
     *            Device id of the light to remove.
     */
    public void removeLight(int deviceId) {
        lights.remove(deviceId);
    }

    /**
     * Remove a group and update devices that belong to the removed group.
     * 
     * @param groupId
     *            Group id of group to remove.
     */
    public void removeLightGroup(int groupId) {
        lightGroups.remove(groupId);
        // Remove this group from all lights and switches.
        for (LightDevice light : lights.values()) {
            if (light.getGroupMembershipValues().contains(groupId)) {
                light.removeGroupId(groupId);
            }
        }
        for (SwitchDevice sw : switches.values()) {
            if (sw.getGroupMembershipValues().contains(groupId)) {
                sw.removeGroupId(groupId);
            }
        }
    }

    /**
     * Remove an existing switch.
     * 
     * @param switchDevice
     *            Device id of the switch to remove.
     */
    public void removeSwitch(int deviceId) {
        switches.remove(deviceId);
    }
}
