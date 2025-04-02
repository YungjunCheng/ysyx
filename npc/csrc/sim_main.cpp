#include "Vtop.h"
#include "verilated.h"
#include "verilated_vcd_c.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vtop* top = new Vtop;

    Verilated::traceEverOn(true);
    VerilatedVcdC* tfp = new VerilatedVcdC;
    top->trace(tfp, 99);
    tfp->open("wave.vcd");

    srand(time(NULL));
    int cycle = 0;
    
    while (cycle < 100) {
        int a = rand() & 1;
        int b = rand() & 1;
        top->a = a;
        top->b = b;
        top->eval();

        tfp->dump(cycle);

        printf("Cycle %2d: a=%d, b=%d, f=%d\n", cycle, a, b, top->f);
        assert(top->f == (a ^ b));

        cycle++;
    }

    tfp->close();
    top->final();
    delete top;
    delete tfp;

    return 0;
}

