package com.intel.realsense.camera;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.AutoCompleteTextView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import com.intel.realsense.librealsense.CameraInfo;
import com.intel.realsense.librealsense.DebugProtocol;
import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.Extension;
import com.intel.realsense.librealsense.HWCommand;
import com.intel.realsense.librealsense.RsContext;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class TerminalActivity extends AppCompatActivity {

    private static final String TAG = "librs camera terminal";
    private Button mSendButton;
    private Button mClearButton;
    private AutoCompleteTextView mInputText;
    private EditText mOutputText;

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

        mClearButton = findViewById(R.id.terminal_clear_button);
        mClearButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mInputText.setText("");
            }
        });
        mInputText = findViewById(R.id.terminal_input_edit_text);
        mInputText.setOnKeyListener(new View.OnKeyListener() {
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                if ((event.getAction() == KeyEvent.ACTION_DOWN) &&
                        (keyCode == KeyEvent.KEYCODE_ENTER)) {
                    send();
                    return true;
                }
                return false;
            }
        });
        mOutputText = findViewById(R.id.terminal_output_edit_text);
        setupAutoCompleteTextView();
    }
    
    private void send() {
        RsContext mRsContext = new RsContext();
        try(DeviceList devices = mRsContext.queryDevices()){
            if(devices.getDeviceCount() == 0)
            {
                Log.e(TAG, "getDeviceCount returned zero");
                Toast.makeText(this, "Connect a camera", Toast.LENGTH_LONG).show();
                return;
            }
            try(Device device = devices.createDevice(0)){
                DebugProtocol dp = device.as(Extension.DEBUG);
                String content = mInputText.getText().toString();

                String res = dp.SendAndReceiveRawData(content);
                mOutputText.setText(res);
            }
            catch(Exception e){
                mOutputText.setText("Error: " + e.getMessage());
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    private void setupAutoCompleteTextView(){
        List<String> hwCommands = new ArrayList<>();
        for(HWCommand hwc : HWCommand.values())
            hwCommands.add(hwc.name());
        String[] spArray = new String[hwCommands.size()];
        hwCommands.toArray(spArray);
        ArrayAdapter<String> adapter = new ArrayAdapter<>(this,
                android.R.layout.simple_dropdown_item_1line, spArray);
        mInputText.setAdapter(adapter);
//        mInputText.setThreshold(1);
    }
}
