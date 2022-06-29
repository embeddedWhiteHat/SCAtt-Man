# SCAtt-man Artifacts


This repository contains two different artifacts used in the paper:
* Android Application (verifier device for the attestation process)
* smartspeaker source code for the [ATOM Echo](https://shop.m5stack.com/products/atom-echo-smart-speaker-dev-kit?variant=34577853415588), an ESP32 prototype board, which we used for the proof-of-concept implementation.

Since we use an Android App in combination with an embedded device, we can't provide any docker or VM image.
The ScattMan Project needs to be evaluated on the specific prototype board (smartspeaker - ATOM ECHO) and any Android smartphone with the appropriate SDK level.
We currently target API 30: Android 11.0.

# Installation Instructions

## Android Application
The Android App implements the remote verifier for the attestation process. Its task is to receive a hash over sound and check its legitimacy.
We used the standard build process, recommended by Google to build this proof-of-concept implementation.

The first step is to download [Android Studio](https://developer.android.com/studio?hl=com).
A short overview on Android Studio can be found [here](https://code.tutsplus.com/tutorials/getting-started-with-android-studio--mobile-22958).
Second, the project (folder: 'AndroidApplication') can be imported as an Android Studio Project with ```File --> New --> Import Project```.
The Project is a 'Gradle' build.

After successfully importing the project, the application can be build and deployed to a hardware device.
The required steps for the build process are described [here](https://developer.android.com/studio/run)
and for the deployment [here](https://developer.android.com/studio/run/device).

## Atom Echo (Smartspeaker)

For the development of the smartspeaker firmware, we use the popular [platformio](https://platformio.org/) framework to simplify the
build and development process. The entire source code of the smartspeaker firmware is available in the folder 'Smartspeaker'.
Platformio is available as an Extension for Visual Studio Code. You can find the installation instructions [here](https://platformio.org/install/ide?install=vscode).

After Installation there will be a Platformio Symbol in the sidebar.
Click on the icon and navigate to ```platforms --> Embedded```, search for Espressif32 and press install.
This will install the ESP32 IDF Framework, we used for our prototyping. The Framework includes useful abstraction layers including a suitable 'FreeRTOS' implementation.
Now open the folder: 'Smartspeaker' on Visual Studio Code. Platformio should automatically recognize it as a Platformio project due to the provided 'platformio.ini' file. The process will take some time.

Once, the process is finished you can click on the platformio icon.
You will have now options called 'Project Tasks'

The next steps should be 'Build' and 'Upload and Monitor'.
Before the build process is started make sure, that you have adjusted the WIFI-settings in src/wifi_connection.h to match an available WIFI network in your area:

```C
#define WIFI_CLIENT_SSID "YourWifi"
#define WIFI_CLIENT_PASS "yourWiFiPW"
```

Furthermore, you need to create your own API-URL with a token for 'Baidu-Rest' to enable
the demo speech to text example. Therefore you need to adjust the URL in src/stt.c:

```C
    char postString[1000];
    sprintf(postString, "http://flow.m5stack.com:5003/pcm?cuid=1A57&token=deadbeaf%s", strbuff);
```

Visit https://flow.m5stack.com/ for API Key generation.

To enable the text-to-speech demo, you need to provide your own IBM Watson API Key in src/tts.c: 

```C
    esp_http_client_config_t config = {
        .host = "api.eu-de.text-to-speech.watson.cloud.ibm.com",
        .path = "/v1/synthesize",
        .event_handler = _http_event_handler_tts,
        .user_data = local_response_buffer,
        .disable_auto_redirect = false,
        .query = query_str,
        .auth_type = HTTP_AUTH_TYPE_BASIC,
        .username = "apikey",
        .password = "please.use.your.own.token",
    };
```

Visit https://cloud.ibm.com/apidocs/text-to-speech for instructions.

After all required source code modifications (i.e. providing WIFI passwords and API Tokens), you can press the Build button in 'Project Task' to build the project. This will generate an ELF file, which can be flashed using the 'Upload' or 'Upload and Monitor' task.
We recommend to use the upload and monitor option, to keep an eye on the debug log.













