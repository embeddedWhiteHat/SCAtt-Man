package com.university.sec.scatt_man;

import android.util.Log;

//AudioCalculator from https://github.com/lucns/Android-Audio-Sample
//One component for determining the frequencies of a tone
public class AudioCalculator {
    private byte[] bytes;
    private int[] amplitudes;
    private double[] decibels;
    private double frequency;
    private int amplitude;
    private int intData;
    private double decibel;
    private String TAG = "AudioCalculator";
    private String bitsData;

    public AudioCalculator() {
    }

    public void setBytes(byte[] bytes) {
        this.bytes = bytes;
        amplitudes = null;
        decibels = null;
        frequency = 0.0D;
        amplitude = 0;
        decibel = 0.0D;
        intData = 0;
        bitsData = "";
    }

    public String getBitsData() {
        if (bitsData.equals("")) bitsData = retrieveBitsData();
        return bitsData;
    }

    private String retrieveBitsData() {
        int length = bytes.length / 2;
        int sampleSize = 8192;
        while (sampleSize > length) {
            sampleSize = sampleSize >> 1;
        }
        //Log.d(TAG, "sampleSize: " + sampleSize);
        FrequencyCalculator frequencyCalculator = new FrequencyCalculator(sampleSize);
        frequencyCalculator.feedData(bytes, length);

        return frequencyCalculator.getBitsDataFromFreq();

    }

}

