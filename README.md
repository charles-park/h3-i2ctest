# h3-i2ctest
ODROID-H3(X86) I2C Read Test.

service install
cp h3-i2ctest.server /etc/systemd/system
systemctl enable h3-i2ctest

modified test i2c(device addr)
/root/h3-i2ctest/default_app.cfg
