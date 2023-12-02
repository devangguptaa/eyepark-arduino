
#include <pgmspace.h>

#define SECRET
#define THINGNAME "THING NAME FROM AWS Iot Core"

const char WIFI_SSID[] = "WiFi SSID";
const char WIFI_PASSWORD[] = "WiFi PASSWORD";
const char AWS_IOT_ENDPOINT[] = "AWS IoT Core Endpoint";
const char *AWS_S3_BUCKET_NAME = "S3 BUCKET NAME";
const char *AWS_REGION = "REGION FOR S3 BUCKET";
const char *AWS_ACCESS_KEY_ID = "AWS USER ACCESS KEY ID";
const char *AWS_SECRET_ACCESS_KEY = "AWS USER SECRET ACCESS KEY";

const char *getPreSignedURL = "AWS LAMBDA FUNCTION URL";

// Amazon Root CA 1
static const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
-----
-----
-----
-----END CERTIFICATE-----
)EOF";

// Device Certificate
static const char AWS_CERT_CRT[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
-----
-----
-----
-----END CERTIFICATE-----
)KEY";

// Device Private Key
static const char AWS_CERT_PRIVATE[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----
-----
-----
-----
-----END RSA PRIVATE KEY-----
)KEY";
