#ifndef __COMPONENT_ZEHNDER_H__
#define __COMPONENT_ZEHNDER_H__

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/spi/spi.h"
#include "esphome/components/fan/fan_state.h"
#include "esphome/components/nrf905/nRF905.h"
#include <cstdint>  // for uint8_t

namespace esphome {
namespace zehnder {

#define FAN_FRAMESIZE 16        // Each frame consists of 16 bytes
#define FAN_TX_FRAMES 4         // Retransmit every transmitted frame 4 times
#define FAN_TX_RETRIES 10       // Retry transmission 10 times if no reply is received
#define FAN_TTL 250             // 0xFA, default time-to-live for a frame
#define FAN_REPLY_TIMEOUT 1000  // Wait 1000ms for receiving a reply when doing a network scan

// Additional Fan Unit Types
#define FAN_UNIT_TYPE_MAIN 0x01
#define FAN_UNIT_TYPE_REMOTE 0x02
#define FAN_NETWORK_LINK_TTL 3
#define FAN_UPDATE_SETTINGS 0x07
#define FAN_SET_SPEED_TIMER 0x03

/* Fan device types */
enum {
  FAN_TYPE_BROADCAST = 0x00,       // Broadcast to all devices
  FAN_TYPE_MAIN_UNIT = 0x01,       // Fans
  FAN_TYPE_REMOTE_CONTROL = 0x03,  // Remote controls
  FAN_TYPE_CO2_SENSOR = 0x18       // CO2 sensors
};

/* Fan commands */
enum {
  FAN_FRAME_SETVOLTAGE = 0x01,     // Set speed (voltage / percentage)
  FAN_FRAME_SETSPEED = 0x02,       // Set speed (preset)
  FAN_FRAME_SETTIMER = 0x03,       // Set speed with timer
  FAN_NETWORK_JOIN_REQUEST = 0x04,
  FAN_FRAME_SETSPEED_REPLY = 0x05,
  FAN_NETWORK_JOIN_OPEN = 0x06,
  FAN_TYPE_FAN_SETTINGS = 0x07,    // Current settings, sent by fan in reply to 0x01, 0x02, 0x10
  FAN_FRAME_0B = 0x0B,
  FAN_NETWORK_JOIN_ACK = 0x0C,
  FAN_TYPE_QUERY_NETWORK = 0x0D,
  FAN_TYPE_QUERY_DEVICE = 0x10,
  FAN_FRAME_SETVOLTAGE_REPLY = 0x1D
};

/* Fan speed presets */
enum {
  FAN_SPEED_AUTO = 0x00,    // Off:      0% or  0.0 volt
  FAN_SPEED_LOW = 0x01,     // Low:     30% or  3.0 volt
  FAN_SPEED_MEDIUM = 0x02,  // Medium:  50% or  5.0 volt
  FAN_SPEED_HIGH = 0x03,    // High:    90% or  9.0 volt
  FAN_SPEED_MAX = 0x04      // Max:    100% or 10.0 volt
};

#define NETWORK_LINK_ID 0xA55A5AA5
#define NETWORK_DEFAULT_ID 0xE7E7E7E7
#define FAN_JOIN_DEFAULT_TIMEOUT 10000

typedef enum { ResultOk, ResultBusy, ResultFailure } Result;

class ZehnderRF : public Component, public fan::Fan {
 public:
  ZehnderRF();

  void setup() override;

  void set_rf(nrf905::nRF905 *const pRf) { rf_ = pRf; }
  void set_update_interval(const uint32_t interval) { interval_ = interval; }

  void dump_config() override;
  fan::FanTraits get_traits() override;
  int get_speed_count() { return this->speed_count_; }

  void discoveryStart(uint8_t deviceId); // Consistent use of uint8_t

  void loop() override;
  void control(const fan::FanCall &call) override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void setSpeed(const uint8_t speed, const uint8_t timer = 0);

  bool timer;
  int voltage;

  uint8_t get_error_status() const { return error_status; }

 protected:
  void queryDevice(void);

  uint8_t createDeviceID(void);

  Result startTransmit(const uint8_t *const pData, const int8_t rxRetries = -1,
                       const std::function<void(void)> callback = NULL);
  void rfComplete(void);
  void rfHandler(void);
  void rfHandleReceived(const uint8_t *const pData, const uint8_t dataLength);

  void sendRfFrame(const uint8_t deviceType, const uint8_t ttl);
  void fanSettingsReceived(const uint8_t speed, const uint8_t voltage, const uint8_t timer);
  uint16_t calculate_crc16(uint8_t *data, size_t length);
  void append_crc_to_payload(uint8_t *payload, size_t length);

  uint8_t error_status{0};

  typedef enum {
    StateStartup,
    StateStartDiscovery,
    StateDiscoveryWaitForLinkRequest,
    StateDiscoveryWaitForLinkAck,
    StateDiscoveryWaitForJoinResponse,
    StateDiscoveryJoinComplete,

    StateIdle,
    StateWaitQueryResponse,
    StateWaitSetSpeedResponse,
    StateWaitSetSpeedConfirm,
    StateWaitQueryForUpdate,

    StateNrOf
  } State;
  State state_{StateStartup};
  int speed_count_{};

  nrf905::nRF905 *rf_;
  uint32_t interval_;

  uint8_t _txFrame[FAN_FRAMESIZE];

  ESPPreferenceObject pref_;

  typedef struct {
    uint32_t fan_networkId;      // Fan (Zehnder/BUVA) network ID
    uint8_t fan_my_device_type;  // Fan (Zehnder/BUVA) device type
    uint8_t fan_my_device_id;    // Fan (Zehnder/BUVA) device ID
    uint8_t fan_main_unit_type;  // Fan (Zehnder/BUVA) main unit type
    uint8_t fan_main_unit_id;    // Fan (Zehnder/BUVA) main unit ID
  } Config;
  Config config_;

  uint32_t lastFanQuery_{0};
  std::function<void(void)> onReceiveTimeout_ = NULL;

  uint32_t msgSendTime_{0};
  uint32_t airwayFreeWaitTime_{0};
  int8_t retries_{-1};

  uint8_t newSpeed{0};
  uint8_t newTimer{0};
  bool newSetting{false};

  typedef enum {
    RfStateIdle,
    RfStateWaitAirwayFree,
    RfStateTxBusy,
    RfStateRxWait,
    RfStateRxBusy,
  } RfState;
  RfState rfState_{RfStateIdle};
};

}  // namespace zehnder
}  // namespace esphome

#endif /* __COMPONENT_ZEHNDER_H__ */
