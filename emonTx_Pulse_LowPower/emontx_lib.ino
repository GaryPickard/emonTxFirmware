#define RETRY_PERIOD 1                                                  // How soon to retry (in seconds) if ACK didn't come in
#define RETRY_LIMIT 10                                                   // Maximum number of times to retry
#define ACK_TIME 10                                                     // Number of milliseconds to wait for an ack

void send_rf_data(){
  for (byte i = 0; i <= RETRY_LIMIT; ++i) {  // tx and wait for ack up to RETRY_LIMIT times
    rf12_sleep(-1);              // Wake up RF module
    while (!rf12_canSend())
    rf12_recvDone();
    rf12_sendStart(RF12_HDR_ACK, &emontx, sizeof emontx);
    rf12_sendWait(2);           // Wait for RF to finish sending while in standby mode
    byte acked = waitForAck();  // Wait for ACK
    rf12_sleep(0);              // Put RF module to sleep
      if (acked == 1) {
        delay(50);
        Serial.print("Ack Completed - "); Serial.print("Attempt: "); Serial.println(i);
        delay(50);
        return; }      // Return if ACK received
    Serial.print("Ack Failed - "); Serial.print("Attempt: "); Serial.println(i);
    delay(RETRY_PERIOD * 1000);
    }
}

static byte waitForAck() {
  MilliTimer ackTimer;
    while (!ackTimer.poll(ACK_TIME)) {
      if (rf12_recvDone() && rf12_crc == 0 &&
        rf12_hdr == (RF12_HDR_DST | RF12_HDR_CTL | nodeID))
        return 1;
      }
    return 0;
    }
