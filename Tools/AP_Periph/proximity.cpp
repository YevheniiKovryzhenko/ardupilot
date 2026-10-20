#include "AP_Periph.h"

#if AP_PERIPH_PROXIMITY_ENABLED

/*
  proximity support
 */

#include <dronecan_msgs.h>
#if AP_PROXIMITY_SF45_CAN_ENABLED && AP_PROXIMITY_LIGHTWARE_SF45B_ENABLED
#include <AP_Proximity/AP_Proximity_LightWareSF45B.h>

// Local wire contract: flag bit 0 identifies raw SF45 sensor-frame observations.
// Legacy Proximity packets use flags=0 and body-frame boundary obstacles.
static void publish_sf45_sample(float yaw, uint16_t distance_cm)
{
    ardupilot_equipment_proximity_sensor_Proximity pkt {};
    pkt.sensor_id = periph.proximity.get_address(0);
    pkt.flags = 1;
    pkt.reading_type = ARDUPILOT_EQUIPMENT_PROXIMITY_SENSOR_PROXIMITY_READING_TYPE_GOOD;
    pkt.yaw = yaw;
    // Keep no-return distinct across float16 quantization near 50 metres.
    pkt.distance = distance_cm > 5000 ? 51.0f : distance_cm * 0.01f;
    uint8_t buffer[ARDUPILOT_EQUIPMENT_PROXIMITY_SENSOR_PROXIMITY_MAX_SIZE];
    const uint16_t size = ardupilot_equipment_proximity_sensor_Proximity_encode(&pkt, buffer, !periph.canfdout());
    periph.canard_broadcast(ARDUPILOT_EQUIPMENT_PROXIMITY_SENSOR_PROXIMITY_SIGNATURE,
                           ARDUPILOT_EQUIPMENT_PROXIMITY_SENSOR_PROXIMITY_ID,
                           CANARD_TRANSFER_PRIORITY_LOW, buffer, size, 0, 20000U);
}
#endif

void AP_Periph_FW::can_proximity_update()
{
    if (proximity.get_type(0) == AP_Proximity::Type::None) {
        return;
    }

#if AP_PROXIMITY_SF45_CAN_ENABLED && AP_PROXIMITY_LIGHTWARE_SF45B_ENABLED
    if (proximity.get_type(0) == AP_Proximity::Type::SF45B) {
        auto *backend = static_cast<AP_Proximity_LightWareSF45B *>(proximity.get_backend(0));
        if (backend != nullptr) {
            backend->set_sample_callback(publish_sf45_sample);
        }
        // Poll at the main-loop rate, independent of coarse proximity publication.
        proximity.update();
        return;
    }
#endif
    uint32_t now = AP_HAL::millis();
    static uint32_t last_update_ms;
    if (g.proximity_max_rate > 0 &&
        now - last_update_ms < 1000/g.proximity_max_rate) {
        // limit to max rate
        return;
    }
    last_update_ms = now;
    proximity.update();
    AP_Proximity::Status status = proximity.get_status();
    if (status <= AP_Proximity::Status::NoData) {
        // don't send any data
        return;
    }

    ardupilot_equipment_proximity_sensor_Proximity pkt {};

    const uint8_t obstacle_count = proximity.get_obstacle_count();

    // if no objects return
    if (obstacle_count == 0) {
        return;
    }

    // calculate maximum roll, pitch values from objects
    for (uint8_t i=0; i<obstacle_count; i++) {
        if (!proximity.get_obstacle_info(i, pkt.yaw, pkt.pitch, pkt.distance)) {
            // not a valid obstacle
            continue;
        }

        pkt.sensor_id = proximity.get_address(0);

        switch (status) {
        case AP_Proximity::Status::NotConnected:
            pkt.reading_type = ARDUPILOT_EQUIPMENT_PROXIMITY_SENSOR_PROXIMITY_READING_TYPE_NOT_CONNECTED;
            break;
        case AP_Proximity::Status::Good:
            pkt.reading_type = ARDUPILOT_EQUIPMENT_PROXIMITY_SENSOR_PROXIMITY_READING_TYPE_GOOD;
            break;
        case AP_Proximity::Status::NoData:
        default:
            pkt.reading_type = ARDUPILOT_EQUIPMENT_PROXIMITY_SENSOR_PROXIMITY_READING_TYPE_NO_DATA;
            break;
        }

        uint8_t buffer[ARDUPILOT_EQUIPMENT_PROXIMITY_SENSOR_PROXIMITY_MAX_SIZE];
        uint16_t total_size = ardupilot_equipment_proximity_sensor_Proximity_encode(&pkt, buffer, !periph.canfdout());

        canard_broadcast(ARDUPILOT_EQUIPMENT_PROXIMITY_SENSOR_PROXIMITY_SIGNATURE,
                        ARDUPILOT_EQUIPMENT_PROXIMITY_SENSOR_PROXIMITY_ID,
                        CANARD_TRANSFER_PRIORITY_LOW,
                        &buffer[0],
                        total_size);

    }
}

#endif // AP_PERIPH_PROXIMITY_ENABLED
