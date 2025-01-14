//NAME: John "Jack" Joseph OConnor IV
//NETID: jjo1
//SIM INPUT:
//OUTPUT: 96
//
// check1.i is a slightly modified version of a 
// block submitted by Jack O'Connor in Fall 2014.
//
// It checks to see that the scanner and parser
// correctly accept all of the ILOC subset operations.
//
// It also checks that the scanner correctly handles
// an input line that is terminated by EOF (end of file)
// in lieu of a new line.
//
        loadI   1024    =>      r0
        loadI   128     =>      r0001
        loadI   32      =>      r2
        loadI   1028    =>      r3
        mult    r2,r02  =>      r4
        rshift  r2,r04  =>      r4
        lshift  r2,r004 =>      r05
        add     r5,r4   =>      r5
        sub     r2,r05  =>      r06
        store   r6      =>      r3
        load    r00003  =>      r6
        add     r06,r1  =>      r7
        nop 
        store   r7      =>      r0
        output 1024
