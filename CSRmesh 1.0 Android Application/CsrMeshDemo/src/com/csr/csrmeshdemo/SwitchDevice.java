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

public class SwitchDevice extends Device {

    private PowerModel.PowerState power = PowerState.OFF;
        
    public SwitchDevice(final int deviceId, final int uuidHash, String name, boolean isGroup, boolean stateKnown, final PowerModel.PowerState power) {
        super(deviceId, uuidHash, name, isGroup, stateKnown);
        this.power = power;
    }

    public SwitchDevice(final int deviceId, final int uuidHash, String name, boolean isGroup) {
        super(deviceId, uuidHash, name, isGroup, false);
    }
    
    /**
     * Copy constructor
     * @param other SwitchDevice object to make a copy of.
     */
    public SwitchDevice(final SwitchDevice other) {
        super(other);
        this.power = other.power;
    }
    
    @Override
    public void copy(final Device other) {
        super.copy(other);
        SwitchDevice otherSwitch = (SwitchDevice)other;
        this.power = otherSwitch.power;
    }
    
    public PowerModel.PowerState getPower() {
        return power;
    }

    public void setPower(PowerModel.PowerState power) {
        this.power = power;
    }

    @Override
    public DeviceType getDeviceType() {
        return DeviceType.SWITCH;
    }
    
    @Override
    public String toString() {
        if (isGroup()) {
            return "Switch Group " + (getDeviceId() - DEVICE_ADDR_BASE);
        }
        else {
            return "Switch " + (getDeviceId() - DEVICE_ADDR_BASE);
        }
    }
}
