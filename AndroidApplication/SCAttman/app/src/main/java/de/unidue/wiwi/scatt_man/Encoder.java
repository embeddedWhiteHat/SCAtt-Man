package com.university.sec.scatt_man;

import android.nfc.tech.TagTechnology;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;
//Encoder encodes the nonce data (eight 4-bit Integers) to bit sequences
public class Encoder {
    private static String TAG = "Encoder";
    public Encoder(){
    }

    //encode data
    public List<String> encode(int[] data){
        List<String> message = new ArrayList<String>();
        message.add(getStartBits());//Start Bits of message

        //Go through every int
        for(int i = 0; i < data.length && i < GlobalManager.maxMessageSize; i++){
            int dataInt = data[i];
            String dataString = Integer.toBinaryString(dataInt);//parse int to binary String
            int stringLength = dataString.length();//get String Length
            if(stringLength <= GlobalManager.numberOfFrequencies){//Check if String Length is not too long for defined numbers of frequencies
                int missingChars = GlobalManager.numberOfFrequencies - stringLength;//get missing chars in String
                for(int j = missingChars; j>0; j--){
                    dataString = "0" + dataString;//fill missing chars with 0
                }
                message.add(dataString);//add data to message
            }else{
                Log.d(TAG, ""  + dataInt + " is too big for number of frequencies");
            }
        }
        return message;
    }

    //get the start block for our DoS protocol
    public String getStartBits(){
        String bits = "";
        for(int i = 0; i<GlobalManager.numberOfFrequencies;i++){
            if(i < GlobalManager.numberOfFrequencies/2) bits = bits + "0";
            else bits = bits + "1";
        }
        return bits;
    }
}
