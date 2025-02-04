// https://github.com/dbuezas/esphome-cc1101

#ifndef CC1101TRANSCEIVER_H
#define CC1101TRANSCEIVER_H

#include <ELECHOUSE_CC1101_SRC_DRV.h>

int CC1101_MODULE_COUNT = 0;
#define get_cc1101(id) (*((CC1101 *)id))

class CC1101 : public PollingComponent, public Sensor {
  int _SCK;
  int _MISO;
  int _MOSI;
  int _CSN;
  int _GDO0;
  float _bandwidth;
  float _freq;
  bool _rssi_on;
  int _modulation;

  float _module_number;
  int _last_rssi = 0;

 public:

  CC1101(
    int SCK,
    int MISO,
    int MOSI,
    int CSN,
    int GDO0,
    float bandwidth = 200, // Bandwidth in kHz
    float freq = 433.92,   // Frequency in mHz
    bool rssi_on = false,  // Disable verbose RSSI
    int modulation = 0,    // Set modulation mode:
                           //   0 = 2-FSK
                           //   1 = GFSK
                           //   2 = ASK/OOK
                           //   3 = 4-FSK
                           //   4 = MSK
  ) : PollingComponent(100) {
    _SCK = SCK;
    _MISO = MISO;
    _MOSI = MOSI;
    _CSN = CSN;
    _GDO0 = GDO0;
    _bandwidth = bandwidth;
    _freq = freq;
    _rssi_on = rssi_on;
    _modulation = modulation;

    _module_number = CC1101_MODULE_COUNT++;
  }

  void setup() override {
    pinMode(_GDO0, INPUT);
    ELECHOUSE_cc1101.addSpiPin(_SCK, _MISO, _MOSI, _CSN, _module_number);
    ELECHOUSE_cc1101.setModul(_module_number);
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.setRxBW(_bandwidth);
    ELECHOUSE_cc1101.setMHZ(_freq);
    ELECHOUSE_cc1101.SetRx();
    ELECHOUSE_cc1101.setModulation(_modulation);
  }

  void update() override {
    int rssi = 0;
    if (_rssi_on) {
      ELECHOUSE_cc1101.setModul(_module_number);
      rssi = ELECHOUSE_cc1101.getRssi();
    }
    if (rssi != _last_rssi) {
      publish_state(rssi);
      _last_rssi = rssi;
    }
  }

  void beginTransmission() {
    ELECHOUSE_cc1101.setModul(_module_number);
    ELECHOUSE_cc1101.SetTx();
    pinMode(_GDO0, OUTPUT);
    noInterrupts();
  }

  void endTransmission() {
    interrupts();
    pinMode(_GDO0, INPUT);
    ELECHOUSE_cc1101.setModul(_module_number);
    ELECHOUSE_cc1101.SetRx();
    ELECHOUSE_cc1101.SetRx();  // yes, twice
  }

  void setBW(float bandwidth) {
    ELECHOUSE_cc1101.setModul(_module_number);
    ELECHOUSE_cc1101.setRxBW(bandwidth);
  }

  void setFreq(float freq) {
    ELECHOUSE_cc1101.setModul(_module_number);
    ELECHOUSE_cc1101.setMHZ(freq);
  }

};

#endif
