#ifndef SYNC_TESTBENCH_H
#define SYNC_TESTBENCH_H


#include "base_testbench.h"


class SyncTestbench : public BaseTestbench
{
protected:
    void runSimulation(int num_cycles = 1)
    {
        // Run simuation for many clock cycles
        for (int i = 0; i < num_cycles; ++i)
        {
            // dump variables into VCD file and toggle clock
            for (int clk = 0; clk < 2; ++clk)
            {
                #ifndef __APPLE__
                tfp->dump(2 * ticks + clk); // picoseconds
                #endif
                top->clk = !top->clk;
                top->eval();
            }

            ticks++;

            if (Verilated::gotFinish())
            {
                exit(0);
            }
        }
    }
private:
    int ticks;
};


#endif      /* SYNC_TESTBENCH_H */