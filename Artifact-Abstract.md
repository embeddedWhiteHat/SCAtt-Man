# Artifact Abstract for SCAtt-Man

The artifact submission consists of software used to implement the presented prototype of SCAtt-man. SCAtt-man consists of two main applications. An Android App, which takes the role of the verifier in the presented attestation process and a smartspeaker (prototyped on ESP32 ATOM-ECHO development board). The smartspeaker is the prover device in the attestation process. 

 We provide the source code (C/C++ files) of the smartspeaker firmware, which can be compiled using the Espressif32 - IDF (Integrated Development Framework). We use Platformio to handle the tool-chain and compilation process. Detailed information on the setup process can be found in the 'Readme.md' file, we provide with the source code. For the Android Application, we provide the Java Source code (gradle build project with Android Studio).

 Both artifacts (smartspeaker firmware and android app) represent the implementation described in Section 6. Both entities also include the implementation of our customized Data-over-Sound protocol, which we have based on SoniTalk. The implementation of the protocol is available in the ```sonitalk_receive.c, sonitalk_send.c and sonitalk.c`` files with their respective C headers for the smartspeaker. In the App it is split over multiple classes such as ToneGenerator, RealDoubleFTT, AudioCalculater etc. 

 For the evaluation, we repeated the Usage Process, described in Section 6.6 multiple times and optimized and modified our implementation and protocol settings to achieve reliable data transmission over sound.

 Since, we operate with physical properties i.e. sound we couldn't automate the tedious task of testing changes in the implementation and protocol. For each change in parameters or implementation, we recompiled and reflashed our devices (phones and development board). Afterwards we manually tested the outcome of the changes.

 We describe the details of our experiments in Section 7.
