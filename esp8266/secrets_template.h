#include <pgmspace.h>

#define SECRET

const char ssid[] = "WiFi SSID";
const char pass[] = "WiFI PASSWORD";

#define THINGNAME "THING NAME from AWS IoT Core"

int8_t TIME_ZONE = 8;

const char MQTT_HOST[] = "AWS IoT Core Endpoint";

// Obtain First CA certificate for Amazon AWS
// https://docs.aws.amazon.com/iot/latest/developerguide/managing-device-certs.html#server-authentication
// Copy contents from CA certificate here ▼
static const char cacert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
-----
-----
-----
-----END CERTIFICATE-----
)EOF";

// Copy contents from XXXXXXXX-certificate.pem.crt here ▼
static const char client_cert[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
-----
-----
-----
-----END CERTIFICATE-----
)KEY";

// Copy contents from  XXXXXXXX-private.pem.key here ▼
static const char privkey[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----
-----
-----
-----
-----END RSA PRIVATE KEY-----
)KEY";