#define HACKED //Limits number of times usbthread loops per second

void usbthread() {
  uint32_t cc = 0;
  elapsedMillis checkTimer;
  while(1) {
    myusb.Task();
    asix1.read();
    if(fnet_netif_is_initialized(current_netif)){
      fnet_poll();
      fnet_service_poll();
      if(checkTimer >= 1000) {
        checkTimer -= 1000;
        checkLink();
      }
    }
    else{
      checkTimer = 0;
    }
    
    #ifdef STATS
    LoopedUSB++;
    #endif
    #ifdef HACKED
    cc++;
    if ( cc > 40 ) {
      cc=0;
      threads.yield();
    }
    #endif
  }
}

void dhcp_cln_callback_updated(fnet_dhcp_cln_desc_t _dhcp_desc, fnet_netif_desc_t netif, void *p ) { //Called when DHCP updates
  struct fnet_dhcp_cln_options current_options;
  fnet_dhcp_cln_get_options(dhcp_desc, &current_options);
  
  uint8_t *ip = (uint8_t*)&current_options.ip_address.s_addr;
  Serial.print("IPAddress: ");
  Serial.print((uint8_t)*ip++);
  Serial.print(".");
  Serial.print((uint8_t)*ip++);
  Serial.print(".");
  Serial.print((uint8_t)*ip++);
  Serial.print(".");
  Serial.println((uint8_t)*ip);

  ip = (uint8_t*)&current_options.netmask.s_addr;
  Serial.print("SubnetMask: ");
  Serial.print((uint8_t)*ip++);
  Serial.print(".");
  Serial.print((uint8_t)*ip++);
  Serial.print(".");
  Serial.print((uint8_t)*ip++);
  Serial.print(".");
  Serial.println((uint8_t)*ip);

  ip = (uint8_t*)&current_options.gateway.s_addr;
  Serial.print("Gateway: ");
  Serial.print((uint8_t)*ip++);
  Serial.print(".");
  Serial.print((uint8_t)*ip++);
  Serial.print(".");
  Serial.print((uint8_t)*ip++);
  Serial.print(".");
  Serial.println((uint8_t)*ip);

  ip = (uint8_t*)&current_options.dhcp_server.s_addr;
  Serial.print("DHCPServer: ");
  Serial.print((uint8_t)*ip++);
  Serial.print(".");
  Serial.print((uint8_t)*ip++);
  Serial.print(".");
  Serial.print((uint8_t)*ip++);
  Serial.print(".");
  Serial.println((uint8_t)*ip);

  
  Serial.print("State: ");
  Serial.println(fnet_dhcp_cln_get_state(_dhcp_desc));
  Serial.println();
  Serial.println();

  

}

void checkLink(){
//  Serial.print("Link: ");
//  Serial.println(asix1.connected);
  if(asix1.connected && fnet_dhcp_cln_is_enabled(dhcp_desc)){
    //Serial.println("DHCP already running!");
  }
  else if(asix1.connected){
    Serial.println("Initializing DHCP");
          fnet_memset_zero(&dhcp_desc, sizeof(dhcp_desc));
          fnet_memset_zero(&dhcp_params, sizeof(dhcp_params));
          dhcp_params.netif = current_netif;
          // Enable DHCP client.
          if((dhcp_desc = fnet_dhcp_cln_init(&dhcp_params))){
              /*Register DHCP event handler callbacks.*/
             fnet_dhcp_cln_set_callback_updated(dhcp_desc, dhcp_cln_callback_updated, (void*)dhcp_desc);
             fnet_dhcp_cln_set_callback_discover(dhcp_desc, dhcp_cln_callback_updated, (void*)dhcp_desc);
             Serial.println("DHCP initialization done!");
             bench_srv_init();
          }
          else{
            Serial.println("ERROR: DHCP initialization failed!");
          }
  }
  else if(!asix1.connected && fnet_dhcp_cln_is_enabled(dhcp_desc)){
    Serial.println("DHCP Released");
    fnet_dhcp_cln_release(dhcp_desc);
    fnet_memset_zero(dhcp_desc, sizeof(dhcp_desc));
    bench_srv_release();
  }
  else if(!asix1.connected){
//    Serial.println("DHCP already released!");
  }
}

void handleOutput(fnet_netif_t *netif, fnet_netbuf_t *nb) { //Called when a message is sent
  if(nb && (nb->total_length >= FNET_ETH_HDR_SIZE)){
    uint8_t* p = (uint8_t*)sbuf;
    _fnet_netbuf_to_buf(nb, 0u, FNET_NETBUF_COPYALL, p);

//    if(nb->total_length >= 12){
//      Serial.print("Message Transmitted: ");
//      Serial.println(nb->total_length);
//      const uint8_t* end = p + nb->total_length;
//      while(p < end){
//        if(*p <= 0x0F) Serial.print("0");
//        Serial.print(*p, HEX);
//        Serial.print(" ");
//        p++;
//      }
//      Serial.println();
//    }
//    p = (uint8_t*)sbuf;
    asix1.sendPacket(p, nb->total_length);
  }
}

uint32_t _totalLength;
uint8_t* _lastIndex;
uint8_t* _rxEnd;
uint8_t* _rxStart;
void handleRecieve(const uint8_t* data, uint32_t length) { //Called when ASIX gets a message
  if(length == 0) return;
  RECIEVE:
  if(((data[0] + data[2]) == 0xFF) && ((data[1] + data[3]) == 0xFF)) { //Check for header
    _lastIndex = (uint8_t*)rbuf;
    _totalLength = (data[1] << 8) | data[0];
    const uint8_t* end = data + (((_totalLength + 6) <= length) ? _totalLength+6 : length);
    _rxEnd = (uint8_t*)rbuf + _totalLength;
    data += 6;
    while(data < end){
      *_lastIndex = *data;
      data++;
      _lastIndex++;
    }
  }
  else if(_lastIndex <= _rxEnd){
    const uint8_t* end = data + length;
    while(data < end){
      *_lastIndex = *data;
      data++;
      _lastIndex++;
    }
  }

  if(_lastIndex >= _rxEnd && _totalLength > 1000) {
//    Serial.print("Length: ");
//    Serial.println(_totalLength);
//    Serial.print("LastUSBLength: ");
//    Serial.println(length);
//    Serial.print("LengthRemaining: ");
//    Serial.println(length-_totalLength-6);
//    Serial.print("Message Recieved: ");
//    for(uint16_t i = 0; i < _totalLength; i++){
//      if(rbuf[i] <= 0x0F) Serial.print("0");
//      Serial.print(rbuf[i], HEX);
//      Serial.print(" ");
//    }
//    Serial.println();
//    Serial.println();
  }
//  Serial.println();
//  Serial.println();
  if(_lastIndex >= _rxEnd) {
    _fnet_eth_input(&fnet_eth0_if, (uint8_t*)rbuf, _totalLength);
    if((length-_totalLength-6) > 3){
//      Serial.println("Recieve Looped");
      length -= (_totalLength + 6);
      goto RECIEVE;
//        Serial.println();
    }
//    Serial.println();
  }
}

void handleSetMACAddress(uint8_t * hw_addr) { //Gets calls on initialization
  Serial.print("SetMACAddress: ");
  for(uint8_t i = 0; i < 6; i++) {
    if(hw_addr[i] <= 0x0F) Serial.print("0");
    Serial.print(hw_addr[i], HEX);
  }
  Serial.println();
}

void handleGetMACAddress(fnet_mac_addr_t * hw_addr) { //Gets called everytime a message is sent
  (*hw_addr)[0] = asix1.nodeID[0];
  (*hw_addr)[1] = asix1.nodeID[1];
  (*hw_addr)[2] = asix1.nodeID[2];
  (*hw_addr)[3] = asix1.nodeID[3];
  (*hw_addr)[4] = asix1.nodeID[4];
  (*hw_addr)[5] = asix1.nodeID[5];
//  Serial.print("GetMACAddress: ");
//  for(uint8_t i = 0; i < 6; i++) {
//    if(hw_addr[i] <= 0x0F) Serial.print("0");
//    Serial.print(hw_addr[i], HEX);
//  }
//  Serial.println();
}

void handlePHYRead(fnet_uint32_t reg_addr, fnet_uint16_t *data) { //Could be called, don't think it works correctly
  asix1.readPHY(reg_addr, data);
  Serial.print("PHYRead: ");
  Serial.print(reg_addr, HEX);
  Serial.print("  ");
  Serial.println(*data, HEX);
}

void handlePHYWrite(fnet_uint32_t reg_addr, fnet_uint16_t data) { //Could be called, might work correctly
  asix1.writePHY(reg_addr, data);
  Serial.println("PHYWrite");
}

void handleMulticastJoin(fnet_netif_t *netif, fnet_mac_addr_t multicast_addr) { //Called when joining multicast group
  //Not setup yet
  Serial.print("MulticastJoin: ");
  for(uint8_t i = 0; i < 6; i++) {
    if(multicast_addr[i] <= 0x0F) Serial.print("0");
    Serial.print(multicast_addr[i], HEX);
  }
  Serial.println();
}

void handleMulticastLeave(fnet_netif_t *netif, fnet_mac_addr_t multicast_addr) { //Called when leaving multicast group
  //Not setup yet
  Serial.println("MulticastLeave: ");
  for(uint8_t i = 0; i < 6; i++) {
    if(multicast_addr[i] <= 0x0F) Serial.print("0");
    Serial.print(multicast_addr[i], HEX);
  }
  Serial.println();
}

fnet_bool_t handleIsConnected() {
  return asix1.connected ? FNET_TRUE : FNET_FALSE;
}

fnet_bench_srv_params_t bench_srv_params;
fnet_bench_srv_desc_t bench_srv_desc;
void bench_srv_init(){
    /* Set Benchmark server parameters.*/
    fnet_memset_zero(&bench_srv_params, sizeof(bench_srv_params));
    bench_srv_params.type = SOCK_STREAM; /* TCP by default */
        if(current_netif){ /* Only on one interface */
            bench_srv_params.address.sa_scope_id = fnet_netif_get_scope_id(current_netif);
        }

        /* Start Benchmark server. */
        bench_srv_desc = fnet_bench_srv_init(&bench_srv_params);
        if(bench_srv_desc){
            /* Instal callbacks */
//            fnet_bench_srv_set_callback_session_begin (fapp_bench_srv_desc, fapp_bench_srv_callback_session_begin, shell_desc);
            fnet_bench_srv_set_callback_session_end (bench_srv_desc, bench_srv_callback_session_end, bench_srv_desc);
        }
}

void bench_srv_release(void){
    fnet_bench_srv_release(bench_srv_desc);
    bench_srv_desc = 0;
}

static void bench_srv_callback_session_end(fnet_bench_srv_desc_t desc, const struct fnet_bench_srv_result *bench_srv_result, void *cookie)
{
  if(bench_srv_result){
    uint64_t totalBytes = (bench_srv_result->megabytes * 1000000) + bench_srv_result->bytes;
    Serial.println("Benchmark results:");
    Serial.print("Megabytes: ");
    Serial.print(bench_srv_result->megabytes);
    Serial.print(".");
    Serial.println(bench_srv_result->bytes);
    Serial.print("Seconds: ");
    Serial.println(bench_srv_result->time_ms/1000.0, 4);
    Serial.print("Bytes/Sec: ");
    Serial.println(totalBytes/(bench_srv_result->time_ms/1000.0), 4);
    Serial.print("KBytes/Sec: ");
    Serial.println((totalBytes/(bench_srv_result->time_ms/1000.0))/1000.0, 4);
    Serial.print("KBits/Sec: ");
    Serial.println((((totalBytes/(bench_srv_result->time_ms/1000.0))/1000.0)*8), 4);
    Serial.println();
  }
}
