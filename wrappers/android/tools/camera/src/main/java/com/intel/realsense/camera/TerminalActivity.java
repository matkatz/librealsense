package com.intel.realsense.camera;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.AutoCompleteTextView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import com.intel.realsense.librealsense.DebugProtocol;
import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.RsContext;

import java.util.Arrays;
import java.util.Map;
import java.util.Set;

public class TerminalActivity extends AppCompatActivity {

    private static final String TAG = "librs camera terminal";
    private Button mSendButton;
    private Button mSaveButton;
    private Button mClearButton;
    private EditText mInputText;
    private EditText mOutputText;
    private AutoCompleteTextView mCommandNameText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_terminal);

        mSendButton = findViewById(R.id.terminal_send_button);
        mSendButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                send();
            }
        });

        mSaveButton = findViewById(R.id.terminal_save_button);
        mSaveButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                SharedPreferences sharedPref = getSharedPreferences(getString(R.string.terminal_commands), Context.MODE_PRIVATE);
                SharedPreferences.Editor editor = sharedPref.edit();
                editor.putString(mCommandNameText.getText().toString(), mInputText.getText().toString());
                editor.commit();
                setupAutoCompleteTextView();
            }
        });

        mClearButton = findViewById(R.id.terminal_clear_button);
        mClearButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mInputText.setText("");
                mCommandNameText.setText("");
            }
        });
        mInputText = findViewById(R.id.terminal_input_edit_text);
        mOutputText = findViewById(R.id.terminal_output_edit_text);
        setupAutoCompleteTextView();
    }

    private void send() {
        RsContext mRsContext = new RsContext();
        DeviceList devices = mRsContext.queryDevices();
        if(devices.getDeviceCount() == 0)
        {
            Log.e(TAG, "getDeviceCount returned zero");
            Toast.makeText(this, "Connect a camera", Toast.LENGTH_LONG).show();
            return;
        }
        Device device = devices.createDevice(0);
        DebugProtocol dp = device.as(DebugProtocol.class);
        String content = mInputText.getText().toString();
        String[] stringArray = content.split(",");
        byte[] buff = new byte[stringArray.length];
        int redix = 16;
        for(int i = 0; i < stringArray.length; i++)
            buff[i] = (byte)Integer.parseInt(stringArray[i], redix);

        byte[] res = dp.SendAndReceiveRawData(buff);

        String strArray[] = new String[res.length];
        for(int i = 0; i < res.length; i++)
            strArray[i] = String.valueOf((int)res[i]);

        mOutputText.setText(Arrays.toString(strArray));
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    private void setupAutoCompleteTextView(){
        mCommandNameText = findViewById(R.id.terminal_input_name_edit_text);
        SharedPreferences sp = getSharedPreferences(getString(R.string.terminal_commands), Context.MODE_PRIVATE);
        Map<String, ?> spMap = sp.getAll();
        Set<String> spKeys = spMap.keySet();
        String[] spArray = new String[spKeys.size()];
        spKeys.toArray(spArray);
        ArrayAdapter<String> adapter = new ArrayAdapter<>(this,
                android.R.layout.simple_dropdown_item_1line, spArray);
        mCommandNameText.setAdapter(adapter);
//        mCommandNameText.setTokenizer(new MultiAutoCompleteTextView.CommaTokenizer());
        mCommandNameText.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> adapterView, View view, int i, long l) {
                SharedPreferences sp = getSharedPreferences(getString(R.string.terminal_commands), Context.MODE_PRIVATE);
                String item = ((TextView)view).getText().toString();
                mInputText.setText(sp.getString(item, ""));
            }
        });
    }
}
