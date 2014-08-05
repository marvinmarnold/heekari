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

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Used to store state of a mesh device.
 * 
 */
public abstract class Device {
    // Addresses in the range 0x0000 - 0x7FFF are intended for groups.
    public static final int GROUP_ADDR_BASE = 0x0000;
    // Addresses from 0x8000 to 0xFFFF are intended for addressing individual devices.
    public static final int DEVICE_ADDR_BASE = 0x8000;

    public static final int DEVICE_ID_UNKNOWN = 0x10000;
    
    // Keys used for json
    private static final String JSON_KEY_ID = "id";
    private static final String JSON_KEY_UUID_HASH = "uuid_hash";
    private static final String JSON_KEY_NAME = "name";
    private static final String JSON_KEY_DEVICE_TYPE = "device_type";
    private static final String JSON_KEY_GROUP_MEMBERSHIP = "group_membership";    
    
    enum DeviceType {
        LIGHT, LIGHT_GROUP, SWITCH, SWITCH_GROUP;

        public static final DeviceType values[] = values();
    }

    private int mDeviceId;
    private boolean mIsGroup;
    private boolean mStateKnown;
    private String mName;
    private int mUuidHash;

    // List of group ids a device belongs to. Not valid for a device that represents a group.
    private ArrayList<Integer> mGroupMembership = new ArrayList<Integer>();

    public Device(final int deviceId, final int uuidHash, final String name, final boolean isGroup,
            final boolean mStateKnown) {
        this.mDeviceId = deviceId;
        this.mUuidHash = uuidHash;
        this.mName = name;
        this.mIsGroup = isGroup;
        this.mStateKnown = mStateKnown;
    }

    /**
     * Copy constructor
     * 
     * @param other
     *            Device object to make a copy of.
     */
    public Device(final Device other) {
        this.mDeviceId = other.mDeviceId;
        this.mIsGroup = other.mIsGroup;
        this.mStateKnown = other.mStateKnown;
        this.mName = other.mName;
        this.mUuidHash = other.mUuidHash;
        for (int groupId : other.mGroupMembership) {
            mGroupMembership.add(groupId);
        }
    }

    /**
     * Copy another device object's fields into this object.
     * 
     * @param other
     *            Device object to copy.
     */
    public void copy(final Device other) {
        this.mDeviceId = other.mDeviceId;
        this.mIsGroup = other.mIsGroup;
        this.mStateKnown = other.mStateKnown;
        this.mName = other.mName;
        this.mUuidHash = other.mUuidHash;
        this.mGroupMembership.clear();
        for (int groupId : other.mGroupMembership) {
            this.mGroupMembership.add(groupId);
        }
    }

    /**
     * Get device identifier.
     * 
     * @return Device identifier.
     */
    public int getDeviceId() {
        return mDeviceId;
    }

    /**
     * Check if this Device object represents a group.
     * 
     * @return True if this is group.
     */
    public boolean isGroup() {
        return mIsGroup;
    }

    /**
     * Get UUID hash of device.
     * 
     * @return
     */
    public int getUuidHash() {
        return mUuidHash;
    }

    /**
     * Check if the state stored in this device object is valid. For lights, valid means an RGB and power state value
     * have been stored after sending these values to a physical light device.
     * 
     * @return True if the state is known.
     */
    public boolean isStateKnown() {
        return mStateKnown;
    }

    /**
     * Set the flag that indicates if the device state is valid.
     * 
     * @param known
     *            True if the state is known.
     */
    public void setStateKnown(boolean known) {
        this.mStateKnown = known;
    }

    public String getName() {
        return mName;
    }

    public void setName(final String name) {
        this.mName = name;
    }

    /**
     * Get the value at all group indices for this device. Includes values set to zero. Not valid for a group device.
     * 
     * @return List of values in order of index.
     */
    public List<Integer> getGroupMembershipValues() {
        if (mIsGroup) {
            throw new UnsupportedOperationException("Groups cannot belong to groups.");
        }
        ArrayList<Integer> result = new ArrayList<Integer>();
        for (Integer id : mGroupMembership) {
            result.add(id);
        }
        return result;
    }

    /**
     * Get the list of groups this device belongs to. Returns the value at all group indices except those set to zero.
     * Not valid for a group device.
     * @return List of values in order of index.
     */
    public List<Integer> getGroupMembership() {
        if (mIsGroup) {
            throw new UnsupportedOperationException("Groups cannot belong to groups.");
        }
        ArrayList<Integer> result = new ArrayList<Integer>();
        for (Integer id : mGroupMembership) {
            if (id != 0) {
                result.add(id);
            }
        }
        return result;
    }
    
    /**
     * Set the group ids that this device belongs to.
     * 
     * @param groupIds
     *            List of integers representing group ids this device belongs to.
     */
    public void setGroupIds(final List<Integer> groupIds) {
        if (mIsGroup) {
            throw new UnsupportedOperationException("Groups cannot belong to groups.");
        }        
        mGroupMembership.clear();
        for (int id : groupIds) {
            if (id != 0) {
                mGroupMembership.add(id);
            }
        }
    }

    /**
     * Set a group id that this device belongs to.
     * 
     * 
     * @param index
     *            Group index to set.
     * @param groupId
     *            Integers representing a group id this device belongs to.
     */
    public void setGroupId(final int index, final int groupId) {
        if (mIsGroup) {
            throw new UnsupportedOperationException("Groups cannot belong to groups.");
        }
        mGroupMembership.set(index, groupId);
    }

    /**
     * Remove a group id from the list of groups this device belongs to.
     * 
     * @param groupId
     *            The group id to remove.
     */
    public void removeGroupId(final int groupId) {
        if (mIsGroup) {
            throw new UnsupportedOperationException("Groups cannot belong to groups.");
        }
        
        int index = mGroupMembership.indexOf(groupId);
        if (index != -1) {
            mGroupMembership.remove(index);
        }        
    }

    /**
     * Clear all group memberships.
     */
    public void clearGroups() {
        if (mIsGroup) {
            throw new UnsupportedOperationException("Groups cannot belong to groups.");
        }
        mGroupMembership.clear();
    }

    /**
     * Set the number of group indices supported by the device. Adds zero entries to the group membership array to pad
     * it to the requested size.
     * 
     * @param num
     *            Number of group indices.
     */
    public void setNumGroupIndices(final int num) {
        int numToAdd = num - mGroupMembership.size();
        for (int i = 0; i < numToAdd; i++) {
            mGroupMembership.add(0);
        }
    }
    
    /**
     * Get the number of supported group indices.
     * @return Integer number of group indices.
     */
    public int getNumGroupIndices() {
        return mGroupMembership.size();
    }
    
    /**
     * Convert a device into its JSON representation.
     * 
     * @return String containing the JSON representation or NULL if operation failed.
     */
    public String toJson() {
        JSONObject json = new JSONObject();
        JSONArray jarray = new JSONArray();

        // Put the group ids into a JSON array.
        if (!mIsGroup) {
            for (int id : mGroupMembership) {
                jarray.put(id);
            }
        }
        try {
            json.put(JSON_KEY_ID, mDeviceId);
            json.put(JSON_KEY_UUID_HASH, mUuidHash);
            json.put(JSON_KEY_NAME, mName);
            json.put(JSON_KEY_DEVICE_TYPE, getDeviceType().toString());
            json.put(JSON_KEY_GROUP_MEMBERSHIP, jarray);
        }
        catch (JSONException e) {
            return null;
        }

        return json.toString();
    }

    /**
     * Create a Device object from a JSON representation.
     * 
     * @param json
     *            JSON string to use as input.
     * @return Created Device object, or NULL of operation failed.
     */
    public static Device fromJson(final String json) {
        Device dev = null;
        try {
            JSONObject jsonData = new JSONObject(json);
            JSONArray groups = jsonData.getJSONArray(JSON_KEY_GROUP_MEMBERSHIP);

            DeviceType type = DeviceType.valueOf(jsonData.getString(JSON_KEY_DEVICE_TYPE));
            String name = jsonData.getString(JSON_KEY_NAME);
            int deviceId = (int) jsonData.getInt(JSON_KEY_ID);
            int uuidHash = (int) jsonData.getInt(JSON_KEY_UUID_HASH);
            switch (type) {
            case LIGHT:
                dev = new LightDevice(deviceId, uuidHash, name, false);
                break;
            case LIGHT_GROUP:
                dev = new LightDevice(deviceId, uuidHash, name, true);
                break;
            case SWITCH:
                dev = new SwitchDevice(deviceId, uuidHash, name, false);
                break;
            default:
                break;
            }

            for (int i = 0; i < groups.length(); i++) {
                dev.mGroupMembership.add(groups.getInt(i));
            }
        }
        catch (JSONException e) {
            dev = null;
        }

        return dev;
    }

    /**
     * Get type of device represented by this object.
     * 
     * @return Device type as enum.
     */
    public abstract DeviceType getDeviceType();

}