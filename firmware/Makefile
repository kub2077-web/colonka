ESPHOME ?= ../.pythonlibs/bin/esphome
CONFIG ?= esp32-s3-speaker.yaml

.PHONY: validate compile upload logs clean

validate:
	$(ESPHOME) config $(CONFIG)

compile:
	$(ESPHOME) compile $(CONFIG)

upload:
	$(ESPHOME) upload $(CONFIG)

logs:
	$(ESPHOME) logs $(CONFIG)

clean:
	$(ESPHOME) clean $(CONFIG)