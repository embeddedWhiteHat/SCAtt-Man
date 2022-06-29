package com.university.sec.scatt_man;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.Manifest;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FileWriter;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

//MainActivity runs the app
public class MainActivity extends AppCompatActivity implements ActivityCompat.OnRequestPermissionsResultCallback{
    final String TAG = "MainActivity";
    private static int VOLUME = 8;
    private static int MICROPHONE_PERMISSION_CODE = 200;
    private final static int WIFI_PERMISSION_CODE = 300;
    private final static int ASK_MULTI_PERMISSIIONS_CODE = 400;
    private String[] PERMISSIONS;

    private Button buttonAttestation;
    private ImageView imageAttestation;
    private AttestationVerifier verifier;
    Context context;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        context = this;

        //Get Micro permission
        if(isMicrophonePresent()){
            //getMicrophonePermission();
            getPermissions();
        }

        //Create Objects
        verifier = new AttestationVerifier(context);

        //Set UI
        initializeUIandLoggerParameters();

        //Set Volume
        AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        audioManager.setStreamVolume(AudioManager.STREAM_MUSIC, VOLUME, 0);
    }

    //Set UI
    public void initializeUIandLoggerParameters(){
        //Set UI Elements
        buttonAttestation = (Button)findViewById(R.id.AttestationButton);
        imageAttestation = (ImageView)findViewById(R.id.AttestationImage);

        //Onclick-Listener
        buttonAttestation.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(buttonAttestation.getText().equals("Setup Attestation")){
                    buttonAttestation.setText("Next");
                    imageAttestation.setImageResource(R.drawable.setup);
                }
                else if(buttonAttestation.getText().equals("Next")){
                    buttonAttestation.setText("Start Attestation");
                    imageAttestation.setImageResource(R.drawable.accesspoint);
                }
                else if(buttonAttestation.getText().equals("Start Attestation")){
                    verifier.startAttestation(buttonAttestation, imageAttestation);
                }
                else if(buttonAttestation.getText().equals("Back to Start")){
                    finish();
                    startActivity(getIntent());
                }
            }
        });
    }


    private boolean isMicrophonePresent(){
        if(this.getPackageManager().hasSystemFeature(PackageManager.FEATURE_MICROPHONE)){
            return true;
        }else{
            return false;
        }
    }

    private void getPermissions(){
        PERMISSIONS = new String[]{
                Manifest.permission.RECORD_AUDIO, Manifest.permission.ACCESS_FINE_LOCATION
        };

        if(!hasPermission(context, PERMISSIONS)){
            ActivityCompat.requestPermissions(this, PERMISSIONS, ASK_MULTI_PERMISSIIONS_CODE);
        }
    }

    private boolean hasPermission(Context context, String[] PERMISSIONS){
        if(context != null && PERMISSIONS != null){
            for(String p: PERMISSIONS){
                if(ActivityCompat.checkSelfPermission(context, p) != PackageManager.PERMISSION_GRANTED){
                    return false;
                }
            }
        }
        return true;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if(requestCode == ASK_MULTI_PERMISSIIONS_CODE){
            if(grantResults[0] == PackageManager.PERMISSION_GRANTED){
                Toast.makeText(context, "Microphone Permission is granted", Toast.LENGTH_LONG).show();
            }
            else{
                Toast.makeText(context, "Microphone Permission is denied", Toast.LENGTH_LONG).show();
            }
            if(grantResults[1] == PackageManager.PERMISSION_GRANTED){
                Toast.makeText(context, "Location Permission is granted", Toast.LENGTH_LONG).show();
            }
            else{
                Toast.makeText(context, "Location Permission is denied", Toast.LENGTH_LONG).show();
            }
        }
    }

}