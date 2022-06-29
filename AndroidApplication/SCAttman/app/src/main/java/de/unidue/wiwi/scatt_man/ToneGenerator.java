package com.university.sec.scatt_man;


import android.content.Context;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.List;


/*
The ToneGenerator generates and plays the tones according to our DoS protocol
 */
public class ToneGenerator {
    private static String TAG = "ToneGenerator";
    private static int sampleRate = 44100;
    private Context context;
    private Boolean isPlaying = false;
    private int playCounter = 0;
    private int sizeOfTonesToPlay, sizeOfMessagesToPlay;
    private long oldTime, newTime;
    private AttestationVerifier verifier;


    public ToneGenerator(Context context, AttestationVerifier verifier){
        this.context = context;
        this.verifier = verifier;
    }

    //generate List of Android AudioTracks based on List of string sequences
    public List<AudioTrack> generateAudioTracks(List<String> message){
        Log.d(TAG, "Generate Audio Tracks");
        List<AudioTrack> audioTracks = new ArrayList<AudioTrack>();
        AudioTrack track;

        for(String m : message){
            if(!m.equals(getEmptyFreqString())){
                track = generateToneFromBuffer(generateMessageBuffer(m));
            }
            else{
                track = generateTone(0, GlobalManager.blockLength);
            }
            audioTracks.add(track);
        }

        return audioTracks;
    }

    //generate an audio signal buffer based on a frequency
    //buffer generation is based on https://gist.github.com/slightfoot/6330866
    public double[] generateFrequencyBuffer(double freqHz){
        freqHz = freqHz/2;
        int count = (int)(44100.0 * 2.0 * (GlobalManager.blockLength / 1000.0)) & ~1;
        double[] samples = new double[count];
        for(int i = 0; i < count; i += 2){
            double sample = (double)(Math.sin(2 * Math.PI * i / (44100.0 / freqHz))); //*GlobalManager.peakValue
            samples[i + 0] = sample;
            samples[i + 1] = sample;
        }

        return samples;
    }

    //generate an audio signal buffer for multiple frequencies based on a bit string representing the specific frequencies
    public short[] generateMessageBuffer(String frequencies){
        List<double[]> freqBuffers = new ArrayList<double[]>();//List of all frequency buffers to play

        for(int i = 0; i<GlobalManager.numberOfFrequencies;i++){
            int f = Character.getNumericValue(frequencies.charAt(i));
            if(f==1){
                int frequency = GlobalManager.baseFrequency + i*GlobalManager.frequencySeparation;
                double[] freqBuffer = generateFrequencyBuffer(frequency);//Generate Buffer
                freqBuffers.add(freqBuffer);
            }
        }

        int numberOfBuffers = freqBuffers.size();
        int bufferLength = freqBuffers.get(0).length;
        double [] resultBuffer = new double[bufferLength];

        for(double[] buffer : freqBuffers){
            for(int i = 0; i<bufferLength; i++){
                resultBuffer[i] += buffer[i];
            }
        }

        for(int i = 0; i < bufferLength; i++){
            resultBuffer[i] /= numberOfBuffers;
        }

        short [] resultShortBuffer = new short[bufferLength];

        for(int i = 0; i<bufferLength;i++){
            resultShortBuffer[i] = (short)(resultBuffer[i]  * GlobalManager.peakValue);
        }

        return resultShortBuffer;
    }

    //generate an Android audio track from an audio signal buffer
    public AudioTrack generateToneFromBuffer(short[] buffer){
        int count = (int)(44100.0 * 2.0 * (GlobalManager.blockLength / 1000.0)) & ~1;
        AudioTrack track = new AudioTrack(AudioManager.STREAM_MUSIC, 44100,
                AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT,
                count * (Short.SIZE / 8), AudioTrack.MODE_STATIC);
        track.write(buffer, 0, count);
        //Log.d(TAG, "generate Track");
        return track;
    }

    //generate an Android audio track for a specific frequency and duration (only used for the "0000" tone)
    public AudioTrack generateTone(double freqHz, int durationMs){
        //Log.d(TAG, "freq: " + freqHz + " dur: " + durationMs);
        freqHz = freqHz/2;
        int count = (int)(44100.0 * 2.0 * (durationMs / 1000.0)) & ~1;
        Log.d(TAG, "Count: " + count);
        short[] samples = new short[count];
        for(int i = 0; i < count; i += 2){
            short sample = (short)(Math.sin(2 * Math.PI * i / (44100.0 / freqHz)) * 0x7FFF);
            samples[i + 0] = sample;
            samples[i + 1] = sample;
        }

        Log.d(TAG, "Samples: " + samples.length);


        AudioTrack track = new AudioTrack(AudioManager.STREAM_MUSIC, 44100,
                AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT,
                count * (Short.SIZE / 8), AudioTrack.MODE_STATIC);
        track.write(samples, 0, count);
        return track;
    }

    //plays a list of tones sequentially and start the receiving of the hash signals in the AttestationVerifier after the last played tone
    public void playTones(List<AudioTrack> tracks){
        sizeOfTonesToPlay = tracks.size();
        Log.d(TAG, "NumberofTones: " + sizeOfTonesToPlay);
        for(AudioTrack track : tracks){
            track.setNotificationMarkerPosition(track.getBufferSizeInFrames() - (int)(track.getBufferSizeInFrames()*0.035));
            track.setPlaybackPositionUpdateListener(new AudioTrack.OnPlaybackPositionUpdateListener() {
                @Override
                public void onMarkerReached(AudioTrack track) {
                    track.stop();
                    int currentCount = playCounter;
                    if(currentCount < sizeOfTonesToPlay-1){
                        tracks.get(currentCount + 1).play();
                        newTime = System.currentTimeMillis();
                        long timeDiff = newTime - oldTime;
                        Log.d(TAG, "Track " + (currentCount + 1) + " is playing after " + timeDiff  + "ms");
                        oldTime = newTime;
                        playCounter++;
                    }
                    else if(currentCount == sizeOfTonesToPlay-1){
                        verifier.receiveHash();
                        playCounter = 0;
                        Log.d(TAG, "End of sending at: " + System.currentTimeMillis());
                    }
                }
                @Override
                public void onPeriodicNotification(AudioTrack track) {
                }
            });
        }
        playCounter = 0;
        Log.d(TAG, "Start of sending at: " + System.currentTimeMillis());
        tracks.get(0).play();
        oldTime = System.currentTimeMillis();
        Log.d(TAG, "Track 0 is playing");
    }

    //get string for the "0" tone (no frequency is activated)
    public String getEmptyFreqString(){
        String empty = "";
        for(int i = 0; i<GlobalManager.numberOfFrequencies;i++){
            empty += "0";
        }
        return empty;
    }
}

