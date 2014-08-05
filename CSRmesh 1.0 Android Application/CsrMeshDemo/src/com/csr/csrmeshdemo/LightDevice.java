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

import com.csr.mesh.PowerModel;
import com.csr.mesh.PowerModel.PowerState;

public class LightDevice extends Device {
    private byte red = 0;
    private byte green = 0;
    private byte blue = 0;
    private PowerModel.PowerState power = PowerState.OFF;
    private byte level = 0;
    private int colourTemp = 0;    
    
    public LightDevice(final int deviceId, final int uuidHash, String name, boolean isGroup, boolean stateKnown, final byte r, final byte g, final byte b,
            final PowerModel.PowerState power, final byte level, final int colourTemp) {
        super(deviceId, uuidHash, name, isGroup, stateKnown);
        this.red = r;
        this.green = g;
        this.blue = b;
        this.power = power;
        this.level = level;
        this.colourTemp = colourTemp;
    }
    
    /**
     * Create a light device with an unknown state
     * @param deviceId Device id.
     * @param name Name to display in UI.
     * @param isGroup True if this is a group.
     */
    public LightDevice(final int deviceId, final int uuidHash, String name, boolean isGroup) {
        super(deviceId, uuidHash, name, isGroup, false);
    }
    
    /**
     * Copy constructor
     * @param other LightDevice object to make a copy of.
     */
    public LightDevice(final LightDevice other) {
        super(other);
        this.red = other.red;
        this.green = other.green;
        this.blue = other.blue;
        this.power = other.power;
        this.level = other.level;
        this.colourTemp = other.colourTemp;
    }
    
    @Override
    public void copy(final Device other) {
        super.copy(other);
        LightDevice otherLight = (LightDevice)other;
        this.red = otherLight.red;
        this.green = otherLight.green;
        this.blue = otherLight.blue;
        this.power = otherLight.power;
        this.level = otherLight.level;
        this.colourTemp = otherLight.colourTemp;        
    }
    
    public byte getRed() {
        return red;
    }

    public void setRed(byte red) {
        this.red = red;
    }

    public byte getGreen() {
        return green;
    }

    public void setGreen(byte green) {
        this.green = green;
    }

    public byte getBlue() {
        return blue;
    }

    public void setBlue(byte blue) {
        this.blue = blue;
    }

    public PowerModel.PowerState getPower() {
        return power;
    }

    public void setPower(PowerModel.PowerState power) {
        this.power = power;
    }

    public byte getLevel() {
        return level;
    }

    public void setLevel(byte level) {
        this.level = level;
    }

    public int getColourTemp() {
        return colourTemp;
    }

    public void setColourTemp(int colourTemp) {
        this.colourTemp = colourTemp;
    }

    @Override
    public DeviceType getDeviceType() {
        if (isGroup()) {
            return DeviceType.LIGHT_GROUP;
        }
        else {
            return DeviceType.LIGHT;
        }
    }       
    
    @Override
    public String toString() {
        if (isGroup()) {
            if (getDeviceId() == Device.GROUP_ADDR_BASE) {
                return "All Lights";
            }
            return "Light Group " + (getDeviceId()); 
        }
        else {
            return "Light " + (getDeviceId() - DEVICE_ADDR_BASE);
        }
    }    
}
