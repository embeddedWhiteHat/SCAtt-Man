package com.university.sec.scatt_man;

import android.util.Log;

import java.io.File;
import java.io.FileWriter;
import java.util.ArrayList;
import java.util.List;

//Decoder determines start block of data transmission and decodes bit-string representation of frequencies into eight 4-bit Integers representing the hash
public class Decoder {
    private DecoderState decoderState;
    private String message;
    private String startBits;
    private String TAG = "Decoder";
    private int numberOfReceivedData;
    private String bitsMessage;
    private List<Integer> hash;
    private AttestationVerifier verifiere;

    public Decoder(AttestationVerifier verifier){
        decoderState = DecoderState.WAITING;
        message = "";
        startBits = getStartBits();
        numberOfReceivedData = 0;
        bitsMessage = "";
        hash = new ArrayList<Integer>();
        verifiere = verifier;
    }

    //processing bit-string by determining start block or decoding hash data
    //calls processingHash function of the AttestationVerifier in the end
    public void receiveBitString(String bits){
        switch(decoderState) {
            case WAITING://Decoder is waiting for start bits
                if (isEqualBits(bits, startBits)) {
                    decoderState = DecoderState.RECEIVING;
                    numberOfReceivedData = 0;

                    //Logging
                    message = message + "Msg: ";
                    bitsMessage += "s_bits;" + bits + "\n";
                    Log.d(TAG, "Start bits received: " + bits);
                }
                break;
            case RECEIVING://Decoder is waiting for data bits
                numberOfReceivedData++;

                //Logging
                int intData = Integer.parseInt(bits, 2);
                hash.add(intData);
                message += "-" + intData;
                bitsMessage += "d_bits;" + bits + "\n";
                Log.d(TAG, "Data Received: " + bits);

                //Check if message is complete
                if (numberOfReceivedData >= GlobalManager.maxMessageSize) {
                    numberOfReceivedData = 0;
                    decoderState = DecoderState.WAITING;
                    message += "\n";
                    Log.d("Attestation", "Received message: " + message);
                    verifiere.processHash(hash);
                }
                break;
        }
    }

    public DecoderState getDecoderState(){
        return decoderState;
    }

    public String getStartBits(){
        String bits = "";
        for(int i = 0; i<GlobalManager.numberOfFrequencies;i++){
            if(i < GlobalManager.numberOfFrequencies/2) bits = bits + "0";
            else bits = bits + "1";
        }
        return bits;
    }

    public boolean isEqualBits(String bits1, String bits2){
        return bits1.equals(bits2);
    }

    public String getMessage(){
        return message;
    }

    public void clearMessage(){
        message = "";
    }

    public String getBitsMessage(){return bitsMessage;}

    public void clearBitsMessage(){bitsMessage = "label;bits\n";}

}

