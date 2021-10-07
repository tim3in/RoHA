#define EIDSP_QUANTIZE_FILTERBANK   0

#include <RoHA_Respiratory_Health_Analyzer__inferencing.h>

#include <M5Core2.h>
#include <driver/i2s.h>
#include <String.h>

#define CONFIG_I2S_BCK_PIN 12
#define CONFIG_I2S_LRCK_PIN 0
#define CONFIG_I2S_DATA_PIN 2
#define CONFIG_I2S_DATA_IN_PIN 34

#define Speak_I2S_NUMBER I2S_NUM_0
#define MODE_MIC 0
#define MODE_SPK 1
#define DATA_SIZE 1024

#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

//uint8_t microphonedata0[1024 * 50];
char payload[100];
int data_offset = 0;

/** Audio buffers, pointers and selectors */

typedef struct {
    int16_t *buffer;
    uint8_t buf_ready;
    uint32_t buf_count;
    uint32_t n_samples;

} inference_t;



static inference_t inference;
static signed short sampleBuffer[2048];
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal

/* *************** Code for AWS IoT ************************** */

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC   "device/33/data"
#define AWS_IOT_SUBSCRIBE_TOPIC "device/33/data"

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.print("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}

void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["status"] = payload;
  
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void messageHandler(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

}
/* *************** Code for AWS IoT ************************** */

bool InitI2SSpeakOrMic(int mode){

    esp_err_t err = ESP_OK;
    i2s_driver_uninstall(Speak_I2S_NUMBER);

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // is fixed at 12bit, stereo, MSB
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 2,
        .dma_buf_len = 128,
    };

    if (mode == MODE_MIC){
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
    }
    else{
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
        i2s_config.use_apll = false;
        i2s_config.tx_desc_auto_clear = true;
    }

    err += i2s_driver_install(Speak_I2S_NUMBER, &i2s_config, 0, NULL);
    i2s_pin_config_t tx_pin_config;
    tx_pin_config.bck_io_num = CONFIG_I2S_BCK_PIN;
    tx_pin_config.ws_io_num = CONFIG_I2S_LRCK_PIN;
    tx_pin_config.data_out_num = CONFIG_I2S_DATA_PIN;
    tx_pin_config.data_in_num = CONFIG_I2S_DATA_IN_PIN;
    err += i2s_set_pin(Speak_I2S_NUMBER, &tx_pin_config);
    err += i2s_set_clk(Speak_I2S_NUMBER, 44100, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
    return true;
}



void DisplayInit(void){
  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setTextColor(BLACK);
  M5.Lcd.setTextSize(2);
 
}

void display_title(){
  M5.Lcd.setTextColor(RED);
  M5.Lcd.setCursor(5, 10);
  M5.Lcd.printf("Reinventing Healthy Spaces");
  M5.Lcd.setTextColor(BLACK);
  M5.Lcd.setCursor(10, 36);
  M5.Lcd.printf("RoHA Firmware");
  M5.Lcd.setCursor(10, 186);
  M5.Lcd.printf("Copyright: ");
  M5.Lcd.setTextColor(BLUE);
  M5.Lcd.setCursor(140, 186);
  M5.Lcd.printf("hackster.io");
  
}

void display_pir(){
   if(digitalRead(36)==1){
    M5.Lcd.setTextColor(WHITE,RED);
    M5.Lcd.setCursor(10, 56);
    /* Enable haptic feedback */
    M5.Axp.SetLDOEnable(3,true);
    delay(100);
    /* Disable haptic feedback */
    M5.Axp.SetLDOEnable(3,false); 
    //Display Alert on LCD
    M5.Lcd.print("Alert! Keep Distance.");
    //Debuging message on serial terminal
    Serial.println("PIR Status: Sensing");
    Serial.println(" value: 1");
  }
  else{
    M5.Lcd.setTextColor(BLACK,WHITE);
    M5.Lcd.setCursor(10, 56);
    M5.Lcd.print("You are doing good.  ");
    //Debuging message on serial terminal
    Serial.println("PIR Status: Not Sensed");
    Serial.println(" value: 0");
  }
  delay(500);
}

void setup() {
    M5.begin(true, true, true, true);
    pinMode(36, INPUT);
    if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  M5.Lcd.drawPngFile(SPIFFS, "/RoHA_Poster.png", 0, 0);
  delay(5000);
  M5.update();
  
  M5.Axp.SetSpkEnable(true);

   DisplayInit();

   
    Serial.println("Edge Impulse Inferencing Demo");
    InitI2SSpeakOrMic(MODE_MIC); //  and enabling MIC mode
   
    // summary of inferencing settings (from model_metadata.h)
    ei_printf("Inferencing settings:\n");
    ei_printf("\tInterval: %.2f ms.\n", (float)EI_CLASSIFIER_INTERVAL_MS);
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
    ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));

    if (microphone_inference_start(EI_CLASSIFIER_RAW_SAMPLE_COUNT) == false) {
        ei_printf("ERR: Failed to setup audio sampling\r\n");

        return;
    }
}


void loop() {
  
  M5.update();
   display_title();
   display_pir();
   TouchPoint_t pos= M5.Touch.getPressPoint();
   if(pos.y > 240)
    if(pos.x < 109)
    {
      if(M5.Touch.ispressed() == true){
         record_and_inference();
         delay(2000);
         send_data();
         delay(2000);
      }
    }

}


void record_and_inference(){

  M5.Lcd.clear(WHITE);
  ei_printf("Starting inferencing in 2 seconds...\n");

    delay(2000);

    ei_printf("Recording...\n");
    M5.Lcd.setCursor(10, 46);
    M5.Lcd.printf("Get ready to record sound.");
    delay(1000);
    M5.Lcd.setCursor(10, 66);
    M5.Lcd.printf("Record now...");
    data_offset = 0;
    
    bool m = microphone_inference_record();
    if (!m) {
        ei_printf("ERR: Failed to record audio...\n");
        return;
    }

    
    ei_printf("Recording done\n");
    M5.Lcd.setCursor(10, 86);
    M5.Lcd.printf("Recording Done!");

    signal_t signal;

    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data = &microphone_audio_signal_get_data;
    ei_impulse_result_t result = { 0 };

    EI_IMPULSE_ERROR r = run_classifier(&signal, &result, debug_nn);

    if (r != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run classifier (%d)\n", r);
        return;
    }

    // print the predictions

    ei_printf("Predictions ");
    ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
    result.timing.dsp, result.timing.classification, result.timing.anomaly);
    ei_printf(": \n");
    M5.Lcd.setCursor(10, 106);
    M5.Lcd.printf("Analyzing...");
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        ei_printf("    %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);
         //M5.Lcd.clearDisplay();
         //Printing inferencing result on M5Stack LCD
         M5.Lcd.setTextColor(WHITE, BLUE);
         M5.Lcd.setCursor(10, 126);
         M5.Lcd.printf("%s: %.5f\n", result.classification[0].label, result.classification[0].value);
         M5.Lcd.setTextColor(WHITE, RED);
         M5.Lcd.setCursor(10, 146);
         M5.Lcd.printf("%s: %.5f\n", result.classification[1].label, result.classification[1].value);

         //checking which label has closest value
         if(result.classification[ix].value > 0.50000){
            //storing the label text in payload
            strcpy(payload, result.classification[ix].label);
         }
         delay(1000);
         
    }

#if EI_CLASSIFIER_HAS_ANOMALY == 1
    ei_printf("anomaly score: %.3f\n", result.anomaly);
    //M5.Lcd.setCursor(10, 116);
    //M5.Lcd.printf("anomaly score: %.3f\n", result.anomaly);
#endif
}

void send_data(){
  
  M5.Lcd.setTextColor(BLACK);
  M5.Lcd.setCursor(10, 166);
  M5.Lcd.printf("Sending to AWS IoT...");
  connectAWS();
  publishMessage();
  client.loop();
  delay(1000);
  M5.Lcd.setCursor(10, 186);
  M5.Lcd.printf("Sent!");
  delay(3000);
  M5.Lcd.clear(WHITE);
}

static bool microphone_inference_start(uint32_t n_samples)
{
    inference.buffer = (int16_t *)malloc(n_samples * sizeof(int16_t));

    if(inference.buffer == NULL) {
        return false;
    }

    inference.buf_count  = 0;
    inference.n_samples  = n_samples;
    inference.buf_ready  = 0;

    return true;
}


static void pdm_data_ready_inference_callback(void)
{

    // read into the sample buffer

    size_t bytesRead;// = startRec();

    i2s_read(Speak_I2S_NUMBER, (char *)(sampleBuffer), DATA_SIZE, &bytesRead, (100 / portTICK_RATE_MS));

    if (inference.buf_ready == 0) {
        for(int i = 0; i < bytesRead>>1; i++) {
            inference.buffer[inference.buf_count++] = sampleBuffer[i];

            if(inference.buf_count >= inference.n_samples) {
                inference.buf_count = 0;
                inference.buf_ready = 1;
                break;
            }
        }
    }
}


static void microphone_inference_end(void)
{
    free(inference.buffer);
}



static bool microphone_inference_record(void)
{
    inference.buf_ready = 0;
    inference.buf_count = 0;

    while(inference.buf_ready == 0) {        
         pdm_data_ready_inference_callback();
    }
    inference.buf_ready = 0;
    return true;
}


static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr)
{
    numpy::int16_to_float(&inference.buffer[offset], out_ptr, length);
    return 0;

}



#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE

#error "Invalid model for current sensor."

#endif
