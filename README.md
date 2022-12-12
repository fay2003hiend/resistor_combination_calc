# Resistor combination calculator

This is a small tool that I created to find combination of two resistors in either parallel or series, to match an expected value.

### Value match mode

Simply enter the value required, can be float point values, can also have `M` `K` or `k`, or `m` units.

### Voltage divider look up mode

In this mode you can specify the `vout` and `vref`, it will try to pick a R2 value from range 8.0K to 30.0K, then use value match mode to find R1

