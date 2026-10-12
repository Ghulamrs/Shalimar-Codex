/* TI-compiled control: verifies the simulator, not Shalimar code generation. */
volatile int probe_result;
void halt_here(void) { for (;;) {} }
int main(void) {
    volatile int a = 17;
    volatile int b = 25;
    probe_result = a + b;
    halt_here();
    return 0;
}
