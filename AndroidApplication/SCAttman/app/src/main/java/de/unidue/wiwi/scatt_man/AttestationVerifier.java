package com.university.sec.scatt_man;

import android.content.Context;
import android.media.AudioTrack;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.util.Log;
import android.widget.Button;
import android.widget.ImageView;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;

/*
AttestationVerifier controls and manages the attestation
 */
public class AttestationVerifier {
    private String TAG = "AttestationVerifier";
    private float recorderTime = 10000;
    private float attestationTimeThreshold = 6700;
    private Button buttonAttestation;
    private ImageView imageAttestation;
    private ToneGenerator toneGenerator;
    private Encoder encoder;
    private Decoder decoder;
    private Recorder recorder;
    private AudioCalculator audioCalculator;
    private WifiManager wifiManager;

    private long systemTime;
    private long receivingStartTime, receivingEndTime;
    boolean isSync = false;
    private List<String> receivedBits;
    int randomID;

    //create AttestationVerifier and choosing a random id for nonce selection
    public AttestationVerifier(Context context){
        toneGenerator = new ToneGenerator(context, this);
        encoder = new Encoder();
        recorder = new Recorder(callback);
        decoder = new Decoder(this);
        audioCalculator = new AudioCalculator();
        wifiManager = (WifiManager) context.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        receivedBits = new ArrayList<String>();
        Random random = new Random();
        randomID = random.nextInt(AttestationData.nonce.length);
        Log.d(TAG, "randomID: " + randomID);
    }

    //starts the attestation by checking WiFi connection and sending the nonce
    public void startAttestation(Button button, ImageView imageView){
        buttonAttestation = button;
        imageAttestation = imageView;
        if(checkWifiConnection()) {
            buttonAttestation.setText("Attestation is running");
            imageAttestation.setImageResource(R.drawable.wait);
            sendNonce();
        }else{
            cancelAttestation("connection");
            Log.d(TAG, "App cannot connect to Smart Speaker");
        }
    }

    //send nonce by encoding nonce, generating audio tracks, and playing them
    public void sendNonce(){
        Log.d(TAG, "Send Nonce");
        List<String> message = encoder.encode(AttestationData.nonce[randomID]);
        for(String m : message){
            Log.d(TAG, m);
        }
        List<AudioTrack> tracks = toneGenerator.generateAudioTracks(message);
        Log.d(TAG, "Start Sending Time: " + System.currentTimeMillis());
        toneGenerator.playTones(tracks);
    }

    //receive hash by starting the recorder
    public void receiveHash(){
        Log.d(TAG, "End Sending Time: " + System.currentTimeMillis());
        Log.d(TAG, "Data is send");
        decoder.clearBitsMessage();
        recorder.start();
        GlobalManager.isSilent = true;
        receivingStartTime = System.currentTimeMillis();
    }

    //process hash by checking attestation time and verifying the received hash
    public void processHash(List<Integer> hash){
        receivingEndTime = System.currentTimeMillis();
        long attestationTime = receivingEndTime - receivingStartTime;
        Log.d(TAG, "Attestation Duration: " + attestationTime);
        String receivedHash = "";
        for(int h : hash){
            receivedHash += h + "-";
        }
        Log.d(TAG, "Hash received: " + receivedHash);

        boolean correct = true;
        isSync = false;
        recorder.stop();

        if(attestationTime > attestationTimeThreshold){
            imageAttestation.setImageResource(R.drawable.fail_timeout);
            buttonAttestation.setText("Back to Start");
        }
        else {

            for (int i = 0; i < AttestationData.hash[randomID].length; i++) {
                if (hash.get(i) != AttestationData.hash[randomID][i]) {
                    correct = false;
                }
            }

            if (correct) {
                Log.d(TAG, "Attestation success");
                imageAttestation.setImageResource(R.drawable.success);
            } else {
                imageAttestation.setImageResource(R.drawable.fail_hash);
            }
            //buttonAttestation.setEnabled(true);
            buttonAttestation.setText("Back to Start");
        }
    }

    //cancel attestation due to wrong hash, timeout, or WiFi connection lost
    public void cancelAttestation(String error){
        isSync = false;
        recorder.stop();
        switch(error){
            case "hash":
                imageAttestation.setImageResource(R.drawable.fail_hash);
                break;
            case "time":
                imageAttestation.setImageResource(R.drawable.fail_timeout);
                break;
            case "connection":
                imageAttestation.setImageResource(R.drawable.fail_wifi);
                break;
        }
        buttonAttestation.setText("Back to Start");
    }

    //processing the recorded buffers by synchronizing the receiving and decoding the audio signals
    //based on https://github.com/lucns/Android-Audio-Sample
    private Callback callback = new Callback() {
        @Override
        public void onBufferAvailable(byte[] buffer) {//buffer is audio input
            //Log.d("onBuffer", "Receive Buffer");
            if(System.currentTimeMillis() - receivingStartTime >= recorderTime){
                cancelAttestation("time");
                Log.d(TAG, "Attestation ran out of time");
            }
            else if(!checkWifiConnection()){
                cancelAttestation("connection");
                Log.d(TAG, "Lost connection to Smart Speaker");
            }
            else {

                //Check if Listener is already synchronized to Sender
                if (!isSync) {
                    short maxVolume = Short.MIN_VALUE;
                    short[] shortBuffer = getShortFromBytes(buffer);
                    for (short b : shortBuffer) {
                        if (b > maxVolume) {
                            maxVolume = b;
                        }
                    }

                    //Log.d(TAG, "Max Volume in Buffer: " + maxVolume);
                    if (maxVolume > 800) {
                        systemTime = System.currentTimeMillis();
                        isSync = true;
                        GlobalManager.isSilent = false;
                        Log.d(TAG, "-------------------------isSync True----------------------");
                        Log.d(TAG, "MaxVolume: " + maxVolume);
                    }
                }
                audioCalculator.setBytes(buffer);
                String bits = audioCalculator.getBitsData();

                //Check if Listener and sender are already synchronized
                if (isSync) {
                    receivedBits.add(bits);//get received Data as bit String

                    final long currentTime = System.currentTimeMillis();
                    final long timeDifference = currentTime - systemTime;

                    //Check if blockLength has passed since last Tone
                    if (timeDifference >= GlobalManager.blockLength + GlobalManager.pauseDuration) {

                        if(receivedBits.size() >= 3) {
                            receivedBits.remove(0);
                            receivedBits.remove(receivedBits.size() - 1);
                        }

                        String stringData = mostCommon(receivedBits);//get mos received data
                        Log.d(TAG, "Received StringBits: " + stringData);

                        decoder.receiveBitString(stringData);//decode data
                        receivedBits.clear();//clear list of received data

                        //Set new system time
                        if (timeDifference > GlobalManager.blockLength) {
                            long timeDelta = timeDifference - GlobalManager.blockLength;
                            systemTime = currentTime - timeDelta;
                        } else {
                            systemTime = currentTime;
                        }

                        //Check Decoder State --> if decoder is waiting, then listener and sender are not synchronized
                        if (decoder.getDecoderState() == DecoderState.WAITING) {
                            isSync = false;
                            Log.d(TAG, "-------------------------isSync False----------------------");
                        }
                    }
                }
            }
        }
    };

    //check WiFi connection to smart spekaer
    private boolean checkWifiConnection(){
        WifiInfo info = wifiManager.getConnectionInfo();
        String ssid  = info.getSSID();
//        return true;
        if(ssid.equals(GlobalManager.speakerSSID)){
            return true;
        }else{
            return false;
        }
    }

    //convert byte buffer to short buffer
    private short[] getShortFromBytes(byte[] buffer) {
        short[] shortBuffer = new short[buffer.length/2];
        for(int i = 0; i < buffer.length/2; i++){
            short buff = buffer[i*2 + 1];
            short buff2 = buffer[i*2];

            buff = (short) ((buff & 0xFF) << 8);
            buff2 = (short) (buff2 & 0xFF);

            shortBuffer[i] = (short) (buff | buff2);
        }

        return shortBuffer;
    }

    //get most common string (audio block) in list of strings
    public static String mostCommon(List<String> list) {
        Map<String, Integer> map = new HashMap<>();

        for (String t : list) {
            Integer val = map.get(t);
            map.put(t, val == null ? 1 : val + 1);
        }

        Map.Entry<String, Integer> max = null;

        for (Map.Entry<String, Integer> e : map.entrySet()) {
            if (max == null || e.getValue() > max.getValue())
                max = e;
        }

        if(max == null){
            Log.d("MostCommon", "max is null");
        }

        return max.getKey();
    }
}
