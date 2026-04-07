pio run -e rachel-sdk -t buildfs
pio run -e rachel-sdk -t uploadfs

pio run -e rachel-sdk
pio run -e rachel-sdk -t upload

pio run -e rachel-sdk -t uploadfs -t upload

