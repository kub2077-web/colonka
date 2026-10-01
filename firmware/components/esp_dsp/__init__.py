import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import esp32


CONFIG_SCHEMA = cv.Schema({})


async def to_code(config):
    # ESP32-audioI2S includes Arduino filesystem/network headers directly.
    # ESPHome disables these Arduino libraries by default even when the
    # Arduino framework is selected, so declare the transitive requirements.
    for library in (
        "FS",
        "FFat",
        "Network",
        "NetworkClientSecure",
        "SD",
        "SD_MMC",
        "SPI",
        "WiFi",
    ):
        cg.add_library(library, None)

    esp32.add_idf_component(
        name="espressif/esp-dsp",
        repo="https://github.com/espressif/esp-dsp.git",
        ref="v1.7.0",
    )