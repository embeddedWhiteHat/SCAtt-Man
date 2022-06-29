package com.university.sec.scatt_man;


import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.Process;
import android.util.Log;

public class Recorder {
    private int audioSource = MediaRecorder.AudioSource.DEFAULT;
    private int channelConfig = AudioFormat.CHANNEL_IN_MONO;
    private int audioEncoding = AudioFormat.ENCODING_PCM_16BIT;
    private int sampleRate = 44100;
    private Thread thread;
    private Callback callback;
    private String TAG = "Recorder";
    private long systemTime;
    private long realSystemTime;

    //Recorder based on https://github.com/lucns/Android-Audio-Sample
    //records audio buffer and calls the callback function of the AttestationVerifier
    public Recorder(Callback callback) {
        this.callback = callback;
    }

    //Recording of Tones
    public void start() {
        if (thread != null) return;
        //Log.d("Attestation", "Recorder started");
        systemTime = System.currentTimeMillis();
        realSystemTime = systemTime;
        thread = new Thread(new Runnable() {
            @Override
            public void run() {
                Log.d("Attestation", "Recorder Thread started");
                Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
                int minBufferSize = AudioRecord.getMinBufferSize(sampleRate, channelConfig, audioEncoding);
                //Log.d("Attestation", "MinBufferSize: " + minBufferSize);
                AudioRecord recorder = new AudioRecord(audioSource, sampleRate, channelConfig, audioEncoding, minBufferSize);

                if (recorder.getState() == AudioRecord.STATE_UNINITIALIZED) {
                    Thread.currentThread().interrupt();
                    Log.d(TAG, "Uninitialized Thread State");
                    return;
                } else {
                    Log.i(Recorder.class.getSimpleName(), "Started.");
                }
                byte[] buffer = new byte[minBufferSize];
                //short[] buffer = new short[minBufferSize];

                recorder.startRecording();


                while(recorder.read(buffer, 0, minBufferSize) <= 0){
                    Log.d("Attestation", "Buffer too small");
                    recorder.stop();
                    //recorder.release();
                    recorder.startRecording();
                    Log.d("Attestation", "Start recorder again");
                }

                while (thread != null && !thread.isInterrupted() && recorder.read(buffer, 0, minBufferSize) > 0) {
                    Log.d(TAG, "Fill Buffer");
                    callback.onBufferAvailable(buffer);
                }

                recorder.stop();
                recorder.release();
                Log.d("Attestation", "Recorder is stopped");
            }
        }, Recorder.class.getName());
        thread.start();
    }

    public void stop() {
        if (thread != null) {
            thread.interrupt();
            thread = null;
        }
    }
}

