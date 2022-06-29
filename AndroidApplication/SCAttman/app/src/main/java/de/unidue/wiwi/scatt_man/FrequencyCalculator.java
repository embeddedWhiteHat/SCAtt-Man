package com.university.sec.scatt_man;


import android.util.Log;

import java.util.Arrays;

//FrequencyCalculator from https://github.com/lucns/Android-Audio-Sample
//One component for determining the frequencies of a tone
public class FrequencyCalculator {
    private String TAG = "FrequencyCalculator";
    private RealDoubleFFT spectrumAmpFFT;
    private double[] spectrumAmpOutCum;
    private double[] spectrumAmpOutTmp;
    private double[] spectrumAmpOut;
    private double[] spectrumAmpOutDB;
    private double[] spectrumAmpIn;
    private double[] spectrumAmpInTmp;
    private double[] wnd;
    private double[][] spectrumAmpOutArray;

    private int fftLen;
    private int spectrumAmpPt;
    private int spectrumAmpOutArrayPt = 0;
    private int nAnalysed = 0;

    private void init(int fftlen) {
        fftLen = fftlen;
        spectrumAmpOutCum = new double[fftlen];
        spectrumAmpOutTmp = new double[fftlen];
        spectrumAmpOut = new double[fftlen];
        spectrumAmpOutDB = new double[fftlen];
        spectrumAmpIn = new double[fftlen];
        spectrumAmpInTmp = new double[fftlen];
        spectrumAmpFFT = new RealDoubleFFT(fftlen);
        spectrumAmpOutArray = new double[(int) Math.ceil((double) 1 / fftlen)][];

        for (int i = 0; i < spectrumAmpOutArray.length; i++) {
            spectrumAmpOutArray[i] = new double[fftlen];
        }
        wnd = new double[fftlen];
        for (int i = 0; i < wnd.length; i++) {
            wnd[i] = Math.asin(Math.sin(Math.PI * i / wnd.length)) / Math.PI * 2;
        }
    }

    public FrequencyCalculator(int fftlen) {
        init(fftlen);
    }

    private short getShortFromBytes(int index) {
        index *= 2;
        short buff = bytes[index + 1];
        short buff2 = bytes[index];

        buff = (short) ((buff & 0xFF) << 8);
        buff2 = (short) (buff2 & 0xFF);

        return (short) (buff | buff2);
    }

    private byte[] bytes;

    public void feedData(byte[] ds, int dsLen) {
        bytes = ds;
        int dsPt = 0;
        while (dsPt < dsLen) {
            while (spectrumAmpPt < fftLen && dsPt < dsLen) {
                double s = getShortFromBytes(dsPt++) / 32768.0;
                spectrumAmpIn[spectrumAmpPt++] = s;
            }
            if (spectrumAmpPt == fftLen) {
                for (int i = 0; i < fftLen; i++) {
                    spectrumAmpInTmp[i] = spectrumAmpIn[i] * wnd[i];
                }
                spectrumAmpFFT.ft(spectrumAmpInTmp);
                fftToAmp(spectrumAmpOutTmp, spectrumAmpInTmp);
                System.arraycopy(spectrumAmpOutTmp, 0, spectrumAmpOutArray[spectrumAmpOutArrayPt], 0, spectrumAmpOutTmp.length);
                spectrumAmpOutArrayPt = (spectrumAmpOutArrayPt + 1) % spectrumAmpOutArray.length;
                for (int i = 0; i < fftLen; i++) {
                    spectrumAmpOutCum[i] += spectrumAmpOutTmp[i];
                }
                nAnalysed++;
                int n2 = spectrumAmpIn.length / 2;
                System.arraycopy(spectrumAmpIn, n2, spectrumAmpIn, 0, n2);
                spectrumAmpPt = n2;
            }
        }
    }

    private void fftToAmp(double[] dataOut, double[] data) {
        double scaler = 2.0 * 2.0 / (data.length * data.length);
        dataOut[0] = data[0] * data[0] * scaler / 4.0;
        int j = 1;
        for (int i = 1; i < data.length - 1; i += 2, j++) {
            dataOut[j] = (data[i] * data[i] + data[i + 1] * data[i + 1]) * scaler;
        }
        dataOut[j] = data[data.length - 1] * data[data.length - 1] * scaler / 4.0;
    }

    public String getBitsDataFromFreq(){
        if (nAnalysed != 0) {
            int outLen = spectrumAmpOut.length;
            double[] sAOC = spectrumAmpOutCum;
            for (int j = 0; j < outLen; j++) {
                sAOC[j] /= nAnalysed;
            }
            System.arraycopy(sAOC, 0, spectrumAmpOut, 0, outLen);
            Arrays.fill(sAOC, 0.0);
            nAnalysed = 0;
            for (int i = 0; i < outLen; i++) {
                spectrumAmpOutDB[i] = 10.0 * Math.log10(spectrumAmpOut[i]);
            }
        }
        Log.d(TAG, "--------------------START---------------------");
        double maxDB = 20 * Math.log10(0.125 / 32768);
        double minDB = 0;
        for(double db : spectrumAmpOutDB){
            if(db > maxDB){
                maxDB = db;
            }
        }
        Log.d(TAG, "Max db: " + maxDB);


        if(GlobalManager.isSilent){
            GlobalManager.maxSilentValues.add(maxDB);
            //Log.d(TAG, "Value to Array: " + maxDB);
        }else{
            if(!GlobalManager.isMeanPrinted){
                double sum = 0;
                Log.d(TAG, "maxSilentValuesSize: " + GlobalManager.maxSilentValues.size());
                for(double value : GlobalManager.maxSilentValues){
                    sum += value;
                }
                GlobalManager.silentMean = sum/GlobalManager.maxSilentValues.size();
                Log.d(TAG, "Mean Of Silence: " + GlobalManager.silentMean);
                GlobalManager.isMeanPrinted = true;
            }
        }


        //double thresholdDB = maxDB * 0.5 - 42;
        double thresholdDB = GlobalManager.silentMean + (maxDB - GlobalManager.silentMean)/2;
        Log.d(TAG, "SilentMean:  " + GlobalManager.silentMean);
        Log.d(TAG, "Threshold: " + thresholdDB);
        int sampleRate = 44100;
        int frequencies[] = new int[GlobalManager.numberOfFrequencies];
        String bitString = "";

        for(int i = 0; i < GlobalManager.numberOfFrequencies; i++){
            frequencies[i] = GlobalManager.baseFrequency + i * GlobalManager.frequencySeparation;
            frequencies[i] = frequencies[i] * fftLen / sampleRate;

            Log.d(TAG, "db at freq " + frequencies[i] + ": " + spectrumAmpOutDB[frequencies[i]]);

            if(spectrumAmpOutDB[frequencies[i]] < minDB){
                minDB = spectrumAmpOutDB[frequencies[i]];
            }

            if(spectrumAmpOutDB[frequencies[i]] >= thresholdDB){
                bitString += "1";
            }else{
                bitString += "0";
            }
            if(maxDB < -50){
                bitString = "0000";
            }
        }

        //Log.d(TAG, "Min db: " + minDB);
        Log.d(TAG, "bits received: " + bitString);
        Log.d(TAG, "--------------------END---------------------");
        return bitString;
    }
}

