# RoHA

## RoHA (Respiratory Health Analyzer)
RoHA Project is an IoT based Sensor Node that analyze a persons respiratory health and report its status on device and web application.

Full project documentation is available at https://www.hackster.io/timothy_malche/roha-respiratory-health-analyzer-7d430a

This project contains following modules:


## RoHA Arduino Firmware (C/C++)
This is the firmware source code that you need to compile and upload on sensor node using Arduino IDE. Before uploading this firmware please develop model using EdgeImpulse as the steps describe in this documentation and then download model as Arduino zip library, import the library and then compile this sketch. You also need to install ESP32 data upload tool and use the tool to upload image file to AWS IoT Edu Kit before you upload the code to the device. Apart from this setup IoT Core, IAM user and DynamoDB as described in this documentation. You also need to update settings in the secrets.h header file.


## RoHA Web Application (HTML, JavaScript, PHP)
To run this application, first install XAMPP server, and then copy paste this application folder to htdocs folder of web server. Before running update settings in getData.php file as described in this documentation. Then execute the application in web browser.


## RoHA Edge Impulse Model Zip Library
Use this model in your Arduino IDE. However you are adviced to create your own models as specified in the documentation. This model may not work perfectly as it contains less data and only one voice.


