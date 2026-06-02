int initialized = 42;
int counter;

void kernel_main(void)
{
    asm volatile("svc #0");
    while (1) {
        initialized++;
        counter++;
    }
}