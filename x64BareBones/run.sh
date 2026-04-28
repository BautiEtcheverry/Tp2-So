#!/bin/bash
# Use coreaudio on macOS; fall back to no audio if unavailable
if [[ "$OSTYPE" == "darwin"* ]]; then
	AUDIO_OPTS="-audiodev id=audio0,driver=coreaudio -machine pcspk-audiodev=audio0"
else
	AUDIO_OPTS="-audiodev id=audio0,driver=alsa -machine pcspk-audiodev=audio0"
fi
qemu-system-x86_64 \
	-hda Image/x64BareBonesImage.qcow2 \
	-m 512 \
	-rtc base=localtime,clock=host \
	$AUDIO_OPTS
