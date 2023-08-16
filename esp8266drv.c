

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SERIAL_DEVICE "/dev/serial/by-id/usb-FTDI_TTL232R-3V3_FTFAZZHZ-if00-port0"

// Modes supported by the ESP8266
#define ESP_8266_MODE_CLIENT       1
#define ESP_8266_MODE_AP           2
#define ESP_8266_MODE_CLIENTPLUSAP 3

// Encryption modes for access point mode
#define ESP_8266_ENCRYPTION_OPEN         0
#define ESP_8266_ENCRYPTION_WEP          1
#define ESP_8266_ENCRYPTION_WPA_PSK      2
#define ESP_8266_ENCRYPTION_WPA2_PSK     3
#define ESP_8266_ENCRYPTION_WPA_WPA2_PSK 4


/*
 * sendATCommand
 *
 * Sends at_command to the esp8266 device. The AT command string should not be
 * terminated by a CR/LF. This function will add that. sendATCommand allocates
 * memory for the device's response which should be freed by the caller.
 *
 * Returns 0 on success, -1 on error.
 */
static int sendATCommand(char *at_command) {
    int err;
    int fd = open(SERIAL_DEVICE, O_RDWR);
    char response[200];
    unsigned char crlf[] = {0x0d, 0x0a};


    if(fd < 0) { // Check for error in open syscall
        char err_str[100];
        strerror_r(errno, err_str, 100); // If we got an error, convert errno to string and print.
        fprintf(stderr, "[sendATCommand] Error opening %s: %s\n", SERIAL_DEVICE, err_str);
        return -1;
    }


    err = write(fd, at_command, strlen(at_command));
    err = write(fd, crlf, 2); // Send a CRLF to indicate end of AT Command

    err = read(fd, response, sizeof(response));
    printf("%s", response);

    close(fd);

    return 0;
}

/*
 * esp8266Reset
 *
 * Resets the ESP8266 device.
 *
 */
int esp8266Reset() {
    char *response;
    int err = sendATCommand("AT+RST");
    int k, retval = -1;

    if(err < 0) { // Make sure sendATCommand didn't return error.
        return -1;
    }

    // Search the response string for "OK" to make sure the reset worked.
    if(strstr(response, "OK") == NULL) { // Couldn't find "OK" in response
        retval = -1;
    } else { // Found "OK" in response
        retval = 0;
    }

    if(response != NULL){
        free(response);
    }
    return retval;
}

/*
 * esp8266SetMode
 *
 * Sets the operating mode of the ESP8266 device. Permissable modes are:
 *    ESP_8266_MODE_CLIENT
 *    ESP_8266_MODE_AP
 *    ESP_8266_MODE_CLIENTPLUSAP
 *
 * Returns 0 on success, -1 on mode input error, -2 on AT command error.
 *
 */
int esp8266SetMode(unsigned int mode) {
    char *response; // Ptr to response
    char at_command[100]; // Buffer for AT Command
    int err, retval;

    if(mode < 1 || mode > 3){ // Sanity check mode
        return -1;// not gonna workout, error message.
    }

    snprintf(at_command, 100, "AT+CWMODE=%d", mode); // Construct the AT command
    sendATCommand(at_command);

    // Search the response string for "OK" to make sure the reset worked.
    if(strstr(response, "OK") == NULL) { // Couldn't find "OK" in response
        retval = -2;
    } else { // Found "OK" in response
        retval = 0;
    }

    if(response != NULL){
        free(response);
    }
    return retval;
}

/*
 * esp8266GetAvailableNetworks
 *
 * Returns a list of available networks.
     AT+CWLA command returns the list of all available network.
 *
 */
int esp8266GetAvailableNetworks() {
        char *response;
        int err = sendATCommand("AT+CWLA");
        int returnValue;

        if(strstr(response, "OK") == NULL) { // Still trying to figure out if OK matters in the list of available networks
        returnValue = -1; // didnt find OK in the response. 
        } else {
            returnValue = 0;
        }
        if (response != NULL){
        free(response);
        }
        return returnValue;        
}


/*
 * esp8266JoinNetwork
 *
 * Joins nwkName using key password.
 *
 */
int esp8266ConfigureAccessPoint(char *nwkName, char *password,
                                unsigned int channel, unsigned int encryption) {
    return 0;
}


/*
 * esp8266GetLocalIPAddress
 *
 * Uses AT+CWLIF to get local IP address 
 *
 */
int esp8266GetLocalIPAddress() { // first attempt, haven't tried working out the code yet.
    char *response;
    int err = sendATCommand("AT+CWLIF");
    int retVal = -1;

    if (err < 0) {
    return -1;
    }
    if (strstr(response, "OK") == NULL) {
    retVal= 0;
    }

    if ( response !=NULL){
    free(response);

   }
   return retVal;
 }


/*
 * esp8266JoinNetwork
 *
 * Joins nwkName using key password.
 *
 */
int esp8266JoinNetwork(char *nwkName, char *password) {
    return 0;
}

