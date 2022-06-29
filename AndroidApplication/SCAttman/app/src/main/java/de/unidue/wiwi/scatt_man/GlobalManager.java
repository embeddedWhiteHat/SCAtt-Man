package com.university.sec.scatt_man;

import java.util.ArrayList;
import java.util.List;
//GlobalManager stores global parameter for the application
public class GlobalManager {
    public static int baseFrequency = 1010; //hz
    public static int numberOfFrequencies = 4;
    public static int frequencySeparation = 200; //hz
    public static int blockLength = 240; //ms
    public static int pauseDuration = 0; //ms
    public static int maxMessageSize = 8;//number of data blocks not bytes
    public static long peakValue = 0x7FFF;
    public static boolean isSilent = false;
    public static boolean isMeanPrinted = false;
    public static List<Double> maxSilentValues = new ArrayList<Double>();
    public static double silentMean = 0;
    public static String speakerSSID = "\"AtomEcho\"";

}