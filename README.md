# TC375-CAN
CAN functions for use on the Hitex TC375 Shieldbuddy board
Attempt at reverse engineering the provided Arduino CAN functions for the Hitex TC375

## Note: 
this is my best attempt at reversing theses function and from testing worked as need but missing some parts

## TODO:
    Work on all cores, currently only works on core 0
    Add check for array bounds in Rx and Tx functions, i.e check if MsgObj_id is greater than the macros
    maybe add away to get the amount of bytes recieved from the recive message function, as is zero and could cause issue
    Test 1234567889

