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
import java.util.Collections;
import java.util.Comparator;
import java.util.UUID;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Fragment;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.text.Editable;
import android.text.InputFilter;
import android.text.InputType;
import android.text.Selection;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.csr.csrmeshdemo.R;
/**
 * Fragment used to discover and associate devices. Devices can be associated by tapping them or by pressing the QR code
 * button and scanning a QR code that will provide the UUID and authorisation code. If authorisation has been enabled on
 * the security settings screen then when a UUID is tapped the short code is prompted for.
 * 
 */
public class AssociationFragment extends Fragment implements AssociationListener, AssociationStartedListener {
    private static final int MAX_SHORT_CODE_LENGTH = 24;
    private DeviceController mController;
    private ArrayList<ScanInfo> mNewDevices = new ArrayList<ScanInfo>();
    private Button mQrButton;

    private UuidResultsAdapter resultsAdapter;
    private ProgressDialog mProgress;

    private int mRemovePosition;
    private boolean mPositionKnown;

    private AssociationFragment mFragment;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mProgress = new ProgressDialog(getActivity());
        mFragment = this;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.association_fragment, container, false);

        resultsAdapter = new UuidResultsAdapter(getActivity(), mNewDevices);
        ListView listView = (ListView) view.findViewById(R.id.device_list);
        listView.setAdapter(resultsAdapter);
        listView.setOnItemClickListener(new OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                int hash = mNewDevices.get(position).uuidHash;
                mPositionKnown = true;
                if (mController.isAuthRequired()) {
                    associateShortCode(hash, position);
                }
                else {
                    mController.associateDevice(hash);
                    mRemovePosition = position;
                    showProgress();
                }
            }
        });

        mQrButton = (Button) view.findViewById(R.id.qr_code);
        mQrButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                mPositionKnown = false;
                mController.associateWithQrCode(mFragment);
            }
        });

        return view;
    }

    @Override
    public void onResume() {
        super.onResume();
        mNewDevices.clear();        
        resultsAdapter.notifyDataSetChanged();
        getActivity().setProgressBarIndeterminateVisibility(true);
        mController.discoverDevices(true, this);
    }

    @Override
    public void onStop() {
        super.onStop();
        mController.discoverDevices(false, null);
        getActivity().setProgressBarIndeterminateVisibility(false);
    }

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
    public void newUuid(UUID uuid, int uuidHash, int rssi) {
        boolean existing = false;
        for (ScanInfo info : mNewDevices) {
            if (info.uuid.equalsIgnoreCase(uuid.toString())) {
                info.rssi = rssi;
                resultsAdapter.notifyDataSetChanged();
                existing = true;
                break;
            }
        }
        if (!existing) {
            mNewDevices.add(new ScanInfo(uuid.toString().toUpperCase(), rssi, uuidHash));        
            resultsAdapter.notifyDataSetChanged();
        }        
    }

    /**
     * Show modal progress dialogue whilst associating a device.
     */
    private void showProgress() {
        getActivity().setProgressBarIndeterminateVisibility(false);
        mProgress.setMessage(getActivity().getString(R.string.associating));
        mProgress.setProgressStyle(ProgressDialog.STYLE_SPINNER);
        mProgress.setIndeterminate(true);
        mProgress.setCancelable(false);
        mProgress.show();
    }

    /**
     * Hide the progress dialogue when association is finished.
     */
    private void hideProgress() {
        mProgress.dismiss();
        getActivity().setProgressBarIndeterminateVisibility(true);
    }

    /**
     * Associate a device after first prompting for a short code.
     * 
     * @param hash
     *            The 31-bit UUID hash of the device to associate.
     * @param position
     *            Position of device in ListView.
     */
    private void associateShortCode(final int hash, final int position) {
        final AlertDialog.Builder shortCodeDialog = new AlertDialog.Builder(getActivity());
        shortCodeDialog.setTitle(getActivity().getString(R.string.short_code_prompt));
        final EditText input = new EditText(getActivity());
        input.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
        input.setFilters(new InputFilter[] { new InputFilter.LengthFilter(MAX_SHORT_CODE_LENGTH) });
        shortCodeDialog.setView(input);

        shortCodeDialog.setPositiveButton(getActivity().getString(R.string.ok), new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                try {
                    if (mController.associateDevice(hash & 0x7FFFFFFF, input.getText().toString())) {
                        mRemovePosition = position;
                        showProgress();
                    }
                    else {
                        Toast.makeText(getActivity(), getActivity().getString(R.string.short_code_match_fail), Toast.LENGTH_LONG).show();
                    }
                }
                catch (IllegalArgumentException e) {
                    Toast.makeText(getActivity(), getActivity().getString(R.string.shortcode_invalid), Toast.LENGTH_LONG).show();
                }
            }
        });
        shortCodeDialog.setNegativeButton(getActivity().getString(R.string.cancel), new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.cancel();
            }
        });

        final AlertDialog dialog = shortCodeDialog.show();

        dialog.getButton(AlertDialog.BUTTON_POSITIVE).setEnabled(false);

        input.addTextChangedListener(new TextWatcher() {
            private boolean isFormatting;
            private boolean deletingHyphen;
            private int hyphenStart;
            private boolean deletingBackward;

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                // Do nothing.
            }

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                if (isFormatting)
                    return;

                // Make sure user is deleting one char, without a selection
                final int selStart = Selection.getSelectionStart(s);
                final int selEnd = Selection.getSelectionEnd(s);
                if (s.length() > 1 // Can delete another character
                        && count == 1 // Deleting only one character
                        && after == 0 // Deleting
                        && s.charAt(start) == '-' // a hyphen
                        && selStart == selEnd) { // no selection
                    deletingHyphen = true;
                    hyphenStart = start;
                    // Check if the user is deleting forward or backward
                    if (selStart == start + 1) {
                        deletingBackward = true;
                    }
                    else {
                        deletingBackward = false;
                    }
                }
                else {
                    deletingHyphen = false;
                }
            }

            @Override
            public void afterTextChanged(Editable text) {
                if (isFormatting)
                    return;

                isFormatting = true;

                // If deleting hyphen, also delete character before or after it
                if (deletingHyphen && hyphenStart > 0) {
                    if (deletingBackward) {
                        if (hyphenStart - 1 < text.length()) {
                            text.delete(hyphenStart - 1, hyphenStart);
                        }
                    }
                    else if (hyphenStart < text.length()) {
                        text.delete(hyphenStart, hyphenStart + 1);
                    }
                }
                if ((text.length() + 1) % 5 == 0) {
                    text.append("-");
                }

                isFormatting = false;

                if (text.length() < MAX_SHORT_CODE_LENGTH) {
                    dialog.getButton(AlertDialog.BUTTON_POSITIVE).setEnabled(false);
                }
                else {
                    dialog.getButton(AlertDialog.BUTTON_POSITIVE).setEnabled(true);
                }
            }
        });

    }

    @Override
    public void deviceAssociated(boolean success) {
        hideProgress();
        if (!success) {
            Toast.makeText(getActivity(), getActivity().getString(R.string.association_fail), Toast.LENGTH_SHORT).show();
        }
        else if (mPositionKnown) {
            mNewDevices.remove(mRemovePosition);            
            resultsAdapter.notifyDataSetChanged();
        }
    }

    @Override
    public void associationStarted() {
        // Association was triggered by MainActivity, so display progress dialogue.
        mProgress.setMessage(getActivity().getString(R.string.associating));
        mProgress.setProgressStyle(ProgressDialog.STYLE_SPINNER);
        mProgress.setIndeterminate(true);
        mProgress.setCancelable(false);
        mProgress.show();
    }
    
    private class UuidResultsAdapter extends BaseAdapter {
        private Activity activity;
        private ArrayList<ScanInfo> data;
        private LayoutInflater inflater = null;

        public UuidResultsAdapter(Activity a, ArrayList<ScanInfo> object) {
            activity = a;
            data = object;
            inflater = (LayoutInflater) activity.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        }

        public int getCount() {
            return data.size();
        }

        public Object getItem(int position) {
            return data.get(position);
        }

        public long getItemId(int position) {
            return position;
        }

        public View getView(int position, View convertView, ViewGroup parent) {
            TextView nameText;
            TextView rssiText;
            if (convertView == null) {
                convertView = inflater.inflate(R.layout.uuid_list_row, null);
                nameText = (TextView) convertView.findViewById(R.id.ass_uuid);
                rssiText = (TextView) convertView.findViewById(R.id.ass_rssi);
                convertView.setTag(new ViewHolder(nameText, rssiText));
            }
            else {
                ViewHolder viewHolder = (ViewHolder) convertView.getTag();
                nameText = viewHolder.uuid;
                rssiText = viewHolder.rssi;
            }

            ScanInfo info = (ScanInfo) data.get(position);            
            nameText.setText(info.uuid);            
            rssiText.setText(String.valueOf(info.rssi) + "dBm");
            
            return convertView;
        }
        
        
    }
   
    private class ScanInfo {
        public String uuid;        
        public int rssi;
        public int uuidHash;
        public ScanInfo(String uuid, int rssi, int uuidHash) {
            this.uuid = uuid;
            this.rssi = rssi;
            this.uuidHash = uuidHash;
        }
    }
    
    private static class ViewHolder {
        public final TextView uuid;
        public final TextView rssi;        

        public ViewHolder(TextView uuid, TextView rssi) {
            this.uuid = uuid;
            this.rssi = rssi;
        }
    }
}
