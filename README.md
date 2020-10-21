# STM32 TM1637 LIBRARY
# Description
TM1637 Runs on a basic protocol based on GPIO switching and since it is not compitable with I2C protocol, all commands are built using GPIO switching.
Altough This library is based on STM32 HAL library, it can be integrated to any project even for different microcontroller in order to handle TM1637 low level protocol only by modifying GPIO set/reset functions
# Freatures
No over heads, which makes the CPU acailable for for ither operations
the library features ASCII to 7-segment mapping function, which makes programming this display user friendly.
Currenly the library is under development stage and more applications will be add in the future.
