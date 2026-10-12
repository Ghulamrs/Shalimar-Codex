/* TI-compiled simulator harness; answer() is emitted by Shalimar DSP backend. */
extern int answer(void);
volatile int probe_result = -1;
void halt_here(void) { for (;;) {} }
int main(void) {
    probe_result = answer();
    halt_here();
    return 0;
}
