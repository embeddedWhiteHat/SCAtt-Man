# Android Application

## General Information
- Targeted SDK Version: 30 (API 30: Android 11.0 (R))
- Min SDK Version: 23
- Compile SDK Version: 31

## Run Attestation
The application guides the user through the attestation.
To perform an attestation, you have to insert your nonce and the respective hash values of your device in the AttestationData manually.
We achieved the best performance of our data over sound (DoS) protocol by placing the speaker and microphone of the smartphone (Redmi Note 10 in our case) 8cm away from the smart speaker.

## Components of the Application
- **MainActivity**: runs the application and vizualizes the user interface
- **AttestationVerifier**: runs and ranages the attestation
- **AttestationData**: stores the nonce and hash values (these must be entered by the user for the respective device)
- **Encoder**: encodes the nonce (32 bit represented by eighth 4-bit Integers) into bit sequences (eight 4-bit blocks)
- **ToneGenerator**: generates the respective tone for every bit sequence according to our DoS protocol
- Components for ReceivingData (based on the [Android-Audio-Sample](https://github.com/lucns/Android-Audio-Sample) project)
  - **Recorder**: record audio buffers
  - **AudioCalculator, FrequencyCalculator, RealDoubleFFT**: perform fast Fourier Transformation to convert tone into bit sequence
- **Decoder**: determines start tone and decodes bit sequences into hash (32 bit represented by eighth 4-bit Integers)
- **GlobalManager**: stores global parameters for the application

