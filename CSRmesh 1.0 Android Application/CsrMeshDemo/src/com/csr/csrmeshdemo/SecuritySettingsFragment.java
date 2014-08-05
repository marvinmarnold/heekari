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

import android.app.Activity;
import android.app.Fragment;
import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import com.csr.csrmeshdemo.R;

/**
 * Fragment to allow user to set network phrase and enable/disable authorisation.
 * 
 */
public class SecuritySettingsFragment extends Fragment {

    private DeviceController mController;

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
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        String phrase = mController.getNetworkKeyPhrase();
        boolean auth = mController.isAuthRequired();
        View view = inflater.inflate(R.layout.security_settings_fragment, container, false);
        Button okBtn = (Button) view.findViewById(R.id.network_assocoiation_ok);
        final CheckBox deviceAuthenticated = (CheckBox) view.findViewById(R.id.checkbox);
        deviceAuthenticated.setChecked(auth);
        final EditText passPhraseView = (EditText) view.findViewById(R.id.network_pass);
        final TextView bridgeAddress = (TextView)view.findViewById(R.id.textBridgeAddress);
        bridgeAddress.setText(mController.getBridgeAddress());
        if (phrase != null) {
            passPhraseView.setText(phrase);
        }
        okBtn.setClickable(true);
        
        okBtn.setOnClickListener(new OnClickListener() {                
            @Override
            public void onClick(View v) {
                if (passPhraseView.getText().toString().trim().length() > 0) {
                    // Hide soft keyboard.
                    InputMethodManager imm =
                            (InputMethodManager) getActivity().getSystemService(Context.INPUT_METHOD_SERVICE);
                    imm.hideSoftInputFromWindow(passPhraseView.getWindowToken(), 0);
                    // Tell MainActivity about security settings. This will also switch to another Fragment.
                    mController.setSecurity(passPhraseView.getText().toString(), deviceAuthenticated.isChecked());
                }
                else {
                    Toast.makeText(getActivity(), getResources().getString(R.string.key_required), Toast.LENGTH_SHORT)
                            .show();
                }
            }
        });

        return view;
    }
}
