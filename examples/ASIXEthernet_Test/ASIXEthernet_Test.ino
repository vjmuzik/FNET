//#define STATS  //Print how many times each thread loops per second

#include <USBHost_t36.h> //USB Host Driver
#include <ASIXEthernet.h> //USB Ethernet Driver
#include <TeensyThreads.h>  //Multi-thread

#include <fnet.h> //TCP/IP stack
#include <port/fnet_usb.h>  //USB settings

USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
ASIXEthernet asix1(myusb);

uint8_t rbuf[2500]; // recieve buffer
uint8_t sbuf[2500]; // send buffer
uint16_t sbuflen; // length of data to send
uint8_t MacAddress[6] = {0x00,0x50,0xB6,0xBE,0x8B,0xB4}; //Not setup yet, but can't be 0

#ifdef STATS
elapsedMillis advertise;
uint32_t Looped;
volatile uint32_t LoopedUSB;
#endif

static fnet_time_t timer_get_ms(void){ //Used for multi-thread version
    fnet_time_t result;
    result =  millis();
    return result;
}
fnet_dhcp_cln_params_t dhcp_params; //DHCP intialization parameters
fnet_dhcp_cln_desc_t dhcp_desc; //DHCP object
fnet_netif_desc_t current_netif; //Network interface, USB Ethernet

void setup() {
  Serial.begin(115200);
  // Wait 1.5 seconds before turning on USB Host.  If connected USB devices
  // use too much power, Teensy at least completes USB enumeration, which
  // makes isolating the power issue easier.
  delay(1500);
  Serial.println("USB Host InputFunctions example");
  delay(10);
  myusb.begin();
  asix1.setHandleRecieve(handleRecieve);
  threads.addThread(usbthread);
  while(!asix1.initialized) {
  }
  Serial.println("USB Ready");
  //All of these have to be set to function correctly
  setHandleOutput(handleOutput);
  setHandleSetMACAddress(handleSetMACAddress);
  setHandleGetMACAddress(handleGetMACAddress);
  setHandlePHYRead(handlePHYRead);
  setHandlePHYWrite(handlePHYWrite);
  setHandleMulticastLeave(handleMulticastLeave);
  setHandleMulticastJoin(handleMulticastJoin);
  static fnet_uint8_t         stack_heap[(30U * 1024U)];
  struct fnet_init_params     init_params;
  
  static const fnet_timer_api_t timer_api = { //Setup multi-thread timer
    .timer_get_ms = timer_get_ms,
    .timer_delay = 0,
  };
  /* Input parameters for FNET stack initialization */
  init_params.netheap_ptr = stack_heap;
  init_params.netheap_size = sizeof(stack_heap);
  init_params.timer_api = &timer_api;
  /* FNET Initialization */
  if (fnet_init(&init_params) != FNET_ERR) {
      Serial.println("TCP/IP stack initialization is done.\n");
      /* You may use FNET stack API */
      /* Initialize networking interfaces using fnet_netif_init(). */
//        Get current net interface.
      if(fnet_netif_init(FNET_CPU_ETH0_IF, MacAddress, 6) != FNET_ERR){
        Serial.println("netif Initialized");
        if((current_netif = fnet_netif_get_default()) == 0){
          Serial.println("ERROR: Network Interface is not configurated!");
        }
        else {
          fnet_memset_zero(&dhcp_params, sizeof(dhcp_params));
          dhcp_params.netif = current_netif;
          // Enable DHCP client.
          if((dhcp_desc = fnet_dhcp_cln_init(&dhcp_params))){
              /*Register DHCP event handler callbacks.*/
             fnet_dhcp_cln_set_callback_updated(dhcp_desc, dhcp_cln_callback_updated, (void*)dhcp_desc);
             fnet_dhcp_cln_set_callback_discover(dhcp_desc, dhcp_cln_callback_updated, (void*)dhcp_desc);
             Serial.println("DHCP initialization done!");
          }
          else{
            Serial.println("ERROR: DHCP initialization failed!");
          }
        }
      }
      else {
        Serial.println("Error:netif initialization failed.\n");
      }
  }
  else {
      Serial.println("Error:TCP/IP stack initialization failed.\n");
  }
}

void loop() {
#ifdef STATS
  Looped++;
  if(advertise >= 1000) {
    advertise -= 1000;
    Serial.print("Looped: ");
    Serial.println(Looped);
    Serial.print("LoopedUSB: ");
    Serial.println(LoopedUSB);
    Looped = 0;
    LoopedUSB = 0;
  }
#endif
}
